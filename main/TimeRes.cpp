#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <mutex>
#include <thread>
#include <map>
#include <fstream>

#include "TFile.h"
#include "TTree.h"
#include "TCut.h"
#include "TF1.h"
#include "TH1.h"
#include "TString.h"

#include "../lib/json.hpp"


// task status
const int StatusError = -1;		// error while running
const int StatusInitial = 0;	// init, prepare for run
const int StatusRunning = 1;	// running
const int StatusFinished = 2;	// finished, but not check by main thread
const int StatusChecked = 3;	// finished and checked by main thread

std::mutex statusLock;			// status lock
std::mutex resultLock;

std::map<std::string, double> result;

void SingleFileTimeRes(std::string inputFile, std::string outputFile, int *status = nullptr) {
	if (status) {
		statusLock.lock();
		*status = StatusRunning;
		statusLock.unlock();
	}

	// input file and tree
	TFile *ipf = new TFile(inputFile.c_str(), "read");
	if (!ipf) {
		std::cerr << "Error open file " << inputFile << "." << std::endl;
		if (status) {
			statusLock.lock();
			*status = StatusError;
			statusLock.unlock();
		}
		return;
	}
	TTree *tree = (TTree*)ipf->Get("tree");

	// output file
	TFile *opf = new TFile(outputFile.c_str(), "recreate");
	if (!opf) {
		std::cerr << "Error recreate file " << outputFile << "." << std::endl;
		if (status) {
			statusLock.lock();
			*status = StatusError;
			statusLock.unlock();
		}
		return;
	}
	// fit 1
	TCut stripCtCut = "fct!=0 && bct != 0";
	tree->Draw("ft-bt+fct-bct+fcp-bcp>>hfbt1(200, -100, 100)", stripCtCut);
	TH1 *hfbt1 = (TH1*)gDirectory->Get("hfbt1");
	TF1 *f1 = new TF1("f1", "gaus", -100, 100);
	hfbt1->Fit(f1, "RQS+");

	// fit 2
	double mean = f1->GetParameter(1);
	double sigma = f1->GetParameter(2);
	const int range = 4;
	double left = mean - range*sigma;
	double right = mean + range*sigma;
	TString expStr;
	expStr.Form("ft-bt+fct-bct+fcp-bcp>>hfbt2(%d, %d, %d)", int(range*sigma*5), int(left), int(right));
	tree->Draw(expStr.Data(), stripCtCut);
	TH1 *hfbt2 = (TH1*)gDirectory->Get("hfbt2");
	TF1 *f2 = new TF1("f2", "gaus", int(mean-3*sigma), int(mean+3*sigma));
	hfbt2->Fit(f2, "RQS+");

	// fit 3
	mean = f2->GetParameter(1);
	sigma = f2->GetParameter(2);
	left = mean - 3*sigma;
	right = mean + 3*sigma;
	expStr.Form("ft-bt+fct-bct+fcp-bcp>>hfbt3(%d, %d, %d)", int(3*sigma*5), int(left), int(right));
	tree->Draw(expStr.Data(), stripCtCut);
	TH1 *hfbt3 = (TH1*)gDirectory->Get("hfbt3");
	TF1 *f3 = new TF1("f3", "gaus", int(mean-3*sigma), int(mean+3*sigma));
	hfbt3->Fit(f3, "RQS+");

	opf->cd();
	hfbt1->Write();
	hfbt2->Write();
	hfbt3->Write();
	opf->Close();
	ipf->Close();

	double res = f3->GetParameter(2)*2.35;
	resultLock.lock();
	result.emplace(inputFile.substr(inputFile.find_last_of('/')+1), res);
	resultLock.unlock();

	if (status) {
		statusLock.lock();
		*status = StatusFinished;
		statusLock.unlock();
	}

	return;
}


int main(int argc, char **argv) {
	if (argc != 2) {
		std::cout << "Usage: " << argv[0] << "  [file]" << std::endl;
		std::cout << "  file          Set the config file path, necessary." << std::endl;
		return -1;
	}

	std::ifstream configFile(argv[1]);
	if (!configFile.good()) {
		std::cerr << "Error open config file " << argv[1] << "." << std::endl;
	}
	nlohmann::json js;
	configFile >> js;
	configFile.close();

	std::string singlePath = js["SinglePath"];
	std::string resPath = js["TimeResPath"];

#ifdef MULTI_THREAD


#else
	for (const auto &entry : std::filesystem::directory_iterator(singlePath)) {
		if (entry.is_directory()) continue;
		auto &path = entry.path();
		std::string inputFileName = std::string(path);
		std::string outputFileName = resPath + std::string(path.filename());

		SingleFileTimeRes(inputFileName, outputFileName);
	}
#endif

	// std::cout << std::left << std::setw(8) << "FL" << std::setw(8) << "FG" << std::setw(8) << "CFDD" << std::setw(8) << "CFDW" << std::setw(8) << "result" << std::endl;
	// size_t index = 0;
	// for (int FL = FLMin; FL <= FLMax; FL += FLStep) {
	// 	for (int FG = FGMin; FG <= FGMax; FG += FGStep) {
	// 		for (int CFDD = CFDDMin; CFDD <= CFDDMax; CFDD += CFDDStep) {
	// 			for (int CFDW = CFDWMin; CFDW <= CFDWMax; CFDW += CFDWStep) {
	// 				std::cout << std::setw(8) << FL << std::setw(8) << FG << std::setw(8) << CFDD << std::setw(8) << CFDW << std::setw(8) << std::setprecision(2) << std::fixed << result[index] << std::endl;
	// 				++index;
	// 			}
	// 		}
	// 	}
	// }
	for (const auto &[file, res] : result) {
		std::cout << file << "  " << std::setprecision(2) << std::fixed << res << std::endl;
	}

	return 0;
}
