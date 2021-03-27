#define HLSL
#include "HLSLCompat.h"
#include "ShaderHelper.hlsli"


StructuredBuffer<Vertex> Vertices: register(t0);
StructuredBuffer<Index> Indices: register(t1);
RaytracingAccelerationStructure SceneBVH : register(t2);
ConstantBuffer<PrimitiveMaterialBuffer> MaterialAttributes : register(b0);
StructuredBuffer<Vertex> light_vertices: register(t3);
StructuredBuffer<Index> light_indices: register(t4);


#define M_PI 3.14159265358979323846   // pi

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

bool get_eval(float3 wi, float3 wo, float3 N, float3 Kd, MaterialType::Type materialType, inout float3 eval) {
	switch (materialType) {
	case MaterialType::Matte:
	{
		// calculate the contribution of diffuse   model
		float cosalpha = dot(N, wo);
		if (cosalpha > 0.0f) {
			float3 diffuse = Kd / M_PI;
			eval = diffuse;
			return true;
		}
		else {
			eval = float3(0.0, 0.0, 0.0);
			return false;
		}
	}
	default:
		eval = float3(0.0, 0.0, 0.0);
		return false;
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


float3 get_light_dir(float3 worldRayDirection, float3 hitWorldPosition, float3 N, inout float4 seed, in UINT curRecursionDepth)
{
	if (curRecursionDepth >= MAX_RAY_RECURSION_DEPTH)
	{
		return float3(0.0, 0.0, 0.0);
	}

	seed = createRandomFloat4(seed);
	float2 ramdomBary = float2(random(seed.xy + seed.yz), random(seed.yz + seed.zw)) / 2;
	uint vertId = ramdomBary.x <= ramdomBary.y ? 0 : 3;

	float3 barycentrics = float3(1.f - ramdomBary.x - ramdomBary.y, ramdomBary.x, ramdomBary.y);

	float3 light_normal = barycentrics.x * light_vertices[light_indices[vertId + 0]].normal +
		barycentrics.y * light_vertices[light_indices[vertId + 1]].normal +
		barycentrics.z * light_vertices[light_indices[vertId + 2]].normal;
	light_normal = normalize(light_normal);

	float3 position = barycentrics.x * light_vertices[light_indices[vertId + 0]].position +
		barycentrics.y * light_vertices[light_indices[vertId + 1]].position +
		barycentrics.z * light_vertices[light_indices[vertId + 2]].position;
	float pdf = 130 * 105;

	// Set the ray's extents.
	float3 direction = position - hitWorldPosition;
	float disPow2 = dot(direction, direction);
	float dis = sqrt(disPow2);
	direction = normalize(direction);

	float3 eval;
	if (get_eval(worldRayDirection, direction, N, MaterialAttributes.Kd.xyz, MaterialType::Matte, eval) == false) {
		return eval;
	}

	RayDesc rayDesc;
	rayDesc.Origin = hitWorldPosition;
	rayDesc.Direction = direction;
	rayDesc.TMin = 0;
	rayDesc.TMax = dis + 1.0f;

	PayLoad rayPayload;
	rayPayload.radiance = float3(0.0f, 0.0f, 0.0f);
	rayPayload.recursionDepth = MAX_RAY_RECURSION_DEPTH;
	rayPayload.seed = seed;
	TraceRay(
		SceneBVH,
		RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
		0xFF,
		0,
		0,
		0,
		rayDesc,
		rayPayload);

	return rayPayload.radiance * eval * dot(direction, N) * dot(-direction, light_normal) / disPow2 * pdf;
}

float3 createSampleRay(float3 wi, float3 N, inout float4 seed, MaterialType::Type materialType) {
	switch (materialType) {
	case MaterialType::Matte:
	{
		seed = createRandomFloat4(seed);
		float4 random_float = seed;
		float x_1 = sqrt(random_float.x), x_2 = random_float.z;
		float z = x_1;
		float r = sqrt(1.0f - z * z), phi = 2 * M_PI * x_2;
		float3 localRay = float3(r * cos(phi), r * sin(phi), z);
		return toWorld(localRay, N);
	}
	default: return float3(0.0, 0.0, 0.0);
	}
}


float3 CastRay(Ray ray, uint curRecursionDepth, float4 seed) {

	if (curRecursionDepth >= MAX_RAY_RECURSION_DEPTH)
		return float3(0.0f, 0.0f, 0.0f);

	RayDesc rayDesc;
	rayDesc.Origin = ray.origin;
	rayDesc.Direction = ray.direction;
	rayDesc.TMin = 0;
	rayDesc.TMax = 100000;

	PayLoad rayPayload;
	rayPayload.radiance = float3(0.0, 0.0, 0.0);
	rayPayload.recursionDepth = curRecursionDepth + 1;
	rayPayload.seed = seed;
	TraceRay(
		SceneBVH,
		RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
		0xFF,
		0,
		0,
		0,
		rayDesc,
		rayPayload
	);

	return rayPayload.radiance;
}


[shader("closesthit")]
void ClosestHit(inout PayLoad payload, BuiltInTriangleIntersectionAttributes attrib)
{
	float3 barycentrics =
		float3(1.f - attrib.barycentrics.x - attrib.barycentrics.y, attrib.barycentrics.x, attrib.barycentrics.y);
	uint vertId = 3 * PrimitiveIndex();

	float3 normal = normalize(cross(Vertices[Indices[vertId + 0]].position - Vertices[Indices[vertId + 1]].position,
		Vertices[Indices[vertId + 0]].position - Vertices[Indices[vertId + 2]].position));
	normal = normalize(mul((float3x3)ObjectToWorld3x4(), normal));

	float3 worldRayDirection = WorldRayDirection();
	float3 hitWorldPosition = HitWorldPosition();
	float4 random_seed = payload.seed;

	if (MaterialAttributes.emit.x >= 0.1) {
		float rate = dot(normal, -normalize(worldRayDirection));
		payload.radiance = MaterialAttributes.emit.xyz * rate;
		return;
	}

	float3 sp_direction = createSampleRay(worldRayDirection, normal, random_seed, MaterialType::Matte);

	float pdf = get_pdf(worldRayDirection, sp_direction, normal, MaterialType::Matte);
	float3 Kd = MaterialAttributes.Kd.xyz;

	float3 eval;
	if (get_eval(worldRayDirection, sp_direction, normal, Kd, MaterialType::Matte, eval) == false) {
		payload.radiance = eval;
		return;
	}

	float cos_value = 0.5f; //dot(sp_direction, normal);
	//float cos_value = dot(sp_direction, normal);
	
	float3 L_intdir = float3(0.0, 0.0, 0.0);

	if (random(float2(random_seed.x + random_seed.y, random_seed.z + random_seed.w)) <= PROBABILITY_RUSSIAN_ROULETTE) {
		Ray ray;
		ray.origin = HitWorldPosition();
		ray.direction = sp_direction;
		L_intdir = CastRay(ray, payload.recursionDepth, createRandomFloat4(random_seed)) * eval * cos_value / pdf / PROBABILITY_RUSSIAN_ROULETTE;
	}

	float3 L_dir = get_light_dir(worldRayDirection, hitWorldPosition, normal, random_seed, payload.recursionDepth);

	payload.radiance = L_intdir + L_dir;
}
