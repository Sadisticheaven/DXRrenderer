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
typedef UINT Index;
#else

using namespace DirectX;
typedef UINT32 Index;

#endif //HLSL


struct PayLoad
{
	XMFLOAT4 colorAndDistance;
	UINT recursionDepth;
};

struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT4 normal;
};

struct SceneConstants
{
	XMMATRIX view;
	XMMATRIX projection;
	XMMATRIX viewI;
	XMMATRIX projectionI;
	UINT spp;
};

namespace MaterialType {
	enum Type {
		Default = 0,
		Matte,      // Lambertian scattering
		Mirror,     // Specular reflector that isn't modified by the Fresnel equations.
		AnalyticalCheckerboardTexture,
		Count
	};
}
struct PrimitiveMaterialBuffer
{
	XMFLOAT3 Kd;
	XMFLOAT3 Ks;
	XMFLOAT3 Kr;
	XMFLOAT3 Kt;
	XMFLOAT3 opacity;
	XMFLOAT3 emit;
	float roughness;
	//BOOL hasDiffuseTexture;
	//BOOL hasNormalTexture;
	//BOOL hasPerVertexTangents;
	MaterialType::Type type;
	float padding;
#ifndef HLSL
	PrimitiveMaterialBuffer() :Kd(0, 0, 0), Ks(0, 0, 0), Kr(0, 0, 0), Kt(0, 0, 0), opacity(0, 0, 0), emit(0, 0, 0), roughness(0), type(MaterialType::Matte), padding(0) {

	}
#endif
};
// Attributes output by the raytracing when hitting a surface,
// here the barycentric coordinates

#endif //RAYTRACINGHLSLCOMPAT_H