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
}

Simulator::~Simulator() {
	if (file) file->Close();
}

// // Add TraceReader to the simulator
// void Simulator::AddReader(TraceReader *reader_) {
// 	reader = reader_;
// 	return;
// }


// // Add slow filter to the simulator
// void Simulator::AddSlowFilter(FilterAlgorithm *algorithm_) {
// 	slowFilter = algorithm_;
// 	return;
// }

// // Add fast filter to the simulator
// void Simulator::AddFastFilter(FilterAlgorithm *algorithm_) {
// 	fastFilter = algorithm_;
// 	return;
// }

// // Add CFD filter to the simulator
// void Simulator::AddCFDFilter(FilterAlgorithm *algorithm_) {
// 	cfdFilter = algorithm_;
// 	return;
// }


// // Add slow picker to the simulator
// void Simulator::AddSlowPicker(Picker *picker_) {
// 	slowPicker = picker_;
// 	return;
// }

// // Add fast picker to the simulator
// void Simulator::AddFastPicker(Picker *picker_) {
// 	fastPicker = picker_;
// 	return;
// }

// // Add slow picker to the simulator
// void Simulator::AddCFDPicker(Picker *picker_) {
// 	cfdPicker = picker_;
// 	return;
// }


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


// // Set fast filter threshold
// void Simulator::SetFastThres(int ft) {
// 	fastThres = ft;
// 	return;
// }

// // Set CFD threshold
// void Simulator::SetCFDThres(int thres) {
// 	cfdThres = thres;
// 	return;
// }

// // Set flags of using cubic CFD
// void Simulator::SetCubicCFD(bool cubic) {
// 	useCubicCFD = cubic;
// 	return;
// }


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

// // find energy in slow filter data
// double Simulator::aveFlatTop(const std::vector<double> &data, int ts, int l, int m) {
// 	int fl = 0;                                             // left and right border of flat top
// 	int fr = 0;
// 	double minEL = 0.0;
// 	double minER = 0.0;
// 	const int diffLen = 8;
// 	for (int i = ts-11+l; i != ts-1+l; ++i) {
// 		double diff = 0.0;
// 		diff += data[i+diffLen]*2 + data[i+diffLen+1] + data[i+diffLen-1];
// 		diff += data[i-diffLen]*2 + data[i-diffLen-1] + data[i+diffLen+1];
// 		diff -= data[i]*4 + data[i-1]*2 + data[i+1]*2;
// 		if (diff < minEL) {
// 			minEL = diff;
// 			fl = i;
// 		}
// 	}
// 	for (int i = ts-11+m; i != ts-1+m; ++i) {
// 		double diff = 0.0;
// 		diff += data[i+diffLen]*2 + data[i+diffLen+1] + data[i+diffLen-1];
// 		diff += data[i-diffLen]*2 + data[i-diffLen-1] + data[i+diffLen+1];
// 		diff -= data[i]*4 + data[i-1]*2 + data[i+1]*2;
// 		if (diff < minER) {
// 			minER = diff;
// 			fr = i;
// 		}
// 	}
// 	return data[fr];
// 	double aveE = 0.0;
// 	for (int i = fl; i != fr; ++i) {
// 		aveE += data[i];
// 	}
// 	aveE /= fr - fl;
// 	return aveE;
// }

// // find timestamp
// int Simulator::timeStamp(const std::vector<double> &data, int thres) {
// 	int ts = 0;
// 	int vsize = data.size();
// 	for (; ts != vsize; ++ts) {
// 		if (data[ts] > thres) break;
// 	}
//     return ts;
// }

// // compute cfd fraction
// double Simulator::linearCfdFraction(const std::vector<double> &data, int thres, short &point) {
// 	bool overThres = false;
// 	size_t vsize = data.size();
// 	for (size_t i = 995; i != vsize-1; ++i) {
// 		if (data[i] > thres) overThres = true;
// 		if (!overThres) continue;
// 		if (data[i] >= 0 && data[i+1] < 0) {
// 			// std::cout << i << ":  " << data[i] << "  " << data[i+1] << std::endl;
// 			point = i;
// 			return data[i] / (data[i]-data[i+1]);
// 		}
// 	}
// 	point = -1;
// 	return 0.0;
// }


