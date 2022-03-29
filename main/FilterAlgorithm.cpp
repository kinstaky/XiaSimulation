#include "FilterAlgorithm.h"
#include <iostream>
#include <cmath>


//--------------------------------------------------
// 				FilterAlgorithm
//--------------------------------------------------

// constructor

FilterAlgorithm::FilterAlgorithm() {
}

// deconstructor, do nothing

FilterAlgorithm::~FilterAlgorithm() {
}

// blank filter, output what inputs

const std::vector<double>& FilterAlgorithm::Filter(const std::vector<double> &trace) {
	return trace;
}


std::unique_ptr<FilterAlgorithm> FilterAlgorithm::Clone() const {
	return std::make_unique<FilterAlgorithm>();
}

//--------------------------------------------------
// 				SlowFilter
//--------------------------------------------------


SlowFilter::SlowFilter(size_t l_, size_t m_):
FilterAlgorithm() {
	l = l_;
	m = m_;
}


SlowFilter::~SlowFilter() {
}



void SlowFilter::GetParameters(size_t &l_, size_t &m_) {
	l_ = l;
	m_ = m;
	return;
}

void SlowFilter::SetParameters(size_t l_, size_t m_) {
	l = l_;
	m = m_;
	return;
}


//--------------------------------------------------
// 				MWDAlgorithm
//--------------------------------------------------

// constructor

MWDAlgorithm::MWDAlgorithm(size_t l_, size_t m_, double alpha_):
SlowFilter(l_, m_), alpha(alpha_) {
}


// constructor

MWDAlgorithm::MWDAlgorithm(unsigned int L, unsigned int G, unsigned int tau, unsigned int dt):
// MWDAlgorithm(L/dt, (L+G)/dt, double(dt)/double(tau)) {
MWDAlgorithm(L/dt, (L+G)/dt, exp(double(dt)/double(tau))-1.0) {
}


// deconstructor

MWDAlgorithm::~MWDAlgorithm() {
}


// clone function
std::unique_ptr<FilterAlgorithm> MWDAlgorithm::Clone() const {
	return std::make_unique<MWDAlgorithm>(l, m ,alpha);
}



// MWD filter
const std::vector<double>& MWDAlgorithm::Filter(const std::vector<double> &trace) {
	// DC offset
	double offset = 0.0;
	for (size_t i = 0; i != l+m; ++i) offset += trace[i];
	offset /= double(l+m);
	// prepare for p
	double pn, pn_1;
	pn = trace[m] - trace[0];
	// prepare for r
	size_t ir = l;
	size_t nir = 0;
	double *r = new double[l+1];
	r[0] = 0.0;
	for (size_t i = 0; i <= m-1; ++i) {
		r[0] += trace[i]-offset;
	}
	r[0] = trace[m] - trace[0] + alpha*r[0];
	// r[0] = trace[m] - trace[0];
	for (size_t i = 1; i != l+1; ++i) {
		pn_1 = pn;
		pn = trace[m+i] - trace[i];
		r[i] = r[i-1] + pn - pn_1 + alpha*pn_1;
		// r[i] = r[i-1] + pn - pn_1;
	}
	// prepare for s(i.e. data)
	data.resize(trace.size());
	for (size_t i = 0; i != l+m; ++i) {
		data[i] = 0.0;
	}
	data[l+m] = 0.0;
	for (size_t i = 0; i != l+1; ++i) {
		data[l+m] += r[i];
	}
	data[l+m] /= double(l);

	// loop
	size_t tsize = trace.size();							// vector size
	for (size_t i = l+m+1; i != tsize; ++i) {
		pn_1 = pn;											// p[n-1] = v[n-1] - v[n-m-1]
		pn = double(trace[i] - trace[i-m]);					// p[n] = v[n] - v[n-m]
		size_t pir = ir;									// previous ir
		ir = nir;											// update ir
		nir = ir == l ? 0 : ir+1;							// next ir
		r[ir] = r[pir] + pn - pn_1 + alpha * pn_1;			// r[n] = r[n-1] + p[n] - p[n-1] + alpha*p[n-1]
		data[i] = data[i-1] + (r[ir] - r[nir]) / l;			// s[n] = s[n-1] + 1/l * (r[n] - r[n-1])
	}
	delete[] r;

	// for (size_t i = 0; i != tsize; ++i) {
	// 	data[i] *= 1.0;
	// }
	return data;
}


