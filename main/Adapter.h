#ifndef __ADPATER_H__
#define __ADPATER_H__

#include "TFile.h"
#include "TTree.h"

class Adapter {
public:
	virtual ~Adapter();

	virtual void Read(Long64_t nentry, size_t pointStart, size_t pointSize) = 0;
	virtual void Write();
protected:
	// method
	Adapter(const char *ff, int pp);

	TFile *opf;
	TTree *tree;

	int points;

	// branch variable
	UShort_t dsize;
	UShort_t *data;
	Long64_t ts;
	UShort_t energy;
	UInt_t le;
	UInt_t ge;
	UInt_t re;
	Double_t base;
	UShort_t strip;
	UShort_t side;
	Short_t cfd;
	bool cfdft;
};

class Pixie100MAdapter: public Adapter {
public:
	Pixie100MAdapter(TTree *tt, const char *ff, int pp);
	virtual ~Pixie100MAdapter();

	virtual void Read(Long64_t entries = 0, size_t pointStart = 0, size_t pointSize = 5000);
protected:
	TTree *readTree;
	Short_t sid;
	Short_t ch;
};


class Pixie250MAdapter: public Adapter {
public:
	Pixie250MAdapter(TTree *tt, const char *ff, int pp);
	virtual ~Pixie250MAdapter();

	virtual void Read(Long64_t entries = 0, size_t pointStart = 0, size_t pointSize = 12500);
protected:
	TTree *readTree;
	Short_t sid;
	Short_t ch;
};



#endif