// double Simulator::cubicCfdFraction(const std::vector<double> &data, int thres, short &point) {
// 	bool overThres = false;
// 	size_t vsize = data.size();
// 	for (size_t i = 995; i != vsize-1; ++i) {
// 		if (data[i] > thres) overThres = true;
// 		if (!overThres) continue;
// 		if (data[i] >= 0 && data[i+1] < 0) {
// 			point = i;

// 			double y1 = data[i-1];
// 			double y2 = data[i];
// 			double y3 = data[i+1];
// 			double y4 = data[i+2];
// 			double c0 = y2;
// 			double c1 = -y4/6.0 + y3 - y2/2.0 - y1/3.0;
// 			double c2 = y3/2.0 - y2 + y1/2.0;
// 			double c3 = y4/6.0 -y3/2.0 + y2/2.0 - y1/6.0;

// 			// std::cout << i << ":  " << y1 << "  " << y2 << "  " << y3 << "  " << y4 << std::endl;


// 			std::function<double(double)> cubic = [=](double x) {
// 				double t = c3;
// 				t = t * x + c2;
// 				t = t * x + c1;
// 				t = t * x + c0;
// 				return t;
// 			};
// 			// binary search for zero point

// 			double l = 0.0;
// 			double r = 1.0;
// 			double m;
// 			const double eps = 1e-6;
// 			const int loop = 1000;
// 			for (int i = 0; i != loop; ++i) {
// 				m = (l+r)/2.0;
// 				double mv = cubic(m);
// 				if (abs(mv) < eps) {
// 					return m;
// 				}
// 				if (mv < 0) {
// 					r = m;
// 				} else {
// 					l = m;
// 				}
// 			}

// 			return (l+r)/2.0;
// 		}
// 	}
// 	point = -1;
// 	return 0.0;
// }


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
	std::cout << "run   0%";
	std::cout.flush();
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
		if (t % entries100 == 0) {
			std::cout << "\b\b\b\b" << std::setw(3) << t/entries100 << "%";
			std::cout.flush();
		}


	}
	std::cout << "\b\b\b\b100%" << std::endl;

	file->cd();
	if (hEnergy) hEnergy->Write();
	if (hTime) hTime->Write();
	if (hCFD) hCFD->Write();
	if (hCFDP) hCFDP->Write();
	tree->Write();

	auto totalTime = readTime + slowFilterTime + fastFilterTime + cfdFilterTime + pickerTime + otherTime;
	std::cout << "total  " << duration_cast<microseconds>(totalTime).count() << " us" << std::endl;
	std::cout << "read   " << duration_cast<microseconds>(readTime).count() << " us" << std::endl;
	std::cout << "slow   " << duration_cast<microseconds>(slowFilterTime).count() << " us" << std::endl;
	std::cout << "fast   " << duration_cast<microseconds>(fastFilterTime).count() << " us" << std::endl;
	std::cout << "cfd    " << duration_cast<microseconds>(cfdFilterTime).count() << " us" << std::endl;
	std::cout << "pick   " << duration_cast<microseconds>(pickerTime).count() << " us" << std::endl;
	std::cout << "other  " << duration_cast<microseconds>(otherTime).count() << " us" << std::endl;

	return;
}


TTree *TTreeSimulator::Tree() {
	return tree;
}


// //--------------------------------------------------
// //			LGTSimulator
// //--------------------------------------------------

// LGTSimulator::LGTSimulator() {
// 	tree = nullptr;
// 	energy = 0;
// }

// LGTSimulator::~LGTSimulator() {
// }


// void LGTSimulator::SetSL(size_t min, size_t max, size_t step) {
// 	SLMin = min;
// 	SLMax = max;
// 	SLStep = step;
// }

