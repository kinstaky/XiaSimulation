#include <vector>
#include <iostream>
#include <iomanip>
#include <vector>
#include <filesystem>
#include <fstream>
#include <thread>
#include <mutex>

#include "TFile.h"
#include "TTree.h"
#include "TSpectrum.h"
#include "TH1.h"
#include "TPolyMarker.h"
#include "TF1.h"
#include "TCut.h"

#include "../lib/json.hpp"
#include "../lib/ThreadPool.h"

int fbw = 50;					// front back width
int sw = 200;					// same side width
bool verbose = false;
bool multiThread = false;

// task status
const int StatusError = -1;		// error while running
const int StatusInitial = 0;	// init, prepare for run
const int StatusRunning = 1;	// running
const int StatusFinished = 2;	// finished, but not check by main thread
const int StatusChecked = 3;	// finished and checked by main thread

std::mutex statusLock;			// status lock

struct Event {
	Double_t energy;
	UShort_t strip;
	Short_t lts;
	Double_t cfd;
	Short_t cfdp;
	bool used;
};

int stripCount[2] = {16, 16};

double xPeaks[32][2];
double yPeaks[32][2];
double xParams[32][2];
double yParams[32][2];


void NormSingle(int xStrip_, int yStrip_, bool direction) {
	if (direction) {					// from x to y
		double nxp0 = xParams[xStrip_][0] + xParams[xStrip_][1] * xPeaks[xStrip_][0];				// normalized x peak 0
		double nxp1 = xParams[xStrip_][0] + xParams[xStrip_][1] * xPeaks[xStrip_][1];				// normalized x peak 1
		yParams[yStrip_][1] = (nxp0 - nxp1) / (yPeaks[yStrip_][0] - yPeaks[yStrip_][1]);
		yParams[yStrip_][0] = nxp0 - yParams[yStrip_][1] * yPeaks[yStrip_][0];
	} else {
		double nyp0 = yParams[yStrip_][0] + yParams[yStrip_][1] * yPeaks[yStrip_][0];				// normalized y peak 0
		double nyp1 = yParams[yStrip_][0] + yParams[yStrip_][1] * yPeaks[yStrip_][1];				// normalized y peak 1
		xParams[xStrip_][1] = (nyp0 - nyp1) / (xPeaks[xStrip_][0] - xPeaks[xStrip_][1]);
		xParams[xStrip_][0] = nyp0 - xParams[xStrip_][1] * xPeaks[xStrip_][0];
	}
	return;
}


void printMap(const std::multimap<Long64_t, Event> &events) {
	int i = 0;
	std::cout << "time    energy     strip    lts    cfd    cfdp" << std::endl;
	for (auto ievent = events.begin(); ievent != events.end(); ++ievent) {
		std::cout << ievent->first << "  " << "  " << ievent->second.energy << "  " << ievent->second.strip;
		std::cout << "  " << ievent->second.lts << "  " << ievent->second.cfd << "  " << ievent->second.cfdp << std::endl;
		++i;
		if (i==10) break;
	}
}

