#ifndef __FILTERALGORITHM_H__
#define __FILTERALGORITHM_H__

#include <vector>
#include <string>
#include <memory>

// base class of FilterAlgorithm
//  This algorithm do nothing and outputs the input.

class FilterAlgorithm {
public:
	FilterAlgorithm();
	virtual ~FilterAlgorithm();

	virtual const std::vector<double>& Filter(const std::vector<double> &trace);
	virtual std::unique_ptr<FilterAlgorithm> Clone() const;
protected:
	// filtered data
	std::vector<double> data;
};


class SlowFilter: public FilterAlgorithm {
public:
	virtual ~SlowFilter();

	virtual void SetParameters(unsigned int L, unsigned int G, unsigned int tau, unsigned int dt) = 0;
	virtual void SetParameters(size_t l_, size_t m_);
	virtual void GetParameters(size_t &l_, size_t &m_);
protected:
	SlowFilter(size_t l_, size_t m_);

	size_t l;
	size_t m;
};


class MWDAlgorithm: public SlowFilter {
public:
	MWDAlgorithm(size_t l_, size_t m_, double alpha_);
	MWDAlgorithm(unsigned int L, unsigned int G, unsigned int tau, unsigned int dt);
	virtual ~MWDAlgorithm();
	virtual std::unique_ptr<FilterAlgorithm> Clone() const override;

	virtual const std::vector<double>& Filter(const std::vector<double> &trace) override;

	virtual void SetParameters(unsigned int L, unsigned int G, unsigned int tau, unsigned int dt) override;
	virtual void SetParameters(size_t l_, size_t m_, unsigned int tau, unsigned int dt);
private:
	double alpha;
	// int avgLen;						// average length, equal to 2^filter_range
};



class XiaSlowFilter: public SlowFilter {
public:
	XiaSlowFilter(size_t l_, size_t m_, double b_);
	XiaSlowFilter(unsigned int L, unsigned int G, unsigned int tau, unsigned int dt);
	virtual ~XiaSlowFilter();
	virtual std::unique_ptr<FilterAlgorithm> Clone() const override;

	virtual const std::vector<double> &Filter(const std::vector<double> &trace) override;

	virtual void SetParameters(unsigned int L, unsigned int G, unsigned int tau, unsigned int dt) override;
	virtual void SetParameters(size_t l_, size_t m_, unsigned int tau, unsigned int dt);
private:
	double b;
};



class FastFilter: public FilterAlgorithm {
public:
	virtual ~FastFilter();

	virtual void SetParameters(unsigned int L, unsigned int G, unsigned int dt);
	virtual void SetParameters(size_t l_, size_t m_);
	virtual void GetParameters(size_t &l_, size_t &m_);
protected:
	FastFilter(size_t l_, size_t m_);

	size_t l;
	size_t m;
};



class XiaFastFilter: public FastFilter {
public:
	XiaFastFilter(size_t l_, size_t m_);
	XiaFastFilter(unsigned int L, unsigned int G, unsigned int dt);
	virtual ~XiaFastFilter();
	virtual std::unique_ptr<FilterAlgorithm> Clone() const override;

	virtual const std::vector<double> &Filter(const std::vector<double> &trace) override;
};



class XiaCFDFilter: public FilterAlgorithm {
public:
	XiaCFDFilter(size_t l_, size_t m_, size_t delay_, unsigned int factor_);
	XiaCFDFilter(unsigned int L_, unsigned int G_, unsigned int D_, unsigned int factor_, unsigned int dt);
	virtual ~XiaCFDFilter();
	virtual std::unique_ptr<FilterAlgorithm> Clone() const override;

	virtual void SetParameters(unsigned int L_, unsigned int G_, unsigned int D_, unsigned int W_, unsigned int dt_);
	virtual void SetParameters(size_t l_, size_t m_, size_t d_, unsigned int w_);
	virtual void SetFastFilterParameters(size_t l_, size_t m_);

	virtual const std::vector<double> &Filter(const std::vector<double> &fast) override;

	// virtual void AddXiaFastFilter(std::unique_ptr<FilterAlgorithm> filter_);
private:
	XiaFastFilter fastFilter;
	size_t l;						// fast l
	size_t m;						// fast m
	size_t d;						// delay
	unsigned int w;
};

#endif