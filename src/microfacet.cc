// \brief
//		microfacet.cc
//

#include "microfacet.h"
#include "bsdf.h"


namespace pbrt {

	inline Float ErfInv(Float x) {
		Float w, p;
		x = Clamp(x, -.99999f, .99999f);
		w = -std::log((1 - x) * (1 + x));
		if (w < 5) {
			w = w - 2.5f;
			p = 2.81022636e-08f;
			p = 3.43273939e-07f + p * w;
			p = -3.5233877e-06f + p * w;
			p = -4.39150654e-06f + p * w;
			p = 0.00021858087f + p * w;
			p = -0.00125372503f + p * w;
			p = -0.00417768164f + p * w;
			p = 0.246640727f + p * w;
			p = 1.50140941f + p * w;
		}
		else {
			w = std::sqrt(w) - 3;
			p = -0.000200214257f;
			p = 0.000100950558f + p * w;
			p = 0.00134934322f + p * w;
			p = -0.00367342844f + p * w;
			p = 0.00573950773f + p * w;
			p = -0.0076224613f + p * w;
			p = 0.00943887047f + p * w;
			p = 1.00167406f + p * w;
			p = 2.83297682f + p * w;
		}
		return p * x;
	}

	inline Float Erf(Float x) {
		// constants
		Float a1 = 0.254829592f;
		Float a2 = -0.284496736f;
		Float a3 = 1.421413741f;
		Float a4 = -1.453152027f;
		Float a5 = 1.061405429f;
		Float p = 0.3275911f;

		// Save the sign of x
		int sign = 1;
		if (x < 0) sign = -1;
		x = std::abs(x);

		// A&S formula 7.1.26
		Float t = 1 / (1 + p * x);
		Float y =
			1 -
			(((((a5 * t + a4) * t) + a3) * t + a2) * t + a1) * t * std::exp(-x * x);

		return sign * y;
	}

    // Microfacet Utility Functions
    static void BeckmannSample11(Float cosThetaI, Float U1, Float U2,
        Float* slope_x, Float* slope_y) {
        /* Special case (normal incidence) */
        if (cosThetaI > .9999f) {
            Float r = std::sqrt(-std::log(1.0f - U1));
            Float sinPhi = std::sin(2 * kPi * U2);
            Float cosPhi = std::cos(2 * kPi * U2);
            *slope_x = r * cosPhi;
            *slope_y = r * sinPhi;
            return;
        }

        /* The original inversion routine from the paper contained
           discontinuities, which causes issues for QMC integration
           and techniques like Kelemen-style MLT. The following code
           performs a numerical inversion with better behavior */
        Float sinThetaI =
            std::sqrt(std::max((Float)0, (Float)1 - cosThetaI * cosThetaI));
        Float tanThetaI = sinThetaI / cosThetaI;
        Float cotThetaI = 1 / tanThetaI;

        /* Search interval -- everything is parameterized
           in the Erf() domain */
        Float a = -1, c = Erf(cotThetaI);
        Float sample_x = std::max(U1, (Float)1e-6f);

        /* Start with a good initial guess */
        // Float b = (1-sample_x) * a + sample_x * c;

        /* We can do better (inverse of an approximation computed in
         * Mathematica) */
        Float thetaI = std::acos(cosThetaI);
        Float fit = 1 + thetaI * (-0.876f + thetaI * (0.4265f - 0.0594f * thetaI));
        Float b = c - (1 + c) * std::pow(1 - sample_x, fit);

        /* Normalization factor for the CDF */
        static const Float SQRT_PI_INV = 1.f / std::sqrt(kPi);
        Float normalization =
            1 /
            (1 + c + SQRT_PI_INV * tanThetaI * std::exp(-cotThetaI * cotThetaI));

        int it = 0;
        while (++it < 10) {
            /* Bisection criterion -- the oddly-looking
               Boolean expression are intentional to check
               for NaNs at little additional cost */
            if (!(b >= a && b <= c)) b = 0.5f * (a + c);

            /* Evaluate the CDF and its derivative
               (i.e. the density function) */
            Float invErf = ErfInv(b);
            Float value =
                normalization *
                (1 + b + SQRT_PI_INV * tanThetaI * std::exp(-invErf * invErf)) -
                sample_x;
            Float derivative = normalization * (1 - invErf * tanThetaI);

            if (std::abs(value) < 1e-5f) break;

            /* Update bisection intervals */
            if (value > 0)
                c = b;
            else
                a = b;

            b -= value / derivative;
        }

        /* Now convert back into a slope value */
        *slope_x = ErfInv(b);

        /* Simulate Y component */
        *slope_y = ErfInv(2.0f * std::max(U2, (Float)1e-6f) - 1.0f);

        PBRT_DOCHECK(!std::isinf(*slope_x));
        PBRT_DOCHECK(!std::isnan(*slope_x));
        PBRT_DOCHECK(!std::isinf(*slope_y));
        PBRT_DOCHECK(!std::isnan(*slope_y));
    }

