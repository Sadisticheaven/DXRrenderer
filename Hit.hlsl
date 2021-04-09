#define HLSL
#include "HLSLCompat.h"
#include "ShaderHelper.hlsli"


StructuredBuffer<Vertex> Vertices: register(t0);
StructuredBuffer<Index> Indices: register(t1);
RaytracingAccelerationStructure SceneBVH : register(t2);
ConstantBuffer<PrimitiveMaterialBuffer> MaterialAttributes : register(b0);
StructuredBuffer<Vertex> light_vertices: register(t3);
StructuredBuffer<Index> light_indices: register(t4);
Texture2D bricksTex : register(t5);
//6个不同类型的采样器
SamplerState gSamPointWrap : register(s0);
SamplerState gSamPointClamp : register(s1);
SamplerState gSamLinearWarp : register(s2);
SamplerState gSamLinearClamp : register(s3);
SamplerState gSamAnisotropicWarp : register(s4);
SamplerState gSamAnisotropicClamp : register(s5);


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


float3 get_light_dir(float3 worldRayDirection, float3 hitWorldPosition, float3 N, float2 uv , inout float4 seed, in UINT curRecursionDepth)
{
	if (curRecursionDepth >= MAX_RAY_RECURSION_DEPTH)
	{
		return float3(0.0, 0.0, 0.0);
	}

	if (MaterialAttributes.type != MaterialType::Lambert && MaterialAttributes.type != MaterialType::Plastic) {
		return float3(0.0, 0.0, 0.0);
	}

	seed = createRandomFloat4(seed);
	float2 ramdomBary = float2(random(seed.xy + seed.yz), random(seed.yz + seed.zw)) / 2;
	uint vertId = ramdomBary.x <= ramdomBary.y ? 0 : 3;

	float3 barycentrics = float3(1.f - ramdomBary.x - ramdomBary.y, ramdomBary.x, ramdomBary.y);

	float3 light_normal = normalize(barycentrics.x * light_vertices[light_indices[vertId + 0]].normal +
		barycentrics.y * light_vertices[light_indices[vertId + 1]].normal +
		barycentrics.z * light_vertices[light_indices[vertId + 2]].normal);

	float3 position = barycentrics.x * light_vertices[light_indices[vertId + 0]].position +
		barycentrics.y * light_vertices[light_indices[vertId + 1]].position +
		barycentrics.z * light_vertices[light_indices[vertId + 2]].position;
	float pdf = 130 * 105 / M_PI;

	// Set the ray's extents.
	float3 direction = position - hitWorldPosition;
	float disPow2 = dot(direction, direction);
	float dis = sqrt(disPow2);
	direction = normalize(direction);

	if (dot(N, direction) <= 0.0f) {
		return float3(0.0, 0.0, 0.0);
	}

	RayDesc rayDesc;
	rayDesc.Origin = hitWorldPosition;
	rayDesc.Direction = direction;
	rayDesc.TMin = 0.01;
	rayDesc.TMax = dis + 1.0f;

	PayLoad rayPayload;
	rayPayload.radiance = float3(0.0f, 0.0f, 0.0f);
	rayPayload.recursionDepth = MAX_RAY_RECURSION_DEPTH;
	rayPayload.seed = seed;
	TraceRay(
		SceneBVH,
		RAY_FLAG_NONE,
		0xFF,
		0,
		0,
		0,
		rayDesc,
		rayPayload);

	return rayPayload.radiance * bricksTex.SampleLevel(gSamAnisotropicWarp, uv, 0).xyz * dot(direction, N) * dot(-direction, light_normal) / disPow2 * pdf;
}

