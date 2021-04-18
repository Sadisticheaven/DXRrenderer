//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#ifndef SHADERHELPER_H
#define SHADERHELPER_H

#include "HLSLCompat.h"

#define INFINITY (1.0/0.0)
#define M_PI 3.14159265358979323846   // pi

//struct Ray
//{
//    float3 origin;
//    float3 direction;
//};

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

float random(float2 p)
{
	float2 K1 = float2(23.14069263277926, 2.665144142690225);
	return abs(frac(cos(dot(p, K1)) * 12345.6789));
}

float4 createRandomFloat4(float4 seed)
{
	return float4(random(seed.xy), random(seed.yz), random(seed.zw), random(seed.wx));
}

float TrowbridgeReitz(in float cos2, in float alpha2)
{
	float x = alpha2 + (1 - cos2) / cos2;
	return alpha2 / (M_PI * cos2 * cos2 * x * x);
}

//float Smith_TrowbridgeReitz(in float3 wi, in float3 wo, in float3 wm, in float3 wn, in float alpha2)
//{
//	if (dot(wo, wm) < 0 || dot(wi, wm) < 0)
//		return 0.0f;
//
//	float cos2 = dot(wn, wo);
//	cos2 *= cos2;
//	float lambda1 = 0.5 * (-1 + sqrt(1 + alpha2 * (1 - cos2) / cos2));
//	cos2 = dot(wn, wi);
//	cos2 *= cos2;
//	float lambda2 = 0.5 * (-1 + sqrt(1 + alpha2 * (1 - cos2) / cos2));
//	return 1 / (1 + lambda1 + lambda2);
//}

float length_toPow2(float2 p)
{
	return dot(p, p);
}

float length_toPow2(float3 p)
{
	return dot(p, p);
}

float sqrt_n(float o, float k) {
	float l = 0, r = max(o, 1.0);
	float mid = (l + r) / 2, pre = r;
	while (abs(mid - pre) > 0.0001) {
		pre = mid;
		if (pow(o, mid) > o)
			l = mid;
		else
			r =
			mid = (l + r) / 2;

	}
}

// Returns a cycling <0 -> 1 -> 0> animation interpolant 
float CalculateAnimationInterpolant(in float elapsedTime, in float cycleDuration)
{
	float curLinearCycleTime = fmod(elapsedTime, cycleDuration) / cycleDuration;
	curLinearCycleTime = (curLinearCycleTime <= 0.5f) ? 2 * curLinearCycleTime : 1 - 2 * (curLinearCycleTime - 0.5f);
	return smoothstep(0, 1, curLinearCycleTime);
}

void swap(inout float a, inout float b)
{
	float temp = a;
	a = b;
	b = temp;
}

bool IsInRange(in float val, in float min, in float max)
{
	return (val >= min && val <= max);
}

// Load three 16 bit indices.
static
uint3 Load3x16BitIndices(uint offsetBytes, ByteAddressBuffer Indices)
{
	uint3 indices;

	// ByteAdressBuffer loads must be aligned at a 4 byte boundary.
	// Since we need to read three 16 bit indices: { 0, 1, 2 } 
	// aligned at a 4 byte boundary as: { 0 1 } { 2 0 } { 1 2 } { 0 1 } ...
	// we will load 8 bytes (~ 4 indices { a b | c d }) to handle two possible index triplet layouts,
	// based on first index's offsetBytes being aligned at the 4 byte boundary or not:
	//  Aligned:     { 0 1 | 2 - }
	//  Not aligned: { - 0 | 1 2 }
	const uint dwordAlignedOffset = offsetBytes & ~3;
	const uint2 four16BitIndices = Indices.Load2(dwordAlignedOffset);

	// Aligned: { 0 1 | 2 - } => retrieve first three 16bit indices
	if (dwordAlignedOffset == offsetBytes)
	{
		indices.x = four16BitIndices.x & 0xffff;
		indices.y = (four16BitIndices.x >> 16) & 0xffff;
		indices.z = four16BitIndices.y & 0xffff;
	}
	else // Not aligned: { - 0 | 1 2 } => retrieve last three 16bit indices
	{
		indices.x = (four16BitIndices.x >> 16) & 0xffff;
		indices.y = four16BitIndices.y & 0xffff;
		indices.z = (four16BitIndices.y >> 16) & 0xffff;
	}

	return indices;
}

