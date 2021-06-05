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
#define MAX_RAY_RECURSION_DEPTH 12
#define PROBABILITY_RUSSIAN_ROULETTE 0.8
#define MINIMUMDISTANCE 0.000002f
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
        Type(Area)   \
		Type(Triangle)      \
		Type(Count)  \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

struct PayLoad{
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
	XMMATRIX transfer;
	XMFLOAT3 position;
	float emitIntensity;//constant need be aligned with 4,so XMFLOAT3 need after float
	LightType::Type type;
	//Spot
	XMFLOAT3 direction;
	float falloffStart;
	float totalWidth;
	//Distant
	float worldRadius;
	// Area
	BOOL useAreaLight;
	UINT objectIndex;
	UINT meshNum;
	float area;
	//Triangle
	XMFLOAT3 position0;
	XMFLOAT3 position1;
	XMFLOAT3 position2;

#ifdef HLSL
	// add some pad for alignment
	// float pad0;
	// float pad1;
	// float pad2;
	// float pad3;
#endif
};

typedef struct Vertex{
	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT2 TexCoords;
	//uv
	XMFLOAT3 Tangent;
	XMFLOAT3 Bitangent;
};
;

struct SceneConstants{
	XMMATRIX view;
	XMMATRIX projection;
	XMMATRIX viewI;
	XMMATRIX projectionI;
	XMMATRIX invViewProj;
	XMFLOAT4 seed;
	float curSampleIdx;
	UINT light_nums;
};

namespace MaterialType {
	enum Type {
		FOREACH_MATERIAL(GENERATE_ENUM)
	};
}



struct PrimitiveMaterialBuffer{
	XMFLOAT4 Kd;
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

	BOOL useDiffuseTexture;
	MaterialType::Type type;
#ifndef HLSL
	PrimitiveMaterialBuffer() :Kd(0.0, 0.0, 0.0, 0.0), index_of_refraction(0.0), emitIntensity(0.0),
		smoothness(0.0), reflectivity(0.0), metallic(0.0), subsurface(0.0), specular(0.0),
		roughness(0.0), specularTint(0.0), anisotropic(0.0), sheen(0.0), sheenTint(0.0),
		clearcoat(0.0), clearcoatGloss(0.0), useDiffuseTexture(0.0), type(MaterialType::Lambert) {}
	bool operator==(const PrimitiveMaterialBuffer& p)const {
		return (p.Kd.x == Kd.x && p.Kd.y == Kd.y && p.Kd.z == Kd.z && p.Kd.w == Kd.w && p.index_of_refraction == index_of_refraction &&
			p.emitIntensity == emitIntensity && p.smoothness == smoothness && p.reflectivity == reflectivity && p.metallic == metallic &&
			p.subsurface == subsurface && p.subsurface == subsurface && p.specular == specular && p.roughness == roughness &&
			p.specularTint == specularTint && p.anisotropic == anisotropic && p.sheen == sheen && p.sheenTint == sheenTint &&
			p.clearcoat == clearcoat && p.clearcoatGloss == clearcoatGloss && p.useDiffuseTexture == useDiffuseTexture && p.type == type);
	}
#endif
};

#endif //RAYTRACINGHLSLCOMPAT_H