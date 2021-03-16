#define HLSL
#include "HLSLCompat.h"
#include "ShaderHelper.hlsli"

StructuredBuffer<Vertex> Vertex : register(t0);
StructuredBuffer<Index> Indices: register(t1);
RaytracingAccelerationStructure l_scene : register(t2);
ConstantBuffer<PrimitiveMaterialBuffer> MaterialAttributes : register(b0);


float3 CastRay(Ray ray, uint curRecursionDepth) {
	if (curRecursionDepth > MAX_RAY_RECURSION_DEPTH)
		return float3(0.0f, 0.0f, 0.0f);
	RayDesc rayDesc;
	rayDesc.Origin = ray.origin;
	rayDesc.Direction = ray.direction;
	rayDesc.TMin = 0;
	rayDesc.TMax = 100000;
	PayLoad rayPayload = { float3(0.0, 0.0, 0.0), curRecursionDepth + 1 };
	TraceRay(
		l_scene,
		RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
		0xFF,
		0,
		0,
		0,
		rayDesc,
		rayPayload
	);
	return rayPayload.irradiance;
}


[shader("closesthit")]
void ClosestHit(inout PayLoad payload, BuiltInTriangleIntersectionAttributes attrib)
{
	//if (MaterialAttributes.emit.x >= 0.01) {
	//	//payload.irradiance = float3(47.8348007, 38.5663986, 31.0807991);
	//	payload.irradiance = MaterialAttributes.emit;
	//	return;
	//}
	float3 barycentrics =
		float3(1.f - attrib.barycentrics.x - attrib.barycentrics.y, attrib.barycentrics.x, attrib.barycentrics.y);
	uint vertId = 3 * PrimitiveIndex();
	//以下方式为计算点法线并插值
	float3 normal = barycentrics.x * Vertex[Indices[vertId + 0]].normal +
		barycentrics.y * Vertex[Indices[vertId + 1]].normal +
		barycentrics.z * Vertex[Indices[vertId + 2]].normal;
	//以下方式为计算面法线
	//float3 normal = normalize(cross(Vertex[Indices[vertId + 0]].position - Vertex[Indices[vertId + 1]].position,
	//								Vertex[Indices[vertId + 0]].position - Vertex[Indices[vertId + 2]].position));
	float3 Kd = float3(0.7, 0.7, 0.7);
	float3 dir = reflect(WorldRayDirection(), normal);
	Ray ray;
	ray.origin = HitWorldPosition();
	ray.direction = dir;
	Kd = MaterialAttributes.Kd * CastRay(ray, payload.recursionDepth);
	//Kd = Vertex[Indices[vertId]].normal;
	payload.irradiance = Kd + MaterialAttributes.emit;
}
