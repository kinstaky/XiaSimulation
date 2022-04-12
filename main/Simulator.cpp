#include <exception>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <sys/wait.h>

#include "TString.h"
#include "TGraph.h"
#include "TLegend.h"
#include "TH1D.h"
#include "TROOT.h"

#include "Simulator.h"

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::microseconds;


//--------------------------------------------------
//			Simulator::RunFlag
//--------------------------------------------------


Simulator::RunFlag operator|(Simulator::RunFlag lhs, Simulator::RunFlag rhs) {
	return static_cast<Simulator::RunFlag>(
		static_cast<std::underlying_type<Simulator::RunFlag>::type>(lhs) |
		static_cast<std::underlying_type<Simulator::RunFlag>::type>(rhs)
	);
}

Simulator::RunFlag operator&(Simulator::RunFlag lhs, Simulator::RunFlag rhs) {
	return static_cast<Simulator::RunFlag>(
		static_cast<std::underlying_type<Simulator::RunFlag>::type>(lhs) &
		static_cast<std::underlying_type<Simulator::RunFlag>::type>(rhs)
	);
}

bool operator!=(Simulator::RunFlag lhs, int rhs) {
	return static_cast<std::underlying_type<Simulator::RunFlag>::type>(lhs) != rhs;
}

bool operator!=(int lhs, Simulator::RunFlag rhs) {
	return lhs != static_cast<std::underlying_type<Simulator::RunFlag>::type>(rhs);
}



//--------------------------------------------------
//				Simulator
//--------------------------------------------------


Simulator::Simulator() {
	reader = nullptr;
	slowFilter = nullptr;
	fastFilter = nullptr;
	cfdFilter = nullptr;
	slowPicker = nullptr;
	fastPicker = nullptr;
	cfdPicker = nullptr;
	file = nullptr;
	path = "";
	fileName = "";
	zeroPoint = 0;
	verbose = true;
}

Simulator::~Simulator() {
	if (file) file->Close();
}

void Simulator::AddReader(std::unique_ptr<TraceReader> reader_) {
	reader = std::move(reader_);
	return;
}


void Simulator::AddSlowFilter(std::unique_ptr<FilterAlgorithm> filter_) {
	slowFilter = std::move(filter_);
	return;
}

void Simulator::AddFastFilter(std::unique_ptr<FilterAlgorithm> filter_) {
	fastFilter = std::move(filter_);
	return;
}

void Simulator::AddCFDFilter(std::unique_ptr<FilterAlgorithm> filter_) {
	cfdFilter = std::move(filter_);
	return;
}


void Simulator::AddSlowPicker(std::unique_ptr<Picker> picker_) {
	slowPicker = std::move(picker_);
	return;
}

void Simulator::AddFastPicker(std::unique_ptr<Picker> picker_) {
	fastPicker = std::move(picker_);
	return;
}


void Simulator::AddCFDPicker(std::unique_ptr<Picker> picker_) {
	cfdPicker = std::move(picker_);
	return;
}


// Set file path
void Simulator::SetPath(const char *p) {
	path = TString(p);
	if (path[path.Length()-1] != '/') path += "/";
	return;
}

// Set record file name
void Simulator::SetFileName(const char *name_) {
	fileName = TString(name_);
	return;
}


// Set zero point
void Simulator::SetZeroPoint(unsigned int zero_) {
	zeroPoint = zero_;
	return;
}

// Set print verbose
void Simulator::SetVerbose(bool verbose_) {
	verbose = verbose_;
	return;
}



//--------------------------------------------------
//				BaseSimulator
//--------------------------------------------------

// constructor, assign null pointer
BaseSimulator::BaseSimulator(): Simulator() {
}


// deconstructor, do nothing
BaseSimulator::~BaseSimulator() {
}

