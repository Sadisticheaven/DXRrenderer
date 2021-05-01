#define HLSL
#include "HLSLCompat.h"
#include "ShaderHelper.hlsli"


StructuredBuffer<Vertex> Vertices: register(t0);
StructuredBuffer<Index> Indices: register(t1);
RaytracingAccelerationStructure SceneBVH : register(t2);
ConstantBuffer<PrimitiveMaterialBuffer> MaterialAttributes : register(b0);
StructuredBuffer<Light> light_vertices: register(t3);
//StructuredBuffer<Index> light_indices: register(t4);
Texture2D bricksTex : register(t4);
ConstantBuffer<SceneConstants> sceneParameter : register(b1);
//ConstantBuffer<Light> global_light : register(b1);
//6个不同类型的采样器
SamplerState gSamPointWrap : register(s0);
SamplerState gSamPointClamp : register(s1);
SamplerState gSamLinearWarp : register(s2);
SamplerState gSamLinearClamp : register(s3);
SamplerState gSamAnisotropicWarp : register(s4);
SamplerState gSamAnisotropicClamp : register(s5);



float3 get_eval_for_light_dir(float3 wi, float3 wo, float3 N, float2 uv) {
	wi = normalize(wi);
	wo = normalize(wo);
	N = normalize(N);
	float3 Kd = MaterialAttributes.Kd;
	if (MaterialAttributes.useDiffuseTexture) {
		Kd = bricksTex.SampleLevel(gSamAnisotropicWarp, uv, 0).xyz;
	}
	switch (MaterialAttributes.type) {
	case MaterialType::Lambert:
	{
		float cosalpha = dot(N, wo);
		if (cosalpha > 0.0f) {
			return Kd * cosalpha;
		}
		return float3(0.0, 0.0, 0.0);
	}
	case MaterialType::Mirror: {

		float3 reflect_dir = reflect(wi, N);
		float cosalpha = dot(reflect_dir, wo);
		if (cosalpha > 0.0f) {
			float s = MaterialAttributes.smoothness;
			float alpha = pow(1000.0f, s);
			return Kd * (alpha + 2) * pow(cosalpha, alpha);
		}
		return float3(0.0, 0.0, 0.0);
	}
	case MaterialType::Glass:
	{
		float3 outward_normal;
		float3 reflected = reflect(wi, N);
		float ref_idx = MaterialAttributes.index_of_refraction;
		float s = MaterialAttributes.smoothness;
		float alpha = pow(1000.0f, s);
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
		if (dot(wo, outward_normal) > 0.0f) {
			if (dot(wo, refracted) > 0.0f) {
				return Kd * (alpha + 2) * pow(dot(wo, refracted), alpha) * (1 - reflect_prob);
			}
			else if (dot(wo, reflected) > 0.0f) {
				return float3(0.0, 0.0, 0.0);
				return Kd * (alpha + 2) * pow(dot(wo, reflected), alpha) * reflect_prob;
			}
		}
		return float3(0.0, 0.0, 0.0);
	}
	case MaterialType::Plastic:
	{
		float s = MaterialAttributes.smoothness;
		float alpha = pow(1000.0f, s);

		float reflectivity = MaterialAttributes.reflectivity;
		float cosalpha = dot(wo, N);
		float3 reflect_dir = reflect(wi, N);
		if (dot(wo, N) > 0.0f) {
			return Kd * (reflectivity * pow(dot(wo, reflect_dir), alpha) + (1.0 - reflectivity) * dot(wo, N));
		}
		return float3(0.0, 0.0, 0.0);
	}
	case MaterialType::Disney_BRDF:
	{
		return  Disney_BRDF(wo, -wi, N, Kd, MaterialAttributes);
	}
	default: return float3(0.0, 0.0, 0.0);
	}
	return float3(0.0, 0.0, 0.0);
}

