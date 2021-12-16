#define HLSL
#include "HLSLCompat.h"
#include "ShaderHelper.hlsli"

// Raytracing output texture, accessed as a UAV
RWTexture2D< float4 > gOutput : register(u0);

// Raytracing acceleration structure, accessed as a SRV
RaytracingAccelerationStructure SceneBVH : register(t0);// declared in CreateRayGenSignature()

// #DXR Extra: Perspective Camera
ConstantBuffer<SceneConstants> sceneParameter : register(b0);

StructuredBuffer<Light> global_light: register(t1);

RWStructuredBuffer<SceneOutput> sceneOut:register(u1);

[shader("raygeneration")]
void RayGen() {

	uint2 launchIndex = DispatchRaysIndex().xy;
	if (sceneParameter.curSampleIdx >= sceneParameter.maxSample && !(launchIndex.x == sceneParameter.xIdx && launchIndex.y == sceneParameter.yIdx))
		return;

	PayLoad payload;
	payload.radiance = float4(0, 0, 0, 0);
	payload.recursionDepth = 0;
	payload.seed = normalize(sceneParameter.seed + gOutput[launchIndex] + float4(DispatchRaysIndex(), 1.0) * float4(0.11, 0.42, 0.75, 1.0));
	float2 dims = float2(DispatchRaysDimensions().xy);
	float2 d = (((launchIndex.xy + 0.5f) / dims.xy) * 2.f - 1.f);

	float aspectRatio = dims.x / dims.y;

	RayDesc ray = GenerateCameraRay(launchIndex, sceneParameter);
	SceneOutput curSceneOutput;
	curSceneOutput.objIdx = -1;
	if (launchIndex.x == sceneParameter.xIdx && launchIndex.y == sceneParameter.yIdx) {
		sceneOut[0] = curSceneOutput;
	}
#if 1
	for (uint i = 0; i < sceneParameter.light_nums; ++i) {
		if (global_light[i].type == LightType::Point || global_light[i].type == LightType::Spot) {
			float3 position = global_light[i].position;
			float3 direction = position - ray.Origin.xyz;
			float disPow2 = dot(direction, direction);
			float dis = sqrt(disPow2);
			float3 normal_dire = normalize(direction);
			float3 row_dire = normalize(ray.Direction.xyz);
			if (dot(row_dire, normal_dire) > 0.999995f) {
				RayDesc rayDesc;
				rayDesc.Origin = ray.Origin;
				rayDesc.Direction = normal_dire;
				rayDesc.TMin = 0.01;
				rayDesc.TMax = dis;

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
				if (rayPayload.tHit == HitDistanceOnMiss) {
					gOutput[launchIndex] = float4(1, 1, 1, 1);
					return;
				}
			}
		}
		else if (global_light[i].type == LightType::Triangle) {

			float T;

			if (rayTriangleIntersect(global_light[i].position0, global_light[i].position1, global_light[i].position2,
				ray.Origin.xyz, ray.Direction.xyz, T, 0, 0)) {

				RayDesc rayDesc;
				rayDesc.Origin = ray.Origin;
				rayDesc.Direction = ray.Direction;
				rayDesc.TMin = 0;
				rayDesc.TMax = T;

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
				if (rayPayload.tHit == HitDistanceOnMiss) {
					gOutput[launchIndex] = float4(1, 1, 1, 1);
					return;
				}
			}
		}
	}
#endif
	TraceRay(
		SceneBVH,
#if 0
		RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
#else
		RAY_FLAG_NONE,
#endif
		0xFF,
		0,
		0,
		0,
		ray,
		payload
	);
	if (launchIndex.x == sceneParameter.xIdx && launchIndex.y == sceneParameter.yIdx) {
		curSceneOutput.objIdx = payload.objIdx;
		sceneOut[0] = curSceneOutput;
	}
	if (sceneParameter.curSampleIdx >= sceneParameter.maxSample)
		return;
	payload.radiance = clamp(payload.radiance, 0.0, 100.f);
	gOutput[launchIndex] = lerp(gOutput[launchIndex], float4(payload.radiance, 1.0f), 1.0 / (float(sceneParameter.curSampleIdx) + 1.0f));
}