// run the simulation
void BaseSimulator::Run(unsigned int times, RunFlag flag) {
// std::cout << "run " << std::endl;
	// check trace reader
	if (!reader) throw std::runtime_error("Error: trace reader not found.");
	// check slow filter
	if ((flag & RunFlag::SlowFilter) != 0) {
		if (!slowFilter) throw std::runtime_error("Error: slow filter not found.");
		if (!slowPicker) throw std::runtime_error("Error: slow picker not found.");
	}
	// check fast filter
	if ((flag & RunFlag::FastFilter) != 0){
		if (!fastFilter) throw std::runtime_error("Error: fast filter not found.");
		if (!fastPicker) throw std::runtime_error("Error: fast picker not found.");
	}

	if ((flag & RunFlag::CFDFilter) != 0) {
		if (!fastFilter) throw std::runtime_error("Error: fast filter not found.");
		if (!fastPicker) throw std::runtime_error("Error: fast picker not found.");
		if (!cfdFilter) throw std::runtime_error("Error: CFD filter not found.");
		if (!cfdPicker) throw std::runtime_error("Error: CFD picker not found.");
	}

	// open file
	if (!file) {
		if (!path.Length()) throw std::runtime_error("Error: simulation file path is empty.");
		if (!fileName.Length()) throw std::runtime_error("Error: simulation file name is empty.");
		file = new TFile(path + fileName, "recreate");
	}

	// get period, dt
	unsigned int dt = reader->GetPeriod();
	// make the x positions of data
	std::vector<double> pointX;
	energy.clear();
	timestamp.clear();
	for (unsigned int t = 0; t != times; ++t) {
		auto &rawData = reader->Read();

		// record to files
		if (t == 0) {
			size_t dsize = rawData.size();
			double px = 0.0;
			for (size_t i = 0; i != dsize; ++i) {
				pointX.push_back(px);
				px += dt;
			}
		}
		// open file
		file->cd();
		// trace
		TGraph *gTrace = new TGraph(rawData.size(), pointX.data(), rawData.data());
		gTrace->SetLineColor(kBlack);
		gTrace->Write(TString::Format("Trace%d", t));

		// std::cout << "Simulation " << t << "  ";

		if ((flag & RunFlag::SlowFilter) != 0) {
			auto &slowData = slowFilter->Filter(rawData);

			// write slow filter
			file->cd();
			TGraph *gSlow = new TGraph(slowData.size(), pointX.data(), slowData.data());
			gSlow->SetLineColor(kRed);
			gSlow->Write(TString::Format("Slow%d", t));

			// calculate energy
			energy.push_back(slowPicker->Pick(slowData));
		}

		if (((flag & RunFlag::FastFilter) != 0) || ((flag & RunFlag::CFDFilter) != 0)) {
			auto &fastData = fastFilter->Filter(rawData);

			// open file
			file->cd();
			// write fast filter
			TGraph *gFast = new TGraph(fastData.size(), pointX.data(), fastData.data());
			gFast->SetLineColor(kBlue);
			gFast->Write(TString::Format("Fast%d", t));

			// calculate timestamp
			timestamp.push_back(int(fastPicker->Pick(fastData)));
		}


		if ((flag & RunFlag::CFDFilter) != 0) {
			auto &cfdData = cfdFilter->Filter(rawData);

			// open file and store the graph
			file->cd();
			TGraph *gCFD = new TGraph(cfdData.size(), pointX.data(), cfdData.data());
			gCFD->SetLineColor(kGreen);
			gCFD->Write(TString::Format("CFD%d", t));

			// calcute cfd fraction
			cfd.push_back(cfdPicker->Pick(cfdData));
		}

	}


	return;
}

std::string BaseSimulator::Result() {
	std::stringstream ss;
	ss << "index  energy  ts    cfdp  cfd" << std::endl;
	size_t esize = energy.size();
	size_t tsize = timestamp.size();
	size_t vsize = esize > tsize ? esize : tsize;
	size_t csize=  cfd.size();
	vsize = vsize > csize ? vsize : csize;
	for (size_t i = 0; i != vsize; ++i) {
		ss << std::left;
		ss << std::setw(7) << i;
		if (i < esize) ss << std::setw(8) << int(energy[i]);
		else ss << "        ";
		if (i < tsize) ss << std::setw(6) << timestamp[i];
		else ss << "      ";
		if (i < csize) ss << std::setw(6) << int(cfd[i]);
		else ss << "      ";
		if (i < csize) ss << int((cfd[i]-int(cfd[i]))*32768);
		ss << std::endl;
	}
	return ss.str();
}

