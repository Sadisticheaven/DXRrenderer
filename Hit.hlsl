#define HLSL
#include "HLSLCompat.h"
#include "ShaderHelper.hlsli"

StructuredBuffer<Vertex> Vertex : register(t0);
StructuredBuffer<Index> Indices: register(t1);
RaytracingAccelerationStructure l_scene : register(t2);
ConstantBuffer<PrimitiveMaterialBuffer> MaterialAttributes : register(b0);

#define M_PI 3.14159265358979323846   // pi

float random(float2 p)
{
	float2 K1 = float2(23.14069263277926, 2.665144142690225);
	return abs(frac(cos(dot(p, K1)) * 12345.6789));
}

float get_pdf(float3 wi, float3 wo, float3 N, MaterialType::Type materialType) {
	switch (materialType) {
	case MaterialType::Matte:
	{
		if (dot(wo, N) > 0.0)
			return 0.5f / M_PI;
		else
			return 0.0f;
	}
	default: return 0;
	}
}

float3 get_eval(float3 wi, float3 wo, float3 N, float3 Kd, MaterialType::Type materialType) {
	switch (materialType) {
	case MaterialType::Matte:
	{
		// calculate the contribution of diffuse   model
		float cosalpha = dot(N, wo);
		if (cosalpha > 0.0f) {
			float3 diffuse = Kd / M_PI;
			return diffuse;
		}
		else
			return float3(0.0, 0.0, 0.0);
		break;
	}
	default: return float3(0.0, 0.0, 0.0);
	}
}

float3 toWorld(float3 a, float3 N) {
	float3 B, C;
	if (abs(N.x) > abs(N.y)) {
		float invLen = 1.0f / sqrt(N.x * N.x + N.z * N.z);
		C = float3(N.z * invLen, 0.0f, -N.x * invLen);
	}
	else {
		float invLen = 1.0f / sqrt(N.y * N.y + N.z * N.z);
		C = float3(0.0f, N.z * invLen, -N.y * invLen);
	}
	B = cross(C, N);
	return a.x * B + a.y * C + a.z * N;
}

float4 createRandomFloat4(float4 seed)
{
	return normalize(float4(random(seed.xy), random(seed.yz), random(seed.zw), random(seed.wx)));

}

float3 createSampleRay(float3 wi, float3 N, inout float4 seed, MaterialType::Type materialType) {
	switch (materialType) {
	case MaterialType::Matte:
	{
		seed = createRandomFloat4(seed);
		float4 random_float = seed;
		float x_1 = random_float.x, x_2 = random_float.z;
		float z = abs(1.0f - 2.0f * x_1);
		float r = sqrt(1.0f - z * z), phi = 2 * M_PI * x_2;
		float3 localRay = float3(r * cos(phi), r * sin(phi), z);
		return toWorld(localRay, N);
	}
	default: return float3(0.0, 0.0, 0.0);
	}
}


float3 CastRay(Ray ray, uint curRecursionDepth, float4 seed) {
	if (curRecursionDepth > MAX_RAY_RECURSION_DEPTH)
		return float3(0.0f, 0.0f, 0.0f);
	RayDesc rayDesc;
	rayDesc.Origin = ray.origin;
	rayDesc.Direction = ray.direction;
	rayDesc.TMin = 0;
	rayDesc.TMax = 100000;
	PayLoad rayPayload;
	rayPayload.irradiance = float3(0.0, 0.0, 0.0);
	rayPayload.recursionDepth = curRecursionDepth + 1;
	rayPayload.seed = seed;
	TraceRay(
		l_scene,
		RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
		0xFF,
		0,
		0,
		0,
		rayDesc,
		rayPayload
	);
	return rayPayload.irradiance;
}


[shader("closesthit")]
void ClosestHit(inout PayLoad payload, BuiltInTriangleIntersectionAttributes attrib)
{
	if (MaterialAttributes.emit.x >= 0.1) {
		payload.irradiance = MaterialAttributes.emit;
		return;
	}
	float3 barycentrics =
		float3(1.f - attrib.barycentrics.x - attrib.barycentrics.y, attrib.barycentrics.x, attrib.barycentrics.y);
	uint vertId = 3 * PrimitiveIndex();
	//以下方式为计算点法线并插值
	float3 normal = barycentrics.x * Vertex[Indices[vertId + 0]].normal +
		barycentrics.y * Vertex[Indices[vertId + 1]].normal +
		barycentrics.z * Vertex[Indices[vertId + 2]].normal;


	//以下方式为计算面法线
	//float3 normal = normalize(cross(Vertex[Indices[vertId + 0]].position - Vertex[Indices[vertId + 1]].position,
	//								Vertex[Indices[vertId + 0]].position - Vertex[Indices[vertId + 2]].position));
	normal = normalize(normal);
	float3 worldRayDirection = WorldRayDirection();
	float4 random_seed = payload.seed;

	float3 sp_direction = createSampleRay(worldRayDirection, normal, random_seed, MaterialType::Matte);
	float pdf = get_pdf(worldRayDirection, sp_direction, normal, MaterialType::Matte);
	float3 Kd = MaterialAttributes.Kd.xyz;
	float3 eval = get_eval(worldRayDirection, sp_direction, normal, Kd, MaterialType::Matte);
	float dot_value = dot(sp_direction, normal);
	//float4 randomFloat = createRandomFloat4(payload.seed);
	//float3 dir = normalize(randomFloat * 2.0 - 1.0 + normal);

	Ray ray;
	ray.origin = HitWorldPosition();
	ray.direction = sp_direction;
	float3 color = CastRay(ray, payload.recursionDepth, createRandomFloat4(random_seed)) * eval * dot_value / pdf;
	//Kd = Vertex[Indices[vertId]].normal;
	payload.irradiance = float4(color, 1.0) + MaterialAttributes.emit;
}
