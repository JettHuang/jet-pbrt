// \brief
//		sampler
//

#pragma once

#include "pbrt.h"
#include "geometry.h"


namespace pbrt
{

// random number generator
// https://github.com/SmallVCM/SmallVCM/blob/master/src/rng.hxx
class FRNG
{
public:
	std::mt19937_64 rng_engine;

	std::uniform_int_distribution<int> int_dist;
	std::uniform_int_distribution<uint32_t> uint_dist;
	std::uniform_real_distribution<Float> float_dist{ (Float)0, (Float)1 };

	// constructor
	FRNG(int seed = 1234)
		: rng_engine(seed)
	{}

	// [0, int_max]
	int uniform_int()
	{
		return int_dist(rng_engine);
	}

	// [0, uint_max]
	uint32_t uniform_uint()
	{
		return uint_dist(rng_engine);
	}

	// [0, 1)
	Float uniform_float()
	{
		return float_dist(rng_engine);
	}

	// [0, 1)  [0, 1)
	FFloat2 uniform_float2()
	{
		return FFloat2(uniform_float(), uniform_float());
	}

};

// camera sample
struct FCameraSample
{
	FPoint2  posfilm; // position on film
};


// sampler
class FSampler
{
public:
	virtual ~FSampler() {}

	FSampler(int samples_per_pixel)
		: samples_per_pixel(samples_per_pixel)
	{}


	virtual std::unique_ptr<FSampler> Clone() = 0;

	virtual int GetSamplesPerPixel()
	{
		return samples_per_pixel;
	}

	virtual void SetSamplesPerPixel(int samples)
	{
		samples_per_pixel = samples;
	}

	virtual void StartPixel()
	{
		current_sample_index = 0;
	}

	virtual bool NextSample()
	{
		current_sample_index++;
		return current_sample_index < samples_per_pixel;
	}

	virtual Float GetFloat() = 0;
	virtual FFloat2 GetFloat2() = 0;
	virtual FCameraSample GetCameraSample(const FPoint2& posfilm) = 0;

protected:
	FRNG	rng;
	int		samples_per_pixel;
	int		current_sample_index;
};


// sampler for debug
class FDebugSampler : public FSampler
{
public:
	using FSampler::FSampler;

	virtual std::unique_ptr<FSampler> Clone() override
	{
		return std::make_unique<FDebugSampler>(samples_per_pixel);
	}

	virtual Float GetFloat() override { return 0.5f; }
	virtual FFloat2 GetFloat2() override { return FFloat2(0.5f, 0.5f); }
	virtual FCameraSample GetCameraSample(const FPoint2& posfilm) override
	{
		FCameraSample sample;

		sample.posfilm = posfilm + FPoint2(0.5f, 0.5f);
	}
};

// random sampler
class FRandomSampler : public FSampler
{
public:
	using FSampler::FSampler;

	virtual std::unique_ptr<FSampler> Clone() override
	{
		return std::make_unique<FRandomSampler>(samples_per_pixel);
	}

	virtual Float GetFloat() override { 
		return rng.uniform_float();
	}

	virtual FFloat2 GetFloat2() override { 
		return rng.uniform_float2();
	}

	virtual FCameraSample GetCameraSample(const FPoint2& posfilm) override
	{
		FCameraSample sample;

		sample.posfilm = posfilm + rng.uniform_float2();

		return sample;
	}
};

// stratified sampler
// TODO:
class FStratifiedSampler : public FSampler
{
public:
	using FSampler::FSampler;

	virtual std::unique_ptr<FSampler> Clone() override
	{
		return std::make_unique<FStratifiedSampler>(samples_per_pixel);
	}

	virtual Float GetFloat() override {
		return rng.uniform_float();
	}

	virtual FFloat2 GetFloat2() override {
		return rng.uniform_float2();
	}

	virtual FCameraSample GetCameraSample(const FPoint2& posfilm) override
	{
		FCameraSample sample;

		sample.posfilm = posfilm + rng.uniform_float2();
		return sample;
	}
};

} // namespace pbrt

