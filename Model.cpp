#include "stdafx.h"
#include "Model.h"

Mesh CreateBox(float width, float height, float depth, uint32 numSubdivisions)
{
	Mesh meshData;

	//
	// Create the vertices.
	//

	TmpVertex v[24];

	float w2 = 0.5f * width;
	float h2 = 0.5f * height;
	float d2 = 0.5f * depth;

	// Fill in the front face vertex data.
	
	v[0] = TmpVertex(-w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

	v[1] = TmpVertex(-w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[2] = TmpVertex(+w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[3] = TmpVertex(+w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// Fill in the back face vertex data.
	v[4] = TmpVertex(-w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[5] = TmpVertex(+w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[6] = TmpVertex(+w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[7] = TmpVertex(-w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	// Fill in the top face vertex data.
	v[8] = TmpVertex(-w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[9] = TmpVertex(-w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[10] = TmpVertex(+w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[11] = TmpVertex(+w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// Fill in the bottom face vertex data.
	v[12] = TmpVertex(-w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[13] = TmpVertex(+w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[14] = TmpVertex(+w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[15] = TmpVertex(-w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	// Fill in the left face vertex data.
	v[16] = TmpVertex(-w2, -h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[17] = TmpVertex(-w2, +h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[18] = TmpVertex(-w2, +h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	v[19] = TmpVertex(-w2, -h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	// Fill in the right face vertex data.
	v[20] = TmpVertex(+w2, -h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
	v[21] = TmpVertex(+w2, +h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	v[22] = TmpVertex(+w2, +h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
	v[23] = TmpVertex(+w2, -h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

	Vertex ver[24];
	for (int i = 0; i < 24; ++i) {
		ver[i].position = v[i].Position;
		ver[i].normal = v[i].Normal;
		ver[i].TexCoords = v[i].TexC;
		ver[i].Tangent = v[i].TangentU;
	}
	meshData.vertices.assign(&ver[0], &ver[24]);

	//
	// Create the indices.
	//

	uint32 i[36];

	// Fill in the front face index data
	i[0] = 0; i[1] = 1; i[2] = 2;
	i[3] = 0; i[4] = 2; i[5] = 3;

	// Fill in the back face index data
	i[6] = 4; i[7] = 5; i[8] = 6;
	i[9] = 4; i[10] = 6; i[11] = 7;

	// Fill in the top face index data
	i[12] = 8; i[13] = 9; i[14] = 10;
	i[15] = 8; i[16] = 10; i[17] = 11;

	// Fill in the bottom face index data
	i[18] = 12; i[19] = 13; i[20] = 14;
	i[21] = 12; i[22] = 14; i[23] = 15;

	// Fill in the left face index data
	i[24] = 16; i[25] = 17; i[26] = 18;
	i[27] = 16; i[28] = 18; i[29] = 19;

	// Fill in the right face index data
	i[30] = 20; i[31] = 21; i[32] = 22;
	i[33] = 20; i[34] = 22; i[35] = 23;

	meshData.indices.assign(&i[0], &i[36]);

	// Put a cap on the number of subdivisions.
	numSubdivisions = std::min<uint32>(numSubdivisions, 6u);

	for (uint32 i = 0; i < numSubdivisions; ++i)
		Subdivide(meshData);

	int size = meshData.vertices.size();
	for (int i = 0; i < size; ++i) {

		meshData.center.x += meshData.vertices[i].position.x;
		meshData.center.y += meshData.vertices[i].position.x;
		meshData.center.z += meshData.vertices[i].position.x;
	}
	meshData.center.x /= size;
	meshData.center.y /= size;
	meshData.center.z /= size;

	return meshData;
}

Mesh CreateSphere(float radius, uint32 sliceCount, uint32 stackCount)
{
	Mesh meshData;

	//
	// Compute the vertices stating at the top pole and moving down the stacks.
	//

	// Poles: note that there will be texture coordinate distortion as there is
	// not a unique point on the texture map to assign to the pole when mapping
	// a rectangular texture onto a sphere.
	TmpVertex topVertex(0.0f, +radius, 0.0f, 0.0f, +1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	Vertex tmpVertex;
	tmpVertex.position = topVertex.Position;
	tmpVertex.normal = topVertex.Normal;
	tmpVertex.TexCoords = topVertex.TexC;
	tmpVertex.Tangent = topVertex.TangentU;
	meshData.vertices.push_back(tmpVertex);
	TmpVertex bottomVertex(0.0f, -radius, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

	

	float phiStep = XM_PI / stackCount;
	float thetaStep = 2.0f * XM_PI / sliceCount;

	// Compute vertices for each stack ring (do not count the poles as rings).
	for (uint32 i = 1; i <= stackCount - 1; ++i)
	{
		float phi = i * phiStep;

		// vertices of ring.
		for (uint32 j = 0; j <= sliceCount; ++j)
		{
			float theta = j * thetaStep;

			TmpVertex v;

			// spherical to cartesian
			v.Position.x = radius * sinf(phi) * cosf(theta);
			v.Position.y = radius * cosf(phi);
			v.Position.z = radius * sinf(phi) * sinf(theta);

			// Partial derivative of P with respect to theta
			v.TangentU.x = -radius * sinf(phi) * sinf(theta);
			v.TangentU.y = 0.0f;
			v.TangentU.z = +radius * sinf(phi) * cosf(theta);

			XMVECTOR T = XMLoadFloat3(&v.TangentU);
			XMStoreFloat3(&v.TangentU, XMVector3Normalize(T));

			XMVECTOR p = XMLoadFloat3(&v.Position);
			XMStoreFloat3(&v.Normal, XMVector3Normalize(p));

			v.TexC.x = theta / XM_2PI;
			v.TexC.y = phi / XM_PI;

			tmpVertex.position = v.Position;
			tmpVertex.normal = v.Normal;
			tmpVertex.Tangent = v.TangentU;
			tmpVertex.TexCoords = v.TexC;

			meshData.vertices.push_back(tmpVertex);
		}
	}

	tmpVertex.position = bottomVertex.Position;
	tmpVertex.normal = bottomVertex.Normal;
	tmpVertex.TexCoords = bottomVertex.TexC;
	tmpVertex.Tangent = bottomVertex.TangentU;
	meshData.vertices.push_back(tmpVertex);

	//
	// Compute indices for top stack.  The top stack was written first to the vertex buffer
	// and connects the top pole to the first ring.
	//

	for (uint32 i = 1; i <= sliceCount; ++i)
	{
		meshData.indices.push_back(0);
		meshData.indices.push_back(i + 1);
		meshData.indices.push_back(i);
	}

	//
	// Compute indices for inner stacks (not connected to poles).
	//

	// Offset the indices to the index of the first vertex in the first ring.
	// This is just skipping the top pole vertex.
	uint32 baseIndex = 1;
	uint32 ringVertexCount = sliceCount + 1;
	for (uint32 i = 0; i < stackCount - 2; ++i)
	{
		for (uint32 j = 0; j < sliceCount; ++j)
		{
			meshData.indices.push_back(baseIndex + i * ringVertexCount + j);
			meshData.indices.push_back(baseIndex + i * ringVertexCount + j + 1);
			meshData.indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);

			meshData.indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);
			meshData.indices.push_back(baseIndex + i * ringVertexCount + j + 1);
			meshData.indices.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
		}
	}

	//
	// Compute indices for bottom stack.  The bottom stack was written last to the vertex buffer
	// and connects the bottom pole to the bottom ring.
	//

	// South pole vertex was added last.
	uint32 southPoleIndex = (uint32)meshData.vertices.size() - 1;

	// Offset the indices to the index of the first vertex in the last ring.
	baseIndex = southPoleIndex - ringVertexCount;

	for (uint32 i = 0; i < sliceCount; ++i)
	{
		meshData.indices.push_back(southPoleIndex);
		meshData.indices.push_back(baseIndex + i);
		meshData.indices.push_back(baseIndex + i + 1);
	}

	int size = meshData.vertices.size();
	for (int i = 0; i < size; ++i) {

		meshData.center.x += meshData.vertices[i].position.x;
		meshData.center.y += meshData.vertices[i].position.x;
		meshData.center.z += meshData.vertices[i].position.x;
	}
	meshData.center.x /= size;
	meshData.center.y /= size;
	meshData.center.z /= size;
	return meshData;
}

void Subdivide(Mesh& meshData)
{
	// Save a copy of the input geometry.
	Mesh inputCopy = meshData;


	meshData.vertices.resize(0);
	meshData.indices.resize(0);

	//       v1
	//       *
	//      / \
	//     /   \
	//  m0*-----*m1
	//   / \   / \
	//  /   \ /   \
	// *-----*-----*
	// v0    m2     v2

	uint32 numTris = (uint32)inputCopy.indices.size() / 3;
	for (uint32 i = 0; i < numTris; ++i)
	{
		/*TmpVertex v0 = inputCopy.vertices[inputCopy.indices[i * 3 + 0]];
		TmpVertex v1 = inputCopy.vertices[inputCopy.indices[i * 3 + 1]];
		TmpVertex v2 = inputCopy.vertices[inputCopy.indices[i * 3 + 2]];*/
		Vertex v0 = inputCopy.vertices[inputCopy.indices[i * 3 + 0]];
		Vertex v1 = inputCopy.vertices[inputCopy.indices[i * 3 + 1]];
		Vertex v2 = inputCopy.vertices[inputCopy.indices[i * 3 + 2]];

		//
		// Generate the midpoints.
		//

		/*TmpVertex m0 = MidPoint(v0, v1);
		TmpVertex m1 = MidPoint(v1, v2);
		TmpVertex m2 = MidPoint(v0, v2);*/
		Vertex m0 = MidPoint(v0, v1);
		Vertex m1 = MidPoint(v1, v2);
		Vertex m2 = MidPoint(v0, v2);

		//
		// Add new geometry.
		//

		meshData.vertices.push_back(v0); // 0
		meshData.vertices.push_back(v1); // 1
		meshData.vertices.push_back(v2); // 2
		meshData.vertices.push_back(m0); // 3
		meshData.vertices.push_back(m1); // 4
		meshData.vertices.push_back(m2); // 5

		meshData.indices.push_back(i * 6 + 0);
		meshData.indices.push_back(i * 6 + 3);
		meshData.indices.push_back(i * 6 + 5);

		meshData.indices.push_back(i * 6 + 3);
		meshData.indices.push_back(i * 6 + 4);
		meshData.indices.push_back(i * 6 + 5);

		meshData.indices.push_back(i * 6 + 5);
		meshData.indices.push_back(i * 6 + 4);
		meshData.indices.push_back(i * 6 + 2);

		meshData.indices.push_back(i * 6 + 3);
		meshData.indices.push_back(i * 6 + 1);
		meshData.indices.push_back(i * 6 + 4);
	}
}

Vertex MidPoint(const Vertex& v0, const Vertex& v1)
{
	XMVECTOR p0 = XMLoadFloat3(&v0.position);
	XMVECTOR p1 = XMLoadFloat3(&v1.position);

	XMVECTOR n0 = XMLoadFloat3(&v0.normal);
	XMVECTOR n1 = XMLoadFloat3(&v1.normal);

	XMVECTOR tan0 = XMLoadFloat3(&v0.Tangent);
	XMVECTOR tan1 = XMLoadFloat3(&v1.Tangent);

	XMVECTOR tex0 = XMLoadFloat2(&v0.TexCoords);
	XMVECTOR tex1 = XMLoadFloat2(&v1.TexCoords);

	// Compute the midpoints of all the attributes.  Vectors need to be normalized
	// since linear interpolating can make them not unit length.  
	XMVECTOR pos = 0.5f * (p0 + p1);
	XMVECTOR normal = XMVector3Normalize(0.5f * (n0 + n1));
	XMVECTOR tangent = XMVector3Normalize(0.5f * (tan0 + tan1));
	XMVECTOR tex = 0.5f * (tex0 + tex1);

	Vertex v;
	XMStoreFloat3(&v.position, pos);
	XMStoreFloat3(&v.normal, normal);
	XMStoreFloat3(&v.Tangent, tangent);
	XMStoreFloat2(&v.TexCoords, tex);

	return v;
}

Mesh CreateGeosphere(float radius, uint32 numSubdivisions)
{
	Mesh meshData;

	// Put a cap on the number of subdivisions.
	//确定细分次数
	numSubdivisions = std::min<uint32>(numSubdivisions, 6u);

	// Approximate a sphere by tessellating an icosahedron.
	//通过对正二十面体进行曲面细分逼近球体

	const float X = 0.525731f;
	const float Z = 0.850651f;

	XMFLOAT3 pos[12] =
	{
		XMFLOAT3(-X, 0.0f, Z),  XMFLOAT3(X, 0.0f, Z),
		XMFLOAT3(-X, 0.0f, -Z), XMFLOAT3(X, 0.0f, -Z),
		XMFLOAT3(0.0f, Z, X),   XMFLOAT3(0.0f, Z, -X),
		XMFLOAT3(0.0f, -Z, X),  XMFLOAT3(0.0f, -Z, -X),
		XMFLOAT3(Z, X, 0.0f),   XMFLOAT3(-Z, X, 0.0f),
		XMFLOAT3(Z, -X, 0.0f),  XMFLOAT3(-Z, -X, 0.0f)
	};

	uint32 k[60] =
	{
		1,4,0,  4,9,0,  4,5,9,  8,5,4,  1,8,4,
		1,10,8, 10,3,8, 8,3,5,  3,2,5,  3,7,2,
		3,10,7, 10,6,7, 6,11,7, 6,0,11, 6,1,0,
		10,1,6, 11,0,9, 2,11,9, 5,2,9,  11,2,7
	};

	meshData.vertices.resize(12);
	meshData.indices.assign(&k[0], &k[60]);

	for (uint32 i = 0; i < 12; ++i)
		meshData.vertices[i].position = pos[i];

	for (uint32 i = 0; i < numSubdivisions; ++i)
		Subdivide(meshData);

	// Project vertices onto sphere and scale.
	//将每一个顶点投影到球面，并推导其纹理坐标
	for (uint32 i = 0; i < meshData.vertices.size(); ++i)
	{
		// Project onto unit sphere.
		//投影到单位球面
		XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&meshData.vertices[i].position));

		// Project onto sphere.
		//投影到球面上
		XMVECTOR p = radius * n;

		XMStoreFloat3(&meshData.vertices[i].position, p);
		XMStoreFloat3(&meshData.vertices[i].normal, n);

		// Derive texture coordinates from spherical coordinates.
		//根据球面坐标推导纹理坐标
		float theta = atan2f(meshData.vertices[i].position.z, meshData.vertices[i].position.x);

		// Put in [0, 2pi].
		//将theta限制在[0,2pi]间
		if (theta < 0.0f)
			theta += XM_2PI;

		float phi = acosf(meshData.vertices[i].position.y / radius);

		meshData.vertices[i].TexCoords.x = theta / XM_2PI;
		meshData.vertices[i].TexCoords.y = phi / XM_PI;

		// Partial derivative of P with respect to theta
		//求出P关于theta的偏导数
		meshData.vertices[i].Tangent.x = -radius * sinf(phi) * sinf(theta);
		meshData.vertices[i].Tangent.y = 0.0f;
		meshData.vertices[i].Tangent.z = +radius * sinf(phi) * cosf(theta);

		XMVECTOR T = XMLoadFloat3(&meshData.vertices[i].Tangent);
		XMStoreFloat3(&meshData.vertices[i].Tangent, XMVector3Normalize(T));
	}

	int size = meshData.vertices.size();
	for (int i = 0; i < size; ++i) {

		meshData.center.x += meshData.vertices[i].position.x;
		meshData.center.y += meshData.vertices[i].position.x;
		meshData.center.z += meshData.vertices[i].position.x;
	}
	meshData.center.x /= size;
	meshData.center.y /= size;
	meshData.center.z /= size;

	return meshData;
}

Mesh CreateCylinder(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount)
{
	Mesh meshData;

	//
	// Build Stacks.
	//构建堆叠层
	// 

	float stackHeight = height / stackCount;

	// Amount to increment radius as we move up each stack level from bottom to top.
	//从下至上遍历每个相邻分层时所需的半径增量
	float radiusStep = (topRadius - bottomRadius) / stackCount;

	uint32 ringCount = stackCount + 1;
	
	// Compute vertices for each stack ring starting at the bottom and moving up.
	//从底面开始，从下至上计算每个堆叠层环上的顶点坐标
	for (uint32 i = 0; i < ringCount; ++i)
	{
		float y = -0.5f * height + i * stackHeight;
		float r = bottomRadius + i * radiusStep;

		// vertices of ring
		//环上的各个顶点
		float dTheta = 2.0f * XM_PI / sliceCount;
		for (uint32 j = 0; j <= sliceCount; ++j)
		{
			TmpVertex vertex;
			Vertex tmpVertex;
			float c = cosf(j * dTheta);
			float s = sinf(j * dTheta);

			vertex.Position = XMFLOAT3(r * c, y, r * s);

			vertex.TexC.x = (float)j / sliceCount;
			vertex.TexC.y = 1.0f - (float)i / stackCount;

			// Cylinder can be parameterized as follows, where we introduce v
			// parameter that goes in the same direction as the v tex-coord
			// so that the bitangent goes in the same direction as the v tex-coord.
			//   Let r0 be the bottom radius and let r1 be the top radius.
			//   y(v) = h - hv for v in [0,1].
			//   r(v) = r1 + (r0-r1)v
			//
			//   x(t, v) = r(v)*cos(t)
			//   y(t, v) = h - hv
			//   z(t, v) = r(v)*sin(t)
			// 
			//  dx/dt = -r(v)*sin(t)
			//  dy/dt = 0
			//  dz/dt = +r(v)*cos(t)
			//
			//  dx/dv = (r0-r1)*cos(t)
			//  dy/dv = -h
			//  dz/dv = (r0-r1)*sin(t)

			// This is unit length.
			//单位长度
			vertex.TangentU = XMFLOAT3(-s, 0.0f, c);

			float dr = bottomRadius - topRadius;
			XMFLOAT3 bitangent(dr * c, -height, dr * s);

			XMVECTOR T = XMLoadFloat3(&vertex.TangentU);
			XMVECTOR B = XMLoadFloat3(&bitangent);
			XMVECTOR N = XMVector3Normalize(XMVector3Cross(T, B));
			XMStoreFloat3(&vertex.Normal, N);
			tmpVertex.position = vertex.Position;
			tmpVertex.normal = vertex.Normal;
			tmpVertex.Tangent = vertex.TangentU;
			tmpVertex.TexCoords = vertex.TexC;
			meshData.vertices.push_back(tmpVertex);
		}
	}

	// Add one because we duplicate the first and last vertex per ring
	// since the texture coordinates are different.
	//+1是希望让每环的第一个顶点和最后一个顶点重合，这是因为它们的纹理坐标不同
	uint32 ringVertexCount = sliceCount + 1;

	// Compute indices for each stack.
	//计算每个侧面块中三角形索引
	for (uint32 i = 0; i < stackCount; ++i)
	{
		for (uint32 j = 0; j < sliceCount; ++j)
		{
			meshData.indices.push_back(i * ringVertexCount + j);
			meshData.indices.push_back((i + 1) * ringVertexCount + j);
			meshData.indices.push_back((i + 1) * ringVertexCount + j + 1);

			meshData.indices.push_back(i * ringVertexCount + j);
			meshData.indices.push_back((i + 1) * ringVertexCount + j + 1);
			meshData.indices.push_back(i * ringVertexCount + j + 1);
		}
	}

	BuildCylinderTopCap(bottomRadius, topRadius, height, sliceCount, stackCount, meshData);
	BuildCylinderBottomCap(bottomRadius, topRadius, height, sliceCount, stackCount, meshData);

	int size = meshData.vertices.size();
	for (int i = 0; i < size; ++i) {

		meshData.center.x += meshData.vertices[i].position.x;
		meshData.center.y += meshData.vertices[i].position.x;
		meshData.center.z += meshData.vertices[i].position.x;
	}
	meshData.center.x /= size;
	meshData.center.y /= size;
	meshData.center.z /= size;

	return meshData;
}

void BuildCylinderTopCap(float bottomRadius, float topRadius, float height,
	uint32 sliceCount, uint32 stackCount, Mesh& meshData)
{
	uint32 baseIndex = (uint32)meshData.vertices.size();

	float y = 0.5f * height;
	float dTheta = 2.0f * XM_PI / sliceCount;

	// Duplicate cap ring vertices because the texture coordinates and normals differ.
	//使圆台端面环上的首尾顶点重合，因为其纹理坐标和法线不同
	for (uint32 i = 0; i <= sliceCount; ++i)
	{
		float x = topRadius * cosf(i * dTheta);
		float z = topRadius * sinf(i * dTheta);

		// Scale down by the height to try and make top cap texture coord area
		// proportional to base.
		//根据圆台的高度使顶面纹理坐标的范围按比例缩小
		float u = x / height + 0.5f;
		float v = z / height + 0.5f;
		TmpVertex vertex(x, y, z, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v);
		Vertex tmpVertex;
		tmpVertex.position = vertex.Position;
		tmpVertex.normal = vertex.Normal;
		tmpVertex.Tangent = vertex.TangentU;
		tmpVertex.TexCoords = vertex.TexC;
		meshData.vertices.push_back(tmpVertex);
	}

	// Cap center vertex.
	//顶面的中心顶点
	TmpVertex vertex(0.0f, y, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f);
	Vertex tmpVertex;
	tmpVertex.position = vertex.Position;
	tmpVertex.normal = vertex.Normal;
	tmpVertex.Tangent = vertex.TangentU;
	tmpVertex.TexCoords = vertex.TexC;
	meshData.vertices.push_back(tmpVertex);

	// Index of center vertex.
	//中心顶点索引
	uint32 centerIndex = (uint32)meshData.vertices.size() - 1;

	for (uint32 i = 0; i < sliceCount; ++i)
	{
		meshData.indices.push_back(centerIndex);
		meshData.indices.push_back(baseIndex + i + 1);
		meshData.indices.push_back(baseIndex + i);
	}
}

void BuildCylinderBottomCap(float bottomRadius, float topRadius, float height,
	uint32 sliceCount, uint32 stackCount, Mesh& meshData)
{
	// 
	// Build bottom cap.
	//

	uint32 baseIndex = (uint32)meshData.vertices.size();
	float y = -0.5f * height;

	// vertices of ring
	float dTheta = 2.0f * XM_PI / sliceCount;
	for (uint32 i = 0; i <= sliceCount; ++i)
	{
		float x = bottomRadius * cosf(i * dTheta);
		float z = bottomRadius * sinf(i * dTheta);

		// Scale down by the height to try and make top cap texture coord area
		// proportional to base.
		float u = x / height + 0.5f;
		float v = z / height + 0.5f;
		TmpVertex vertex(x, y, z, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v);
		Vertex tmpVertex;
		tmpVertex.position = vertex.Position;
		tmpVertex.normal = vertex.Normal;
		tmpVertex.Tangent = vertex.TangentU;
		tmpVertex.TexCoords = vertex.TexC;
		meshData.vertices.push_back(tmpVertex);
	}

	// Cap center vertex.
	TmpVertex vertex(0.0f, y, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f);
	Vertex tmpVertex;
	tmpVertex.position = vertex.Position;
	tmpVertex.normal = vertex.Normal;
	tmpVertex.Tangent = vertex.TangentU;
	tmpVertex.TexCoords = vertex.TexC;
	meshData.vertices.push_back(tmpVertex);

	// Cache the index of center vertex.
	uint32 centerIndex = (uint32)meshData.vertices.size() - 1;

	for (uint32 i = 0; i < sliceCount; ++i)
	{
		meshData.indices.push_back(centerIndex);
		meshData.indices.push_back(baseIndex + i);
		meshData.indices.push_back(baseIndex + i + 1);
	}
}

Mesh CreateGrid(float width, float depth, uint32 m, uint32 n)
{
	Mesh meshData;

	uint32 vertexCount = m * n;
	uint32 faceCount = (m - 1) * (n - 1) * 2;

	//
	// Create the vertices.
	//

	float halfWidth = 0.5f * width;
	float halfDepth = 0.5f * depth;

	float dx = width / (n - 1);
	float dz = depth / (m - 1);

	float du = 1.0f / (n - 1);
	float dv = 1.0f / (m - 1);

	meshData.vertices.resize(vertexCount);
	for (uint32 i = 0; i < m; ++i)
	{
		float z = halfDepth - i * dz;
		for (uint32 j = 0; j < n; ++j)
		{
			float x = -halfWidth + j * dx;

			meshData.vertices[i * n + j].position = XMFLOAT3(x, 0.0f, z);
			meshData.vertices[i * n + j].normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
			meshData.vertices[i * n + j].Tangent = XMFLOAT3(1.0f, 0.0f, 0.0f);

			// Stretch texture over grid.
			meshData.vertices[i * n + j].TexCoords.x = j * du;
			meshData.vertices[i * n + j].TexCoords.y = i * dv;
		}
	}

	//
	// Create the indices.
	//

	meshData.indices.resize(faceCount * 3); // 3 indices per face

	// Iterate over each quad and compute indices.
	uint32 k = 0;
	for (uint32 i = 0; i < m - 1; ++i)
	{
		for (uint32 j = 0; j < n - 1; ++j)
		{
			meshData.indices[k] = i * n + j;
			meshData.indices[k + 1] = i * n + j + 1;
			meshData.indices[k + 2] = (i + 1) * n + j;

			meshData.indices[k + 3] = (i + 1) * n + j;
			meshData.indices[k + 4] = i * n + j + 1;
			meshData.indices[k + 5] = (i + 1) * n + j + 1;

			k += 6; // next quad
		}
	}

	int size = meshData.vertices.size();
	for (int i = 0; i < size; ++i) {

		meshData.center.x += meshData.vertices[i].position.x;
		meshData.center.y += meshData.vertices[i].position.x;
		meshData.center.z += meshData.vertices[i].position.x;
	}
	meshData.center.x /= size;
	meshData.center.y /= size;
	meshData.center.z /= size;

	return meshData;
}

Mesh CreateQuad(float x, float y, float w, float h, float depth)
{
	Mesh meshData;

	meshData.vertices.resize(4);
	meshData.indices.resize(6);

	// Position coordinates specified in NDC space.
	TmpVertex vertex(
		x, y - h, depth,
		0.0f, 0.0f, -1.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f);
	Vertex tmpVertex;
	tmpVertex.position = vertex.Position;
	tmpVertex.normal = vertex.Normal;
	tmpVertex.Tangent = vertex.TangentU;
	tmpVertex.TexCoords = vertex.TexC;
	meshData.vertices[0] = tmpVertex;

	TmpVertex vertex2(
		x, y, depth,
		0.0f, 0.0f, -1.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 0.0f);
	tmpVertex.position = vertex2.Position;
	tmpVertex.normal = vertex2.Normal;
	tmpVertex.Tangent = vertex2.TangentU;
	tmpVertex.TexCoords = vertex2.TexC;
	meshData.vertices[1] = tmpVertex;

	TmpVertex vertex3(
		x + w, y, depth,
		0.0f, 0.0f, -1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f);
	tmpVertex.position = vertex3.Position;
	tmpVertex.normal = vertex3.Normal;
	tmpVertex.Tangent = vertex3.TangentU;
	tmpVertex.TexCoords = vertex3.TexC;
	meshData.vertices[2] =  tmpVertex;

	TmpVertex vertex4(
		x + w, y - h, depth,
		0.0f, 0.0f, -1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 1.0f);
	tmpVertex.position = vertex4.Position;
	tmpVertex.normal = vertex4.Normal;
	tmpVertex.Tangent = vertex4.TangentU;
	tmpVertex.TexCoords = vertex4.TexC;
	meshData.vertices[3] = tmpVertex;

	meshData.indices[0] = 0;
	meshData.indices[1] = 1;
	meshData.indices[2] = 2;

	meshData.indices[3] = 0;
	meshData.indices[4] = 2;
	meshData.indices[5] = 3;

	int size = meshData.vertices.size();
	for (int i = 0; i < size; ++i) {

		meshData.center.x += meshData.vertices[i].position.x;
		meshData.center.y += meshData.vertices[i].position.x;
		meshData.center.z += meshData.vertices[i].position.x;
	}
	meshData.center.x /= size;
	meshData.center.y /= size;
	meshData.center.z /= size;

	return meshData;
}