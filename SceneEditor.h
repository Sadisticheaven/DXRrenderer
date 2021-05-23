//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once

#include "DXSample.h"
#include <dxcapi.h>
#include <vector>
//#include "HLSLCompat.h"
#include "nv_helpers_dx12/TopLevelASGenerator.h"
#include "nv_helpers_dx12/ShaderBindingTableGenerator.h"
//#include <DirectXMath.h>
//#include "d3dx12.h"
//#include <dxgi1_4.h>
#include <Camera.h>
#include "Model.h"
#include <memory>
#include <string.h>
#include <unordered_map>
using namespace DirectX;

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
// An example of this can be found in the class method: OnDestroy().
using Microsoft::WRL::ComPtr;


class SceneEditor : public DXSample
{
public:
	SceneEditor(UINT width, UINT height, std::wstring name);

	virtual void OnInit();
	virtual void OnUpdate();
	virtual void OnRender();
	virtual void OnDestroy();
	virtual void OnResize(HWND hWnd, int width, int height);
	virtual void OnMouseDown(WPARAM btnState, int x, int y);
	virtual void OnMouseUp(WPARAM btnState, int x, int y);
	virtual void OnMouseMove(WPARAM btnState, int x, int y);
	virtual void OnKeyDown(UINT8 key);
	virtual void OnResetSpp() { needRefreshScreen = true; }
	virtual void StartImgui();

private:
	static const UINT FrameCount = 2;
	//DXGI_FORMAT m_outPutFormat = DXGI_FORMAT_R8G8B8A8_UNORM ;
	DXGI_FORMAT m_outPutFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	char* m_MaterialType[MaterialType::Count + 1] = { FOREACH_MATERIAL(GENERATE_STRING) };

	// Pipeline objects.
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;
	ComPtr<IDXGISwapChain3> m_swapChain;
	//ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12Device6> m_device;
	ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
	ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	ComPtr<ID3D12PipelineState> m_pipelineState;
	//ComPtr<ID3D12GraphicsCommandList> m_commandList;
	ComPtr<ID3D12GraphicsCommandList4> m_commandList;
	UINT m_rtvDescriptorSize;

	// App resources.
	struct ObjResource {
		ComPtr<ID3D12Resource> vertexBuffer;
		int vertexCount;
		ComPtr<ID3D12Resource> indexBuffer;
		int indexCount;
		PrimitiveMaterialBuffer materialAttributes;
		ComPtr<ID3D12Resource> MaterialBuffer;
		int texOfObj;
		std::string str_objName;
		std::wstring ws_hitGroupName;
		std::wstring ws_shadowHitGroupName;
		ComPtr<ID3D12Resource> bottomLevelAS;
		XMMATRIX originTransform;
		XMFLOAT3 center;
		float surfaceArea;
	};
	std::vector<ObjResource> m_objects;
	std::unordered_map<std::string, int> m_idxOfObj;

	int m_MaterialBufferSize = SizeOfIn256(PrimitiveMaterialBuffer);
	void CreateMaterialBufferAndSetAttributes(int bufferIndex, MaterialType::Type type, XMFLOAT4 Kd, float emitIntensity = 0.f,
		float smoothness = 0.0f, float index_of_refraction = 1.0f, float  reflectivity = 0.3f, UINT hasDiffuseTexture = false);
	void CreateMaterialBufferAndSetAttributes(PrimitiveMaterialBuffer& desc, int bufferIndex = 0);
	//void CreateLightBuffer(Light& desc);


	// Synchronization objects.
	UINT m_frameIndex;
	HANDLE m_fenceEvent;
	ComPtr<ID3D12Fence> m_fence;
	UINT64 m_fenceValue;
	SceneConstants matrices;
	bool needRefreshScreen;

	//bool m_raster = true;

	void LoadPipeline();
	void LoadAssets();
	void PopulateCommandList();
	void PopulateRaytracingCmdList();
	void WaitForPreviousFrame();
	void AllocateUploadGeometryBuffer(Model& model, std::string objname);
	void CheckRaytracingSupport();

	// #DXR-AccelerationStructure
	//<----------***************************************************
	struct AccelerationStructureBuffers
	{
		ComPtr<ID3D12Resource> pScratch;      // Scratch memory for AS builder
		ComPtr<ID3D12Resource> pResult;       // Where the AS is
		ComPtr<ID3D12Resource> pInstanceDesc; // Hold the matrices of the instances
	};
	AccelerationStructureBuffers m_topLevelASBuffers;

