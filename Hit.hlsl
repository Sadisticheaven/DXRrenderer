#define HLSL
#include "HLSLCompat.h"


StructuredBuffer<Vertex> BTriVertex : register(t0);
StructuredBuffer<int> indices: register(t1);

[shader("closesthit")] 
void ClosestHit(inout PayLoad payload, BuiltInTriangleIntersectionAttributes attrib)
{
	/*
	The following structure is declared in HLSL to represent hit attributes for fixed-function triangle intersection:
		struct BuiltInTriangleIntersectionAttributes
		{
			float2 barycentrics;
		};
	*/
	float3 barycentrics =
		float3(1.f - attrib.barycentrics.x - attrib.barycentrics.y, attrib.barycentrics.x, attrib.barycentrics.y);

	uint vertId = 3 * PrimitiveIndex();
	float3 hitColor = BTriVertex[indices[vertId + 0]].color * barycentrics.x +
		BTriVertex[indices[vertId + 1]].color * barycentrics.y +
		BTriVertex[indices[vertId + 2]].color * barycentrics.z;

  //payload.colorAndDistance = float4(1, 1, 0, RayTCurrent());
  payload.colorAndDistance = float4(hitColor, RayTCurrent());
}
