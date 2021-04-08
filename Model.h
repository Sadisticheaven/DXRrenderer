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
};

typedef struct Model
{
	std::vector<Mesh> meshes;
};