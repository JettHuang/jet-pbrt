// \brief
//		geometry
//

#pragma once

#include "pbrt.h"
#include "color.h"


namespace pbrt
{

enum eAxis
{
	AXIS_X = 0,
	AXIS_Y = 1,
	AXIS_Z = 2
};

// Vector2
class FVector2
{
public:
	Float x, y;

	FVector2() : x(0), y(0) { }
	FVector2(Float vx, Float vy) : x(vx), y(vy) {}

	Float operator[] (int i) const 
	{ 
		PBRT_DOCHECK(i >= 0 && i <= 1); 
		
		return  i == 0 ? x : y;
	}

	Float& operator[] (int i)
	{
		PBRT_DOCHECK(i >= 0 && i <= 1);

		return  i == 0 ? x : y;
	}

	FVector2& operator +=(const FVector2& v) { x += v.x; y += v.y; return *this; }
	FVector2& operator -=(const FVector2& v) { x -= v.x; y -= v.y; return *this; }

	FVector2 operator +(const FVector2& v) const {
		return FVector2(x + v.x, y + v.y);
	}
	FVector2 operator -(const FVector2& v) const {
		return FVector2(x - v.x, y - v.y);
	}

	friend FVector2 operator* (Float s, const FVector2& v)
	{
		return FVector2(v.x * s, v.y * s);
	}
};

typedef FVector2 FPoint2;
typedef FVector2 FFloat2;


// Vector3
class FVector3
{
public:
	Float x, y, z;

	FVector3() : x(0), y(0), z(0) {}
	FVector3(Float vx, Float vy, Float vz) : x(vx), y(vy), z(vz) {}

	Float operator[] (int i) const
	{
		PBRT_DOCHECK(i >= 0 && i <= 2);

		if (i == 0) return x;
		if (i == 1) return y;
		return z;
	}

	Float& operator[] (int i)
	{
		PBRT_DOCHECK(i >= 0 && i <= 2);

		if (i == 0) return x;
		if (i == 1) return y;
		return z;
	}

	FVector3 operator-() const { return FVector3(-x, -y, -z); }

