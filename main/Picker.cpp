#include <functional>
#include <iostream>
#include <exception>
#include <string>

#include "Picker.h"

//--------------------------------------------------
//					help functions
//--------------------------------------------------

double zeroPointCubicBinary(double y0, double y1, double y2, double y3) {
	double c0 = y1;
	double c1 = -y3/6.0 + y2 - y1/2.0 - y0/3.0;
	double c2 = y2/2.0 - y1 + y0/2.0;
	double c3 = y3/6.0 - y2/2.0 + y1/2.0 - y0/6.0;

	std::function<double(double)> cubic = [=](double x) {
		double t = c3;
		t = t * x + c2;
		t = t * x + c1;
		t = t * x + c0;
		return t;
	};

	// binary search for zero cross point
	double l = 0.0;
	double r = 1.0;
	double m;
	const double eps = 1e-6;
	const int loop = 1000;
	for (int j = 0; j != loop; ++j) {
		m = (l+r)/2.0;
		double valm = cubic(m);
		if (abs(valm) < eps) {
			return m;
		}
		if (valm < 0) {
			r = m;
		} else {
			l = m;
		}
	}

	return (l+r)/2.0;
}


//--------------------------------------------------
//						Picker
//--------------------------------------------------
Picker::Picker() {
}


Picker::~Picker() {
}


//--------------------------------------------------
//						MaxPicker
//--------------------------------------------------

MaxPicker::MaxPicker(): Picker() {
}


MaxPicker::~MaxPicker() {
}


std::unique_ptr<Picker> MaxPicker::Clone() const{
	return std::make_unique<MaxPicker>();
}


double MaxPicker::Pick(const std::vector<double> &data) {
	double maxPoint = data[0];
	for (auto &d : data) {
		maxPoint = maxPoint < d ? d : maxPoint;
	}
	return maxPoint;
}





//--------------------------------------------------
//					BasePicker
//--------------------------------------------------

/*
 * constructor
 *  @len_: Set the length of the range to calculate the base.
 *  @start_: Set the start of the range to calculate the base.
 */
BasePicker::BasePicker(size_t len_, size_t start_) {
	len = len_;
	start = start_;
}


/*
 * deconstructor
 *  do nothing now
 */
BasePicker::~BasePicker() {
}


std::unique_ptr<Picker> BasePicker::Clone() const {
	return std::make_unique<BasePicker>(len, start);
}


/*
 * Pick
 *  Calculate the average value of the range slected by start and len.
 *
 *  @data: The trace being processed.
 */
double BasePicker::Pick(const std::vector<double> &data) {
	if (start < 0) throw std::runtime_error("Error: BasePicker's start point is smaller than 0: " + std::to_string(start) + ".");
	if (start+len > data.size()) throw std::runtime_error("Error: BasePicker's range overflow: data size: " + std::to_string(data.size()) + ", start: " + std::to_string(start) + ", len: " + std::to_string(len) + ".");
	double ret = 0.0;
	for (size_t i = start; i != len; ++i) {
		ret += data[i];
	}
	ret /= len;
	return ret;
}


//--------------------------------------------------
//					TopBasePicker
//--------------------------------------------------

/*
 * constructor
 *  @len_: Set the length of the range to calculate the top base.
 *  @stop_: Set the stop point of the range to calculate the top base.
 */
TopBasePicker::TopBasePicker(size_t len_, size_t stop_) {
	len = len_;
	stop = stop_;
}


/*
 * Clone function
 *  Clone the picker.
 */
std::unique_ptr<Picker> TopBasePicker::Clone() const {
	return std::make_unique<TopBasePicker>(len, stop);
}


/*
 * Pick
 *  Calculate the average value from the end of the data,
 *  with the range selected by len and stop parameters.
 */
double TopBasePicker::Pick(const std::vector<double> &data) {
	if (stop < 0) throw std::runtime_error("Error: TopBasePicker's stop point is smaller than 0: " + std::to_string(stop) + ".");
	if (stop+len > data.size()) throw std::runtime_error("Error: TopBasePicker's range overflow: data size: " + std::to_string(data.size()) + ", stop: " + std::to_string(stop) + ", len: " + std::to_string(len) + ".");
	double ret = 0.0;
	size_t vsize = data.size();
	for (size_t i = vsize-stop-len; i != vsize-stop; ++i) {
		ret += data[i];
	}
	ret /= len;
	return ret;
}


//--------------------------------------------------
//				TrapezoidTopPicker
//--------------------------------------------------

// This picker picks the value from trapezoidal trace.
// It picks the rightmost point of the trapezoid's top base.
TrapezoidTopPicker::TrapezoidTopPicker(size_t ts_, size_t l_, size_t m_): Picker() {
	l = l_;
	m = m_;
	ts = ts_;
}




TrapezoidTopPicker::~TrapezoidTopPicker() {
}


std::unique_ptr<Picker> TrapezoidTopPicker::Clone() const {
	return std::make_unique<TrapezoidTopPicker>(ts, l, m);
}


