#include <Windows.h>
#include <cstdint>
#include <vector>
#include <cmath>
#include <chrono>

using namespace std;

// Globale Variablen
static bool g_Running = true;
static BITMAPINFO g_BitmapInfo;
static vector<uint32_t> g_PixelBuffer;
static vector<float> g_ZBuffer;
static int g_Width = 800;
static int g_Height = 600;

const wchar_t* WINDOW_CLASS_NAME = L"Simple3DEngineClass";

// Mathe-Hilfsfunktionen
struct Vec3
{
	float x, y, z;
};

struct Vec2
{
	float x, y;
};

struct Mat4
{
	float m[16];
};

inline Mat4 Mat4Identity()
{
	Mat4 mat{};
	mat.m[0] = mat.m[5] = mat.m[10] = mat.m[15] = 1.0f;
	return mat;
}

inline Mat4 Mat4Multiply(const Mat4& a, const Mat4& b)
{
	Mat4 res{};

	for (int row = 0; row < 4; ++row)
	{
		for (int col = 0; col < 4; ++col)
		{
			res.m[row * 4 + col] =
				a.m[row * 4 + 0] * b.m[0 * 4 + col] +
				a.m[row * 4 + 1] * b.m[1 * 4 + col] +
				a.m[row * 4 + 2] * b.m[2 * 4 + col] +
				a.m[row * 4 + 3] * b.m[3 * 4 + col];
		}
	}

	return res;
}

inline Vec3 Mat4Apply(const Mat4& m, const Vec3& v)
{
	float x = v.x * m.m[0] + v.y * m.m[4] + v.z * m.m[8] + m.m[12];
	float y = v.x * m.m[1] + v.y * m.m[5] + v.z * m.m[9] + m.m[13];
	float z = v.x * m.m[2] + v.y * m.m[6] + v.z * m.m[10] + m.m[14];
	float w = v.x * m.m[3] + v.y * m.m[7] + v.z * m.m[11] + m.m[15];

	if (w != 0.0f)
	{
		w /= w; y /= w; z /= w;
	}

	return { x,y,z };
}

inline Mat4 Mat4Persperctive(float fov, float aspect, float znear, float zfar)
{
	Mat4 m{};
	float tanHalfFov = tanf(fov / 2.0f);
	m.m[0] = 1.0f / (aspect * tanHalfFov);
	m.m[5] = 1.0f / (tanHalfFov);
	m.m[10] = -(zfar + znear) / (zfar - znear);
	m.m[11] = -1.0f;
	m.m[14] = -(2.0f * zfar * znear) / (zfar - znear);

	return m;
}

inline Mat4 Mat4Translate(float x, float y, float z)
{
	Mat4 m = Mat4Identity();
	m.m[12] = x; m.m[13] = y; m.m[14] = z;
	return m;
}

inline Mat4 Mat4RotateY(float angle)
{
	Mat4 m = Mat4Identity();
	m.m[0] = cosf(angle);
	m.m[2] = sinf(angle);
	m.m[8] = -sinf(angle);
	m.m[10] = cosf(angle);


}

inline Mat4 Mat4LookAt(const Vec3& eye, const Vec3& target, const Vec3& up)
{
	Vec3 zaxis = { target.x - eye.x, target.y - eye.y, target.z - eye.z };
	float len = sqrtf(zaxis.x * zaxis.x + zaxis.y * zaxis.y + zaxis.z * zaxis.z);
	zaxis = { zaxis.x / len, zaxis.y / len, zaxis.z / len };

	Vec3 xaxis =
	{
		up.y * zaxis.z - up.z * zaxis.y,
		up.z * zaxis.x - up.x * zaxis.z,
		up.x * zaxis.y - up.y * zaxis.x
	};

	len = sqrtf(xaxis.x * xaxis.x + xaxis.y * xaxis.y + xaxis.z * xaxis.z);
	xaxis = { xaxis.x / len, xaxis.y / len, xaxis.z / len };

	Vec3 yaxis =
	{
		zaxis.y * xaxis.z - zaxis.z * xaxis.y,
		zaxis.z * xaxis.x - zaxis.x * xaxis.z,
		zaxis.x * xaxis.y - zaxis.y * xaxis.x
	};

	Mat4 view = Mat4Identity();

	view.m[0] = xaxis.x; view.m[1] = yaxis.x; view.m[2] = zaxis.x;
	view.m[4] = xaxis.y; view.m[5] = yaxis.y; view.m[6] = zaxis.y;
	view.m[8] = xaxis.z; view.m[9] = yaxis.z; view.m[10] = zaxis.z;

	view.m[12] = -(xaxis.x * eye.x + xaxis.y * eye.y + xaxis.z * eye.z);
	view.m[13] = -(yaxis.x * eye.x + yaxis.y * eye.y + yaxis.z * eye.z);
	view.m[14] = -(zaxis.x * eye.x + zaxis.y * eye.y + zaxis.z * eye.z);

	return view;
	
}

