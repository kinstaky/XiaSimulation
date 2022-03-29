#include "Adapter.h"
#include "../lib/json.hpp"

#include <cstring>
#include <iostream>
#include <string>
#include <fstream>

void printUsage(const char *name) {
	std::cout << "Usage: " << name << " [options] [file]" << std::endl;
	std::cout << "    file          Set the config file." << std::endl;
	std::cout << std::endl;
	std::cout << "  options:" << std::endl;
	std::cout << "    -h            Print this help information." << std::endl;
	std::cout << "    -v            Print the version information." << std::endl;
	std::cout << std::endl;
	std::cout << "  Produced by pwl." << std::endl;
	return;
}

void printVersion() {
	std::cout << "adapt version 1.0" << std::endl;
	return;
}

int main(int argc, char **argv) {
	if (argc != 2) {
		printUsage(argv[0]);
		return -1;
	}
	if (argv[1][0] == '-'){
		switch (argv[1][1]) {
			case 'h':
				printUsage(argv[0]);
				return 0;
			case 'v':
				printVersion();
				return 0;
			default:
				std::cerr << "Error: unknown option " << argv[1][1] << "." << std::endl;
				return -1;
		}
	}

	// read path and parameters from config.json
	std::ifstream configFile(argv[1]);
	if (!configFile.good()) {
		std::cerr << "Error open file " << argv[1] << std::endl;
		exit(-1);
	}
	nlohmann::json js;
	configFile >> js;
	configFile.close();

	std::string RawPath = js["RawPath"];
	std::string RawFile = js["RawFile"];
	std::string TracePath = js["TracePath"];
	std::string TraceFile = js["TraceFile"];
	unsigned int rate = js["SamplingRate"];
	size_t traceStart = js["TraceStart"];
	size_t traceLength = js["TraceLength"];
	Long64_t entries = js["Entries"];

	std::string rawFileName = std::string(RawPath) + std::string(RawFile);
	TFile *ipf = new TFile(rawFileName.c_str(), "read");
	std::cout << "read " << rawFileName << std::endl;
	if (!ipf) {
		std::cerr << "open file error" << std::endl;
		return -2;
	}
	TTree *ipt = (TTree*)ipf->Get("tree");
	if (!ipt) {
		std::cerr << "get tree error" << std::endl;
		return -2;
	}

	std::string traceFileName = std::string(TracePath) + std::string(TraceFile);
	Adapter *adapter;
	if (rate == 100) adapter = new Pixie100MAdapter(ipt, traceFileName.c_str(), 12500);
	else if (rate == 250) adapter = new Pixie250MAdapter(ipt, traceFileName.c_str(), 12500);
	else {
		std::cerr << "Sampling rate not supported: " << rate << "M." << std::endl;
		return -2;
	}
	adapter->Read(entries, traceStart, traceLength);
	std::cout << "write " << traceFileName << std::endl;
	adapter->Write();

	delete adapter;

	ipf->Close();
	return 0;
}
