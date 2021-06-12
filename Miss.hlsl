#define HLSL
#include "HLSLCompat.h"

[shader("miss")]
void Miss(inout PayLoad payload)
{
	uint2 launchIndex = DispatchRaysIndex().xy;

	payload.radiance = float3(0.0f, 0.0f, 0.0f);
	payload.objIdx = -1;
}
[shader("miss")]
void Miss_ShadowRay(inout ShadowRayPayload payload)
{
	payload.tHit = HitDistanceOnMiss;
}