float3 get_light_dir(float3 worldRayDirection, float3 hitWorldPosition, float3 N, float2 uv, inout float4 seed, in UINT curRecursionDepth)
{
	float3 radiance = float3(0.0, 0.0, 0.0);
	for (uint i = 0; i < sceneParameter.light_nums; ++i) {
		Light global_light = light_vertices[i];
		float3 position = global_light.position;

		float3 direction = position - hitWorldPosition;
		float disPow2 = dot(direction, direction);
		float dis = sqrt(disPow2);
		float3 normal_dire = normalize(direction);


		if (dot(N, normal_dire) <= 0.0f) {
			continue;
		}

		float3 eval = get_eval_for_light_dir(worldRayDirection, N, normal_dire, uv);
		float3 Kd = MaterialAttributes.Kd;
		float emitIntensity = global_light.emitIntensity;


		RayDesc rayDesc;
		rayDesc.Origin = hitWorldPosition;
		rayDesc.Direction = normal_dire;
		rayDesc.TMin = 0.01;
		rayDesc.TMax = dis + 1.f;

		if (global_light.type == LightType::Distant) {
			float3 distantDirection = normalize(global_light.direction);
			rayDesc.Direction = -distantDirection;
			rayDesc.TMax = 2 * global_light.worldRadius;
		}

		ShadowRayPayload rayPayload;
		rayPayload.tHit = HitDistanceOnMiss;
		TraceRay(
			SceneBVH,
			RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,
			0xFF,
			1,//shadowhit
			0,
			1,//shadowmiss
			rayDesc,
			rayPayload);


		if (rayPayload.tHit != HitDistanceOnMiss) {
			continue;
		}
		if (global_light.type == LightType::Distant) {
			float3 distantDirection = normalize(global_light.direction);
			float cosx = dot(N, -distantDirection);
			if (cosx < 0.f) {
				continue;
			}
			radiance += emitIntensity * emitIntensity * eval * cosx;
			continue;
		}
		if (global_light.type == LightType::Point) {
			radiance += emitIntensity * emitIntensity * eval / disPow2;
			continue;
		}
		if (global_light.type == LightType::Spot) {
			float cosx = dot(-normal_dire, normalize(global_light.direction));
			float cosFalloffStart = cos(global_light.falloffStart);
			float cosTotalWidth = cos(global_light.totalWidth);
			float delta;
			if (cosx > cosFalloffStart) {
				delta = 1.f;
			}
			else if (cosx < cosTotalWidth) {
				delta = 0.f;
			}
			else {
				delta = (cosx - cosTotalWidth) / (cosFalloffStart - cosTotalWidth);
			}
			radiance += emitIntensity * emitIntensity * eval * delta * delta * delta * delta / disPow2;
			continue;
		}
	}
	return radiance;
}


