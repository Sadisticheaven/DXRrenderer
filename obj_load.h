#pragma once
#include <optional>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <math.h>
#include "HLSLCompat.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Model.h"
using namespace DirectX;
typedef UINT32 Index;


namespace math
{
	// XMFLOAT3 Cross Product
	XMFLOAT3 CrossV3(const XMFLOAT3 a, const XMFLOAT3 b)
	{
		return XMFLOAT3(a.y * b.z - a.z * b.y,
			a.z * b.x - a.x * b.z,
			a.x * b.y - a.y * b.x);
	}

	// XMFLOAT3 Magnitude Calculation
	float MagnitudeV3(const XMFLOAT3 in)
	{
		return (sqrtf(powf(in.x, 2) + powf(in.y, 2) + powf(in.z, 2)));
	}

	// XMFLOAT3 DotProduct
	float DotV3(const XMFLOAT3 a, const XMFLOAT3 b)
	{
		return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
	}

	// Angle between 2 XMFLOAT3 Objects
	float AngleBetweenV3(const XMFLOAT3 a, const XMFLOAT3 b)
	{
		float angle = DotV3(a, b);
		angle /= (MagnitudeV3(a) * MagnitudeV3(b));
		return angle = acosf(angle);
	}

	// Projection Calculation of a onto b
	XMFLOAT3 ProjV3(const XMFLOAT3 a, const XMFLOAT3 b)
	{
		float mv = MagnitudeV3(b);
		XMFLOAT3 bn = XMFLOAT3(b.x / mv, b.y / mv, b.z / mv);
		float dot = DotV3(a, bn);
		bn = XMFLOAT3(bn.x * dot, bn.y * dot, bn.z * dot);
		return bn;
	}
}

namespace algorithm
{
	// XMFLOAT3 Multiplication Opertor Overload
	XMFLOAT3 operator*(const float& left, const XMFLOAT3& right)
	{
		return XMFLOAT3(right.x * left, right.y * left, right.z * left);
	}

	XMFLOAT3 operator-(const XMFLOAT3& left, const XMFLOAT3& right)
	{
		return XMFLOAT3(left.x - right.x, left.y - right.y, left.z - right.z);
	}

	XMFLOAT3 operator+(const XMFLOAT3& left, const XMFLOAT3& right)
	{
		return XMFLOAT3(left.x + right.x, left.y + right.y, left.z + right.z);
	}

	void operator+=(XMFLOAT3& left, const XMFLOAT3& right)
	{
		left.x += right.x;
		left.y += right.y;
		left.z += right.z;
	}

	// A test to see if P1 is on the same side as P2 of a line segment ab
	bool SameSide(XMFLOAT3 p1, XMFLOAT3 p2, XMFLOAT3 a, XMFLOAT3 b)
	{
		XMFLOAT3 cp1 = math::CrossV3(b - a, p1 - a);
		XMFLOAT3 cp2 = math::CrossV3(b - a, p2 - a);

		if (math::DotV3(cp1, cp2) >= 0)
			return true;
		else
			return false;
	}

	// Generate a cross produect normal for a triangle
	XMFLOAT3 GenTriNormal(XMFLOAT3 t1, XMFLOAT3 t2, XMFLOAT3 t3)
	{
		XMFLOAT3 u = t2 - t1;
		XMFLOAT3 v = t3 - t1;

		XMFLOAT3 normal = math::CrossV3(u, v);

		/*XMVECTOR normal = XMVector3Normalize(XMVector3Cross(XMLoadFloat3(&u), XMLoadFloat3(&v)));*/

		return normal;
	}

	// Check to see if a XMFLOAT3 Point is within a 3 XMFLOAT3 Triangle
	bool inTriangle(XMFLOAT3 point, XMFLOAT3 tri1, XMFLOAT3 tri2, XMFLOAT3 tri3)
	{
		// Test to see if it is within an infinite prism that the triangle outlines.
		bool within_tri_prisim = SameSide(point, tri1, tri2, tri3) && SameSide(point, tri2, tri1, tri3)
			&& SameSide(point, tri3, tri1, tri2);

		// If it isn't it will never be on the triangle
		if (!within_tri_prisim)
			return false;

		// Calulate Triangle's Normal
		XMFLOAT3 n = GenTriNormal(tri1, tri2, tri3);

		// Project the point onto this normal
		XMFLOAT3 proj = math::ProjV3(point, n);

		// If the distance from the triangle to the point is 0
		//	it lies on the triangle
		if (math::MagnitudeV3(proj) == 0)
			return true;
		else
			return false;
	}