double TrapezoidTopPicker::Pick(const std::vector<double> &data) {
	// size_t fl = 0;
	size_t fr = 0;
	// T minL = T(0);
	double minR = 0.0;
	const size_t diffLen = 8;
	// for (size_t i = ts-11+l; i != ts-1+l; ++i) {
	// 	T diff = T(0);
	// 	diff += data[i+diffLen]*2 + data[i+diffLen+1] + data[i+diffLen-1];
	// 	diff += data[i-diffLen]*2 + data[i-diffLen-1] + data[i+diffLen+1];
	// 	diff -= data[i]*4 + data[i-1]*2 + data[i+1]*2;
	// 	if (diff < minL) {
	// 		minL = diff;
	// 		fl = i;
	// 	}
	// }
	for (size_t i = ts-11+m; i != ts-1+m; ++i) {
		double diff = 0.0;
		diff += data[i+diffLen]*2 + data[i+diffLen+1] + data[i+diffLen-1];
		diff += data[i-diffLen]*2 + data[i-diffLen-1] + data[i+diffLen+1];
		diff -= data[i]*4 + data[i-1]*2 + data[i+1]*2;
		if (diff < minR) {
			minR = diff;
			fr = i;
		}
	}
	return data[fr];
}





//--------------------------------------------------
//					LeadingEdgePicker
//--------------------------------------------------
LeadingEdgePicker::LeadingEdgePicker(unsigned int thres_): Picker() {
	threshold = thres_;
}


LeadingEdgePicker::~LeadingEdgePicker() {
}


std::unique_ptr<Picker> LeadingEdgePicker::Clone() const {
	return std::make_unique<LeadingEdgePicker>(threshold);
}


double LeadingEdgePicker::Pick(const std::vector<double> &data) {
// std::cout << "le-picker: size " << data.size() << std::endl;
	size_t ts = 0;
	for (auto &d : data) {
		if (d > threshold) break;
		++ts;
	}
// std::cout << "le-picker: threshold " << threshold << "  data " << data[ts] << "  " << data[ts+1] << std::endl;
	return double(ts);
}



//--------------------------------------------------
//					ZeroCrossPicker
//--------------------------------------------------
ZeroCrossPicker::ZeroCrossPicker(size_t ts_, unsigned int thres_, bool cubic_): Picker() {
	ts = ts_;
	threshold = thres_;
	cubic = cubic_;
}


ZeroCrossPicker::~ZeroCrossPicker() {
}


std::unique_ptr<Picker> ZeroCrossPicker::Clone() const {
	return std::make_unique<ZeroCrossPicker>(ts, threshold, cubic);
}


double ZeroCrossPicker::Pick(const std::vector<double> &data) {
	if (cubic) {		// cubic fit

		bool overThres = false;
		size_t vsize = data.size()-2;
		for (size_t i = ts; i != vsize; ++i) {
			if (data[i] > threshold) overThres = true;
			if (!overThres) continue;
			if (data[i] >= 0 && data[i] < 0) {
				return i + zeroPointCubicBinary(data[i-1], data[i], data[i+1], data[i+2]);
			}
		}

	} else {			// linear fit
		bool overThres = false;
		size_t vsize = data.size()-1;
		for (size_t i = ts; i != vsize; ++i) {
			if (data[i] > threshold) overThres = true;
			if (!overThres) continue;
			if (data[i] >= 0 && data[i+1] < 0) {
				return i + data[i] / (data[i] - data[i+1]);
			}
		}
	}
	return -1.0;
}



//--------------------------------------------------
//					DigitalFractionPicker
//--------------------------------------------------

DigitalFractionPicker::DigitalFractionPicker(size_t ts_, double fraction_, bool cubic_, size_t baseLen_):
topPicker(baseLen_), basePicker(baseLen_)
{
	ts = ts_;
	fraction = fraction_;
	cubic = cubic_;
	baseLen = baseLen_;
}


DigitalFractionPicker::~DigitalFractionPicker() {
}


std::unique_ptr<Picker> DigitalFractionPicker::Clone() const {
	return std::make_unique<DigitalFractionPicker>(ts, fraction, cubic, baseLen);
}


double DigitalFractionPicker::Pick(const std::vector<double> &data) {
	double base = basePicker.Pick(data);
	double topBase = topPicker.Pick(data);
	double threshold = base + (topBase-base) * fraction;
	if (cubic) {							// cubic
		size_t vsize = data.size()-2;
		for (size_t i = ts; i != vsize; ++i) {
			if (data[i] <= threshold && data[i+1] > threshold) {
				return i + zeroPointCubicBinary(threshold-data[i-1], threshold-data[i], threshold-data[i+1], threshold-data[i+2]);
			}
		}
	} else {								// linear
// std::cout << "base  " << base << "  top  " << topBase << "  thres  " << threshold << std::endl;
// for (size_t i = 0; i != 60; ++i) {
// 	std::cout << data[i] << " \n"[(i+1)%10==0];
// }
		size_t vsize = data.size()-1;
		for (size_t i = ts; i != vsize; ++i) {
			if (data[i] <= threshold && data[i+1] > threshold) {
// std::cout << "dfp: thres  " << threshold << "  " << data[i] << "  " << data[i+1] << std::endl;
				return i + (threshold - data[i]) / (data[i+1] - data[i]);
			}
		}
	}
	return -1.0;
}

