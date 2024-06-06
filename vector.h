#pragma once

#include <math.h>

#define M_PI 3.14159265358979323846f
#define RADPI 57.295779513082f
#define DEG2RAD(x)((float)(x) * (float)((float)(PI) / 180.0f))
#define RAD2DEG(x)((float)(x) * (float)(180.0f / (float)(PI)))

class Vector2
{
public:
	Vector2(void)
	{
		x = y = 0.0f;
	}

	Vector2(float X, float Y)
	{
		x = X; y = Y;
	}

	Vector2(float* v)
	{
		x = v[0]; y = v[1];
	}

	Vector2(const float* v)
	{
		x = v[0]; y = v[1];
	}

	Vector2(const Vector2& v)
	{
		x = v.x; y = v.y;
	}

	Vector2& operator=(const Vector2& v)
	{
		x = v.x; y = v.y; return *this;
	}

	float& operator[](int i)
	{
		return ((float*)this)[i];
	}

	float operator[](int i) const
	{
		return ((float*)this)[i];
	}

	Vector2& operator+=(const Vector2& v)
	{
		x += v.x; y += v.y; return *this;
	}

	Vector2& operator-=(const Vector2& v)
	{
		x -= v.x; y -= v.y; return *this;
	}

	Vector2& operator*=(const Vector2& v)
	{
		x *= v.x; y *= v.y; return *this;
	}

	Vector2& operator/=(const Vector2& v)
	{
		x /= v.x; y /= v.y; return *this;
	}

	Vector2& operator+=(float v)
	{
		x += v; y += v; return *this;
	}

	Vector2& operator-=(float v)
	{
		x -= v; y -= v; return *this;
	}

	Vector2& operator*=(float v)
	{
		x *= v; y *= v; return *this;
	}

	Vector2& operator/=(float v)
	{
		x /= v; y /= v; return *this;
	}

	Vector2 operator+(const Vector2& v) const
	{
		return Vector2(x + v.x, y + v.y);
	}

	Vector2 operator-(const Vector2& v) const
	{
		return Vector2(x - v.x, y - v.y);
	}

	Vector2 operator*(const Vector2& v) const
	{
		return Vector2(x * v.x, y * v.y);
	}

	Vector2 operator/(const Vector2& v) const
	{
		return Vector2(x / v.x, y / v.y);
	}

	Vector2 operator+(float v) const
	{
		return Vector2(x + v, y + v);
	}

	Vector2 operator-(float v) const
	{
		return Vector2(x - v, y - v);
	}

	Vector2 operator*(float v) const
	{
		return Vector2(x * v, y * v);
	}

	Vector2 operator/(float v) const
	{
		return Vector2(x / v, y / v);
	}

	void Set(float X = 0.0f, float Y = 0.0f)
	{
		x = X; y = Y;
	}

	float Length(void) const
	{
		return sqrtf(x * x + y * y);
	}

	float LengthSqr(void) const
	{
		return (x * x + y * y);
	}


	float DistTo(const Vector2& v) const
	{
		return (*this - v).Length();
	}

	float DistToSqr(const Vector2& v) const
	{
		return (*this - v).LengthSqr();
	}

	float Dot(const Vector2& v) const
	{
		return (x * v.x + y * v.y);
	}

	bool IsZero(void) const
	{
		return (x > -0.01f && x < 0.01f &&
			y > -0.01f && y < 0.01f);
	}

public:
	float x, y;
};

class Vector3
{
public:
	Vector3() : x(0.f), y(0.f), z(0.f)
	{

	}

	Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z)
	{

	}
	~Vector3()
	{

	}

	float x;
	float y;
	float z;

	inline float Dot(Vector3 v)
	{
		return x * v.x + y * v.y + z * v.z;
	}

	inline float Distance(Vector3 v)
	{
		float x = this->x - v.x;
		float y = this->y - v.y;
		float z = this->z - v.z;

		return sqrtf((x * x) + (y * y) + (z * z)) * 0.03048f;
	}

	Vector3 operator+(Vector3 v)
	{
		return Vector3(x + v.x, y + v.y, z + v.z);
	}

	Vector3 operator-(Vector3 v)
	{
		return Vector3(x - v.x, y - v.y, z - v.z);
	}

	Vector3 operator*(float number) const {
		return Vector3(x * number, y * number, z * number);
	}
};

struct FQuat
{
	float x;
	float y;
	float z;
	float w;
};

struct FTransform
{
	FQuat rot;
	Vector3 translation;
	char pad[4];
	Vector3 scale;
	char pad1[4];

	D3DMATRIX ToMatrixWithScale()
	{
		D3DMATRIX m;
		m._41 = translation.x;
		m._42 = translation.y;
		m._43 = translation.z;

		float x2 = rot.x + rot.x;
		float y2 = rot.y + rot.y;
		float z2 = rot.z + rot.z;

		float xx2 = rot.x * x2;
		float yy2 = rot.y * y2;
		float zz2 = rot.z * z2;
		m._11 = (1.0f - (yy2 + zz2)) * scale.x;
		m._22 = (1.0f - (xx2 + zz2)) * scale.y;
		m._33 = (1.0f - (xx2 + yy2)) * scale.z;

		float yz2 = rot.y * z2;
		float wx2 = rot.w * x2;
		m._32 = (yz2 - wx2) * scale.z;
		m._23 = (yz2 + wx2) * scale.y;

		float xy2 = rot.x * y2;
		float wz2 = rot.w * z2;
		m._21 = (xy2 - wz2) * scale.y;
		m._12 = (xy2 + wz2) * scale.x;

		float xz2 = rot.x * z2;
		float wy2 = rot.w * y2;
		m._31 = (xz2 + wy2) * scale.z;
		m._13 = (xz2 - wy2) * scale.x;

		m._14 = 0.0f;
		m._24 = 0.0f;
		m._34 = 0.0f;
		m._44 = 1.0f;

		return m;
	}
};