	// Split a String into a string array at a given token
	inline void split(const std::string& in,
		std::vector<std::string>& out,
		std::string token)
	{
		out.clear();

		std::string temp;

		for (int i = 0; i < int(in.size()); i++)
		{
			std::string test = in.substr(i, token.size());

			if (test == token)
			{
				if (!temp.empty())
				{
					out.push_back(temp);
					temp.clear();
					i += (int)token.size() - 1;
				}
				else
				{
					out.push_back("");
				}
			}
			else if (i + token.size() >= in.size())
			{
				temp += in.substr(i, token.size());
				out.push_back(temp);
				break;
			}
			else
			{
				temp += in[i];
			}
		}
	}

	// Get tail of string after first token and possibly following spaces
	inline std::string tail(const std::string& in)
	{
		size_t token_start = in.find_first_not_of(" \t");
		size_t space_start = in.find_first_of(" \t", token_start);
		size_t tail_start = in.find_first_not_of(" \t", space_start);
		size_t tail_end = in.find_last_not_of(" \t");
		if (tail_start != std::string::npos && tail_end != std::string::npos)
		{
			return in.substr(tail_start, tail_end - tail_start + 1);
		}
		else if (tail_start != std::string::npos)
		{
			return in.substr(tail_start);
		}
		return "";
	}

	// Get first token of string
	inline std::string firstToken(const std::string& in)
	{
		if (!in.empty())
		{
			size_t token_start = in.find_first_not_of(" \t");
			size_t token_end = in.find_first_of(" \t", token_start);
			if (token_start != std::string::npos && token_end != std::string::npos)
			{
				return in.substr(token_start, token_end - token_start);
			}
			else if (token_start != std::string::npos)
			{
				return in.substr(token_start);
			}
		}
		return "";
	}

	// Get element at given index position
	template <class T>
	inline const T& getElement(const std::vector<T>& elements, std::string& index)
	{
		int idx = std::stoi(index);
		if (idx < 0)
			idx = int(elements.size()) + idx;
		else
			idx--;
		return elements[idx];
	}
}