// Retrieve hit world position.
float3 HitWorldPosition()
{
	return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

// Retrieve attribute at a hit position interpolated from vertex attributes using the hit's barycentrics.
float3 HitAttribute(float3 vertexAttribute[3], float2 barycentrics)
{
	return vertexAttribute[0] +
		barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]) +
		barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
}

// Generate a ray in world space for a camera pixel corresponding to an index from the dispatched 2D grid.
RayDesc GenerateCameraRay(uint2 index, SceneConstants sceneParameter)
{
	float2 launchIndex = index;
	float2 dims = float2(DispatchRaysDimensions().xy);
	float2 d = (((launchIndex.xy + 0.5f) / dims.xy) * 2.f - 1.f);

	float aspectRatio = dims.x / dims.y;

	RayDesc ray;
	ray.Origin = mul(sceneParameter.viewI, float4(0, 0, 0, 1));
	float4 target = mul(sceneParameter.projectionI, float4(d.x, -d.y, 1, 1));
	ray.Direction = mul(sceneParameter.viewI, float4(target.xyz, 0));

	ray.TMin = 0;
	ray.TMax = 100000;

	return ray;
}

// Test if a hit is culled based on specified RayFlags.
bool IsCulled(in Ray ray, in float3 hitSurfaceNormal)
{
	float rayDirectionNormalDot = dot(ray.direction, hitSurfaceNormal);

	bool isCulled =
		((RayFlags() & RAY_FLAG_CULL_BACK_FACING_TRIANGLES) && (rayDirectionNormalDot > 0))
		||
		((RayFlags() & RAY_FLAG_CULL_FRONT_FACING_TRIANGLES) && (rayDirectionNormalDot < 0));

	return isCulled;
}

// Test if a hit is valid based on specified RayFlags and <RayTMin, RayTCurrent> range.
bool IsAValidHit(in Ray ray, in float thit, in float3 hitSurfaceNormal)
{
	return IsInRange(thit, RayTMin(), RayTCurrent()) && !IsCulled(ray, hitSurfaceNormal);
}

// Texture coordinates on a horizontal plane.
float2 TexCoords(in float3 position)
{
	return position.xz;
}


float sqr(float x) { return x * x; }

float SchlickFresnel(float u)
{
	float m = clamp(1 - u, 0, 1);
	float m2 = m * m;
	return m2 * m2 * m; // pow(m,5)
}

float GTR1(float NdotH, float a)
{
	if (a >= 1) return 1 / M_PI;
	float a2 = a * a;
	float t = 1 + (a2 - 1) * NdotH * NdotH;
	return (a2 - 1) / (M_PI * log(a2) * t);
}

float GTR2(float NdotH, float a)
{
	float a2 = a * a;
	float t = 1 + (a2 - 1) * NdotH * NdotH;
	return a2 / (M_PI * t * t);
}

float GTR2_aniso(float NdotH, float HdotX, float HdotY, float ax, float ay)
{
	return 1 / (M_PI * ax * ay * sqr(sqr(HdotX / ax) + sqr(HdotY / ay) + NdotH * NdotH));
}

float smithG_GGX(float NdotV, float alphaG)
{
	float a = alphaG * alphaG;
	float b = NdotV * NdotV;
	return 1 / (NdotV + sqrt(a + b - a * b));
}

float Smith_TrowbridgeReitz(in float3 wi, in float3 wo, in float3 wm, in float3 wn, in float alpha2)
{
	if (dot(wo, wm) < 0 || dot(wi, wm) < 0)
		return 0.0f;

	float cos2 = dot(wn, wo);
	cos2 *= cos2;
	float lambda1 = 0.5 * (-1 + sqrt(1 + alpha2 * (1 - cos2) / cos2));
	cos2 = dot(wn, wi);
	cos2 *= cos2;
	float lambda2 = 0.5 * (-1 + sqrt(1 + alpha2 * (1 - cos2) / cos2));
	return 1 / (1 + lambda1 + lambda2);
}