// void LGTSimulator::SetSG(size_t min, size_t max, size_t step) {
// 	SGMin = min;
// 	SGMax = max;
// 	SGStep = step;
// }

// void LGTSimulator::SetST(size_t min, size_t max, size_t step) {
// 	STMin = min;
// 	STMax = max;
// 	STStep = step;
// }


// void LGTSimulator::Run(unsigned int entries, RunFlag flag) {
// 	if (!reader) throw std::runtime_error("Error: Trace reader not found.");
// 	// check slow filter
// 	if ((flag & RunFlag::SlowFilter) != RunFlag::None) {
// 		if (!slowFilter) throw std::runtime_error("Error: slow filter not found.");
// 		if (!slowPicker) throw std::runtime_error("Error: slow picker not found.");
// 	} else {
// 		throw std::runtime_error("Error: this LGTSimulator only run in energy slow filter mode.");
// 	}

// 	if (!path.Length()) throw std::runtime_error("Error: simulation file path is empty.");

// 	for (size_t SL = SLMin; SL <= SLMax; SL += SLStep) {
// 		for (size_t SG = SGMin; SG <= SGMax; SG += SGStep) {
// 			for (size_t ST = STMin; ST <= STMax; ST += STStep) {
// 				TFile *file = new TFile(TString::Format("%sSL%luSG%luST%lu.root", path.Data(), SL, SG, ST));
// 				tree = new TTree("tree", "tree of simulation energy with different filter parameters");
// 				tree->Branch("e", &energy, "energy/s");

// 				// change filter parameters
// 				((SlowFilter*)slowFilter)->SetParameters(SL, SG);

// 				unsigned int entries100 = entries / 100;
// 				std::cout << "SL  " << SL << "  SG  " << SG << "  ST  " << ST << "    run   0%";
// 				std::cout.flush();
// 				reader->Reset();
// 				for (unsigned int t = 0; t != entries; ++t) {
// 					auto &rawData = reader->Read();

// 					auto &slowData = slowFilter->Filter(rawData);

// 					energy = UShort_t(slowPicker->Pick(slowData));
// 					tree->Fill();

// 					if (t % entries100 == 0) {
// 						std::cout << "\b\b\b\b" << std::setw(3) << t / entries100 << "%";
// 						std::cout.flush();
// 					}
// 				}
// 				std::cout << "\b\b\b\b100%" << std::endl;

// 				file->cd();
// 				tree->Write();
// 				file->Close();
// 			}
// 		}
// 	}
// 	return;
// }



// CFDTimeSimulator::CFDTimeSimulator() {
// 	FLMin = 2;
// 	FLMax = 2;
// 	FLStep = 1;
// 	FGMin = 0;
// 	FGMax = 0;
// 	FGStep = 1;
// 	CFDDMin = 1;
// 	CFDDMax = 1;
// 	CFDDStep = 1;
// 	CFDWMin = 0;
// 	CFDWMax = 0;
// 	CFDWStep = 1;
// }

// CFDTimeSimulator::~CFDTimeSimulator() {
// }


// // void CFDTimeSimulator::singleRun(size_t FL, size_t FG, size_t CFDD, size_t CFDW, unsigned int entries_, unsigned int *jentry) {

// // 	TFile *opf = new TFile(TString::Format("%sFL%luFG%luD%luW%lu.root", path.Data(), FL, FG, CFDD, CFDW), "recreate");
// // 	TTree *tree = new TTree("tree", "simulated cfd tree");

// // 	TH1D *hlts = nullptr;
// // 	TH1D *hcfd = nullptr;
// // 	TH1D *hcfdp = nullptr;

// // 	// input data
// // 	Short_t lts, cfdPoint;
// // 	Double_t cfd;
// // 	// branch
// // 	tree->Branch("lts", &lts, "lts/S");
// // 	tree->Branch("cfd", &cfd, "cfd/D");
// // 	tree->Branch("cfdp", &cfdPoint, "cfdp/S");

