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
#include "HLSLCompat.h"
#include "nv_helpers_dx12/TopLevelASGenerator.h"
#include "nv_helpers_dx12/ShaderBindingTableGenerator.h"
//#include <DirectXMath.h>
//#include "d3dx12.h"
//#include <dxgi1_4.h>
#include <Camera.h>

using namespace DirectX;

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
// An example of this can be found in the class method: OnDestroy().
using Microsoft::WRL::ComPtr;

namespace SceneObject {
	enum Enum {
		floor = 0,
		shortbox,
		tallbox,
		left,
		right,
		light,
		Count
	};
}



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

private:
	static const UINT FrameCount = 2;


	// Pipeline objects.
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;
	ComPtr<IDXGISwapChain3> m_swapChain;
	//ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12Device5> m_device;
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
	ComPtr<ID3D12Resource> m_vertexBuffer[SceneObject::Count];
	int m_vertexCount[SceneObject::Count];

	ComPtr<ID3D12Resource> m_indexBuffer[SceneObject::Count];
	int m_indexCount[SceneObject::Count];

	PrimitiveMaterialBuffer m_MaterialAttributes[SceneObject::Count];
	ComPtr<ID3D12Resource> m_MaterialBuffer[SceneObject::Count];
	int m_MaterialBufferSize = SizeOfIn256(PrimitiveMaterialBuffer);
	void CreateMaterialBufferAndSetAttributes(XMFLOAT4 Kd , XMFLOAT4 emit = { 0.0f,0.0f,0.0f,0.0f }, int bufferIndex = 0);
	void CreateMaterialBufferAndSetAttributes(PrimitiveMaterialBuffer &desc, int bufferIndex = 0);


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
	void SceneEditor::AllocateUploadGeometryBuffer(std::vector<Vertex> vertices, std::vector<Index> indices, int bufferIndex);
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

	std::vector<ComPtr<ID3D12Resource>> m_bottomLevelAS; // Storage for the bottom Level AS
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

	// #DXR--RootSignature¡¢Pipeline
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

	// #DXR--output¡¢Shader resource
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
	void CreateCameraBuffer();
	void UpdateCameraBuffer();
	ComPtr<ID3D12Resource> m_cameraBuffer;
	ComPtr<ID3D12DescriptorHeap> m_constHeap;
	uint32_t m_cameraBufferSize = 0;
	Camera m_camera;
	POINT m_lastMousePos;

};