using namespace algorithm;
bool LoadObjFile(std::string Path, std::vector<Vertex> &vertices, std::vector<Index> &indices)
{
	// If the file is not an .obj file return false
	if (Path.substr(Path.size() - 4, 4) != ".obj")
		return false;

	indices.clear();
	vertices.clear();

	std::ifstream file(Path);

	if (!file.is_open())
		return false;

	std::vector<XMFLOAT3> Positions;
	std::vector<XMFLOAT2> TCoords;
	std::vector<XMFLOAT3> Normals;

	std::vector<Vertex> Vertices;
	//std::vector<unsigned int> Indices;

	std::vector<std::string> MeshMatNames;

	bool listening = false;
	std::string meshname;



	std::string curline;
	int vnorCount = 0;
	while (std::getline(file, curline))
	{

		//// Generate a Mesh Object or Prepare for an object to be created
		//if (algorithm::firstToken(curline) == "o" || algorithm::firstToken(curline) == "g" || curline[0] == 'g')
		//{
		//	if (!listening)
		//	{
		//		listening = true;

		//		if (algorithm::firstToken(curline) == "o" || algorithm::firstToken(curline) == "g")
		//		{
		//			meshname = algorithm::tail(curline);
		//		}
		//		else
		//		{
		//			meshname = "unnamed";
		//		}
		//	}
		//	else
		//	{
		//		// Generate the mesh to put into the array

		//		if (!Indices.empty() && !Vertices.empty())
		//		{
		//			// Create Mesh
		//			tempMesh = Mesh(Vertices, Indices);
		//			tempMesh.MeshName = meshname;

		//			// Insert Mesh
		//			LoadedMeshes.push_back(tempMesh);

		//			// Cleanup
		//			Vertices.clear();
		//			Indices.clear();
		//			meshname.clear();

		//			meshname = algorithm::tail(curline);
		//		}
		//		else
		//		{
		//			if (algorithm::firstToken(curline) == "o" || algorithm::firstToken(curline) == "g")
		//			{
		//				meshname = algorithm::tail(curline);
		//			}
		//			else
		//			{
		//				meshname = "unnamed";
		//			}
		//		}
		//	}
		//}
		// Generate a Vertex Position
		if (algorithm::firstToken(curline) == "v")
		{
			std::vector<std::string> spos;
			XMFLOAT3 vpos;
			algorithm::split(algorithm::tail(curline), spos, " ");

			vpos.x = std::stof(spos[0]);
			vpos.y = std::stof(spos[1]);
			vpos.z = std::stof(spos[2]);

			Positions.push_back(vpos);
			Normals.emplace_back();
		}
		// Generate a Vertex Texture Coordinate
		if (algorithm::firstToken(curline) == "vt")
		{
			std::vector<std::string> stex;
			XMFLOAT2 vtex;
			algorithm::split(algorithm::tail(curline), stex, " ");

			vtex.x = std::stof(stex[0]);
			vtex.y = std::stof(stex[1]);

			TCoords.push_back(vtex);
		}
		// Generate a Vertex Normal;
		if (algorithm::firstToken(curline) == "vn")
		{
			std::vector<std::string> snor;
			XMFLOAT3 vnor;
			algorithm::split(algorithm::tail(curline), snor, " ");

			vnor.x = std::stof(snor[0]);
			vnor.y = std::stof(snor[1]);
			vnor.z = std::stof(snor[2]);
			
			//Normals.push_back(vnor);
			Normals[vnorCount++] += vnor;
		}
		// Generate a Face (vertices & indices)
		if (algorithm::firstToken(curline) == "f")
		{
			std::vector<std::string> snor;
			//XMFLOAT3 vnor;
			algorithm::split(algorithm::tail(curline), snor, " ");
			std::vector<int> index;
			index.emplace_back(std::stof(snor[0]) - 1);
			index.emplace_back(std::stof(snor[1]) - 1);
			index.emplace_back(std::stof(snor[2]) - 1);
			indices.emplace_back(index[0]);
			indices.emplace_back(index[1]);
			indices.emplace_back(index[2]);
		
			XMFLOAT3 normal = GenTriNormal(Positions[index[0]], Positions[index[1]], Positions[index[2]]);
			
			for (auto no : index) {
				XMFLOAT3 preNormal = Normals[no];
				XMFLOAT3 newNormal = normal + preNormal;
				DirectX::XMStoreFloat3(&Normals[no], XMVector3Normalize(XMLoadFloat3(&newNormal)));
			}
			//Normals.push_back(vnor);
		}
		// Get Mesh Material Name
		if (algorithm::firstToken(curline) == "usemtl")
		{
		}
		// Load Materials
		if (algorithm::firstToken(curline) == "mtllib")
		{

		}
	}

	file.close();

	/*for (auto vertex : Positions) {
		vertices.push_back({ vertex ,XMFLOAT4(0.0f,0.0f,0.0f,0.0f)});
	}*/
	for (int i = 0; i < Positions.size(); ++i) {
		vertices.push_back({ Positions[i], Normals[i] });
	}
	// Set Materials for each Mesh
	return true;
}