	nv_helpers_dx12::TopLevelASGenerator m_topLevelASGenerator;
	std::vector<std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>> m_instances;

	/// Create the acceleration structure of an instance
	///
	/// \param     vVertexBuffers : pair of buffer and vertex count
	/// \return    AccelerationStructureBuffers for TLAS
	AccelerationStructureBuffers
		CreateBottomLevelAS(std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vVertexBuffers);
	AccelerationStructureBuffers
		CreateBottomLevelAS(std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vVertexBuffers, std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vIndexBuffers);

	/// Create the main acceleration structure that holds
	/// all instances of the scene
	/// \param     instances : pair of BLAS and transform
	void CreateTopLevelAS(
		const std::vector<std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>>& instances);

	/// Create all acceleration structures, bottom and top
	void CreateAccelerationStructures();

	//***************************************************--------->

	// #DXR--RootSignature??Pipeline
	//<----------***************************************************
	ComPtr<ID3D12RootSignature> CreateRayGenSignature();
	ComPtr<ID3D12RootSignature> CreateMissSignature();
	ComPtr<ID3D12RootSignature> CreateHitSignature();

	void CreateRaytracingPipeline();

	ComPtr<IDxcBlob> m_rayGenLibrary;//DXIL libraries of the shaders
	ComPtr<IDxcBlob> m_hitLibrary;
	ComPtr<IDxcBlob> m_missLibrary;

	ComPtr<ID3D12RootSignature> m_rayGenSignature;
	ComPtr<ID3D12RootSignature> m_hitSignature;
	ComPtr<ID3D12RootSignature> m_missSignature;

	// Ray tracing pipeline state
	ComPtr<ID3D12StateObject> m_rtStateObject;
	// Ray tracing pipeline state properties, retaining the shader identifiers
	// to use in the Shader Binding Table
	ComPtr<ID3D12StateObjectProperties> m_rtStateObjectProps;
	//***************************************************--------->

	// #DXR--output??Shader resource
	//<----------***************************************************
	void CreateRaytracingOutputBuffer();
	void CreateShaderResourceHeap();
	ComPtr<ID3D12Resource> m_outputResource;
	ComPtr<ID3D12DescriptorHeap> m_srvUavHeap;
	//***************************************************--------->

	// #DXR--SBT
	//<----------***************************************************
	void CreateShaderBindingTable();
	nv_helpers_dx12::ShaderBindingTableGenerator m_sbtHelper;
	ComPtr<ID3D12Resource> m_sbtStorage;
	//***************************************************--------->

	// #DXR Extra: Perspective Camera
	void CreateSceneParametersBuffer();
	void UpdateSceneParametersBuffer();
	ComPtr<ID3D12Resource> m_sceneParameterBuffer;
	ComPtr<ID3D12DescriptorHeap> m_constHeap;
	uint32_t m_sceneParameterBufferSize = 0;
	Camera m_camera;
	POINT m_lastMousePos;

	//update value by imgui
	void UpadteMaterialParameter(int bufferIndex);

	// Texture 
	std::vector<Texture> m_textures;
	std::vector<std::string> m_texNames;
	D3D12_CPU_DESCRIPTOR_HANDLE m_texSrvHeapStart; // store start point of textures srv 
	void UpdateTexOfObj(int objIdx);

	void UpdateInstances();

	//light
	Light lightDesc;
	struct AreaLightResource {
		ComPtr<ID3D12Resource> vtxBuffer;
		ComPtr<ID3D12Resource> idxBuffer;
	};
	AreaLightResource m_areaRes;
	std::vector<Light> lightsInScene;
	ComPtr<ID3D12Resource> m_lightsBuffer;
	std::vector<char*> lightIdxChar;

	char* m_LightType[LightType::Count + 1] = { FOREACH_LIGHT(GENERATE_STRING) };
	//std::vector<LightSource> m_lights;
	void UpdateLight();
	void CreateAreaLightVerticesBuffer();
	void UpdateAreaLightVerticesBuffer(UINT index);
	void AllocateUploadLightBuffer();

};