// // 	// ger period, dt
// // 	int dt = reader->GetPeriod();
// // 	XiaFastFilter singleFastFilter(FL, FG, dt);
// // 	XiaCFDFilter singleCFDFilter(CFDD, CFDW, dt);

// // 	for (*jentry = 0; *jentry != entries_; ++*jentry) {
// // 		// read raw data
// // 		auto &rawData = reader->Read(std::this_thread::get_id());

// // 		// fast filter
// // 		auto &fastData = singleFastFilter.Filter(rawData);

// // 		// calculate timestamp
// // 		int ts = int(fastPicker->Pick(fastData));
// // 		lts = Short_t(ts - zeroPoint);

// // 		if (!hlts) {
// // 			opf->cd();
// // 			hlts = new TH1D("ht", "local time distribution", 200, -100, 100);
// // 		}
// // 		hlts->Fill(lts);

// // 		// cfd filter
// // 		auto &cfdData = singleCFDFilter.Filter(fastData);

// // 		// calcute cfd fraction
// // 		cfd = cfdPicker->Pick(cfdData);
// // 		cfdPoint -= int(cfd) - zeroPoint;
// // 		cfd -= int(cfd);

// // 		if (!hcfd) {
// // 			opf->cd();
// // 			hcfd = new TH1D("hcfd", "cfd distribution", 1000, 0, 1);
// // 		}
// // 		hcfd->Fill(cfd);

// // 		if (!hcfdp) {
// // 			opf->cd();
// // 			hcfdp = new TH1D("hcfdp", "cfd point distribution", 200, -100, 100);
// // 		}
// // 		hcfdp->Fill(cfdPoint);

// // 		tree->Fill();
// // 	}

// // 	opf->cd();
// // 	tree->Write();
// // 	hlts->Write();
// // 	hcfd->Write();
// // 	hcfdp->Write();
// // 	opf->Close();
// // 	return;
// // }

// void CFDTimeSimulator::Run(unsigned int entries_, RunFlag flag_) {
// 	if (!reader) throw std::runtime_error("Error: Trace reader not found.");
// 	// check cfd filter
// 	if ((flag_ & RunFlag::CFDFilter) != 0) {
// 		if (!fastFilter) throw std::runtime_error("Error: Fast filter not found.");
// 		if (!fastPicker) throw std::runtime_error("Error: Fast picker not found.");
// 		if (!cfdFilter) throw std::runtime_error("Error: CFD filter not found.");
// 		if (!cfdPicker) throw std::runtime_error("Error: CFD picker not found.");
// 	} else {
// 		throw std::runtime_error("Error: This simulator only run in cfd filter mode.");
// 	}

// #ifdef MULTI_PROCESS
// 	ROOT::EnableImplicitMT(true);
// 	int totalTasks = (FLMax - FLMin) / FLStep + 1;
// 	totalTasks *= (FGMax - FGMin) / FGStep + 1;
// 	totalTasks *= (CFDDMax - CFDDMin) / CFDDStep + 1;
// 	totalTasks *= (CFDWMax - CFDWMin) / CFDWStep + 1;

// 	struct paramSettings {
// 		size_t FL;
// 		size_t FG;
// 		size_t CFDD;
// 		unsigned int CFDW;
// 	};
// 	std::vector<paramSettings> parSets;
// 	for (size_t FL = FLMin; FL <= FLMax; FL += FLStep) {
// 		for (size_t FG = FGMin; FG <= FGMax; FG += FGStep) {
// 			for (size_t CFDD = CFDDMin; CFDD <= CFDDMax; CFDD += CFDDStep) {
// 				for (unsigned int CFDW = CFDWMin; CFDW <= CFDWMax; CFDW += CFDWStep) {
// 					parSets.push_back(paramSettings{FL, FG, CFDD, CFDW});
// 				}
// 			}
// 		}
// 	}

// 	const int process = 7;
// 	pid_t pids[process];
// 	int aveTask = (totalTasks-1)/process + 1;
// 	int remainTask = totalTasks % process;

