#include <cstring>
#include <iostream>
#include <iomanip>

#include "Adapter.h"


//--------------------------------------------------
// 				Adapter
//--------------------------------------------------


// Adapter() constructor
//  @ff		-- output file name
//	@pp 	-- data size, points
Adapter::Adapter(const char *ff, int pp) {
	points = pp;
	data = new UShort_t[points];
	opf = new TFile(ff, "recreate");
	tree = new TTree("tree", "trace tree");
	tree->Branch("dsize", &dsize, "dsize/s");
	tree->Branch("data", data, "data[dsize]/s");
	tree->Branch("ts", &ts, "ts/L");
	tree->Branch("e", &energy, "energy/s");
	// tree->Branch("base", &base, "base/D");
	tree->Branch("s", &strip, "strip/s");
	tree->Branch("side", &side, "side/s");
	tree->Branch("cfd", &cfd, "cfd/S");
	tree->Branch("cfdft", &cfdft, "cfdft/O");
}

Adapter::~Adapter() {
	opf->Close();
	delete[] data;
}

// Write()
//  Write tree to file
void Adapter::Write() {
	opf->cd();
	tree->Write();
	return;
}


//--------------------------------------------------
// 				Pixie100MAdapter
//--------------------------------------------------

// Pixie100MAdapter()
//  constructor, set read tree and branch address
//  @tt 		-- read tree
//  @nn 		-- output file name
//  @pp 		-- data size, points
Pixie100MAdapter::Pixie100MAdapter(TTree *tt, const char *ff, int pp):
Adapter(ff, pp) {

	readTree = tt;
	// set branch address
	readTree->SetBranchAddress("ltra", &dsize);
	readTree->SetBranchAddress("data", data);
	readTree->SetBranchAddress("sid", &sid);
	readTree->SetBranchAddress("ch", &ch);
	readTree->SetBranchAddress("ts", &ts);
	readTree->SetBranchAddress("evte", &energy);
	// readTree->SetBranchAddress("base", &base);
	readTree->SetBranchAddress("cfd", &cfd);
	readTree->SetBranchAddress("cfdft", &cfdft);
}

Pixie100MAdapter::~Pixie100MAdapter() {
}


void Pixie100MAdapter::Read(Long64_t entries, size_t pointStart, size_t pointSize) {
// std::cout << nentry << "  " << pointStart << "  " << pointSize << std::endl;
	std::cout << "read   0%";
	std::cout.flush();
	Long64_t filled = 0;
	Long64_t nentry = readTree->GetEntries();
	Long64_t nentry100 = nentry / 100 + 1;
	for (Long64_t jentry = 0; jentry != nentry; ++jentry) {
		readTree->GetEntry(jentry);

		// 100M mapping code
		if (sid % 2) continue;
		if (energy < 1000) continue;
		if (dsize != 5000) continue;
		side = sid == 2 ? 0 : 1;
		strip = ch;
		dsize = pointSize;
		memmove(data, data+pointStart, pointSize*sizeof(UShort_t));
		// for (size_t i = 0; i != pointSize; ++i) {
		// 	data[i] = data[i+pointStart];
		// }

		// fill
		tree->Fill();
		++filled;
		if (entries && filled >= entries) break;

		if (jentry % nentry100 == 0) {
			std::cout << "\b\b\b\b" << std::setw(3) << jentry/nentry100 << "%";
			std::cout.flush();
		}
	}
	std::cout << "\b\b\b\b100%" << std::endl;;
	return;
}

//--------------------------------------------------
//				Pixie250MAdapter
//--------------------------------------------------

// Pixie250MAdapter()
//  constructor, set read tree and branch address
//  @tt 		-- read tree
//  @ff 		-- output file name
//  @pp 		-- data size, points
Pixie250MAdapter::Pixie250MAdapter(TTree *tt, const char *ff, int pp):
Adapter(ff, pp) {

	readTree = tt;
	// set branch address
	readTree->SetBranchAddress("ltra", &dsize);
	readTree->SetBranchAddress("data", data);
	readTree->SetBranchAddress("sid", &sid);
}

Pixie250MAdapter::~Pixie250MAdapter() {
}


void Pixie250MAdapter::Read(Long64_t entries, size_t pointStart, size_t pointSize) {
	std::cout << "read   0%";
	std::cout.flush();
	Long64_t filled = 0;
	Long64_t nentry = readTree->GetEntries();
	Long64_t nentry100 = nentry / 100 + 1;
	for (Long64_t jentry = 0; jentry != nentry; ++jentry) {
		readTree->GetEntry(jentry);
		if (sid != 3) continue;
		strip = ch + 16;

		// fill
		tree->Fill();
		++filled;

		if (entries && filled >= entries) break;

		if (jentry % nentry100 == 0) {
			std::cout << "\b\b\b\b" << std::setw(3) << jentry/nentry100 << "%";
			std::cout.flush();
		}
	}
	std::cout << "\b\b\b\b100%" << std::endl;
	return;
}
