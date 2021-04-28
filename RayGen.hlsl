 #define HLSL
#include "HLSLCompat.h"
#include "ShaderHelper.hlsli"

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
	payload.seed = normalize(sceneParameter.seed + gOutput[launchIndex] + float4(DispatchRaysIndex(), 1.0) * float4(0.11, 0.42, 0.75, 1.0));
	float2 dims = float2(DispatchRaysDimensions().xy);
	float2 d = (((launchIndex.xy + 0.5f) / dims.xy) * 2.f - 1.f);

	float aspectRatio = dims.x / dims.y;

	RayDesc ray = GenerateCameraRay(launchIndex, sceneParameter);

	TraceRay(
		SceneBVH,
		/*RAY_FLAG_CULL_BACK_FACING_TRIANGLES*/RAY_FLAG_NONE,
		0xFF,
		0,
		0,
		0,
		ray,
		payload
	);

	//gOutput[launchIndex] = float4(payload.radiance, 1.f);
	payload.radiance = clamp(payload.radiance, 0.0, 300.f);
	gOutput[launchIndex] = lerp(gOutput[launchIndex], float4(payload.radiance, 1.0f), 1.0 / (float(sceneParameter.curSampleIdx) + 1.0f));
}
