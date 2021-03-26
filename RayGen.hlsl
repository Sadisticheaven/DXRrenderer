#define HLSL
#include "HLSLCompat.h"

// Raytracing output texture, accessed as a UAV
RWTexture2D< float4 > gOutput : register(u0);

// Raytracing acceleration structure, accessed as a SRV
RaytracingAccelerationStructure SceneBVH : register(t0);// declared in CreateRayGenSignature()

// #DXR Extra: Perspective Camera
ConstantBuffer<SceneConstants> sceneParameter : register(b0);

[shader("raygeneration")]
void RayGen() {
	uint2 launchIndex = DispatchRaysIndex().xy;
	PayLoad payload;
	payload.radiance = float4(0, 0, 0, 0);
	payload.recursionDepth = 0;
	payload.seed = sceneParameter.seed + gOutput[launchIndex] + float4(DispatchRaysIndex(), 1.0) * float4(0.11, 0.42, 0.75, 1.0);
	float2 dims = float2(DispatchRaysDimensions().xy);
	float2 d = (((launchIndex.xy + 0.5f) / dims.xy) * 2.f - 1.f);

	float aspectRatio = dims.x / dims.y;

	RayDesc ray;
	ray.Origin = mul(sceneParameter.viewI, float4(0, 0, 0, 1));
	float4 target = mul(sceneParameter.projectionI, float4(d.x, -d.y, 1, 1));
	ray.Direction = mul(sceneParameter.viewI, float4(target.xyz, 0));

	ray.TMin = 0;
	ray.TMax = 100000;

	TraceRay(
		SceneBVH,
		RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
		0xFF,
		0,
		0,
		0,
		ray,
		payload
	);

	//gOutput[launchIndex] = float4(payload.radiance, 1.f);
	payload.radiance = clamp(payload.radiance, 0.0, 300);
	gOutput[launchIndex] = lerp(gOutput[launchIndex], float4(payload.radiance, 1.0f), 1.0 / (float(sceneParameter.CurrSampleIdx) + 1.0f));
}
