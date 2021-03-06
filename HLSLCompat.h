// Hit information, aka ray payload
// This sample only carries a shading color and hit distance.
// Note that the payload should be kept as small as possible,
// and that its size must be declared in the corresponding
// D3D12_RAYTRACING_SHADER_CONFIG pipeline subobjet.



#ifndef RAYTRACINGHLSLCOMPAT_H
#define RAYTRACINGHLSLCOMPAT_H

//**********************************************************************************************
//
// RaytracingHLSLCompat.h
//
// A header with shared definitions for C++ and HLSL source files. 
//
//**********************************************************************************************

#ifdef HLSL
typedef float2 XMFLOAT2;
typedef float3 XMFLOAT3;
typedef float4 XMFLOAT4;
typedef float4 XMVECTOR;
typedef float4x4 XMMATRIX;
typedef uint UINT;
#else

using namespace DirectX;
typedef UINT32 Index;

#endif //HLSL


struct HitInfo
{
	XMFLOAT4 colorAndDistance;
};

struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT4 color;
};

// Attributes output by the raytracing when hitting a surface,
// here the barycentric coordinates

#endif //RAYTRACINGHLSLCOMPAT_H