    static FVector3 BeckmannSample(const FVector3& wi, Float alpha_x, Float alpha_y,
        Float U1, Float U2) {
        // 1. stretch wi
        FVector3 wiStretched =
            Normalize(FVector3(alpha_x * wi.x, alpha_y * wi.y, wi.z));

        // 2. simulate P22_{wi}(x_slope, y_slope, 1, 1)
        Float slope_x, slope_y;
        BeckmannSample11(cos_theta(wiStretched), U1, U2, &slope_x, &slope_y);

        // 3. rotate
        Float tmp = CosPhi(wiStretched) * slope_x - SinPhi(wiStretched) * slope_y;
        slope_y = SinPhi(wiStretched) * slope_x + CosPhi(wiStretched) * slope_y;
        slope_x = tmp;

        // 4. unstretch
        slope_x = alpha_x * slope_x;
        slope_y = alpha_y * slope_y;

        // 5. compute normal
        return Normalize(FVector3(-slope_x, -slope_y, 1.f));
    }

    // MicrofacetDistribution Method Definitions
    MicrofacetDistribution::~MicrofacetDistribution() {}

    Float BeckmannDistribution::D(const FVector3& wh) const {
        Float tan2Theta = Tan2Theta(wh);
        if (std::isinf(tan2Theta)) return 0.;
        Float cos4Theta = Cos2Theta(wh) * Cos2Theta(wh);
        return std::exp(-tan2Theta * (Cos2Phi(wh) / (alphax * alphax) +
            Sin2Phi(wh) / (alphay * alphay))) /
            (kPi * alphax * alphay * cos4Theta);
    }

    Float TrowbridgeReitzDistribution::D(const FVector3& wh) const {
        Float tan2Theta = Tan2Theta(wh);
        if (std::isinf(tan2Theta)) return 0.;
        const Float cos4Theta = Cos2Theta(wh) * Cos2Theta(wh);
        Float e =
            (Cos2Phi(wh) / (alphax * alphax) + Sin2Phi(wh) / (alphay * alphay)) *
            tan2Theta;
        return 1 / (kPi * alphax * alphay * cos4Theta * (1 + e) * (1 + e));
    }

    Float BeckmannDistribution::Lambda(const FVector3& w) const {
        Float absTanTheta = std::abs(TanTheta(w));
        if (std::isinf(absTanTheta)) return 0.;
        // Compute _alpha_ for direction _w_
        Float alpha =
            std::sqrt(Cos2Phi(w) * alphax * alphax + Sin2Phi(w) * alphay * alphay);
        Float a = 1 / (alpha * absTanTheta);
        if (a >= 1.6f) return 0;
        return (1 - 1.259f * a + 0.396f * a * a) / (3.535f * a + 2.181f * a * a);
    }

    Float TrowbridgeReitzDistribution::Lambda(const FVector3& w) const {
        Float absTanTheta = std::abs(TanTheta(w));
        if (std::isinf(absTanTheta)) return 0.;
        // Compute _alpha_ for direction _w_
        Float alpha =
            std::sqrt(Cos2Phi(w) * alphax * alphax + Sin2Phi(w) * alphay * alphay);
        Float alpha2Tan2Theta = (alpha * absTanTheta) * (alpha * absTanTheta);
        return (-1 + std::sqrt(1.f + alpha2Tan2Theta)) / 2;
    }

    FVector3 BeckmannDistribution::Sample_wh(const FVector3& wo,
        const FPoint2& u) const {
        if (!sampleVisibleArea) {
            // Sample full distribution of normals for Beckmann distribution

            // Compute $\tan^2 \theta$ and $\phi$ for Beckmann distribution sample
            Float tan2Theta, phi;
            if (alphax == alphay) {
                Float logSample = std::log(1 - u[0]);
                PBRT_DOCHECK(!std::isinf(logSample));
                tan2Theta = -alphax * alphax * logSample;
                phi = u[1] * 2 * kPi;
            }
            else {
                // Compute _tan2Theta_ and _phi_ for anisotropic Beckmann
                // distribution
                Float logSample = std::log(1 - u[0]);
                PBRT_DOCHECK(!std::isinf(logSample));
                phi = std::atan(alphay / alphax *
                    std::tan(2 * kPi * u[1] + 0.5f * kPi));
                if (u[1] > 0.5f) phi += kPi;
                Float sinPhi = std::sin(phi), cosPhi = std::cos(phi);
                Float alphax2 = alphax * alphax, alphay2 = alphay * alphay;
                tan2Theta = -logSample /
                    (cosPhi * cosPhi / alphax2 + sinPhi * sinPhi / alphay2);
            }

            // Map sampled Beckmann angles to normal direction _wh_
            Float cosTheta = 1 / std::sqrt(1 + tan2Theta);
            Float sinTheta = std::sqrt(std::max((Float)0, 1 - cosTheta * cosTheta));
            FVector3 wh = Spherical_2_Direction(sinTheta, cosTheta, phi);
            if (!same_hemisphere(wo, wh)) wh = -wh;
            return wh;
        }
        else {
            // Sample visible area of normals for Beckmann distribution
            FVector3 wh;
            bool flip = wo.z < 0;
            wh = BeckmannSample(flip ? -wo : wo, alphax, alphay, u[0], u[1]);
            if (flip) wh = -wh;
            return wh;
        }
    }

