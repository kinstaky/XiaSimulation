#ifndef __TRACEREADER_H__
#define __TRACEREADER_H__

#include <vector>
#include <string>
#include <map>
#include <thread>
#include <mutex>

#include "TF1.h"
#include "TRandom3.h"
#include "TTree.h"
#include <memory>

#include "TFile.h"

// abstrac class

class TraceReader {
public:
	virtual ~TraceReader();
	virtual std::unique_ptr<TraceReader> Clone() const = 0;

	virtual const std::vector<double> &Read() = 0;
	virtual unsigned int GetPeriod() const;
	virtual double GetBase();
	virtual void Reset();
protected:
	TraceReader(unsigned int dt);

	unsigned int period;						// the period between samplings, Delta t, ns
	std::vector<double> data;					// sampling data
};


class FunctionTraceReader: public TraceReader {
public:
	FunctionTraceReader(TF1 *f_, unsigned int dt_, unsigned int len_);
	virtual ~FunctionTraceReader();
	virtual std::unique_ptr<TraceReader> Clone() const override;

	virtual const std::vector<double> &Read();
private:
	TF1 *func;
	unsigned int dt;
	unsigned int len;
	size_t points;
	TRandom3 *generator;
};



class TTreeTraceReader: public TraceReader {
public:
	TTreeTraceReader(const char *file_, const char *tree_, unsigned int dt_);
	virtual ~TTreeTraceReader();
	virtual std::unique_ptr<TraceReader> Clone() const override;

	virtual const std::vector<double> &Read();
	virtual double GetBase();
	virtual Long64_t GetTreeEntries() const;
	virtual void Reset();
private:
	std::string fileName, treeName;
	unsigned int dt;
	TFile *file;
	TTree *tree;
	Long64_t jentry;
	UShort_t *rawData;
	UShort_t points;
	Double_t base;
};

#endif