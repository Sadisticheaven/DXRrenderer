#define HLSL
#include "HLSLCompat.h"

[shader("miss")]
void Miss(inout PayLoad payload)
{
	uint2 launchIndex = DispatchRaysIndex().xy;
	float2 dims = float2(DispatchRaysDimensions().xy);

	float ramp = launchIndex.y / dims.y;
	//payload.irradiance = float3(0.0f, 0.2f, 0.7f - 0.3f*ramp);
	payload.irradiance = float3(0.0f, 0.0f, 0.0f);
}