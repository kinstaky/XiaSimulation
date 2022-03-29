#include <thread>
#include <fstream>
#include <memory>

#include "TF1.h"

#include "Simulator.h"
#include "../lib/json.hpp"
#include "../lib/ThreadPool.h"

typedef Simulator::RunFlag rFlag;
const rFlag energyRun = rFlag::SlowFilter;
const rFlag timeRun = rFlag::FastFilter;
const rFlag cfdRun = rFlag::CFDFilter;
const rFlag allRun = energyRun | timeRun | cfdRun;

// A * (exp(-t/tau) - exp(-t/theta))
Double_t ExpDecay(Double_t *x, Double_t *par) {
	Double_t t = x[0];
	Double_t t0 = par[3];
	if (t < t0) return 0.0;

	Double_t A = par[0];
	Double_t tau = par[1];
	Double_t theta = par[2];

	t -= t0;
	Double_t ret = A * (exp(-t/tau) - exp(-t/theta));
	return ret;
}


// void ExpDecayMWDSim() {
// 	// Reader
// 	TF1 f1("ExpDecay", ExpDecay, 0, 20000, 4);
// 	f1.SetParameter(0, 1000.0);
// 	f1.SetParameter(1, 10000.0);
// 	f1.SetParameter(2, 30.0);
// 	f1.SetParameter(3, 9860.0);
// 	FunctionTraceReader reader(&f1, 10, 20000);

// 	// Filter
// 	MWDAlgorithm slowFilter(2000, 2000, 10000, 10);
// 	XiaFastFilter fastFilter(200, 0, 10);

// 	// Picker
// 	TrapezoidTopPicker slowPicker(1000, 200, 200);
// 	LeadingEdgePicker fastPicker(50);

// 	BaseSimulator simulator;
// 	simulator.AddReader(&reader);
// 	simulator.AddSlowFilter(&slowFilter);
// 	simulator.AddFastFilter(&fastFilter);
// 	simulator.AddSlowPicker(&slowPicker);
// 	simulator.AddFastPicker(&fastPicker);

// 	simulator.SetPath("/data/Simulation/Xia/");
// 	simulator.SetFileName("tmp.root");
// 	simulator.SetZeroPoint(0);

// 	try {
// 		simulator.Run(5, energyRun | timeRun);
// 	} catch (const std::exception &e) {
// 		std::cerr << e.what() << std::endl;
// 		exit(-1);
// 	}

// 	std::cout << simulator.Result() << std::endl;

// }



