#include "Resolution.h"

#include <vector>

#include "TF1.h"
#include "TH1.h"
#include "TSpectrum.h"
#include "TFile.h"
#include "TDirectoryFile.h"



Resolution::Resolution(TTree *tree_, int sMin_, int sMax_) {
	tree = tree_;
	sMin = sMin_;
	sMax = sMax_;
}

Resolution::~Resolution() {
}


double Resolution::Run(TString var, TString cut, TDirectoryFile *opf) {
	TSpectrum *spectrum = new TSpectrum(10);
	std::vector<Double_t> vecResPu;

	for (int si = sMin; si < sMax; ++si) {
		// get the energy histogram for single strip
		TString hName, hCut;
		hName.Form("hes%d", si);
		hCut.Form("strip==%d", si);
		if (cut.Length()) hCut += "&&" + cut;
		tree->Draw((var+">>"+hName+"(500, 2000, 3500)").Data(), hCut.Data());
		TH1 *hesi = (TH1*)gDirectory->Get(hName.Data());
		// use TSpectru to search peaks and save in the spectrum sub directory
		spectrum->Search(hesi, 8, "", 0.2);
		Double_t *xPeaks = spectrum->GetPositionX();

		// fit
		TF1 *PuFit = new TF1(TString::Format("PuFit%d", si), "gaus", xPeaks[0]-15, xPeaks[0]+40);
		hesi->Fit(PuFit, "QRS+");
		Double_t meanPu = PuFit->GetParameter(1);
		Double_t sigmaPu = PuFit->GetParameter(2);
		Double_t resPu = sigmaPu / meanPu * 2.355;

		// TF1 *AmFit = new TF1(TString::Format("AmFit%d", fsi), "gaus", xPeaks[1]-15, xPeaks[1]+40);
		// hfesi->Fit(AmFit, "RSQ+");
		// Double_t meanAm = AmFit->GetParameter(1);
		// Double_t sigmaAm = AmFit->GetParameter(2);
		// Double_t resAm = sigmaAm / meanAm * 2.355;

    	vecResPu.push_back(resPu);
    	if (opf) {
    		opf->cd();
 			hesi->Write(hName);
 		}
	}

	// average resolution
	Double_t aveRes = 0.0;
	for (auto &res : vecResPu) {
		aveRes += res;
	}
	aveRes /= vecResPu.size();
	return aveRes;
}