float3 createSampleRay(float3 wi, float3 N, inout float3 eval, float2 uv, inout float4 seed) {

	wi = normalize(wi);

	switch (MaterialAttributes.type) {
	case MaterialType::Lambert:
	{
		//common
		seed = createRandomFloat4(seed);
		float4 random_float = seed;
		float x_1 = pow(random_float.x, 1.0 / 2), x_2 = random_float.z;
		float z = x_1;
		float r = sqrt(1.0f - z * z), phi = 2 * M_PI * x_2;
		float3 localRay = float3(r * cos(phi), r * sin(phi), z);
		//end_common

		float3 wo = toWorld(localRay, N);
		float cosalpha = dot(N, wo);
		if (cosalpha > 0.0f) {
			float3 diffuse = MaterialAttributes.Kd;
			eval = diffuse;
			eval = bricksTex.SampleLevel(gSamAnisotropicWarp, uv, 0).xyz;
		}
		else {
			eval = float3(0.0, 0.0, 0.0);
		}
		return wo;
	}
	case MaterialType::Mirror: {
		//common
		seed = createRandomFloat4(seed);
		float4 random_float = seed;
		float s = MaterialAttributes.smoothness;
		float alpha = pow(1000.0f, s);
		float x_1 = pow(random_float.x, 1.0 / alpha), x_2 = random_float.z;
		float z = x_1;
		float r = sqrt(1.0f - z * z), phi = 2 * M_PI * x_2;
		float3 localRay = float3(r * cos(phi), r * sin(phi), z);
		//end_common

		float3 reflect_dir = reflect(wi, N);
		float3 wo = toWorld(localRay, reflect_dir);
		float cosalpha = dot(N, wo);
		if (cosalpha > 0.0f) {
			float3 Kd = MaterialAttributes.Kd;

			float s = MaterialAttributes.smoothness;
			float alpha = pow(1000.0f, s);
			float3 f_r = Kd;
			eval = f_r;
		}
		else {
			eval = float3(0.0, 0.0, 0.0);
		}
		return wo;
	}
	case MaterialType::Glass:
	{
		//common
		seed = createRandomFloat4(seed);
		float4 random_float = seed;
		float s = MaterialAttributes.smoothness;
		float alpha = pow(1000.0f, s);
		float x_1 = pow(random_float.x, 1.0 / alpha), x_2 = random_float.z;
		float z = x_1;
		float r = sqrt(1.0f - z * z), phi = 2 * M_PI * x_2;
		float3 localRay = float3(r * cos(phi), r * sin(phi), z);
		//end_common

		float3 outward_normal;
		float3 reflected = reflect(wi, N);
		float ref_idx = MaterialAttributes.index_of_refraction;
		float ni_over_nt;
		float3 refracted;
		float reflect_prob;
		float cosine;
		if (dot(wi, N) > 0) {
			outward_normal = -N;
			ni_over_nt = ref_idx;
			cosine = ref_idx * dot(wi, N);
		}
		else {
			outward_normal = N;
			ni_over_nt = 1.0 / ref_idx;
			cosine = -dot(wi, N);
		}
		if (canRefract(wi, outward_normal, ni_over_nt, refracted)) {
			reflect_prob = m_schlick(cosine, ref_idx);
		}
		else {
			reflect_prob = 1.0;
		}
		float3 wo;
		if (seed.w < reflect_prob)
			wo = toWorld(localRay, reflected);
		else
			wo = toWorld(localRay, refracted);
		eval = MaterialAttributes.Kd;
		return wo;
	}
	case MaterialType::Plastic:
	{
		//common
		seed = createRandomFloat4(seed);
		float4 random_float = seed;
		float s = MaterialAttributes.smoothness;
		float alpha = pow(1000.0f, s);
		float x_1, x_2, r, z, phi;
		float3 localRay, wo = float3(0.0, 0.0, 0.0), H;
		//end_common

		float reflectivity = MaterialAttributes.smoothness;
		if (seed.w < reflectivity) {
			x_1 = pow(random_float.x, 1.0 / alpha), x_2 = random_float.z;
			z = x_1;
			r = sqrt(1.0f - z * z), phi = 2 * M_PI * x_2;
			localRay = float3(r * cos(phi), r * sin(phi), z);
			float3 reflect_dir = reflect(wi, N);
			wo = toWorld(localRay, reflect_dir);
		}
		else {
			x_1 = pow(random_float.x, 1.0 / 2.0), x_2 = random_float.z;
			z = x_1;
			r = sqrt(1.0f - z * z), phi = 2 * M_PI * x_2;
			localRay = float3(r * cos(phi), r * sin(phi), z);
			wo = toWorld(localRay, N);
		}

		if (dot(wo, N) < 0) {
			eval = float3(0.0, 0.0, 0.0);
		}
		else {
			eval = MaterialAttributes.Kd;
		}
		return wo;
	}
	default: return float3(0.0, 0.0, 0.0);
	}
	return float3(0.0, 0.0, 0.0);
}