void TTreeSimulate(const char *configFileName) {
	std::ifstream configFile(configFileName);
	if (!configFile.good()) {
		std::cerr << "Error open file " << configFileName << std::endl;
		exit(-1);
	}
	nlohmann::json js;
	configFile >> js;
	configFile.close();

	std::string tracePath = js["TracePath"];
	std::string traceFile = js["TraceFile"];
	std::string simPath = js["SimPath"];
	std::string simFile = js["SimFile"];
	bool multiThread = js["MultiThread"];
	size_t threads = js["Threads"];
	unsigned int samplingRate = js["SamplingRate"];
	unsigned int dt = 1000 / samplingRate;
	rFlag runFlag = js["RunFlag"];
	unsigned int entries = js["Entries"];
	size_t zeroPoint = js["ZeroPoint"];

	std::string slowFilterType = js["SlowFilter"];
	std::string slowPickerType = js["SlowPicker"];
	std::string fastFilterType = js["FastFilter"];
	std::string fastPickerType = js["FastPicker"];
	std::string cfdFilterType = js["CFDFilter"];
	std::string cfdPickerType = js["CFDPicker"];


	// readers' preparing
	std::string traceFileName = tracePath + traceFile;
	std::vector<std::string> simFileNames;
	std::vector<std::string> slowNames, fastNames, cfdNames;

	// Filter and Picker
	std::vector<std::unique_ptr<FilterAlgorithm>> slowFilters;
	std::vector<std::unique_ptr<FilterAlgorithm>> fastFilters;
	std::vector<std::unique_ptr<FilterAlgorithm>> cfdFilters;
	std::vector<std::unique_ptr<Picker>> slowPickers;
	std::vector<std::unique_ptr<Picker>> fastPickers;
	std::vector<std::unique_ptr<Picker>> cfdPickers;

	if ((runFlag & rFlag::SlowFilter) != 0) {
		// slow filter
		if (slowFilterType == "empty") {

			slowFilters.push_back(std::make_unique<FilterAlgorithm>());
			slowNames.push_back("");

		} else if (slowFilterType == "mwd") {

			unsigned int SL[3], SG[3], ST[3];
			for (size_t i = 0; i != 3; ++i) {
				SL[i] = js["SL"][i];
				SG[i] = js["SG"][i];
				ST[i] = js["ST"][i];
			}
			for (unsigned int sl = SL[0]; sl <= SL[1]; sl += SL[2]) {
				for (unsigned int sg = SG[0]; sg <= SG[1]; sg += SG[2]) {
					for (unsigned int st = ST[0]; st <= ST[1]; st += ST[2]) {
						// slowFilters.push_back(new MWDAlgorithm(sl*dt, sg*dt, st, dt));
						slowFilters.push_back(std::make_unique<MWDAlgorithm>(sl*dt, sg*dt, st, dt));
						slowNames.push_back("SL" + std::to_string(sl) + "SG" + std::to_string(sg));
					}
				}
			}

		} else if (slowFilterType == "xia") {

			unsigned int SL[3], SG[3], ST[3];
			for (size_t i = 0; i != 3; ++i) {
				SL[i] = js["SL"][i];
				SG[i] = js["SG"][i];
				ST[i] = js["ST"][i];
			}
			for (unsigned int sl = SL[0]; sl <= SL[1]; sl += SL[2]) {
				for (unsigned int sg = SG[0]; sg <= SG[1]; sg += SG[2]) {
					for (unsigned int st = ST[0]; st <= ST[1]; st += ST[2]) {
						// slowFilters.push_back(new XiaSlowFilter(sl*dt, sg*dt, st, dt));
						slowFilters.push_back(std::make_unique<XiaSlowFilter>(sl*dt, sg*dt, st, dt));
						slowNames.push_back("SL" + std::to_string(sl) + "SG" + std::to_string(sg));
					}
				}
			}

		} else {

			std::cerr << "Error: invalid slow filter type " << slowFilterType << "." << std::endl;
			return;

		}

		// slow picker
		if (slowPickerType == "max") {

			size_t vsize = slowFilters.size();
			for (size_t i = 0; i != vsize; ++i) {
				slowPickers.push_back(std::make_unique<MaxPicker>());
			}

		} else if (slowPickerType == "trapezoid-top") {

			size_t slowPoint = zeroPoint-20 > 0 ? zeroPoint-20 : 0;
			size_t SL[3], SG[3], ST[3];
			for (size_t i = 0; i != 3; ++i) {
				SL[i] = js["SL"][i];
				SG[i] = js["SG"][i];
				ST[i] = js["ST"][i];
			}
			for (size_t sl = SL[0]; sl <= SL[1]; sl += SL[2]) {
				for (size_t sg = SG[0]; sg <= SG[1]; sg += SG[2]) {
					for (size_t st = ST[0]; st <= ST[1]; st += ST[2]) {
						slowPickers.push_back(std::make_unique<TrapezoidTopPicker>(slowPoint, sl, sl+sg));
					}
				}
			}

		} else {

			std::cerr << "Error: invalid slow picker type " << slowPickerType << "." << std::endl;
			return;

		}
	}


	if ((runFlag & rFlag::FastFilter) != 0) {
		// fast filter
		if (fastFilterType == "empty") {

			fastFilters.push_back(std::make_unique<FilterAlgorithm>());
			fastNames.push_back("");

		} else if (fastFilterType == "xia") {

			size_t FL[3], FG[3];
			for (size_t i = 0; i != 3; ++i) {
				FL[i] = js["FL"][i];
				FG[i] = js["FG"][i];
			}
			for (size_t fl = FL[0]; fl <= FL[1]; fl += FL[2]) {
				for (size_t fg = FG[0]; fg <= FG[1]; fg += FG[2]) {
					fastFilters.push_back(std::make_unique<XiaFastFilter>(fl, fl+fg));
					fastNames.push_back("FL" + std::to_string(fl) + "FG" + std::to_string(fg));
				}
			}

		} else {

			std::cerr << "Error: invalid fast filter type " << fastFilterType << "." << std::endl;
			return;

		}

		// fast picker
		if (fastPickerType == "max") {

			size_t vsize = fastFilters.size();
			for (size_t i = 0; i != vsize; ++i) {
				fastPickers.push_back(std::make_unique<MaxPicker>());
			}

		} else if (fastPickerType == "leading-edge") {

			unsigned int FT = js["FT"];
			size_t vsize = fastFilters.size();
			for (size_t i = 0; i != vsize; ++i) {
				fastPickers.push_back(std::make_unique<LeadingEdgePicker>(FT));
			}

		} else {

			std::cerr << "Error: invalid fast picker type " << fastPickerType << "." << std::endl;
			return;

		}


	}



	if ((runFlag & rFlag::CFDFilter) != 0) {
		// cfd filter
		if (cfdFilterType == "empty") {

			cfdFilters.push_back(std::make_unique<FilterAlgorithm>());
			cfdNames.push_back("");

		} else if (cfdFilterType == "xia") {

			if (fastFilterType == "xia") {
				size_t CFDD[3];
				unsigned int CFDW[3];
				for (size_t i = 0; i != 3; ++i) {
					CFDD[i] = js["CFDD"][i];
					CFDW[i] = js["CFDW"][i];
				}
				for (size_t cfdd = CFDD[0]; cfdd <= CFDD[1]; cfdd += CFDD[2]) {
					for (unsigned int cfdw = CFDW[0]; cfdw <= CFDW[1]; cfdw += CFDW[2]) {
						cfdFilters.push_back(std::make_unique<XiaCFDFilter>(1, 0, cfdd, cfdw));
						cfdNames.push_back("D" + std::to_string(cfdd) + "W" + std::to_string(cfdw));
					}
				}
			} else {

				size_t FL[3], FG[3], CFDD[3];
				unsigned int CFDW[3];
				for (size_t i = 0; i != 3; ++i) {
					FL[i] = js["FL"][i];
					FG[i] = js["FG"][i];
					CFDD[i] = js["CFDD"][i];
					CFDW[i] = js["CFDW"][i];
				}

				for (size_t fl = FL[0]; fl <= FL[1]; fl += FL[2]) {
					for (size_t fg = FG[0]; fg <= FG[1]; fg += FG[2]) {
						for (size_t cfdd = CFDD[0]; cfdd <= CFDD[1]; cfdd += CFDD[2]) {
							for (unsigned int cfdw = CFDW[0]; cfdw <= CFDW[1]; cfdw += CFDW[2]) {
								cfdFilters.push_back(std::make_unique<XiaCFDFilter>(fl, fg, cfdd, cfdw));
								cfdNames.push_back(std::to_string(fl) + std::to_string(fg) + std::to_string(cfdd) + std::to_string(cfdw));
							}
						}
					}
				}

			}

		} else {

			std::cerr << "Error: invalid cfd filter type " << cfdFilterType << "." << std::endl;
			return;

		}


		// cfd picker
		if (cfdPickerType == "max") {

			cfdPickers.push_back(std::make_unique<MaxPicker>());

		} else if (cfdPickerType == "zero-cross") {

			// size_t cfdPoint = zeroPoint - 5 > 0 ? zeroPoint - 5 : 0;
			size_t cfdPoint = zeroPoint - 10;
			bool cubic = js["CFDCubic"];
			size_t vsize = cfdFilters.size();
			for (size_t i = 0; i != vsize; ++i) {
				cfdPickers.push_back(std::make_unique<ZeroCrossPicker>(cfdPoint, dt, cubic));
				cfdNames[i] += cubic ? "c" : "l";
			}

		} else if (cfdPickerType == "digital-fraction") {

			// size_t cfdPoint = zeroPoint - 5 > 0 ? zeroPoint - 5 : 0;
			size_t cfdPoint = zeroPoint - 10;
			double cfdFraction[3];
			for (size_t i = 0; i != 3; ++i){
				cfdFraction[i] = js["CFDFraction"][i];
			}
			bool cubic = js["CFDCubic"];
			// size_t baseLen = cfdPoint >= 12 ? 10 : 5;
			size_t baseLen = 10;
			size_t vsize = cfdFilters.size();
			// supplement filters
			for (double frac = cfdFraction[0]+cfdFraction[2]; frac <= cfdFraction[1]; frac += cfdFraction[2]) {
				for (size_t i = 0; i != vsize; ++i) {
					cfdFilters.push_back(cfdFilters[i]->Clone());
					cfdNames.push_back(cfdNames[i]);
				}
			}
			// add pickers
			size_t index = 0;
			for (double frac = cfdFraction[0]; frac <= cfdFraction[1]; frac += cfdFraction[2]) {
				for (size_t i = 0; i != vsize; ++i) {
					cfdPickers.push_back(std::make_unique<DigitalFractionPicker>(cfdPoint, frac, cubic, baseLen));
					cfdNames[index] += "DF" + std::to_string(int(frac*100.0)) + (cubic ? "c" : "l");
					++index;
				}
			}

		} else {

			std::cerr << "Error: invaild cfd picker type " << cfdPickerType << "." << std::endl;
			return;

		}

	}

	// check size
	if (slowFilters.size() != slowPickers.size() || slowFilters.size() != slowNames.size()) {
		std::cerr << "Assertion failed: slow file names, filters or pickers size not equal." << std::endl;
		std::cerr << "file " << slowNames.size() << "  fliter " << slowFilters.size() << "  picker " << slowPickers.size() << std::endl;
		return;
	}
	if (fastFilters.size() != fastPickers.size() || fastFilters.size() != fastNames.size()) {
		std::cerr << "Assertion failed: fast file names, filters or pickers size not equal." << std::endl;
		std::cerr << "file " << fastNames.size() << "  fliter " << fastFilters.size() << "  picker " << fastPickers.size() << std::endl;
		return;
	}
	if (cfdFilters.size() != cfdPickers.size() || cfdFilters.size() != cfdFilters.size()) {
		std::cerr << "Assertion failed: cfd file names, filters or pickers size not equal." << std::endl;
		std::cerr << "file " << cfdNames.size() << "  fliter " << cfdFilters.size() << "  picker " << cfdPickers.size() << std::endl;
		return;
	}


	// supplement to at least 1
	if (!slowFilters.size()) {
		slowFilters.push_back(std::make_unique<FilterAlgorithm>());
		slowPickers.push_back(std::make_unique<MaxPicker>());
	}
	if (!fastFilters.size()) {
		fastFilters.push_back(std::make_unique<FilterAlgorithm>());
		fastPickers.push_back(std::make_unique<MaxPicker>());
	}
	if (!cfdFilters.size()) {
		cfdFilters.push_back(std::make_unique<FilterAlgorithm>());
		cfdPickers.push_back(std::make_unique<MaxPicker>());
	}

	// print information
	std::cout << "Slow " << slowFilters.size() << "  Fast " << fastFilters.size() << "  CFD " << cfdFilters.size() << std::endl;

	// print names
	std::cout << "Slow names:" << std::endl;
	for (auto &name : slowNames) std::cout << name << std::endl;
	std::cout << "Fast names:" << std::endl;
	for (auto &name : fastNames) std::cout << name << std::endl;
	std::cout << "CFD names:" << std::endl;
	for (auto &name : cfdNames) std::cout << name << std::endl;

	std::vector<std::unique_ptr<Simulator>> simulators;
	std::vector<TFile*> ipfs;
	size_t index = 0;
	for (size_t i = 0; i != slowFilters.size(); ++i) {
		for (size_t j = 0; j != fastFilters.size(); ++j) {
			for (size_t k = 0; k != cfdFilters.size(); ++k) {

				std::string simulatorType = js["Simulator"];
				if (simulatorType == "base") {

					simulators.push_back(std::make_unique<BaseSimulator>());

				} else if (simulatorType == "tree") {

					simulators.push_back(std::make_unique<TTreeSimulator>());

				} else {

					std::cerr << "Error: invalid simulator type " << simulatorType << "." << std::endl;

				}

				auto &simulator = simulators.back();

				std::unique_ptr<TraceReader> reader = std::make_unique<TTreeTraceReader>(traceFileName.c_str(), "tree", dt);
				if (i==0 && j==0 && k==0) {
					entries = entries > 0 ? entries : (unsigned int)(((TTreeTraceReader*)reader.get())->GetTreeEntries());
				}
				simulator->AddReader(reader->Clone());
				simulator->AddSlowFilter(slowFilters[i]->Clone());
				simulator->AddFastFilter(fastFilters[j]->Clone());
				if (cfdFilterType == "xia" && fastFilterType == "xia") {
					std::unique_ptr<FilterAlgorithm> cfdFilter = cfdFilters[k]->Clone();
					size_t l, m;
					XiaFastFilter *fastFilter = (XiaFastFilter*)(fastFilters[j].get());
					fastFilter->GetParameters(l, m);
					XiaCFDFilter *cfdFilterPtr = (XiaCFDFilter*)(cfdFilter.get());
					cfdFilterPtr->SetFastFilterParameters(l, m);
					simulator->AddCFDFilter(std::move(cfdFilter));
				} else {
					simulator->AddCFDFilter(cfdFilters[k]->Clone());
				}
				simulator->AddSlowPicker(slowPickers[i]->Clone());
				simulator->AddFastPicker(fastPickers[j]->Clone());
				simulator->AddCFDPicker(cfdPickers[k]->Clone());

				simulator->SetPath(simPath.c_str());
				std::string simFileName = "";
				if (slowNames.size() > 1) {
					simFileName += slowNames[i];
				}
				if (fastNames.size() > 1) {
					if (simFileName.size() && fastNames[j].size()) simFileName += "-";
					simFileName += fastNames[j];
				}
				if (cfdNames.size() > 1) {
					if (simFileName.size() && cfdNames[k].size()) simFileName += "-";
					simFileName += cfdNames[k];
				}
				if (!simFileName.size()) {
					simFileName = simFile;
				} else {
					simFileName += ".root";
				}
				simulator->SetFileName(simFileName.c_str());
				simulator->SetZeroPoint(zeroPoint);

				++index;
			}
		}
	}


	try {
		if (multiThread) {
			ThreadPool pool(threads);
			for (auto &simulator : simulators) {
				pool.enqueue(&Simulator::Run, simulator.get(), entries, runFlag);
			}
		} else {
			for (auto &simulator : simulators) {
				simulator->Run(entries, runFlag);
			}
		}
	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
		exit(-1);
	}

	// for (auto ipf : ipfs) {
	// 	ipf->Close();
	// }
}