	FVector3& operator += (const FVector3& v) { x += v.x; y += v.y; z += v.z; return *this; }
	FVector3& operator -= (const FVector3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
	FVector3& operator *= (Float s) { x *= s; y *= s; z *= s; return *this; }
	FVector3& operator /= (Float s) { x /= s; y /= s; z /= s; return *this; }

	FVector3 operator + (const FVector3& v) const { return FVector3(x + v.x, y + v.y, z + v.z); }
	FVector3 operator - (const FVector3& v) const { return FVector3(x - v.x, y - v.y, z - v.z); }
	FVector3 operator * (Float s) const { return FVector3(x * s, y * s, z * s); }
	FVector3 operator / (Float s) const { return FVector3(x / s, y / s, z / s); }

	Float Length2() const { return x * x + y * y + z * z; }
	Float Length() const { return std::sqrt(Length2()); }

	FVector3 Normalize() const { return *this / Length(); }
	bool IsUnit() const { return isEqual(Length2(), (Float)1); }

	Float Dot(const FVector3& v) const { return x * v.x + y * v.y + z * v.z; }
	FVector3 Cross(const FVector3& v) const
	{
		/*
			|  i  j  k |
			|  x  y  z |
			| vx vy vz |
		*/
		return FVector3(
			y * v.z - z * v.y,
			z * v.x - x * v.z,
			x * v.y - y * v.x);
	}

	operator FColor() { return FColor(x, y, z); }

	// debug
	bool IsValid() const { return isValid(x) && isValid(y) && isValid(z); }
	bool IsZero() const { return (x == 0) && (y == 0) && (z == 0); }
	bool HasNegative() const { return (x < 0) || (y < 0) || (z < 0); }
	bool LessThan(const FVector3& v) const { return (x < v.x) || (y < v.y) || (z < v.z); }


	// friends
	friend FVector3 operator* (Float s, const FVector3& v) { return FVector3(v.x * s, v.y * s, v.z * s); }
	friend Float Dot(const FVector3& u, const FVector3& v) { return u.Dot(v); }
	friend Float AbsDot(const FVector3& u, const FVector3& v) { return std::abs(u.Dot(v)); }
	friend FVector3 Cross(const FVector3& u, const FVector3& v) { return u.Cross(v); }
	friend FVector3 Normalize(const FVector3& v) { return v.Normalize(); }

	friend Float Cosine(const FVector3& u, const FVector3& v) { return u.Dot(v); }
	friend Float AbsCosine(const FVector3& u, const FVector3& v) { return std::abs(u.Dot(v)); }

	friend FVector3 Lerp(const FVector3& u, const FVector3& v, Float t) { return u + t * (v - u); }

	// per-component
	friend FVector3 Min(const FVector3& a, const FVector3& b)
	{
		return FVector3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
	}
	friend FVector3 Max(const FVector3& a, const FVector3& b)
	{
		return FVector3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
	}

	friend std::ostream& operator <<(std::ostream& os, const FVector3& v)
	{
		os << "{" << v.x << "," << v.y << "," << v.z << "}";
		return os;
	}
};

typedef FVector3 FPoint3;
typedef FVector3 FNormal3;

inline Float Distance(const FPoint3& p1, const FPoint3& p2)
{
	return (p1 - p2).Length();
}

inline Float Distance2(const FPoint3& p1, const FPoint3& p2)
{
	return (p1 - p2).Length2();
}


/*
	 z(0, 0, 1)
		  |
		  | theta/
		  |    /
		  |  /
		  |/_ _ _ _ _ _ x(1, 0, 0)
		 / \
		/ phi\
	   /       \
	  /          \
 y(0, 1, 0)

   https://www.pbr-book.org/3ed-2018/Shapes/Spheres
*/
// unit direction vector ---> spherical coordinate
inline Float SphericalTheta(const FVector3 &v)
{
	return std::acos(Clamp(v.z, (Float)-1, (Float)1));
}

inline Float SphericalPhi(const FVector3& v)
{
	Float phi = std::atan2(v.y, v.x);
	return (phi < 0) ? (phi + k2Pi) : phi;
}

// convert spherical coordinate (¦È theta, ¦Õ phi) into direction vector (x, y, z)
inline FVector3 Spherical_2_Direction(Float sin_theta, Float cos_theta, Float phi)
{
	return FVector3(
		sin_theta * std::cos(phi),
		sin_theta * std::sin(phi),
		cos_theta);
}

// takes three basis vectors representing the x, y, and z axis and
// returns the appropriate direction vector with respect to the coordinate frame defined by them
inline FVector3 Spherical_2_Direction(
	Float sin_theta, Float cos_theta, Float phi,
	FVector3 x, FVector3 y, FVector3 z)
{
	return
		sin_theta * std::cos(phi) * x +
		sin_theta * std::sin(phi) * y +
		cos_theta * z;
}


/* axis align bounding box
			  y         z
			  |       /
			  |     /
			  |   /
			  | /
			  o - - - - x

					   bounds3_t.max
				  3-------2
				 /|      /|
				4-------1 |
				| |     | |
				| 7-----|-6
				|/      |/
				8-------5
		bounds3_t.min
*/
class FRay;

class FBounds3
{
public:
	FPoint3 _min, _max;

	FBounds3()
	{
		PBRT_CONSTEXPR Float min = std::numeric_limits<Float>::lowest();
		PBRT_CONSTEXPR Float max = std::numeric_limits<Float>::max();

		_min = FPoint3(max, max, max);
		_max = FPoint3(min, min, min);
	}

	explicit FBounds3(const FPoint3& p) : _min(p), _max(p) {}

	FBounds3(const FPoint3& p1, const FPoint3& p2)
		: _min(std::min(p1.x, p2.x), std::min(p1.y, p2.y), std::min(p1.z, p2.z))
		, _max(std::max(p1.x, p2.x), std::max(p1.y, p2.y), std::max(p1.z, p2.z))
	{
	}

