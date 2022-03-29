#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <fstream>

#include "TFile.h"
#include "TTree.h"

#include "../lib/json.hpp"

int main(int argc, char **argv) {
	if (argc != 2) {
		std::cout << "./Usage: " << argv[0] << "  [file]" << std::endl;
		std::cout << "    [file]          Set the config file." << std::endl;
		return -1;
	}

	std::ifstream configFile(argv[1]);
	if (!configFile.good()) {
		std::cerr << "Error open config file " << argv[1] << std::endl;
		return -2;
	}
	nlohmann::json js;
	configFile >> js;
	configFile.close();

	std::string tracePath = js["TracePath"];
	std::string traceFile = js["TraceFile"];
	std::string traceFileName = tracePath + traceFile;
	std::string sepPath = js["SepPath"];
	std::string sepFile = js["SepFile"];
	std::string sepFileName = sepPath + sepFile;

	TFile *ipf = new TFile(traceFileName.c_str(), "read");
	if (!ipf) {
		std::cerr << "Error open file " << traceFileName << std::endl;
	}
	TTree *ipt = (TTree*)ipf->Get("tree");

	// input data
	Long64_t ts;
	UShort_t energy, strip, side;


	ipt->SetBranchAddress("ts", &ts);
	ipt->SetBranchAddress("e", &energy);
	ipt->SetBranchAddress("s", &strip);
	ipt->SetBranchAddress("side", &side);


	// output
	TFile *opf = new TFile(sepFileName.c_str(), "recreate");
	TTree *opt = new TTree("tree", "tree without trace");
	opt->Branch("ts", &ts, "ts/L");
	opt->Branch("e", &energy, "energy/s");
	opt->Branch("s", &strip, "strip/s");
	opt->Branch("side", &side, "side/s");

	std::cout << "Filling new tree   0%";
	std::cout.flush();
	Long64_t nentry = ipt->GetEntries();
	Long64_t nentry100 = nentry / 100 + 1;
	for (Long64_t jentry = 0; jentry != nentry; ++jentry) {
		ipt->GetEntry(jentry);
		opt->Fill();

		if (jentry % nentry100 == 0) {
			std::cout << "\b\b\b\b" << std::setw(3) << jentry / nentry100 << "%";
			std::cout.flush();
		}
	}
	std::cout << "\b\b\b\b100%" << std::endl;

	opt->Write();
	opf->Close();
	ipf->Close();

	return 0;
}