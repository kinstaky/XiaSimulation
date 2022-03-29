#include "TraceReader.h"
#include "TRandom3.h"


//--------------------------------------------------
//				TraceReader
//--------------------------------------------------


// constructor

TraceReader::TraceReader(unsigned int dt) {
	period = dt;
}


// deconstructor, do nothin

TraceReader::~TraceReader() {
}


// Get period

unsigned int TraceReader::GetPeriod() const {
	return period;
}

// Get base

double TraceReader::GetBase() {
	return 0.0;
}


void TraceReader::Reset() {
	return;
}

//--------------------------------------------------
//				FunctionTraceReader
//--------------------------------------------------

// constructor
FunctionTraceReader::FunctionTraceReader(TF1 *f_, unsigned int dt_, unsigned int len_):
TraceReader(dt_), func(f_), points(len_/dt_) {
	dt = dt_;
	len = len_;
	data.resize(points);
	generator = new TRandom3();
}

// deconstructor, do nothing
FunctionTraceReader::~FunctionTraceReader() {
}


std::unique_ptr<TraceReader> FunctionTraceReader::Clone() const {
	return std::make_unique<FunctionTraceReader>(func, dt, len);
}



// read data from TF1 function
const std::vector<double>& FunctionTraceReader::Read() {
	// get random initial offset
	unsigned int offset = generator->Rndm() * period;
	for (size_t i = 0; i != points; ++i) {
		data[i] = func->Eval(offset);
		offset += period;
	}
	return data;
}


// // read data from TF1 function, the same as the single thread version

// const std::vector<double>& FunctionTraceReader::Read(std::thread::id tid_) {
// 	return Read();
// }

//--------------------------------------------------
//				TTreeTraceReader
//--------------------------------------------------


// constructor

TTreeTraceReader::TTreeTraceReader(const char *file_, const char *tree_, unsigned int dt_):
TraceReader(dt_) {
	dt = dt_;
	fileName = file_;
	treeName = tree_;

	jentry = 0;
	file = new TFile(file_, "read");
	if (!file) {
		throw std::runtime_error("Error read file " + std::string(file_) + ".");
	}
	tree = (TTree*)file->Get(tree_);
	// set branch address
	tree->SetBranchAddress("dsize", &points);
	tree->GetEntry(0);
	rawData = new UShort_t[points];
	tree->SetBranchAddress("data", rawData);
	// tree->SetBranchAddress("base", &base);

	// resize data
	data.resize(points);
}


// deconstructor
TTreeTraceReader::~TTreeTraceReader() {
	delete[] rawData;
	file->Close();
}


std::unique_ptr<TraceReader> TTreeTraceReader::Clone() const {
	return std::make_unique<TTreeTraceReader>(fileName.c_str(), treeName.c_str(), dt);
}


// read data from tree, single thread version
const std::vector<double>& TTreeTraceReader::Read() {
	tree->GetEntry(jentry);
	if (points != data.size()) {
		std::string info("read data size ");
		info += std::to_string(points) + " != " + std::to_string(data.size()) + " .";
		throw std::runtime_error(info);
	}
	for (int i = 0; i != points; ++i) {
		data[i] = double(rawData[i]);
	}
	jentry++;
	return data;
}


// // read data from tree, multi thread version

// const std::vector<double>& TTreeTraceReader::Read(std::thread::id tid_) {
// 	std::lock_guard<std::mutex> guard(lck);

// 	auto iter = entryMap.find(tid_);
// 	if (iter == entryMap.end()) {
// 		entryMap.insert(std::make_pair(tid_, 0));
// 		dataMap.insert(std::make_pair(tid_, std::vector<double>(points, 0.0)));
// 	}
// 	tree->GetEntry(entryMap[tid_]);
// 	if (points != data.size()) {
// 		std::string info("read data size ");
// 		info += std::to_string(points) + " != " + std::to_string(data.size()) + " .";
// 		throw std::runtime_error(info);
// 	}
// 	for (int i = 0; i != points; ++i) {
// 		dataMap[tid_][i] = rawData[i];
// 	}
// 	++entryMap[tid_];
// 	return dataMap[tid_];
// }



double TTreeTraceReader::GetBase() {
	return base;
}


void TTreeTraceReader::Reset() {
	jentry = 0;
	return;
}


Long64_t TTreeTraceReader::GetTreeEntries() const {
	return tree->GetEntries();
}