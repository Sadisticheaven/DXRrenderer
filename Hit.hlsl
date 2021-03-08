#define HLSL
#include "HLSLCompat.h"


StructuredBuffer<Vertex> Vertex : register(t0);
StructuredBuffer<Index> Indices: register(t1);
ConstantBuffer<PrimitiveMaterialBuffer> MaterialAttributes : register(b0);


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
	//float3 normal = cross( Vertex[Indices[vertId + 1]].position - Vertex[Indices[vertId + 2]].position, Vertex[Indices[vertId + 0]].position - Vertex[Indices[vertId + 1]].position);

	float3 Kd = abs(MaterialAttributes.Kd);
  payload.colorAndDistance = float4(Kd, RayTCurrent());
}