Mesh processMesh(aiMesh* mesh, const aiScene* scene)
{
	// Ҫ��д������
	std::vector<Vertex> vertices;
	std::vector<Index> indices;
	//std::vector<Texture> textures;

	//����ļ���������
	if (mesh->mNormals != nullptr) {
		// ����ÿ������Ķ���
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex;
			// λ��		
			vertex.position.x = mesh->mVertices[i].x;
			vertex.position.y = mesh->mVertices[i].y;
			vertex.position.z = mesh->mVertices[i].z;
			// ����
			vertex.normal.x = mesh->mNormals[i].x;
			vertex.normal.y = mesh->mNormals[i].y;
			vertex.normal.z = mesh->mNormals[i].z;
			//// ��������
			//if (mesh->mTextureCoords[0]) // �����Ƿ�����������ꣿ
			//{
			//	glm::vec2 vec;
			//	// �������ɰ���8����ͬ���������ꡣ ��ˣ����Ǽ������ǲ���ʹ�ö�����Ծ��ж�����������ģ�ͣ�����������ǲ��õ�һ�����ϣ�0����
			//	vec.x = mesh->mTextureCoords[0][i].x;
			//	vec.y = mesh->mTextureCoords[0][i].y;
			//	vertex.TexCoords = vec;
			//}
			//else
			//	vertex.TexCoords = glm::vec2(0.0f, 0.0f);
			//// u����
			//vector.x = mesh->mTangents[i].x;
			//vector.y = mesh->mTangents[i].y;
			//vector.z = mesh->mTangents[i].z;
			//vertex.Tangent = vector;
			//// v����
			//vector.x = mesh->mBitangents[i].x;
			//vector.y = mesh->mBitangents[i].y;
			//vector.z = mesh->mBitangents[i].z;
			//vertex.Bitangent = vector;
			vertices.push_back(vertex);
		}
		//���ڱ���ÿ�������棨һ������һ�������ε����񣩲�������Ӧ�Ķ���������
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			// ����������������������Ǵ洢������������
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}
	}
	else {
		for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
		{
			Vertex vertex;
			// λ��		
			vertex.position.x = mesh->mVertices[i].x;
			vertex.position.y = mesh->mVertices[i].y;
			vertex.position.z = mesh->mVertices[i].z;
			vertex.normal = XMFLOAT3(0.f, 0.f, 0.f);
			vertices.push_back(vertex);
		}
		//����ÿ�������棨һ������һ�������ε����񣩲�������Ӧ�Ķ���������
		//ͬʱ�������϶���ķ���
		for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
		{
			aiFace face = mesh->mFaces[i];
			// ����������������������Ǵ洢������������
			std::vector<Index> tmpIndex;
			for (unsigned int j = 0; j < face.mNumIndices; ++j) {
				indices.push_back(face.mIndices[j]);	
				tmpIndex.push_back(face.mIndices[j]);
			}		
			XMFLOAT3 normal = GenTriNormal(vertices[tmpIndex[0]].position, vertices[tmpIndex[1]].position, vertices[tmpIndex[2]].position);
			for (unsigned int j = 0; j < tmpIndex.size(); ++j) {
				XMFLOAT3 preNormal = vertices[tmpIndex[j]].normal;
				XMFLOAT3 newNormal = normal + preNormal;
				DirectX::XMStoreFloat3(&vertices[tmpIndex[j]].normal, XMVector3Normalize(XMLoadFloat3(&newNormal)));
			}
		}	
	}
	
	//// �ӹ�����
	//aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
	//// ���Ǽ�����ɫ���еĲ���������Լ���� ÿ������������Ӧ����Ϊ'texture_diffuseN'������N�Ǵ�1��MAX_SAMPLER_NUMBER�����кš�
	////ͬ�����������������������ܽ᣺
	//// diffuse: texture_diffuseN
	//// specular: texture_specularN
	//// normal: texture_normalN

	//// 1. ��������ͼ
	//vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
	//textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
	//// 2. �߹���ͼ
	//vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
	//textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
	//// 3.������ͼ
	//std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
	//textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
	//// 4. �߶���ͼ
	//std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
	//textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

	//// ���ش���ȡ���������ݴ������������
	//return Mesh(vertices, indices, textures);
	Mesh tmpMesh;
	tmpMesh.vertices = vertices;
	tmpMesh.indices = indices;
	return tmpMesh;
}
// �Եݹ鷽ʽ����ڵ㡣 ����λ�ڽڵ㴦��ÿ���������񣬲������ӽڵ㣨����У����ظ��˹��̡�
void processNode(aiNode* node, const aiScene* scene, std::vector<Mesh> &meshes)
{
	// ����λ�ڵ�ǰ�ڵ��ÿ������
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		// �ڵ��������������������������е�ʵ�ʶ���
		// ���������������ݣ��ڵ�ֻ��Ϊ������֯�ı��涫������ڵ�֮��Ĺ�ϵ����
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.push_back(processMesh(mesh, scene));
	}
	// �����Ǵ�����������������еĻ��������ǻ�ݹ鴦��ÿ���ӽڵ�
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		processNode(node->mChildren[i], scene, meshes);
	}
}

bool LoadModelFile(std::string Path, Model &model) {
	// ͨ��ASSIMP���ļ�
	Assimp::Importer importer;
	Model tmpModel;
	const aiScene* scene = importer.ReadFile(Path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
	// ������
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // �������0
	{
		AllocConsole();
		printf("����::ASSIMP:: %s", importer.GetErrorString());
		FreeConsole();
		return false;
	}
	// �����ļ�·����Ŀ¼·��
	std::string directory = Path.substr(0, Path.find_last_of('/'));

	// �Եݹ鷽ʽ����ASSIMP�ĸ��ڵ�
	processNode(scene->mRootNode, scene, tmpModel.meshes);
	model = tmpModel;
	return true;
}


