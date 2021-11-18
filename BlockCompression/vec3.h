#ifndef VEC3
#define VEC3

#include <string>

#define ushort unsigned short

// struct to hold per axis data
template<typename T>
struct vec3
{
	T x;
	T y;
	T z;

	unsigned long long volume()
	{
		return (unsigned long long)x * (unsigned long long)y * (unsigned long long)z;
	}

	T operator[] (int index)
	{
		switch (index) {
		case 0:
			return x;
		case 1:
			return y;
		case 2:
			return z;
		}
	}

	vec3<T> operator* (vec3<T> b)
	{
		return { (T)(x * b.x), (T)(y * b.y), (T)(z * b.z) };
	}

	vec3<T> operator/ (vec3<T> b)
	{
		return { (T)(x / b.x), (T)(y / b.y), (T)(z / b.z) };
	}

	vec3<T> operator+ (vec3<T> b)
	{
		return { (T)(x + b.x), (T)(y + b.y), (T)(z + b.z) };
	}

	vec3<T> operator- (vec3<T> b)
	{
		return { (T)(x - b.x), (T)(y - b.y), (T)(z - b.z) };
	}

	bool operator== (vec3<T> b)
	{
		return x == b.x && y == b.y && z == b.z;
	}

	std::string to_string()
	{
		return std::to_string(x) + "," + std::to_string(y) + "," + std::to_string(z);
	}
};

#endif