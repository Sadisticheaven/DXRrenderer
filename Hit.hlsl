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
	float3 Kd = float3(0.7, 0.7, 0.7);
	switch (InstanceID())
	{
	case 0:
		Kd = cross(Vertex[Indices[vertId + 2]].position - Vertex[Indices[vertId + 1]].position, Vertex[Indices[vertId + 1]].position - Vertex[Indices[vertId + 0]].position);
		break;
	case 1:
		Kd = abs(MaterialAttributes.Kd);
		break;
	case 2:
		Kd = Vertex[Indices[vertId + 0]].color * barycentrics.x + Vertex[Indices[vertId + 1]].color * barycentrics.y + Vertex[Indices[vertId + 2]].color * barycentrics.z;
		break;
	}
	Kd = MaterialAttributes.Kd;
  payload.colorAndDistance = float4(Kd, RayTCurrent());
}