void MWDAlgorithm::SetParameters(unsigned int L, unsigned int G, unsigned int tau, unsigned int dt) {
	l = L / dt;
	m = (L+G) / dt;
	alpha = exp(double(dt)/double(tau))-1.0;
	return;
}


void MWDAlgorithm::SetParameters(size_t l_, size_t m_, unsigned int tau, unsigned int dt) {
	l = l_;
	m = m_;
	alpha = exp(double(dt)/double(tau))-1.0;
	return;
}




//--------------------------------------------------
// 				XiaSlowFilter
//--------------------------------------------------

XiaSlowFilter::XiaSlowFilter(size_t l_, size_t m_, double b_):
SlowFilter(l_, m_), b(b_) {
}



XiaSlowFilter::XiaSlowFilter(unsigned int L, unsigned int G, unsigned int tau, unsigned int dt):
XiaSlowFilter(L/dt, (L+G)/dt, exp(-double(dt)/double(tau))) {
}



XiaSlowFilter::~XiaSlowFilter() {
}


std::unique_ptr<FilterAlgorithm> XiaSlowFilter::Clone() const {
	return std::make_unique<XiaSlowFilter>(l, m , b);
}


const std::vector<double> &XiaSlowFilter::Filter(const std::vector<double> &trace) {
	double c0 = -(1.0-b) * 4.0 * pow(b, double(l))  / (1.0 - pow(b, double(l)));
	double c1 = (1.0-b) * 4.0;
	double c2 = (1.0-b) * 4.0 / (1.0 - pow(b, double(l)));
// std::cout << "b = " << b << std::endl;
// std::cout << "c0= " << c0 << std::endl;
// std::cout << "c1= " << c1 << std::endl;
// std::cout << "c2= " << c2 << std::endl;

	// compute base
	double bsum0, bsum1, bsum2;
	bsum0 = bsum1 = bsum2 = 0.0;
	for (size_t i = 0; i < l; ++i) {
		bsum0 += trace[i];
	}
	for (size_t i = l; i != m; ++i) {
		bsum1 += trace[i];
	}
	for (size_t i = m; i != m+l; ++i) {
		bsum2 += trace[i];
	}
	double cbase = c0*bsum0 + c1*bsum1 + c2*bsum2;

	// compute filter result
	data.resize(trace.size());
	double esum0, esum1, esum2;
	esum0 = esum1 = esum2 = 0.0;
	for (size_t i = 0; i != l; ++i) {
		esum0 += trace[i];
	}
	for (size_t i = l; i != m; ++i) {
		esum1 += trace[i];
	}
	for (size_t i = m; i != l+m; ++i) {
		esum2 += trace[i];
	}
	data[l+m] = c0*esum0 + c1*esum1 + c2*esum2 - cbase;

	size_t tsize = trace.size();
	for (size_t i = l+m+1; i != tsize; ++i) {
		esum0 += trace[i-m] - trace[i-l-m-1];
		esum1 += trace[i-l] - trace[i-m-1];
		esum2 += trace[i-1] - trace[i-l-1];

		data[i] = c0*esum0 + c1*esum1 + c2*esum2 - cbase;
	}
	for (size_t i = 0; i != l+m; ++i) {
		data[i] = data[l+m];
	}

	return data;
}




void XiaSlowFilter::SetParameters(unsigned int L, unsigned int G, unsigned int tau, unsigned int dt) {
	l = L / dt;
	m = (L+G) / dt;
	b = exp(-double(dt)/double(tau));
	return;
}



void XiaSlowFilter::SetParameters(size_t l_, size_t m_, unsigned int tau, unsigned int dt) {
	l = l_;
	m = m_;
	b = exp(-double(dt)/double(tau));
	return;
}



//--------------------------------------------------
// 				FastFilter
//--------------------------------------------------


FastFilter::FastFilter(size_t l_, size_t m_):
FilterAlgorithm() {
	l = l_;
	m = m_;
}



FastFilter::~FastFilter() {
}



void FastFilter::GetParameters(size_t &l_, size_t &m_) {
	l_ = l;
	m_ = m;
	return;
}



void FastFilter::SetParameters(unsigned int L, unsigned int G, unsigned int dt) {
	l = L / dt;
	m = (L+G) / dt;
	return;
}


void FastFilter::SetParameters(size_t l_, size_t m_) {
	l = l_;
	m = m_;
	return;
}


