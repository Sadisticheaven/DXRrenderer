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



float get_pdf(float3 wi, float3 wo, float3 N) {
	//if (MaterialAttributes.type == MaterialType::Glass)
	return 1.0f;
	if (dot(wo, N) > 0.0)
		return 1.0f;
	else
		return 0.0f;
	/*
	switch (MaterialAttributes.type) {
	case MaterialType::Lambert:
	{
		if (dot(wo, N) > 0.0)
			return 1.0f;
		else
			return 0.0f;
	}
	case MaterialType::Mirror:
	{
		if (dot(wo, N) > 0.0)
			return  1.0f;
		else
			return 0.0f;
	}
	default: return 0;
	}*/
}

bool get_eval(float3 wi, float3 wo, float3 N, inout float3 eval) {
	/*
	float cosalpha = dot(N, wo);
	if (cosalpha > 0.0f) {
		float3 Kd = MaterialAttributes.Kd;
		float3 Ks = MaterialAttributes.Ks;
		float s = MaterialAttributes.smoothness;
		float alpha = pow(1000.0f,  s);
		float3 f_r = Kd + Ks * (alpha + 2) / (alpha + 1);
		eval = f_r;
		return true;
	}
	else {
		eval = float3(0.0, 0.0, 0.0);
		return false;
	}*/
	switch (MaterialAttributes.type) {
	case MaterialType::Lambert:
	{
		float cosalpha = dot(N, wo);
		if (cosalpha > 0.0f) {
			float3 diffuse = MaterialAttributes.Kd;
			eval = diffuse;
			return true;
		}
		else {
			eval = float3(0.0, 0.0, 0.0);
			return false;
		}
	}
	case MaterialType::Mirror:
	{
		float cosalpha = dot(N, wo);
		if (cosalpha > 0.0f) {
			float3 Kd = MaterialAttributes.Kd;
			float3 Ks = MaterialAttributes.Ks;
			float s = MaterialAttributes.smoothness;
			float alpha = pow(1000.0f, s);
			float3 f_r = Kd + Ks * (alpha + 2) / (alpha + 1);
			eval = f_r;
			return true;
		}
		else {
			eval = float3(0.0, 0.0, 0.0);
			return false;
		}
	}
	case MaterialType::Glass:
	{
		eval = MaterialAttributes.Kd;
		return  true;
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
	if (curRecursionDepth >= MAX_RAY_RECURSION_DEPTH || MaterialAttributes.type == MaterialType::Glass)
	{
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

	float3 eval;
	if (get_eval(worldRayDirection, direction, N, eval) == false) {
		return eval;
	}

	RayDesc rayDesc;
	rayDesc.Origin = hitWorldPosition;
	rayDesc.Direction = direction;
	rayDesc.TMin = 0.5;
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

	return rayPayload.radiance * eval * dot(direction, N) * dot(-direction, light_normal) / disPow2 * pdf;
}

bool canRefract(float3 v, float3 n, float ni_over_nt, inout float3 refracted) {
	float3 uv = normalize(v);
	float dt = dot(uv, n);
	float discriminant = 1.0 - ni_over_nt * (1 - dt * dt);
	if (discriminant > 0) {
		refracted = ni_over_nt * (uv - n * dt) - n * sqrt(discriminant);
		return true;
	}
	return false;
}

float m_schlick(float cosine, float ref_idx) {
	float r0 = (1 - ref_idx) / (1 + ref_idx);
	r0 *= r0;
	return r0 + (1 - r0) * pow(1 - cosine, 5);
}

float3 createSampleRay(float3 wi, float3 N, inout float4 seed) {

	seed = createRandomFloat4(seed);
	float4 random_float = seed;
	float s = MaterialAttributes.smoothness;
	float alpha = pow(1000.0f, s);
	float x_1 = pow(random_float.x, 1.0 / alpha), x_2 = random_float.z;
	float z = x_1;
	float r = sqrt(1.0f - z * z), phi = 2 * M_PI * x_2;
	float3 localRay = float3(r * cos(phi), r * sin(phi), z);

	switch (MaterialAttributes.type) {
	case MaterialType::Lambert:
	{
		return toWorld(localRay, N);
	}
	case MaterialType::Mirror: {
		float3 reflect_dir = reflect(wi, N);
		return toWorld(localRay, reflect_dir);
	}
	case MaterialType::Glass:
	{
		wi = normalize(wi);
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
		if (seed.w < reflect_prob)
			return toWorld(localRay, reflected);
		else
			return toWorld(localRay, refracted);
		//return toWorld(localRay, wi);
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
	rayDesc.TMin = 0.5;
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

float3 get_light_indir(float3 worldRayDirection, float3 normal, float3 hitWorldPosition, uint curRecursionDepth, inout float4 random_seed) {
	float3 L_intdir = float3(0.0, 0.0, 0.0);
	float3 sp_direction = createSampleRay(worldRayDirection, normal, random_seed);
	float pdf = get_pdf(worldRayDirection, sp_direction, normal);

	float3 eval;
	if (get_eval(worldRayDirection, sp_direction, normal, eval) == false) {
		return L_intdir;
	}

	if (random(float2(random_seed.x + random_seed.y, random_seed.z + random_seed.w)) <= PROBABILITY_RUSSIAN_ROULETTE) {
		Ray ray;
		ray.origin = hitWorldPosition;
		if (MaterialAttributes.type == MaterialType::Glass) {
			//if (curRecursionDepth >= MAX_RAY_RECURSION_DEPTH)
				//--curRecursionDepth;
		}
		ray.direction = sp_direction;
		L_intdir = CastRay(ray, curRecursionDepth, createRandomFloat4(random_seed)) * eval * pdf / PROBABILITY_RUSSIAN_ROULETTE;
	}
	/*
	switch (MaterialAttributes.type) {
	case MaterialType::Lambert:
	{
		float3 sp_direction = createSampleRay(worldRayDirection, normal, random_seed);
		float pdf = get_pdf(worldRayDirection, sp_direction, normal);

		float3 eval;
		if (get_eval(worldRayDirection, sp_direction, normal, eval) == false) {
			return L_intdir;
		}

		if (random(float2(random_seed.x + random_seed.y, random_seed.z + random_seed.w)) <= PROBABILITY_RUSSIAN_ROULETTE) {
			Ray ray;
			ray.origin = hitWorldPosition;
			ray.direction = sp_direction;
			L_intdir = CastRay(ray, curRecursionDepth, createRandomFloat4(random_seed)) * eval * pdf / PROBABILITY_RUSSIAN_ROULETTE;
		}
		break;
	}
	case MaterialType::Mirror:
	{
		float3 sp_direction = createSampleRay(worldRayDirection, normal, random_seed);
		float pdf = get_pdf(worldRayDirection, sp_direction, normal);

		float3 eval;
		if (get_eval(worldRayDirection, sp_direction, normal, eval) == false) {
			return L_intdir;
		}
		if (random(float2(random_seed.x + random_seed.y, random_seed.z + random_seed.w)) <= PROBABILITY_RUSSIAN_ROULETTE) {
			Ray ray;
			ray.origin = hitWorldPosition;
			ray.direction = sp_direction;
			L_intdir = CastRay(ray, curRecursionDepth, createRandomFloat4(random_seed)) * eval * pdf / PROBABILITY_RUSSIAN_ROULETTE;
		}
		break;
	}
	case MaterialType::Glass:
	{
		float3 sp_direction = createSampleRay(worldRayDirection, normal, random_seed);
		float pdf = get_pdf(worldRayDirection, sp_direction, normal);

		float3 eval;
		if (get_eval(worldRayDirection, sp_direction, normal, eval) == false) {
			return L_intdir;
		}
		if (random(float2(random_seed.x + random_seed.y, random_seed.z + random_seed.w)) <= PROBABILITY_RUSSIAN_ROULETTE) {
			Ray ray;
			ray.origin = hitWorldPosition;
			ray.direction = sp_direction;
			L_intdir = CastRay(ray, curRecursionDepth, createRandomFloat4(random_seed)) * eval * pdf / PROBABILITY_RUSSIAN_ROULETTE;
		}
		break;
	}
	default:
		break;
	}*/
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


	float3 L_indir = get_light_indir(worldRayDirection, normal, hitWorldPosition, payload.recursionDepth, random_seed);
	float3 L_dir = get_light_dir(worldRayDirection, hitWorldPosition, normal, random_seed, payload.recursionDepth);

	payload.radiance = MaterialAttributes.emit.xyz * emit_rate + L_indir + L_dir;
}
