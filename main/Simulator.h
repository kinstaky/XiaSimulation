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
	// virtual void AddReader(TraceReader *reader_);
	// virtual void AddSlowFilter(FilterAlgorithm *alogrithm_);
	// virtual void AddFastFilter(FilterAlgorithm *alogrithm_);
	// virtual void AddCFDFilter(FilterAlgorithm *alogrithm_);

	// virtual void AddSlowPicker(Picker *picker_);
	// virtual void AddFastPicker(Picker *picker_);
	// virtual void AddCFDPicker(Picker *picker_);

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

	// virtual void SetFastThres(int ft);
	// virtual void SetCFDThres(int thres_);
	// virtual void SetCubicCFD(bool cubic = true);

	virtual void Run(unsigned int, RunFlag) = 0;

protected:
	Simulator();

	// parts of the simulator
	// TraceReader *reader;
	// FilterAlgorithm *slowFilter;
	// FilterAlgorithm *fastFilter;
	// FilterAlgorithm *cfdFilter;
	// Picker *slowPicker;
	// Picker *fastPicker;
	// Picker *cfdPicker;
	std::unique_ptr<TraceReader> reader;
	std::unique_ptr<FilterAlgorithm> slowFilter;
	std::unique_ptr<FilterAlgorithm> fastFilter;
	std::unique_ptr<FilterAlgorithm> cfdFilter;
	std::unique_ptr<Picker> slowPicker;
	std::unique_ptr<Picker> fastPicker;
	std::unique_ptr<Picker> cfdPicker;

	// int fastThres;
	// int cfdThres;
	// bool useCubicCFD;

	// record file
	TFile *file;
	TString path;
	TString fileName;

	unsigned int zeroPoint;

	// // help functions
	// double aveFlatTop(const std::vector<double> &data, int ts, int l, int m);
	// int timeStamp(const std::vector<double> &data, int thres);
	// double linearCfdFraction(const std::vector<double> &data, int thres, short &point);
	// double cubicCfdFraction(const std::vector<double> &data, int thres, short &point);
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


// class LGTSimulator: public Simulator {
// public:
// 	LGTSimulator();
// 	virtual ~LGTSimulator();

// 	virtual void Run(unsigned int entries, RunFlag flag);
// 	virtual void SetSL(size_t min, size_t max, size_t step);
// 	virtual void SetSG(size_t min, size_t max, size_t step);
// 	virtual void SetST(size_t min, size_t max, size_t step);
// private:
// 	// filter parameters
// 	size_t SLMin, SLMax, SLStep;
// 	size_t SGMin, SGMax, SGStep;
// 	size_t STMin, STMax, STStep;

// 	TTree *tree;
// 	UShort_t energy;
// };


// class CFDTimeSimulator: public Simulator {
// public:
// 	CFDTimeSimulator();
// 	virtual ~CFDTimeSimulator();

// 	virtual void Run(unsigned int entries, RunFlag flag);
// 	virtual void SetFL(size_t min, size_t max, size_t step);
// 	virtual void SetFG(size_t min, size_t max, size_t step);
// 	virtual void SetCFDD(size_t min, size_t max, size_t step);
// 	virtual void SetCFDW(unsigned int min, unsigned int max, unsigned int step);
// private:
// 	// filter paramters
// 	size_t FLMin, FLMax, FLStep;
// 	size_t FGMin, FGMax, FGStep;
// 	size_t CFDDMin, CFDDMax, CFDDStep;
// 	unsigned int CFDWMin, CFDWMax, CFDWStep;

// 	// void singleRun(size_t FL, size_t FG, size_t CFDD, size_t CFDW, unsigned int entries_, unsigned int *jentry);
// };


#endif