float3 CastRay(Ray ray, uint curRecursionDepth, float4 seed) {

	if (curRecursionDepth >= MAX_RAY_RECURSION_DEPTH)
		return float3(0.0f, 0.0f, 0.0f);

	RayDesc rayDesc;
	rayDesc.Origin = ray.origin;
	rayDesc.Direction = ray.direction;
	rayDesc.TMin = 0.01;
	rayDesc.TMax = 100000;

	PayLoad rayPayload;
	rayPayload.radiance = float3(0.0, 0.0, 0.0);
	rayPayload.recursionDepth = curRecursionDepth + 1;
	rayPayload.seed = seed;
	TraceRay(
		SceneBVH,
		RAY_FLAG_NONE,
		0xFF,
		0,
		0,
		0,
		rayDesc,
		rayPayload
	);

	return rayPayload.radiance;
}

float3 get_light_indir(float3 worldRayDirection, float3 normal, float3 hitWorldPosition, float2 uv, uint curRecursionDepth, inout float4 random_seed) {
	float3 L_intdir = float3(0.0, 0.0, 0.0);
	float3 eval;
	float3 sp_direction = createSampleRay(worldRayDirection, normal, eval, uv, random_seed);

	if (random(float2(random_seed.x + random_seed.y, random_seed.z + random_seed.w)) <= PROBABILITY_RUSSIAN_ROULETTE) {
		Ray ray;
		ray.origin = hitWorldPosition;
		ray.direction = sp_direction;
		L_intdir = CastRay(ray, curRecursionDepth, createRandomFloat4(random_seed)) * eval / PROBABILITY_RUSSIAN_ROULETTE;
	}
	return L_intdir;
}


[shader("closesthit")]
void ClosestHit(inout PayLoad payload, BuiltInTriangleIntersectionAttributes attrib)
{
	float3 barycentrics =
		float3(1.f - attrib.barycentrics.x - attrib.barycentrics.y, attrib.barycentrics.x, attrib.barycentrics.y);
	uint vertId = 3 * PrimitiveIndex();

	float3 normal = barycentrics.x * Vertices[Indices[vertId + 0]].normal +
		barycentrics.y * Vertices[Indices[vertId + 1]].normal +
		barycentrics.z * Vertices[Indices[vertId + 2]].normal;

	//float3 normal = normalize(cross(Vertices[Indices[vertId + 0]].position - Vertices[Indices[vertId + 1]].position,
	//	Vertices[Indices[vertId + 0]].position - Vertices[Indices[vertId + 2]].position));
	normal = normalize(mul((float3x3)ObjectToWorld3x4(), normal));

	float3 worldRayDirection = WorldRayDirection();
	float3 hitWorldPosition = HitWorldPosition();
	float4 random_seed = payload.seed;


	float emit_rate = dot(normal, -normalize(worldRayDirection));


	float3 L_indir = get_light_indir(worldRayDirection, normal, hitWorldPosition, barycentrics.yz, payload.recursionDepth, random_seed);
	float3 L_dir = get_light_dir(worldRayDirection, hitWorldPosition, normal, barycentrics.yz,random_seed, payload.recursionDepth);

	payload.radiance = MaterialAttributes.Kd.xyz * MaterialAttributes.emitIntensity * emit_rate + L_indir + L_dir;
}
