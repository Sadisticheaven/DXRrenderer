#pragma once
#include "HLSLCompat.h"
#include <vector>

struct Texture
{
	//材质名（唯一）
	std::string Name;
	//材质路径
	std::wstring Filename;
	//载有图像数据的纹理资源 
	Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
	//纹理资源，作为上传堆，将图像数据复制到默认堆   
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap = nullptr;
};

typedef struct Mesh
{
	std::vector<Vertex> vertices;
	std::vector<Index> indices;
	XMFLOAT3 center;
};

typedef struct Model
{
	std::vector<Mesh> meshes;
};

using uint16 = std::uint16_t;
using uint32 = std::uint32_t;

struct TmpVertex {
	TmpVertex() {}
	TmpVertex(
		const DirectX::XMFLOAT3& p,
		const DirectX::XMFLOAT3& n,
		const DirectX::XMFLOAT3& t,
		const DirectX::XMFLOAT2& uv) :
		Position(p),
		Normal(n),
		TangentU(t),
		TexC(uv) {}
	TmpVertex(
		float px, float py, float pz,
		float nx, float ny, float nz,
		float tx, float ty, float tz,
		float u, float v) :
		Position(px, py, pz),
		Normal(nx, ny, nz),
		TangentU(tx, ty, tz),
		TexC(u, v) {}

	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT3 TangentU;
	DirectX::XMFLOAT2 TexC;
};
	///<summary>
	/// Creates a box centered at the origin with the given dimensions, where each
	/// face has m rows and n columns of vertices.
	///</summary>
	Mesh CreateBox(float width, float height, float depth, uint32 numSubdivisions);

	///<summary>
	/// Creates a sphere centered at the origin with the given radius.  The
	/// slices and stacks parameters control the degree of tessellation.
	///</summary>
	Mesh CreateSphere(float radius, uint32 sliceCount, uint32 stackCount);

	///<summary>
	/// Creates a geosphere centered at the origin with the given radius.  The
	/// depth controls the level of tessellation.
	///</summary>
	Mesh CreateGeosphere(float radius, uint32 numSubdivisions);

	///<summary>
	/// Creates a cylinder parallel to the y-axis, and centered about the origin.  
	/// The bottom and top radius can vary to form various cone shapes rather than true
	// cylinders.  The slices and stacks parameters control the degree of tessellation.
	///</summary>
	Mesh CreateCylinder(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount);

	///<summary>
	/// Creates an mxn grid in the xz-plane with m rows and n columns, centered
	/// at the origin with the specified width and depth.
	///</summary>
	Mesh CreateGrid(float width, float depth, uint32 m, uint32 n);

	///<summary>
	/// Creates a quad aligned with the screen.  This is useful for postprocessing and screen effects.
	///</summary>
	Mesh CreateQuad(float x, float y, float w, float h, float depth);

	void Subdivide(Mesh& meshData);
	Vertex MidPoint(const Vertex& v0, const Vertex& v1);
	void BuildCylinderTopCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount, Mesh& meshData);
	void BuildCylinderBottomCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount, Mesh& meshData);