void SingleFileSingleHit(const std::string &inputFile, const std::string &sepFile, const std::string &outputFile, int *status = nullptr) {
	if (status) {
		statusLock.lock();
		*status = StatusRunning;				// running
		statusLock.unlock();
	}
	TFile *ipf = new TFile(inputFile.c_str(), "read");
	if (!ipf) {
		std::cerr << "Error open file " << inputFile << std::endl;
		if (status) {
			statusLock.lock();
			*status = StatusError;
			statusLock.unlock();
		}
		return;
	}
	TTree *ipt = (TTree*)ipf->Get("tree");

	ipt->AddFriend("st=tree", sepFile.c_str());

	// output file
	TFile *opf = new TFile(outputFile.c_str(), "recreate");
	if (!opf) {
		std::cerr << "Error recreate file " << outputFile << std::endl;
		if (status) {
			statusLock.lock();
			*status = StatusError;
			statusLock.unlock();
		}
		return;
	}

	// normal
	TSpectrum *spectrum = new TSpectrum(10);
	opf->cd();
	for (int side = 0; side != 2; ++side) {
		for (int strip = 0; strip != stripCount[side]; ++strip) {
			TString hName;
			hName.Form("he%c%d", "xy"[side], strip);
			ipt->Draw("st.energy>>"+hName+"(200, 2200, 3200)", TString::Format("st.side==%d && st.strip==%d", side, strip));
			TH1 *hesi = (TH1*)gDirectory->Get(hName);

			// use TSpectru to search peaks and save in the spectrum sub directory
			spectrum->Search(hesi, 8, "", 0.2);
			Double_t *peaksX = spectrum->GetPositionX();
			TPolyMarker *pm = (TPolyMarker*)hesi->GetListOfFunctions()->FindObject("TPolyMarker");
			pm->SetMarkerStyle(32);
			pm->SetMarkerColor(kGreen);
			pm->SetMarkerSize(7);

			// fit
			TF1 *PuFit = new TF1(TString::Format("PuFit%d", strip), "gaus", peaksX[0]-20, peaksX[0]+30);
			hesi->Fit(PuFit, "RQS+");
			Double_t meanPu = PuFit->GetParameter(1);
			// Double_t sigmaPu = PuFit->GetParameter(2);
			// Double_t resPu = sigmaPu / meanPu * 2.355;

			TF1 *AmFit = new TF1(TString::Format("AmFit%d", strip), "gaus", peaksX[1]-25, peaksX[1]+35);
			hesi->Fit(AmFit, "RQS+");
			Double_t meanAm = AmFit->GetParameter(1);
			// Double_t sigmaAm = AmFit->GetParameter(2);
			// Double_t resAm = sigmaAm / meanAm * 2.355;
			if (side == 0) {
				xPeaks[strip][0] = meanPu;
				xPeaks[strip][1] = meanAm;
			} else {
				yPeaks[strip][0] = meanPu;
				yPeaks[strip][1] = meanAm;
			}

			hesi->Write(hName.Data());
		}
	}

	if (verbose) {
		std::cout << "----------x peaks----------" << std::endl;
		for (int i = 0; i != stripCount[0]; ++i) {
			std::cout << i << "  " << xPeaks[i][0] << "  " << xPeaks[i][1] << std::endl;
		}
		std::cout << "----------y peaks----------" << std::endl;
		for (int i = 0; i != stripCount[1]; ++i) {
			std::cout << i << "  " << yPeaks[i][0] << "  " << yPeaks[i][1] << std::endl;
		}

	}

	// normalizing
	for (int i = 0; i != 32; ++i) {
		xParams[i][0] = 0.0;
		xParams[i][1] = 1.0;
		yParams[i][0] = 0.0;
		yParams[i][1] = 1.0;
	}

	// from x to y
	for (int i = 0; i != stripCount[1]; ++i) {
		NormSingle(0, i, true);
	}
	for (int i = 1; i != stripCount[0]; ++i) {
		NormSingle(i, 0, false);
	}

	if (verbose) {
		std::cout << "----------x parameters----------" << std::endl;
		for (int i = 0; i != stripCount[0]; ++i) {
			std::cout << i << "  " << xParams[i][0] << "  " << xParams[i][1] << std::endl;
		}
		std::cout << "----------y parameters----------" << std::endl;
		for (int i = 0; i != stripCount[1]; ++i) {
			std::cout << i << "  " << yParams[i][0] << "  " << yParams[i][1] << std::endl;
		}
	}

	// input and output data
	Long64_t ts;
	UShort_t energy, strip, side;
	Short_t lts, cfdp;
	Double_t cfd;


	ipt->SetBranchAddress("st.ts", &ts);
	ipt->SetBranchAddress("st.e", &energy);
	ipt->SetBranchAddress("st.side", &side);
	ipt->SetBranchAddress("st.s", &strip);
	ipt->SetBranchAddress("lts", &lts);
	ipt->SetBranchAddress("cfd", &cfd);
	ipt->SetBranchAddress("cfdp", &cfdp);

	std::multimap<Long64_t, Event> frontEvents;
	std::multimap<Long64_t, Event> backEvents;

	// loop and fill the map
	if (verbose) {
		printf("Filling map   0%%");
		fflush(stdout);
	}
	Long64_t nentry = ipt->GetEntries();
	Long64_t nentry100 = nentry / 100 + 1;
	for (Long64_t jentry = 0; jentry != nentry; ++jentry) {
		ipt->GetEntry(jentry);

		Long64_t nts = ts * 10;
		Short_t nlts = lts * 10;
		Double_t ncfd = cfd * 10.0;
		Short_t ncfdp = cfdp * 10;

		if (side == 0) {
			Double_t ne = xParams[strip][0] + xParams[strip][1]*energy;
			frontEvents.insert(std::make_pair(nts, Event{ne, strip, nlts, ncfd, ncfdp, false}));
		} else {
			Double_t ne = yParams[strip][0] + yParams[strip][1]*energy;
			backEvents.insert(std::make_pair(nts, Event{ne, strip, nlts, ncfd, ncfdp, false}));
		}

		if (verbose && (jentry % nentry100 == 0)) {
			printf("\b\b\b\b%3lld%%", jentry / nentry100);
			fflush(stdout);
		}
	}
	if (verbose) {
		printf("\b\b\b\b100%%\n");

		printMap(frontEvents);
		printMap(backEvents);
	}

	opf->cd();
	TTree *opt = new TTree("tree", "single hit tree");

	Double_t fe, be;		// energy
	UShort_t fs, bs;		// strip
	Long64_t ft, bt;		// timestamp, ns
	Short_t flts, blts;		// simulated local timestamp, ns
	Double_t fct, bct;		// simulated cfd time, ns
	Short_t fcp, bcp;		// simulated cfd point, ns

	opt->Branch("ft", &ft, "ft/L");
	opt->Branch("fe", &fe, "fe/D");
	opt->Branch("fs", &fs, "fs/s");
	opt->Branch("flts", &flts, "flts/S");
	opt->Branch("fct", &fct, "fct/D");
	opt->Branch("fcp", &fcp, "fcp/S");
	opt->Branch("bt", &bt, "bt/L");
	opt->Branch("be", &be, "be/D");
	opt->Branch("bs", &bs, "bs/s");
	opt->Branch("blts", &blts, "blts/S");
	opt->Branch("bct", &bct, "bct/D");
	opt->Branch("bcp", &bcp, "bcp/S");

	if (verbose) {
		printf("Filling new tree   0%%");
		fflush(stdout);
	}
	Long64_t jentry = 0;
	nentry = frontEvents.size();
	nentry100 = nentry / 100 + 1;
	nentry = 0;
	for (auto ievent = frontEvents.begin(); ievent != frontEvents.end(); ++ievent) {
		if (ievent->second.used) continue;
		ft = ievent->first;
		fe = ievent->second.energy;
		fs = ievent->second.strip;
		flts = ievent->second.lts;
		fct = ievent->second.cfd;
		fcp = ievent->second.cfdp;
		bool frontSingle = true;

		// check front single hit
		for (auto jevent = frontEvents.lower_bound(ievent->first); jevent != frontEvents.upper_bound(ievent->first+sw); ++jevent) {
			if (jevent == ievent) continue;
			if (jevent->second.used) continue;
			jevent->second.used = true;
			frontSingle = false;
		}

		bool backSingle = false;
		bool backHit = false;
		for (auto jevent = backEvents.lower_bound(ievent->first-fbw); jevent != backEvents.upper_bound(ievent->first+fbw); ++jevent) {
			if (jevent->second.used) continue;
			if (backSingle) {
				backSingle = false;
				continue;
			}
			bt = jevent->first;
			be = jevent->second.energy;
			bs = jevent->second.strip;
			blts = jevent->second.lts;
			bct = jevent->second.cfd;
			bcp = jevent->second.cfdp;

			if (!backHit) backSingle = true;
			backHit = true;
		}

		if (backSingle && frontSingle) {
			opt->Fill();
			++nentry;
		}
		if (!backHit) {
			ievent->second.used = false;
			// status turn back
			for (auto jevent = frontEvents.lower_bound(ievent->first); jevent != frontEvents.upper_bound(ievent->first+sw); ++jevent) {
				jevent->second.used = false;
			}
		}

		jentry++;
		if (verbose && (jentry % nentry100 == 0)) {
			printf("\b\b\b\b%3lld%%", jentry / nentry100);
			fflush(stdout);
		}
	}
	if (verbose) {
		printf("\b\b\b\b100%%\n");

		std::cout << "front one hit rate " << nentry << "/" << frontEvents.size() << "   " << double(nentry)/double(frontEvents.size()) << std::endl;
		std::cout << "back one hit rate " << nentry << "/" << backEvents.size() << "   " << double(nentry)/double(backEvents.size()) << std::endl;
	}


	opt->Write();
	opf->Close();
	ipf->Close();

	if (status) {
		statusLock.lock();
		*status = StatusFinished;
		statusLock.unlock();
	}

	return;
}