//--------------------------------------------------
// 				XiaFastFilter
//--------------------------------------------------

// constructor

XiaFastFilter::XiaFastFilter(size_t l_, size_t m_):
FastFilter(l_, m_) {
}



XiaFastFilter::XiaFastFilter(unsigned int L, unsigned int G, unsigned int dt):
XiaFastFilter(L/dt, (L+G)/dt) {
}


// deconstructor

XiaFastFilter::~XiaFastFilter() {

}


// clone
std::unique_ptr<FilterAlgorithm> XiaFastFilter::Clone() const {
	return std::make_unique<XiaFastFilter>(l, m);
}


const std::vector<double> &XiaFastFilter::Filter(const std::vector<double> &trace) {
	// DC offset
	double offset = 0.0;
	for (size_t i = 0; i != l+m; ++i) offset += trace[i];
	offset /= double(l+m);
	// prepare for r
	size_t ir = l-1;
	size_t nir = l;
	double *r = new double[l+1];
	for (size_t i = 0; i != l; ++i) {
		r[i] = trace[m+i+1] - trace[i+1];
	}
	// prepare for s(i.e. data)
	data.resize(trace.size());
	for (size_t i = 0; i != l+m; ++i) {
		data[i] = 0;
	}
	data[l+m] = 0.0;
	for (size_t i = 0; i != l; ++i) {
		data[l+m] += r[i];
	}
	data[l+m] /= l;

	// loop
	size_t tsize = trace.size();							// vector size
	for (size_t i = l+m+1; i != tsize; ++i) {
		ir = nir;											// update ir
		nir = ir == l ? 0 : ir+1;							// next ir
		r[ir] = trace[i] - trace[i-m];						// r[n] = v[n] - v[n-m]
		data[i] = data[i-1] + (r[ir] - r[nir]) / l;			// s[n] = s[n-1] + 1/l * (r[n] -r[n-l])
	}
	delete[] r;

	return data;
}



//--------------------------------------------------
//					XiaCFDFilter
//--------------------------------------------------

// constructor
XiaCFDFilter::XiaCFDFilter(size_t l_, size_t m_, size_t delay_, unsigned int factor_):
FilterAlgorithm(), fastFilter(l_, m_), d(delay_), w(factor_) {
	l = l_;
	m = m_;
}

// constructor
XiaCFDFilter::XiaCFDFilter(unsigned int L_, unsigned int G_, unsigned int D_, unsigned int factor_, unsigned int dt_):
XiaCFDFilter(L_/dt_, (L_+G_)/dt_, D_/dt_, factor_) {
}

// deconstructor
XiaCFDFilter::~XiaCFDFilter() {
}


// clone
std::unique_ptr<FilterAlgorithm> XiaCFDFilter::Clone() const {
	return std::make_unique<XiaCFDFilter>(l, m, d, w);
}


// Filter
// CFD[i] = FF[i]*(1-w/8) - FF[i-D]
const std::vector<double> &XiaCFDFilter::Filter(const std::vector<double> &trace) {
	const std::vector<double> &fast = fastFilter.Filter(trace);

	double factor  = 1.0 - double(w) / 8.0;
	size_t vsize = fast.size();
	data.resize(vsize);
	for (size_t i = d; i != vsize; ++i) {
		data[i] = fast[i] * factor - fast[i-d];
	}
	for (size_t i = 0; i != d; ++i) {
		data[i] = data[d];
	}
	return data;
}

// set parameters
void XiaCFDFilter::SetParameters(unsigned int L_, unsigned int G_, unsigned int D_, unsigned int W_, unsigned int dt_) {
	SetParameters(L_/dt_, (L_+G_)/dt_, D_/dt_, W_);
	return;
}


// set paraemters
void XiaCFDFilter::SetParameters(size_t l_, size_t m_, size_t d_, unsigned int w_) {
	l = l_;
	m = m_;
	fastFilter.SetParameters(l, m);
	d = d_;
	w = w_;
	return;
}


// // add xia fast filter
// void XiaCFDFilter::AddXiaFastFilter(std::unique_ptr<FilterAlgorithm> filter_) {
// 	fastFilter = std::move(filter_);
// 	return;
// }


void XiaCFDFilter::SetFastFilterParameters(size_t l_, size_t m_) {
	SetParameters(l_, m_, d, w);
	return;
}