// 	std::cout << "total " << totalTasks << "  ave " << aveTask << "  remain  " << remainTask << std::endl;
// 	for (int i = 0; i != process; ++i) {
// 		int start = aveTask*i;
// 		start -= i <= remainTask ? 0 : i-remainTask;
// 		int ends = aveTask*(i+1);
// 		ends -= (i+1) <= remainTask ? 0 : i+1-remainTask;
// 		std::cout << i << "  " << start << "  " << ends << std::endl;
// 	}

// 	// fork childs
// 	for (int i = 0; i != process; ++i) {
// 		pids[i] = fork();
// 		if (pids[i] < 0) {
// 			std::cerr << "fork: " << strerror(errno) << std::endl;
// 			abort();
// 		} else if (pids[i] == 0) {
// 			int start = aveTask*i;
// 			start -= i <= remainTask ? 0 : i-remainTask;
// 			int ends = aveTask*(i+1);
// 			ends -= (i+1) <= remainTask ? 0 : i+1-remainTask;
// 			for (int j = start; j != ends; ++j) {
// 				size_t FL = parSets[j].FL;
// 				size_t FG = parSets[j].FG;
// 				size_t CFDD = parSets[j].CFDD;
// 				unsigned int CFDW = parSets[j].CFDW;
// 				((XiaFastFilter*)fastFilter)->SetParameters(FL, FG);
// 				((XiaCFDFilter*)cfdFilter)->SetParameters(CFDD, CFDW);

// 				TFile *opf = new TFile(TString::Format("%sFL%luFG%luD%luW%u.root", path.Data(), FL, FG, CFDD, CFDW), "recreate");
// 				TTree *tree = new TTree("tree", "simulated cfd tree");
// 				TH1D *hlts = nullptr;
// 				TH1D *hcfd = nullptr;
// 				TH1D *hcfdp = nullptr;

// 				// input data
// 				Short_t lts, cfdPoint;
// 				Double_t cfd;
// 				// branch
// 				tree->Branch("lts", &lts, "lts/S");
// 				tree->Branch("cfd", &cfd, "cfd/D");
// 				tree->Branch("cfdp", &cfdPoint, "cfdp/S");

// 				reader->Reset();
// 				for (int jentry = 0; jentry != entries_; ++jentry) {
// 					// read raw data
// 					auto &rawData = reader->Read();
// 					// fast filter
// 					auto &fastData = fastFilter->Filter(rawData);

// 					// calculate timestamp
// 					int ts = int(fastPicker->Pick(fastData));
// 					lts = Short_t(ts - zeroPoint);

// 					if (!hlts) hlts = new TH1D("ht", "local time distribution", 200, -100, 100);
// 					hlts->Fill(lts);


// 					// cfd filter
// 					auto &cfdData = cfdFilter->Filter(fastData);

// 					// calcute cfd fraction
// 					cfd = cfdPicker->Pick(cfdData);
// 					cfdPoint = UShort_t(cfd) - zeroPoint;
// 					cfd -= int(cfd);

// 					if (!hcfd) hcfd = new TH1D("hcfd", "cfd distribution", 1000, 0, 1);
// 					hcfd->Fill(cfd);

// 					if (!hcfdp) hcfdp = new TH1D("hcfdp", "cfd point distribution", 200, -100, 100);
// 					hcfdp->Fill(cfdPoint);

// 					tree->Fill();

// 				}

// 				tree->Write();
// 				hlts->Write();
// 				hcfd->Write();
// 				hcfdp->Write();
// 				opf->Close();
// 			}
// 			exit(0);
// 		}
// 	}

// 	int status;
// 	pid_t pid;
// 	int n = process;
// 	while (n > 0) {
// 		pid = wait(&status);
// 		std::cout << "Child with pid " << pid << " exit with status " << status << "." << std::endl;
// 		--n;
// 	}