	void Expand(const FPoint3& p)
	{
		_min = Min(_min, p);
		_max = Max(_max, p);
	}

	void Expand(const FBounds3& b)
	{
		_min = Min(_min, b._min);
		_max = Max(_max, b._max);
	}

	// union, merge
	FBounds3 Join(const FPoint3& p) const
	{
		return FBounds3(Min(_min, p), Max(_max, p));
	}

	FBounds3 Join(const FBounds3& b) const
	{
		return FBounds3(Min(_min, b._min), Max(_max, b._max));
	}

	friend FBounds3 Join(const FBounds3& b, const FPoint3& p) { return b.Join(p); }
	friend FBounds3 Join(const FBounds3& b1, const FBounds3& b2) { return b1.Join(b2); }

	bool IsContain(const FPoint3& p) const
	{
		return p.x >= _min.x && p.x <= _max.x
			&& p.y >= _min.y && p.y <= _max.y
			&& p.z >= _min.z && p.z <= _max.z;
	}

	// return a sphere that hold this bounding box
	void BoundingSphere(FPoint3& center, Float& radius) const
	{
		center = Lerp(_min, _max, (Float)0.5);
		radius = IsContain(center) ? Distance(center, _max) : (Float)0;
	}

	bool Intersect(const FRay& ray) const;

};

// matrix4x4
class FMatrix44
{

};


// https://github.com/SmallVCM/SmallVCM/blob/master/src/frame.hxx
// tangent space frame
class FFrame
{
public:
	FVector3 s, t;
	FNormal3 n;

	FFrame() 
		: s(1, 0, 0) // x-axis
		, t(0, 1, 0) // y-axis
		, n(0, 0, 1) // z-axis
	{}

	FFrame(const FVector3 &ss, const FVector3 &tt, const FNormal3 &nn)
		: s(ss.Normalize())
		, t(tt.Normalize())
		, n(nn.Normalize())
	{}

	FFrame(const FNormal3 &nn)
		: n(nn.Normalize())
	{
		SetFromZ();
	}

	// think if {s, t, n} is (1, 0, 0), (0, 1, 0), (0, 0, 1)
	FVector3 ToLocal(const FVector3 &wv) const
	{
		return FVector3(
			Dot(s, wv),
			Dot(t, wv),
			Dot(n, wv));
	}

	FVector3 ToWorld(const FVector3 lv) const
	{
		return
			s * lv.x +
			t * lv.y +
			n * lv.z;
	}

	const FVector3& Binormal() const { return s; }
	const FVector3& Tangent() const { return t; }
	const FVector3& Normal() const { return n; }

private:
	void SetFromZ()
	{
		FVector3 tmp_s = (std::abs(n.x) > 0.99f) ? FVector3(0, 1, 0) : FVector3(1, 0, 0);
		t = Normalize(Cross(n, tmp_s));
		s = Normalize(Cross(t, n));
	}
};

// Ray
// 
//  ----+-------------+--->
//      min_t      max_t
class FRay
{
public:
	FPoint3	origin;
	FVector3	dir; // unit vector
	mutable Float min_t;
	mutable	Float max_t;

	FRay() 
		: origin(0, 0, 0)
		, dir(1, 0, 0)
		, min_t(0.001f)
		, max_t(kInfinity)
	{}

	FRay(const FPoint3& org, const FVector3& indir, Float t0=0.001f, Float t1 = kInfinity)
		: origin(org)
		, dir(indir)
		, min_t(t0)
		, max_t(t1)
	{}

	const FPoint3& Origin() const { return origin; }
	const FVector3& Dir() const { return dir; }
	Float MinT() const { return min_t; }
	Float MaxT() const { return max_t; }

	void SetMinT(Float t) const { min_t = t; }
	void SetMaxT(Float t) const { max_t = t; }

	FPoint3 operator()(Float t) const
	{
		PBRT_DOCHECK(t >= 0);
		return origin + t * dir;
	}

};

} // namespace pbrt
