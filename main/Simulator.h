#ifndef __SIMULATOR_H__
#define __SIMULATOR_H__

#include <vector>
#include <string>
#include <memory>

#include "TFile.h"

#include "TraceReader.h"
#include "FilterAlgorithm.h"
#include "Picker.h"


class Simulator {
public:
	virtual ~Simulator();

	enum class RunFlag: int{
		None = 0,
		SlowFilter = 1,
		FastFilter = 2,
		CFDFilter = 4
	};

	// Add parts of the simulator
	virtual void AddReader(std::unique_ptr<TraceReader> reader_);
	virtual void AddSlowFilter(std::unique_ptr<FilterAlgorithm> filter_);
	virtual void AddFastFilter(std::unique_ptr<FilterAlgorithm> filter_);
	virtual void AddCFDFilter(std::unique_ptr<FilterAlgorithm> filter_);

	virtual void AddSlowPicker(std::unique_ptr<Picker> picker_);
	virtual void AddFastPicker(std::unique_ptr<Picker> picker_);
	virtual void AddCFDPicker(std::unique_ptr<Picker> picker_);

	virtual void SetPath(const char *p);
	virtual void SetFileName(const char *name_);
	virtual void SetZeroPoint(unsigned int zero_);
	virtual void SetVerbose(bool verbose_ = true);

	virtual void Run(unsigned int, RunFlag) = 0;

protected:
	Simulator();

	// parts of the simulator
	std::unique_ptr<TraceReader> reader;
	std::unique_ptr<FilterAlgorithm> slowFilter;
	std::unique_ptr<FilterAlgorithm> fastFilter;
	std::unique_ptr<FilterAlgorithm> cfdFilter;
	std::unique_ptr<Picker> slowPicker;
	std::unique_ptr<Picker> fastPicker;
	std::unique_ptr<Picker> cfdPicker;

	// record file
	TFile *file;
	TString path;
	TString fileName;

	// options
	bool verbose;

	unsigned int zeroPoint;
};


Simulator::RunFlag operator|(Simulator::RunFlag lhs, Simulator::RunFlag rhs);
Simulator::RunFlag operator&(Simulator::RunFlag lhs, Simulator::RunFlag rhs);
bool operator!=(Simulator::RunFlag lhs, int rhs);
bool operator!=(int lhs, Simulator::RunFlag rhs);


class BaseSimulator: public Simulator {
public:
	BaseSimulator();
	virtual ~BaseSimulator();

public:
	// run several times of simulation
	virtual void Run(unsigned int times, RunFlag flag);
	// get result
	virtual std::string Result();
	virtual std::vector<double> &GetEnergy();
	virtual std::vector<int> &GetTime();
	virtual std::vector<double> &GetCFD();
private:
	std::vector<double> energy;
	std::vector<int> timestamp;
	std::vector<double> cfd;
};


class TTreeSimulator: public Simulator {
public:
	TTreeSimulator();
	virtual ~TTreeSimulator();

	// run
	virtual void Run(unsigned int entries, RunFlag flag);
	virtual TTree* Tree();
private:
	TTree *tree;			// simulation tree
	UShort_t energy;		// simulation energy
	Short_t timestamp;		// simulation local timestamp
	Short_t cfdPoint;		// cfd and ts offset
	Double_t cfd;			// cfd value
};


#endif