// #else					// single thread
// 	for (size_t FL = FLMin; FL <= FLMax; FL += FLStep) {
// 		for (size_t FG = FGMin; FG <= FGMax; FG += FGStep) {
// 			for (size_t CFDD = CFDDMin; CFDD <= CFDDMax; CFDD += CFDDStep) {
// 				for (unsigned int CFDW = CFDWMin; CFDW <= CFDWMax; CFDW += CFDWStep) {
// 					((XiaFastFilter*)fastFilter)->SetParameters(FL, FG);
// 					((XiaCFDFilter*)cfdFilter)->SetParameters(CFDD, CFDW);

// 					TFile *opf = new TFile(TString::Format("%sFL%luFG%luD%luW%u.root", path.Data(), FL, FG, CFDD, CFDW), "recreate");
// 					TTree *tree = new TTree("tree", "simulated cfd tree");
// 					TH1D *hlts = nullptr;
// 					TH1D *hcfd = nullptr;
// 					TH1D *hcfdp = nullptr;

// 					// input data
// 					Short_t lts, cfdPoint;
// 					Double_t cfd;
// 					// branch
// 					tree->Branch("lts", &lts, "lts/S");
// 					tree->Branch("cfd", &cfd, "cfd/D");
// 					tree->Branch("cfdp", &cfdPoint, "cfdp/S");

// 					std::cout << "FL  " << FL << "  FG  " << FG << "  CFDD  " << CFDD << "  CFDW  " << CFDW;
// 					std::cout << "    " << std::setw(3) << 0 << "%";
// 					std::cout.flush();
// 					int entries100 = entries_ / 100;
// 					reader->Reset();
// 					for (unsigned int jentry = 0; jentry != entries_; ++jentry) {
// 						// read raw data
// 						auto &rawData = reader->Read();
// 						// fast filter
// 						auto &fastData = fastFilter->Filter(rawData);

// 						// calculate timestamp
// 						int ts = int(fastPicker->Pick(fastData));
// 						lts = Short_t(ts - zeroPoint);

// 						if (!hlts) hlts = new TH1D("ht", "local time distribution", 200, -100, 100);
// 						hlts->Fill(lts);


// 						// cfd filter
// 						auto &cfdData = cfdFilter->Filter(fastData);

// 						// calcute cfd fraction
// 						cfd = cfdPicker->Pick(cfdData);
// 						cfdPoint = UShort_t(cfd) - zeroPoint;
// 						cfd -= int(cfd);

// 						if (!hcfd) hcfd = new TH1D("hcfd", "cfd distribution", 1000, 0, 1);
// 						hcfd->Fill(cfd);

// 						if (!hcfdp) hcfdp = new TH1D("hcfdp", "cfd point distribution", 200, -100, 100);
// 						hcfdp->Fill(cfdPoint);

// 						tree->Fill();


// 						if (jentry % entries100 == 0) {
// 							std::cout << "\b\b\b\b" << std::setw(3) << jentry / entries100 << "%";
// 							std::cout.flush();
// 						}
// 					}
// 					std::cout << "\b\b\b\b100%" << std::endl;

// 					tree->Write();
// 					hlts->Write();
// 					hcfd->Write();
// 					hcfdp->Write();
// 					opf->Close();
// 				}
// 			}
// 		}
// 	}

// #endif
// }



// void CFDTimeSimulator::SetFL(size_t min, size_t max, size_t step) {
// 	FLMin = min;
// 	FLMax = max;
// 	FLStep = step;
// 	return;
// }


// void CFDTimeSimulator::SetFG(size_t min, size_t max, size_t step) {
// 	FGMin = min;
// 	FGMax = max;
// 	FGStep = step;
// 	return;
// }

// void CFDTimeSimulator::SetCFDD(size_t min, size_t max, size_t step) {
// 	CFDDMin = min;
// 	CFDDMax = max;
// 	CFDDStep = step;
// 	return;
// }


// void CFDTimeSimulator::SetCFDW(unsigned int min, unsigned int max, unsigned int step) {
// 	CFDWMin = min;
// 	CFDWMax = max;
// 	CFDWStep = step;
// 	return;
// }