// void Pixie100LGT() {
// 	// Reader
// 	TFile *opf = new TFile(Source100M, "read");
// 	if (!opf) {
// 		std::cerr << "Error open root file " << Source100M << std::endl;
// 		return;
// 	}
// 	opf->cd();
// 	TTree *tree = (TTree*)opf->Get("tree");
// 	TTreeTraceReader reader("pixie100", tree, 10);
// 	// Filter
// 	XiaSlowFilter slowFilter("Xia", 3520, 800, 25000, 10);
// 	// MWDAlgorithm slowFilter("MWD", 3520, 800, 2500, 10);
// 	XiaFastFilter fastFilter("Xia", 200, 0, 10);

// 	LGTSimulator simulator;
// 	simulator.AddReader(&reader);
// 	simulator.AddSlowFilter(&slowFilter);
// 	simulator.AddFastFilter(&fastFilter);
// 	simulator.SetFastThres(90);
// 	simulator.SetPath(SimLogPath);
// 	// simulator.SetSL(2600, 3400, 50);
// 	// simulator.SetSG(100, 600, 50);
// 	// simulator.SetST(18000, 23000, 1000);
// 	simulator.SetSL(2000, 4000, 200);
// 	simulator.SetSG(100, 1000, 200);
// 	simulator.SetST(18000, 22000, 2000);