float smithG_GGX_aniso(float NdotV, float VdotX, float VdotY, float ax, float ay)
{
	return 1 / (NdotV + sqrt(sqr(VdotX * ax) + sqr(VdotY * ay) + sqr(NdotV)));
}

float3 mon2lin(float3 x)
{
	return float3(pow(x.x, 2.2), pow(x.y, 2.2), pow(x.z, 2.2));
}

float3 lin2mon(float3 x)
{
	return float3(pow(x.x, 1 / 2.2), pow(x.y, 1 / 2.2), pow(x.z, 1 / 2.2));
}

//目前使用该版本
float3 Disney_BRDF_diffuse(float3 L, float3 V, float3 N, float3 Kd, PrimitiveMaterialBuffer Mat)
{
	float metallic = Mat.metallic;
	float subsurface = Mat.subsurface;
	float specular = Mat.specular;
	float roughness = Mat.roughness;
	float specularTint = Mat.specularTint;
	float sheen = Mat.sheen;
	float sheenTint = Mat.sheenTint;
	float NdotL = dot(normalize(N), normalize(L));
	float NdotV = dot(normalize(N), normalize(V));

	if (NdotL < 0 || NdotV < 0) return float3(0, 0, 0);

	float3 H = normalize(L + V);
	float NdotH = dot(N, H);
	float LdotH = dot(L, H);

	float3 Cdlin = mon2lin(Kd);
	float Cdlum = 0.3 * Cdlin.x + 0.6 * Cdlin.y + 0.1 * Cdlin.z; // luminance approx.

	float3 Ctint = Cdlum > 0 ? Cdlin / Cdlum : float3(1, 1, 1); // normalize lum. to isolate hue+sat
	float3 Cspec0 = lerp(specular * 0.08 * lerp(float3(1, 1, 1), Ctint, specularTint), Cdlin, metallic);
	float3 Csheen = lerp(float3(1, 1, 1), Ctint, sheenTint);

	float FL = SchlickFresnel(NdotL), FV = SchlickFresnel(NdotV);
	float Fd90 = 0.5 + 2 * LdotH * LdotH * roughness;
	float Fd = lerp(1.0, Fd90, FL) * lerp(1.0, Fd90, FV);

	float Fss90 = LdotH * LdotH * roughness;
	float Fss = lerp(1.0, Fss90, FL) * lerp(1.0, Fss90, FV);
	float ss = 1.25 * (Fss * (1 / (NdotL + NdotV) - 0.5) + 0.5);

	float FH = SchlickFresnel(LdotH);
	float3 Fsheen = FH * sheen * Csheen;


	return ((1 / M_PI) * lerp(Fd, ss, subsurface) * Cdlin + Fsheen);
}

float3 Disney_BRDF_specular(float3 L, float3 V, float3 N, float3 Kd, PrimitiveMaterialBuffer Mat)
{
	float metallic = Mat.metallic;
	float specular = Mat.specular;
	float roughness = Mat.roughness;
	float specularTint = Mat.specularTint;
	float NdotL = dot(normalize(N), normalize(L));
	float NdotV = dot(normalize(N), normalize(V));

	if (NdotL < 0 || NdotV < 0) return float3(0, 0, 0);

	float3 H = normalize(L + V);
	float NdotH = dot(N, H);
	float LdotH = dot(L, H);
	float VdotH = dot(V, H);

	float3 Cdlin = mon2lin(Kd);
	float Cdlum = 0.3 * Cdlin.x + 0.6 * Cdlin.y + 0.1 * Cdlin.z; // luminance approx.

	float3 Ctint = Cdlum > 0 ? Cdlin / Cdlum : float3(1, 1, 1); // normalize lum. to isolate hue+sat
	float3 Cspec0 = lerp(specular * 0.08 * lerp(float3(1, 1, 1), Ctint, specularTint), Cdlin, metallic);

	//float Ds = Smith_TrowbridgeReitz(L, V, H, N, roughness * roughness);
	float FH = SchlickFresnel(LdotH);
	float3 Fs = lerp(Cspec0, float3(1, 1, 1), FH);
	float Gs = Smith_TrowbridgeReitz(L, V, H, N, roughness * roughness);

	return Gs * Fs / NdotL / NdotV / NdotH * VdotH;
}

