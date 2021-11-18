#pragma once

#include "cuda_runtime.h"
#include "DataTypes.h"
#include <string>

// return ceiling of (a / b)
constexpr uint divideCeil(uint a, uint b)
{
    return (a + b - 1) / b;
}

// error check cuda
#define cudaCheckErrors(msg) \
    do { \
        cudaError_t __err = cudaGetLastError(); \
        if (__err != cudaSuccess) { \
            fprintf(stderr, "Fatal error: %s (%s at line %d)\n", \
                msg, cudaGetErrorString(__err), \
                __LINE__); \
            fprintf(stderr, "*** FAILED - ABORTING\n"); \
            exit(1); \
        } \
    } while (0)

// to add vectors 
inline ushort3 operator+(const ushort3& a, const ushort3& b)
{ 
    return { (ushort)(a.x + b.x), (ushort)(a.y + b.y), (ushort)(a.z + b.z) };
}

// to multiply vectors 
inline ushort3 operator*(const ushort3& a, const ushort3& b)
{
    return { (ushort)(a.x * b.x), (ushort)(a.y * b.y), (ushort)(a.z * b.z) };
}

// to multiply vectors
inline uint3 operator*(const uint3& a, const uint3& b)
{
    return { (uint)(a.x * b.x), (uint)(a.y * b.y), (uint)(a.z * b.z) };
}

// to divide vectors 
inline ushort3 operator/(const ushort3& a, const ushort3& b)
{
    return { (ushort)(a.x / b.x), (ushort)(a.y / b.y), (ushort)(a.z / b.z) };
}

template <class T>
unsigned long long getVolume(T vec)
{
    return vec.x * vec.y * vec.z;
}

template <class T>
std::string coordToString(T x, T y, T z)
{
    return std::to_string(x) + ',' + std::to_string(y) + ',' + std::to_string(z) + ',';
}

template <class T>
std::string coordToString(T vec)
{
    return std::to_string(vec.x) + ',' + std::to_string(vec.y) + ',' + std::to_string(vec.z) + ',';
}