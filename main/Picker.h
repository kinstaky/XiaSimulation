#ifndef __PICKER_H__
#define __PICKER_H__

#include <vector>
#include <memory>


// Picker, pick the energy, timestamp, cfd from the filtered trace
class Picker {
public:
	virtual ~Picker();
	virtual std::unique_ptr<Picker> Clone() const = 0;
	virtual double Pick(const std::vector<double> &data) = 0;
protected:
	Picker();
};


// pick the max value
class MaxPicker: public Picker {
public:
	MaxPicker();
	virtual ~MaxPicker();
	virtual std::unique_ptr<Picker> Clone() const override;
	virtual double Pick(const std::vector<double> &data) override;
};


// pick the base
class BasePicker: public Picker {
public:
	BasePicker(size_t len_, size_t start_ = 0);
	virtual ~BasePicker();
	virtual std::unique_ptr<Picker> Clone() const override;
	virtual double Pick(const std::vector<double> &data) override;
protected:
	size_t len;
private:
	size_t start;
};


// pick the top base
class TopBasePicker: public Picker {
public:
	TopBasePicker(size_t len_, size_t stop_ = 0);
	virtual ~TopBasePicker() = default;
	virtual std::unique_ptr<Picker> Clone() const override;
	virtual double Pick(const std::vector<double> &data) override;
private:
	size_t len;
	size_t stop;
};


// pick the right most point at the top of the trapezoid
class TrapezoidTopPicker: public Picker {
public:
	TrapezoidTopPicker(size_t ts_, size_t l_, size_t m_);
	virtual ~TrapezoidTopPicker();
	virtual std::unique_ptr<Picker> Clone() const override;
	virtual double Pick(const std::vector<double> &data) override;
private:
	size_t ts;
	size_t l;
	size_t m;
};


// pick the first point over threshold
class LeadingEdgePicker: public Picker {
public:
	LeadingEdgePicker(unsigned int thres_);
	virtual ~LeadingEdgePicker();
	virtual std::unique_ptr<Picker> Clone() const override;
	virtual double Pick(const std::vector<double> &data) override;
private:
	unsigned int threshold;
};


// pick the root that cross zero point
class ZeroCrossPicker: public Picker {
public:
	ZeroCrossPicker(size_t ts_, unsigned int thres_, bool cubic_);
	virtual ~ZeroCrossPicker();
	virtual std::unique_ptr<Picker> Clone() const override;
	virtual double Pick(const std::vector<double> &data) override;
private:
	size_t ts;
	unsigned int threshold;
	bool cubic;
};


// pick the value that over the const fraction threshold
class DigitalFractionPicker: public Picker {
public:
	DigitalFractionPicker(size_t ts_, double fraction_, bool cubic_, size_t baseLen_);
	virtual ~DigitalFractionPicker();
	virtual std::unique_ptr<Picker> Clone() const override;
	virtual double Pick(const std::vector<double> &data) override;
private:
	size_t ts;
	double fraction;
	bool cubic;
	size_t baseLen;

	TopBasePicker topPicker;
	BasePicker basePicker;
};





#endif