    static void TrowbridgeReitzSample11(Float cosTheta, Float U1, Float U2,
        Float* slope_x, Float* slope_y) {
        // special case (normal incidence)
        if (cosTheta > .9999) {
            Float r = sqrt(U1 / (1 - U1));
            Float phi = 6.28318530718f * U2;
            *slope_x = r * cos(phi);
            *slope_y = r * sin(phi);
            return;
        }

        Float sinTheta =
            std::sqrt(std::max((Float)0, (Float)1 - cosTheta * cosTheta));
        Float tanTheta = sinTheta / cosTheta;
        Float a = 1 / tanTheta;
        Float G1 = 2 / (1 + std::sqrt(1.f + 1.f / (a * a)));

        // sample slope_x
        Float A = 2 * U1 / G1 - 1;
        Float tmp = 1.f / (A * A - 1.f);
        if (tmp > 1e10) tmp = 1e10;
        Float B = tanTheta;
        Float D = std::sqrt(
            std::max(Float(B * B * tmp * tmp - (A * A - B * B) * tmp), Float(0)));
        Float slope_x_1 = B * tmp - D;
        Float slope_x_2 = B * tmp + D;
        *slope_x = (A < 0 || slope_x_2 > 1.f / tanTheta) ? slope_x_1 : slope_x_2;

        // sample slope_y
        Float S;
        if (U2 > 0.5f) {
            S = 1.f;
            U2 = 2.f * (U2 - .5f);
        }
        else {
            S = -1.f;
            U2 = 2.f * (.5f - U2);
        }
        Float z =
            (U2 * (U2 * (U2 * 0.27385f - 0.73369f) + 0.46341f)) /
            (U2 * (U2 * (U2 * 0.093073f + 0.309420f) - 1.000000f) + 0.597999f);
        *slope_y = S * z * std::sqrt(1.f + *slope_x * *slope_x);

        PBRT_DOCHECK(!std::isinf(*slope_y));
        PBRT_DOCHECK(!std::isnan(*slope_y));
    }

    static FVector3 TrowbridgeReitzSample(const FVector3& wi, Float alpha_x,
        Float alpha_y, Float U1, Float U2) {
        // 1. stretch wi
        FVector3 wiStretched =
            Normalize(FVector3(alpha_x * wi.x, alpha_y * wi.y, wi.z));

        // 2. simulate P22_{wi}(x_slope, y_slope, 1, 1)
        Float slope_x, slope_y;
        TrowbridgeReitzSample11(cos_theta(wiStretched), U1, U2, &slope_x, &slope_y);

        // 3. rotate
        Float tmp = CosPhi(wiStretched) * slope_x - SinPhi(wiStretched) * slope_y;
        slope_y = SinPhi(wiStretched) * slope_x + CosPhi(wiStretched) * slope_y;
        slope_x = tmp;

        // 4. unstretch
        slope_x = alpha_x * slope_x;
        slope_y = alpha_y * slope_y;

        // 5. compute normal
        return Normalize(FVector3(-slope_x, -slope_y, 1.));
    }

    FVector3 TrowbridgeReitzDistribution::Sample_wh(const FVector3& wo,
        const FPoint2& u) const {
        FVector3 wh;
        if (!sampleVisibleArea) {
            Float cosTheta = 0, phi = (2 * kPi) * u[1];
            if (alphax == alphay) {
                Float tanTheta2 = alphax * alphax * u[0] / (1.0f - u[0]);
                cosTheta = 1 / std::sqrt(1 + tanTheta2);
            }
            else {
                phi =
                    std::atan(alphay / alphax * std::tan(2 * kPi * u[1] + .5f * kPi));
                if (u[1] > .5f) phi += kPi;
                Float sinPhi = std::sin(phi), cosPhi = std::cos(phi);
                const Float alphax2 = alphax * alphax, alphay2 = alphay * alphay;
                const Float alpha2 =
                    1 / (cosPhi * cosPhi / alphax2 + sinPhi * sinPhi / alphay2);
                Float tanTheta2 = alpha2 * u[0] / (1 - u[0]);
                cosTheta = 1 / std::sqrt(1 + tanTheta2);
            }
            Float sinTheta =
                std::sqrt(std::max((Float)0., (Float)1. - cosTheta * cosTheta));
            wh = Spherical_2_Direction(sinTheta, cosTheta, phi);
            if (!same_hemisphere(wo, wh)) wh = -wh;
        }
        else {
            bool flip = wo.z < 0;
            wh = TrowbridgeReitzSample(flip ? -wo : wo, alphax, alphay, u[0], u[1]);
            if (flip) wh = -wh;
        }
        return wh;
    }

    Float MicrofacetDistribution::Pdf(const FVector3& wo,
        const FVector3& wh) const {
        if (sampleVisibleArea)
            return D(wh) * G1(wo) * AbsDot(wo, wh) / abs_cos_theta(wo);
        else
            return D(wh) * abs_cos_theta(wh);
    }

}  // namespace pbrt
