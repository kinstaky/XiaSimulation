#ifndef __RESOLUTION_H__
#define __RESOLUTION_H__

#include "TFile.h"
#include "TTree.h"
#include "TString.h"

class Resolution {
public:
	Resolution(TTree* tt, int ssMin, int ssMax);
	virtual ~Resolution();

	virtual double Run(TString var, TString cut = "", TDirectoryFile *opf = nullptr);
private:
	TFile *logFile;
	TTree *tree;
	int sMin;
	int sMax;
};


#endif