// 	try {
// 		simulator.Run(int(tree->GetEntries()), energyRun);
// 	} catch (const std::exception &e) {
// 		std::cerr << e.what() << std::endl;
// 		exit(-1);
// 	}

// 	opf->Close();
// }



// void Annular150u100MCFDTime() {
// 	// Reader
// 	TFile *ipf = new TFile(SourceAnnular150u100M, "read");
// 	if (!ipf) {
// 		std::cerr << "Error open root file " << SourceAnnular150u100M << std::endl;
// 		return;
// 	}
// 	ipf->cd();
// 	TTree *tree = (TTree*)ipf->Get("tree");
// 	TTreeTraceReader reader("Annular150u100M", tree, 10);
// 	// Filter
// 	XiaFastFilter fastFilter("Xia", 60, 0, 10);
// 	XiaCFDFilter cfdFilter("Xia", 30, 1, 10);

// 	CFDTimeSimulator simulator;
// 	simulator.AddReader(&reader);
// 	simulator.AddFastFilter(&fastFilter);
// 	simulator.AddCFDFilter(&cfdFilter);
// 	simulator.SetFastThres(75);
// 	simulator.SetCFDThres(10);
// 	simulator.SetPath("/data/Simulation/CFDTime1");
// 	simulator.SetFL(10, 80, 10);
// 	simulator.SetFG(0, 30, 100);
// 	simulator.SetCFDD(10, 60, 100);
// 	simulator.SetCFDW(0, 7, 10);
// 	simulator.SetCubicCFD(false);

// 	try {
// 		simulator.Run(int(tree->GetEntries()), cfdRun);
// 	} catch (const std::exception &e) {
// 		std::cerr << e.what() << std::endl;
// 		exit(-1);
// 	}

// 	ipf->Close();
// }






int main(int argc, char **argv) {

	// ExpDecayMWDSim();
	TTreeSimulate(argv[1]);
	return 0;
}