float3 createSampleRay(float3 wi, float3 N, inout float3 eval, float2 uv, inout float4 seed) {

	wi = normalize(wi);
	float3 Kd = MaterialAttributes.Kd;
	if (MaterialAttributes.useDiffuseTexture) {
		Kd = bricksTex.SampleLevel(gSamAnisotropicWarp, uv, 0).xyz;
	}
	switch (MaterialAttributes.type) {
	case MaterialType::Lambert:
	{
		//common
		seed = createRandomFloat4(seed);
		float4 random_float = seed;
		//float x_1 = pow(random_float.x, 1.0 / 2), x_2 = random_float.z;
		//float z = x_1;
		float x_1 = random_float.x, x_2 = random_float.z;
		float z = pow(random_float.x, 1.0 / 2);
		float r = sqrt(1.0f - x_1), phi = 2 * M_PI * x_2;
		float3 localRay = float3(r * cos(phi), r * sin(phi), z);
		//end_common

		float3 wo = toWorld(localRay, N);
		float cosalpha = dot(N, wo);
		if (cosalpha > 0.0f) {
			eval = Kd;
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
			float s = MaterialAttributes.smoothness;
			float alpha = pow(1000.0f, s);
			eval = Kd;
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
		eval = Kd;
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

		float reflectivity = MaterialAttributes.reflectivity;
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
			eval = Kd;
		}
		return wo;
	}
	case MaterialType::Disney_BRDF:
	{
		//common
		seed = createRandomFloat4(seed);
		float4 random_float = seed;
		float x_1 = random_float.x, x_2 = random_float.y;
		float z = x_1;
		float r = sqrt(1.0f - z * z), phi = 2 * M_PI * x_2;
		float3 localRay = float3(r * cos(phi), r * sin(phi), z);
		//end_common

		float metallic = MaterialAttributes.metallic;
		float clearcoat = MaterialAttributes.clearcoat;
		float roughness = MaterialAttributes.roughness;

		float3 wo = toWorld(localRay, N);
		if (seed.z < min(0.8, 1 - metallic)) {
			eval = Disney_BRDF_diffuse(wo, -wi, N, Kd, MaterialAttributes);
		}
		else if (seed.w < 1 / (1 + clearcoat / 2)) {
			float u = random_float.x;
			float v = random_float.y;
			float tan2theta = roughness * roughness * (u / (1 - u));
			float cos2theta = 1 / (1 + tan2theta);
			float sinTheta = sqrt(1 - cos2theta);
			float phi = 2 * M_PI * v;
			localRay = float3(sinTheta * cos(phi), sinTheta * sin(phi), sqrt(cos2theta));
			wo = toWorld(localRay, N);
			wo = 2 * dot(-wi, wo) * wo + wi;
			eval = Disney_BRDF_specular(wo, -wi, N, Kd, MaterialAttributes);// *max(1e-4, (sin((roughness* roughness)* M_PI / 2)));
		}
		else {
			float u = random_float.x;
			float v = random_float.y;
			float tan2theta = 0.25 * 0.25 * u / (1 - u);
			float cos2theta = 1 / (1 + tan2theta);
			float sinTheta = sqrt(1 - cos2theta);
			float phi = 2 * M_PI * v;
			localRay = float3(sinTheta * cos(phi), sinTheta * sin(phi), sqrt(cos2theta));
			wo = toWorld(localRay, N);
			wo = 2 * dot(-wi, wo) * wo + wi;
			eval = Disney_BRDF_clearcoat(wo, -wi, N, Kd, MaterialAttributes);
		}
		//eval = Disney_BRDF(wo, -wi, N, Kd, MaterialAttributes);
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

	/*float3 normal = barycentrics.x * Vertices[Indices[vertId + 0]].normal +
		barycentrics.y * Vertices[Indices[vertId + 1]].normal +
		barycentrics.z * Vertices[Indices[vertId + 2]].normal;*/

	float3 vertexNormal[3] = {
		Vertices[Indices[vertId + 0]].normal,
		Vertices[Indices[vertId + 1]].normal,
		Vertices[Indices[vertId + 2]].normal };
	float3 normal = HitAttribute(vertexNormal, attrib);

	//float3 normal = normalize(cross(Vertices[Indices[vertId + 0]].position - Vertices[Indices[vertId + 1]].position,
	//	Vertices[Indices[vertId + 0]].position - Vertices[Indices[vertId + 2]].position));
	normal = normalize(mul((float3x3)ObjectToWorld3x4(), normal));

	float3 worldRayDirection = WorldRayDirection();
	float3 hitWorldPosition = HitWorldPosition();
	float4 random_seed = payload.seed;


	float emit_rate = dot(normal, -normalize(worldRayDirection));

	float3 Kd = MaterialAttributes.Kd;
	float emitIntensity2 = MaterialAttributes.emitIntensity * MaterialAttributes.emitIntensity;
	/*float2 uv = barycentrics.x * Vertices[Indices[vertId + 0]].TexCoords +
		barycentrics.y * Vertices[Indices[vertId + 1]].TexCoords +
		barycentrics.z * Vertices[Indices[vertId + 2]].TexCoords;*/
	float2 vertexTex[3] = {
		Vertices[Indices[vertId + 0]].TexCoords,
		Vertices[Indices[vertId + 1]].TexCoords,
		Vertices[Indices[vertId + 2]].TexCoords };
	float2 uv = HitAttribute(vertexTex, attrib);
	if (MaterialAttributes.useDiffuseTexture) {
		Kd = bricksTex.SampleLevel(gSamAnisotropicWarp, uv, 0).xyz;
	}

	float3 L_indir = get_light_indir(worldRayDirection, normal, hitWorldPosition, uv, payload.recursionDepth, random_seed);
	float3 L_dir = get_light_dir(worldRayDirection, hitWorldPosition, normal, uv, random_seed, payload.recursionDepth);

	payload.radiance = Kd * emitIntensity2 * emit_rate + L_indir + L_dir;
}


[shader("closesthit")]
void ClosestHit_ShadowRay(inout ShadowRayPayload payload, BuiltInTriangleIntersectionAttributes attrib) {
	payload.tHit = RayTCurrent();
}