std::vector<double> &BaseSimulator::GetEnergy() {
	return energy;
}

std::vector<int> &BaseSimulator::GetTime() {
	return timestamp;
}

std::vector<double> &BaseSimulator::GetCFD() {
	return cfd;
}


//--------------------------------------------------
//				TTreeSimulator
//--------------------------------------------------

// constructor
TTreeSimulator::TTreeSimulator() {
	tree = nullptr;
	energy = 0;
	timestamp = 0;
	cfd = 0.0;
	cfdPoint = 0;
}

// deconstructor
TTreeSimulator::~TTreeSimulator() {
}

void TTreeSimulator::Run(unsigned int entries, RunFlag flag) {

	if (!reader) throw std::runtime_error("Error: Trace reader not found.");
	// check slow filter
	if ((flag & RunFlag::SlowFilter) != 0) {
		if (!slowFilter) throw std::runtime_error("Error: slow filter not found.");
		if (!slowPicker) throw std::runtime_error("Error: slow picker not found.");
	}
	// check fast filter
	if ((flag & RunFlag::FastFilter) != 0) {
		if (!fastFilter) throw std::runtime_error("Error: fast filter not found.");
		if (!fastPicker) throw std::runtime_error("Error: fast picker not found.");
	}
	// check cfd filter
	if ((flag & RunFlag::CFDFilter) != 0) {
		if (!fastFilter) throw std::runtime_error("Error: fast filter not found.");
		if (!fastPicker) throw std::runtime_error("Error: fast picker not found.");
		if (!cfdFilter) throw std::runtime_error("Error: CFD filter not found.");
		if (!cfdPicker) throw std::runtime_error("Error: CFD picker not found.");
	}


	// open file
	if (!file) {
		if (!path.Length()) throw std::runtime_error("Error: simulation file path is empty.");
		if (!fileName.Length()) throw std::runtime_error("Error: simulation file name is empty.");
		file = new TFile(path+fileName, "recreate");
// std::cout << "open file " << path+fileName << std::endl;
	}


	if (!tree) {
		tree = new TTree("tree", "tree of simulation energy");
		if ((flag & RunFlag::SlowFilter) != 0) {
			tree->Branch("e", &energy, "energy/s");
		}
		if ((flag & RunFlag::FastFilter) != 0) {
			tree->Branch("lts", &timestamp, "lts/S");
		}
		if ((flag & RunFlag::CFDFilter) != 0) {
			tree->Branch("cfd", &cfd, "cfd/D");
			tree->Branch("cfdp", &cfdPoint, "cfdp/S");
		}
	}


	TH1D *hEnergy = nullptr;
	TH1D *hTime = nullptr;
	TH1D *hCFD = nullptr;
	TH1D *hCFDP = nullptr;


	auto start = std::chrono::high_resolution_clock::now();
	auto stop = start;
	auto readTime = duration_cast<microseconds>(stop - start);
	auto slowFilterTime = duration_cast<microseconds>(stop - start);
	auto fastFilterTime = duration_cast<microseconds>(stop - start);
	auto cfdFilterTime = duration_cast<microseconds>(stop -start);
	auto pickerTime = duration_cast<microseconds>(stop -start);
	auto otherTime = duration_cast<microseconds>(stop - start);

	unsigned int entries100 = entries / 100 + 1;
	if (verbose) {
		std::cout << "run   0%";
		std::cout.flush();
	}
	for (unsigned int t = 0; t != entries; ++t) {

		stop = std::chrono::high_resolution_clock::now();
		start = stop;
		otherTime += duration_cast<microseconds>(stop - start);

		// read raw data
		const std::vector<double> &rawData = reader->Read();

		stop = std::chrono::high_resolution_clock::now();
		readTime += duration_cast<microseconds>(stop - start);
		start = stop;

		// slow filter
		if ((flag & RunFlag::SlowFilter) != 0) {

			auto &slowData = slowFilter->Filter(rawData);


			stop = std::chrono::high_resolution_clock::now();
			slowFilterTime += duration_cast<microseconds>(stop - start);
			start = stop;


			double e = slowPicker->Pick(slowData);
			energy = UShort_t(e);
// std::cout << "energy  " << energy << std::endl;

			stop = std::chrono::high_resolution_clock::now();
			pickerTime += duration_cast<microseconds>(stop - start);
			start = stop;


			if (!hEnergy) hEnergy = new TH1D("he", "energy spectrum", 500, 2000, 3500);
			hEnergy->Fill(e);

			stop = std::chrono::high_resolution_clock::now();
			otherTime += duration_cast<microseconds>(stop - start);
			start = stop;

		}


		if (((flag & RunFlag::FastFilter) != 0) || (flag & RunFlag::CFDFilter) != 0) {
			auto &fastData = fastFilter->Filter(rawData);

			stop = std::chrono::high_resolution_clock::now();
			fastFilterTime += duration_cast<microseconds>(stop - start);
			start = stop;


			int ts = fastPicker->Pick(fastData);
// if (t<10) std::cout << "==========ts: " << ts  << std::endl;
			timestamp = Short_t(ts-zeroPoint);

			stop = std::chrono::high_resolution_clock::now();
			pickerTime += duration_cast<microseconds>(stop -start);
			start = stop;

			if (!hTime) hTime = new TH1D("ht", "local time distribution", 200, -100, 100);
			hTime->Fill(timestamp);

			stop = std::chrono::high_resolution_clock::now();
			otherTime += duration_cast<microseconds>(stop - start);
			start = stop;
		}


		if ((flag & RunFlag::CFDFilter) != 0) {
			auto &cfdData = cfdFilter->Filter(rawData);

			stop = std::chrono::high_resolution_clock::now();
			cfdFilterTime += duration_cast<microseconds>(stop - start);
			start = stop;

			// calcute cfd fraction
			cfd = cfdPicker->Pick(cfdData);
			cfdPoint = int(cfd) - zeroPoint;
			cfd -= int(cfd);
// std::cout << "cfd  " << cfd << "  " << cfdPoint << std::endl;

			stop = std::chrono::high_resolution_clock::now();
			pickerTime += duration_cast<microseconds>(stop - start);
			start = stop;


			if (!hCFD) hCFD = new TH1D("hcfd", "cfd distribution", 1000, 0, 1);
			hCFD->Fill(cfd);

			if (!hCFDP) hCFDP = new TH1D("hcfdp", "cfd point distribution", 200, -100, 100);
			hCFDP->Fill(cfdPoint);

			stop = std::chrono::high_resolution_clock::now();
			otherTime += duration_cast<microseconds>(stop - start);
			start = stop;
		}


		tree->Fill();
		if (verbose && (t % entries100 == 0)) {
			std::cout << "\b\b\b\b" << std::setw(3) << t/entries100 << "%";
			std::cout.flush();
		}


	}
	if (verbose){
		std::cout << "\b\b\b\b100%" << std::endl;
	}

	file->cd();
	if (hEnergy) hEnergy->Write();
	if (hTime) hTime->Write();
	if (hCFD) hCFD->Write();
	if (hCFDP) hCFDP->Write();
	tree->Write();

	if (verbose) {
		auto totalTime = readTime + slowFilterTime + fastFilterTime + cfdFilterTime + pickerTime + otherTime;
		std::cout << "total  " << duration_cast<microseconds>(totalTime).count() << " us" << std::endl;
		std::cout << "read   " << duration_cast<microseconds>(readTime).count() << " us" << std::endl;
		std::cout << "slow   " << duration_cast<microseconds>(slowFilterTime).count() << " us" << std::endl;
		std::cout << "fast   " << duration_cast<microseconds>(fastFilterTime).count() << " us" << std::endl;
		std::cout << "cfd    " << duration_cast<microseconds>(cfdFilterTime).count() << " us" << std::endl;
		std::cout << "pick   " << duration_cast<microseconds>(pickerTime).count() << " us" << std::endl;
		std::cout << "other  " << duration_cast<microseconds>(otherTime).count() << " us" << std::endl;
	}

	return;
}


TTree *TTreeSimulator::Tree() {
	return tree;
}