float3 Disney_BRDF_clearcoat(float3 L, float3 V, float3 N, float3 Kd, PrimitiveMaterialBuffer Mat)
{
	float clearcoat = Mat.clearcoat;
	float clearcoatGloss = Mat.clearcoatGloss;
	float NdotL = dot(normalize(N), normalize(L));
	float NdotV = dot(normalize(N), normalize(V));

	if (NdotL < 0 || NdotV < 0) return float3(0, 0, 0);

	float3 H = normalize(L + V);
	float NdotH = dot(N, H);
	float LdotH = dot(L, H);
	float VdotH = dot(V, H);

	float FH = SchlickFresnel(LdotH);

	//float Dr = GTR1(NdotH, lerp(0.1, 0.001, clearcoatGloss));
	float Fr = lerp(0.04, 1.0, FH);
	float Gr = smithG_GGX(NdotL, 0.25) * smithG_GGX(NdotV, 0.25);

	return  0.25 * clearcoat * Gr * Fr / NdotL / NdotV / NdotH * VdotH;
}

//以下版本保留，暂不使用
float3 Disney_BRDF(float3 L, float3 V, float3 N, float3 Kd, PrimitiveMaterialBuffer Mat)
{
	float metallic = Mat.metallic;
	float subsurface = Mat.subsurface;
	float specular = Mat.specular;
	float roughness = Mat.roughness;
	float specularTint = Mat.specularTint;
	float sheen = Mat.sheen;
	float sheenTint = Mat.sheenTint;
	float clearcoat = Mat.clearcoat;
	float clearcoatGloss = Mat.clearcoatGloss;
	float NdotL = dot(normalize(N), normalize(L));
	float NdotV = dot(normalize(N), normalize(V));

	if (NdotL < 0 || NdotV < 0) return float3(0, 0, 0);

	float3 H = normalize(L + V);
	float NdotH = dot(N, H);
	float LdotH = dot(L, H);

	float3 Cdlin = mon2lin(Kd);
	float Cdlum = 0.3 * Cdlin.x + 0.6 * Cdlin.y + 0.1 * Cdlin.z; // luminance approx.

	float3 Ctint = Cdlum > 0 ? Cdlin / Cdlum : float3(1, 1, 1); // normalize lum. to isolate hue+sat
	float3 Cspec0 = lerp(specular * 0.08 * lerp(float3(1, 1, 1), Ctint, specularTint), Cdlin, metallic);
	float3 Csheen = lerp(float3(1, 1, 1), Ctint, sheenTint);

	float FL = SchlickFresnel(NdotL), FV = SchlickFresnel(NdotV);
	float Fd90 = 0.5 + 2 * LdotH * LdotH * roughness;
	float Fd = lerp(1.0, Fd90, FL) * lerp(1.0, Fd90, FV);

	float Fss90 = LdotH * LdotH * roughness;
	float Fss = lerp(1.0, Fss90, FL) * lerp(1.0, Fss90, FV);
	float ss = 1.25 * (Fss * (1 / (NdotL + NdotV) - 0.5) + 0.5);

	float Ds = GTR2(NdotH, max(roughness, 0.0002));
	float FH = SchlickFresnel(LdotH);
	float3 Fs = lerp(Cspec0, float3(1, 1, 1), FH);
	float Gs = smithG_GGX(NdotL, max(roughness, 0.0002)) * smithG_GGX(NdotV, max(roughness, 0.0002));

	float3 Fsheen = FH * sheen * Csheen;

	float Dr = GTR1(NdotH, lerp(0.1, 0.001, clearcoatGloss));
	float Fr = lerp(0.04, 1.0, FH);
	float Gr = smithG_GGX(NdotL, 0.25) * smithG_GGX(NdotV, 0.25);

	return ((1 / M_PI) * lerp(Fd, ss, subsurface) * Cdlin + Fsheen) * (1 - metallic)
		+ Gs * Fs * Ds + 0.25 * clearcoat * Gr * Fr * Dr;
}


#endif // RAYTRACINGSHADERHELPER_H