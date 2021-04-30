// Hit information, aka ray payload
// This sample only carries a shading color and hit distance.
// Note that the payload should be kept as small as possible,
// and that its size must be declared in the corresponding
// D3D12_RAYTRACING_SHADER_CONFIG pipeline subobjet.



#ifndef HLSLCOMPAT_H
#define HLSLCOMPAT_H

//**********************************************************************************************
//
// HLSLCompat.h
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
typedef int BOOL;
#else
using namespace DirectX;
typedef UINT32 Index;

#endif //HLSL

#define HitDistanceOnMiss 0
#define MAX_RAY_RECURSION_DEPTH 5
#define PROBABILITY_RUSSIAN_ROULETTE 0.8
#define FOREACH_MATERIAL(Type) \
        Type(Lambert)   \
        Type(Mirror)  \
        Type(Glass)   \
        Type(Plastic)  \
        Type(Disney_BRDF)  \
		Type(Count)  \

#define FOREACH_LIGHT(Type) \
        Type(Point)   \
        Type(Spot)  \
        Type(Distant)   \
		Type(Count)  \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

struct PayLoad
{
	XMFLOAT3 radiance;
	XMFLOAT4 seed;
	UINT recursionDepth;
};

struct Ray {
	XMFLOAT3 origin;
	XMFLOAT3 direction;
};

struct ShadowRayPayload {
	float tHit;         // Hit time <0,..> on Hit. -1 on miss.
};

namespace LightType {
	enum Type {
		FOREACH_LIGHT(GENERATE_ENUM)
	};
}

struct Light {
	XMFLOAT3 position;
	float emitIntensity;//constant need be aligned with 4,so XMFLOAT3 need after float
	LightType::Type type;
	XMFLOAT3 direction;
	float falloffStart;
	float totalWidth;
	float worldRadius;
};

typedef struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT2 TexCoords;
	//uv
	XMFLOAT3 Tangent;
	XMFLOAT3 Bitangent;
};

struct SceneConstants
{
	XMMATRIX view;
	XMMATRIX projection;
	XMMATRIX viewI;
	XMMATRIX projectionI;
	XMMATRIX invViewProj;
	XMFLOAT4 seed;
	float curSampleIdx;
	UINT point_light_nums;
};

namespace MaterialType {
	/*enum Type {
		Lambert,     
		Mirror,     
		Glass,
		Disney_brdf_Plastic,
		Count
	};*/
	enum Type {
		FOREACH_MATERIAL(GENERATE_ENUM)
	};
}



struct PrimitiveMaterialBuffer
{
	XMFLOAT4 Kd;
	//XMFLOAT4 Ks;
	//XMFLOAT4 emit;
	float index_of_refraction;
	float emitIntensity;
	float smoothness;
	float reflectivity;
	//disney_brdf
	float metallic;
	float subsurface;
	float specular;
	float roughness;
	float specularTint;
	float anisotropic;
	float sheen;
	float sheenTint;
	float clearcoat;
	float clearcoatGloss;
	//disney_brdf
	BOOL useDiffuseTexture;
	//BOOL hasDiffuseTexture;
	//BOOL hasNormalTexture;
	//BOOL hasPerVertexTangents;
	MaterialType::Type type;
	//LightType::Type light_type;
#ifndef HLSL
	//PrimitiveMaterialBuffer() :Kd(0, 0, 0), Ks(0, 0, 0), Kr(0, 0, 0), Kt(0, 0, 0), opacity(0, 0, 0), emit(0, 0, 0), roughness(0), type(MaterialType::Matte), padding(0) {

	//}
#endif
};




#endif //RAYTRACINGHLSLCOMPAT_H