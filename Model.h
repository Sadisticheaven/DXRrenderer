#pragma once
#include "HLSLCompat.h"
#include <vector>


struct Texture
{
	//��������Ψһ��
	std::string Name;
	//����·��
	std::wstring Filename;
	//����ͼ�����ݵ�������Դ 
	Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
	//������Դ����Ϊ�ϴ��ѣ���ͼ�����ݸ��Ƶ�Ĭ�϶�   
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