int main(int argc, char **argv) {
	if (argc > 2 || argc < 2) {
		std::cout << "Usage: " << argv[0] << "  [file]" << std::endl;
		std::cout << "    file          Set the config file path, necessary." << std::endl;
		return -1;
	}


	std::ifstream configFile(argv[1]);
	if (!configFile.good()) {
		std::cerr << "Error open file " << argv[1] << std::endl;
		exit(-1);
	}
	nlohmann::json js;
	configFile >> js;
	configFile.close();


	std::string simPath = js["SimPath"];
	std::string singlePath = js["SinglePath"];
	std::string sepPath = js["SepPath"];
	std::string sepFile = js["SepFile"];
	std::string sepFileName = sepPath + sepFile;
	stripCount[0] = js["Strips"][0];
	stripCount[1] = js["Strips"][1];
	fbw = js["fbw"];
	sw = js["sw"];
	verbose = js["Verbose"];
	multiThread = js["MultiThread"];

	if (multiThread) {
		ThreadPool pool(js["Threads"]);
		// count of files
		unsigned int totalTasks = 0;
		for (const auto &entry : std::filesystem::directory_iterator(simPath)) {
			if (entry.is_directory()) continue;
			++totalTasks;
		}
		int *taskStatus = new int[totalTasks];
		for (size_t i = 0; i != totalTasks; ++i) {
			taskStatus[i] = StatusInitial;
		}
		std::vector<std::string> fileNames;
		size_t index = 0;
		for (const auto &entry : std::filesystem::directory_iterator(simPath)) {
			if (entry.is_directory()) continue;
			auto &path = entry.path();
			std::string inputFileName = std::string(path);
			std::string outputFileName = singlePath + std::string(path.filename());
			fileNames.emplace_back(path.filename());
			pool.enqueue(SingleFileSingleHit, inputFileName, sepFileName, outputFileName, taskStatus+index);
			++index;
		}

		unsigned int checkTasks = 0;
		while (checkTasks < totalTasks) {
			statusLock.lock();
			index = 0;
			for (size_t i = 0; i != totalTasks; ++i) {
				if (taskStatus[i] == StatusFinished) {
					taskStatus[i] = StatusChecked;
					++checkTasks;
					std::cout << "[" << checkTasks << "/" << totalTasks << "]" << "  " << fileNames[index] << "  finished." << std::endl;
				} else if (taskStatus[i] == StatusError) {
					taskStatus[i] = StatusChecked;
					++checkTasks;
					std::cout << "[" << checkTasks << "/" << totalTasks << "]" << "  " << fileNames[index] << "  ERROR!!!" << std::endl;
				}
				++index;
			}
			statusLock.unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds(300));
		}


		delete[] taskStatus;

	} else {

		for (const auto &entry : std::filesystem::directory_iterator(simPath)) {
			if (entry.is_directory()) continue;
			auto &path = entry.path();
			std::string inputFileName = std::string(path);
			std::string outputFileName = singlePath + std::string(path.filename());

			SingleFileSingleHit(inputFileName, sepFileName, outputFileName);
		}
	}


	return 0;
}