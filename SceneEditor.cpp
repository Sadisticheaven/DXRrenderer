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
#include "stdafx.h"
#include "SceneEditor.h"
#include <stdexcept>
#include "obj_load.h"
#include "DXRHelper.h"
#include "nv_helpers_dx12/BottomLevelASGenerator.h"
#include "nv_helpers_dx12/RaytracingPipelineGenerator.h"  
#include "nv_helpers_dx12/RootSignatureGenerator.h"


const std::wstring ws_raygenShaderName = L"RayGen";
const std::vector<std::wstring> ws_missShaderNames = { L"Miss" };
const std::vector<std::wstring> ws_closestHitShaderNames = { L"ClosestHit" };

const std::vector<std::wstring> ws_hitGroupNames = {
		L"Hit_floor",
		L"Hit_shortbox",
		L"Hit_tallbox",
		L"Hit_left",
		L"Hit_right",
		L"Hit_light",
		L"Hit_car",
		L"Hit_nanosuit"
};



SceneEditor::SceneEditor(UINT width, UINT height, std::wstring name) :
	DXSample(width, height, name),
	m_frameIndex(0),
	m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
	m_scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
	m_rtvDescriptorSize(0)
{
}

void SceneEditor::OnInit()
{
	LoadPipeline();

	m_imguiManager = ImguiManager(m_device);

	LoadAssets();
	// Check the raytracing capabilities of the device
	CheckRaytracingSupport();

	// Setup the acceleration structures (AS) for raytracing. When setting up
	// geometry, each bottom-level AS has its own transform matrix.
	CreateAccelerationStructures();

	// Command lists are created in the recording state, but there is
	// nothing to record yet. The main loop expects it to be closed, so
	// close it now.
	ThrowIfFailed(m_commandList->Close());

	// Create the raytracing pipeline, associating the shader code to symbol names
	// and to their root signatures, and defining the amount of memory carried by
	// rays (ray payload)
	CreateRaytracingPipeline(); // #DXR

	// Allocate the buffer storing the raytracing output, with the same dimensions
	// as the target image
	CreateRaytracingOutputBuffer(); // #DXR

	// Create the buffer containing the raytracing result (always output in a
	// UAV), and create the heap referencing the resources used by the raytracing,
	// such as the acceleration structure
	CreateShaderResourceHeap(); // #DXR

	// Create the shader binding table and indicating which shaders
	// are invoked for each instance in the  AS
	CreateShaderBindingTable();

	//imgui srv heap
	m_imguiManager.CreateSRVHeap4Imgui();

}


// Load the rendering pipeline dependencies.
void SceneEditor::LoadPipeline()
{
	UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();

			// Enable additional debug layers.
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif

	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

	if (m_useWarpDevice)
	{
		ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			warpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_device)
		));
	}
	else
	{
		ComPtr<IDXGIAdapter1> hardwareAdapter;
		GetHardwareAdapter(factory.Get(), &hardwareAdapter);

		ThrowIfFailed(D3D12CreateDevice(
			hardwareAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_device)
		));
	}

	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Width = m_width;
	swapChainDesc.Height = m_height;
	swapChainDesc.Format = m_outPutFormat;//DXGI_FORMAT_R16G16B16A16_FLOAT;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed(factory->CreateSwapChainForHwnd(
		m_commandQueue.Get(),		// Swap chain needs the queue so that it can force a flush on it.
		Win32Application::GetHwnd(),
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	));

	// This sample does not support fullscreen transitions.
	ThrowIfFailed(factory->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain.As(&m_swapChain));
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// Create descriptor heaps.
	{
		// Describe and create a render target view (RTV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// Create frame resources.
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

		// Create a RTV for each frame.
		for (UINT n = 0; n < FrameCount; n++)
		{
			ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
			m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);
		}
	}

	ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
}

void SceneEditor::AllocateUploadGeometryBuffer(std::vector<Vertex> vertices, std::vector<Index> indices, int bufferIndex)
{
	m_vertexCount[bufferIndex] = static_cast<UINT>(vertices.size());
	m_indexCount[bufferIndex] = static_cast<UINT>(indices.size());

	const UINT VertexBufferSize = static_cast<UINT>(vertices.size()) * sizeof(Vertex);
	{
		CD3DX12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC bufferResource = CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize);
		ThrowIfFailed(m_device->CreateCommittedResource(
			&heapProperty, D3D12_HEAP_FLAG_NONE, &bufferResource, //
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_vertexBuffer[bufferIndex])));

		// Copy the triangle data to the vertex buffer.
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(m_vertexBuffer[bufferIndex]->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, vertices.data(), VertexBufferSize);
		m_vertexBuffer[bufferIndex]->Unmap(0, nullptr);
	}
	// Initialize the vertex buffer view.
	//m_mengerVBView.BufferLocation = m_mengerVB->GetGPUVirtualAddress();
	//m_mengerVBView.StrideInBytes = sizeof(Vertex);
	//m_mengerVBView.SizeInBytes = mengerVBSize;
	const UINT IndexBufferSize = static_cast<UINT>(indices.size()) * sizeof(UINT);

	// Note: using upload heaps to transfer static data like vert buffers is not
	// recommended. Every time the GPU needs it, the upload heap will be
	// marshalled over. Please read up on Default Heap usage. An upload heap is
	// used here for code simplicity and because there are very few verts to
	// actually transfer.
	{
		CD3DX12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC bufferResource = CD3DX12_RESOURCE_DESC::Buffer(IndexBufferSize);
		ThrowIfFailed(m_device->CreateCommittedResource(
			&heapProperty, D3D12_HEAP_FLAG_NONE, &bufferResource, //
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_indexBuffer[bufferIndex])));

		// Copy the triangle data to the index buffer.
		UINT8* pIndexDataBegin;
		CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(m_indexBuffer[bufferIndex]->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
		memcpy(pIndexDataBegin, indices.data(), IndexBufferSize);
		m_indexBuffer[bufferIndex]->Unmap(0, nullptr);
	}
}

void SceneEditor::AllocateUploadGeometryBuffer(Model &model, int bufferIndex)
{
	std::vector<Vertex> vertices;
	std::vector<Index> indices;
	// store all meshes of model into one vertices nad indices
	for (int i = 0, offset = 0; i < model.meshes.size(); ++i) {
		vertices.insert(vertices.end(), model.meshes[i].vertices.begin(), model.meshes[i].vertices.end());
		// each indices of meshes are started from 0, so offset the subsequent indices
		for (int j = 0; j < model.meshes[i].indices.size(); ++j)
			model.meshes[i].indices[j] += offset;
		indices.insert(indices.end(), model.meshes[i].indices.begin(), model.meshes[i].indices.end());
		offset += model.meshes[i].vertices.size();
	}

	m_vertexCount[bufferIndex] = static_cast<UINT>(vertices.size());
	m_indexCount[bufferIndex] = static_cast<UINT>(indices.size());

	const UINT VertexBufferSize = static_cast<UINT>(vertices.size()) * sizeof(Vertex);
	{
		CD3DX12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC bufferResource = CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize);
		ThrowIfFailed(m_device->CreateCommittedResource(
			&heapProperty, D3D12_HEAP_FLAG_NONE, &bufferResource, //
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_vertexBuffer[bufferIndex])));

		// Copy the triangle data to the vertex buffer.
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(m_vertexBuffer[bufferIndex]->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, vertices.data(), VertexBufferSize);
		m_vertexBuffer[bufferIndex]->Unmap(0, nullptr);
	}
	// Initialize the vertex buffer view.
	//m_mengerVBView.BufferLocation = m_mengerVB->GetGPUVirtualAddress();
	//m_mengerVBView.StrideInBytes = sizeof(Vertex);
	//m_mengerVBView.SizeInBytes = mengerVBSize;
	const UINT IndexBufferSize = static_cast<UINT>(indices.size()) * sizeof(UINT);
	{
		CD3DX12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC bufferResource = CD3DX12_RESOURCE_DESC::Buffer(IndexBufferSize);
		ThrowIfFailed(m_device->CreateCommittedResource(
			&heapProperty, D3D12_HEAP_FLAG_NONE, &bufferResource, //
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_indexBuffer[bufferIndex])));

		// Copy the triangle data to the index buffer.
		UINT8* pIndexDataBegin;
		CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(m_indexBuffer[bufferIndex]->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
		memcpy(pIndexDataBegin, indices.data(), IndexBufferSize);
		m_indexBuffer[bufferIndex]->Unmap(0, nullptr);
	}
}

void SceneEditor::CreateMaterialBufferAndSetAttributes(int bufferIndex, MaterialType::Type type, XMFLOAT4 Kd, XMFLOAT4 emit, XMFLOAT4 Ks, float smoothness, float index_of_refraction) {
	m_MaterialBufferSize = SizeOfIn256(PrimitiveMaterialBuffer);
	//int k = sizeof(PrimitiveMaterialBuffer);
	m_MaterialBuffer[bufferIndex] = nv_helpers_dx12::CreateBuffer(
		m_device.Get(), m_MaterialBufferSize, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);
	m_MaterialAttributes[bufferIndex].Kd = Kd;
	m_MaterialAttributes[bufferIndex].Ks = Ks;
	m_MaterialAttributes[bufferIndex].index_of_refraction = index_of_refraction;
	m_MaterialAttributes[bufferIndex].emit = emit;
	m_MaterialAttributes[bufferIndex].type = type;
	m_MaterialAttributes[bufferIndex].smoothness = smoothness;
	uint8_t* pData;
	ThrowIfFailed(m_MaterialBuffer[bufferIndex]->Map(0, nullptr, (void**)&pData));
	memcpy(pData, &(m_MaterialAttributes[bufferIndex]), sizeof(PrimitiveMaterialBuffer));
	m_MaterialBuffer[bufferIndex]->Unmap(0, nullptr);
}

void SceneEditor::CreateMaterialBufferAndSetAttributes(PrimitiveMaterialBuffer& desc, int bufferIndex)
{
	//m_MaterialBufferSize = SizeOfIn256(PrimitiveMaterialBuffer);
	m_MaterialBuffer[bufferIndex] = nv_helpers_dx12::CreateBuffer(
		m_device.Get(), m_MaterialBufferSize, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);
	uint8_t* pData;
	ThrowIfFailed(m_MaterialBuffer[bufferIndex]->Map(0, nullptr, (void**)&pData));
	memcpy(pData, &desc, m_MaterialBufferSize);
	m_MaterialBuffer[bufferIndex]->Unmap(0, nullptr);
}

// Load the sample assets.

void SceneEditor::LoadAssets()
{
	// Create an empty root signature.
	{
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
	}

	needRefreshScreen = false;

	// Create the command list.
	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));
	{
		auto Float4Multi = [&](const float& f, const XMFLOAT4 vec3) {
			return XMFLOAT4(f * vec3.x, f * vec3.y, f * vec3.z, 0.0f);
		};
		XMFLOAT4 not_emit(0.0f, 0.0f, 0.0f, 0.0f);
		XMFLOAT4 red(0.63f, 0.065f, 0.05f, 0.0f);
		XMFLOAT4 green(0.14f, 0.45f, 0.091f, 0.0f);
		XMFLOAT4 white(0.725f, 0.71f, 0.68f, 0.0f);
		XMFLOAT4 light_kd(0.65f, 0.65f, 0.65f, 0.0f);
		XMFLOAT4 test(0.5, 0.8, 0.9, 1.0);
		XMFLOAT4 le1 = Float4Multi(8.0f, XMFLOAT4(0.747f + 0.058f, 0.747f + 0.258f, 0.747f, 0.0f));
		XMFLOAT4 le2 = Float4Multi(15.6f, XMFLOAT4(0.740f + 0.287f, 0.740f + 0.160f, 0.740f, 0.0f));
		XMFLOAT4 le3 = Float4Multi(18.4f, XMFLOAT4(0.737f + 0.642f, 0.737f + 0.159f, 0.737f, 0.0f));
		XMFLOAT4 light_emit(le1.x + le2.x + le3.x, le1.y + le2.y + le3.y, le1.z + le2.z + le3.z, 0.0f);
		XMFLOAT4 default_Ks(0.04f, 0.04f, 0.04f, 0.0f);
		CreateMaterialBufferAndSetAttributes(SceneObject::floor, MaterialType::Lambert, white, not_emit);
		CreateMaterialBufferAndSetAttributes(SceneObject::shortbox, MaterialType::Glass, white, not_emit, default_Ks, 2.1f, 1.02f);
		CreateMaterialBufferAndSetAttributes(SceneObject::tallbox, MaterialType::Mirror, white, not_emit, default_Ks, 1.0);
		CreateMaterialBufferAndSetAttributes(SceneObject::left, MaterialType::Lambert, red, not_emit);
		CreateMaterialBufferAndSetAttributes(SceneObject::right, MaterialType::Lambert, green, not_emit);
		CreateMaterialBufferAndSetAttributes(SceneObject::light, MaterialType::Lambert, light_kd, light_emit);
		CreateMaterialBufferAndSetAttributes(SceneObject::car, MaterialType::Glass,white, not_emit, default_Ks, 2.1f, 5.02f);
		CreateMaterialBufferAndSetAttributes(SceneObject::nanosuit, MaterialType::Glass,white, not_emit, default_Ks, 2.1f, 5.02f);
	}
	// Create the vertex and index buffer.
	{
		// Define the geometry for a triangle.
		Model model;
		LoadModelFile("./cornellbox/floor.obj", model);
		AllocateUploadGeometryBuffer(model, SceneObject::floor);

		LoadModelFile("./cornellbox/shortbox.obj", model);
		AllocateUploadGeometryBuffer(model, SceneObject::shortbox);

		LoadModelFile("./cornellbox/tallbox.obj", model);
		AllocateUploadGeometryBuffer(model, SceneObject::tallbox);

		LoadModelFile("./cornellbox/left.obj", model);
		AllocateUploadGeometryBuffer(model, SceneObject::left);

		LoadModelFile("./cornellbox/right.obj", model);
		AllocateUploadGeometryBuffer(model, SceneObject::right);

		LoadModelFile("./cornellbox/light.obj", model);
		AllocateUploadGeometryBuffer(model, SceneObject::light);

		LoadModelFile("./cornellbox/car.fbx", model);
		AllocateUploadGeometryBuffer(model, SceneObject::car);

		LoadModelFile("./cornellbox/nanosuit.obj", model);
		AllocateUploadGeometryBuffer(model, SceneObject::nanosuit);

		
	}

	{
		ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
		m_fenceValue = 1;

		// Create an event handle to use for frame synchronization.
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}

		WaitForPreviousFrame();
	}
}

// Update frame-based values.
void SceneEditor::OnUpdate()
{
	// #DXR Extra: Perspective Camera
	UpdateSceneParameterBuffer();
	UpadteMaterialParameter(m_imguiManager.m_currentObjeectItem);
}

// Render the scene.
void SceneEditor::OnRender()
{
	// Record all the commands we need to render the scene into the command list.
	PopulateCommandList();

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame.
	ThrowIfFailed(m_swapChain->Present(1, 0));

	WaitForPreviousFrame();
}

void SceneEditor::OnDestroy()
{
	// Ensure that the GPU is no longer referencing resources that are about to be
	// cleaned up by the destructor.
	WaitForPreviousFrame();

	CloseHandle(m_fenceEvent);
}

void SceneEditor::OnResize(HWND hWnd, int width, int height)
{
	WaitForPreviousFrame();

	m_width = width;
	m_height = height;

	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

	for (UINT i = 0; i < FrameCount; i++)
		if (m_renderTargets[i])
		{
			m_renderTargets[i].Reset();
			//m_renderTargets[i] = nullptr;
		}

	// Resize the swap chain.
	ThrowIfFailed(m_swapChain->ResizeBuffers(
		FrameCount,
		width, height,
		m_outPutFormat,//DXGI_FORMAT_R16G16B16A16_FLOAT,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

	// Create a RTV for each frame.
	for (UINT n = 0; n < FrameCount; n++)
	{
		ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
		m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
		rtvHandle.Offset(1, m_rtvDescriptorSize);
	}
	// Execute the resize commands.
	ThrowIfFailed(m_commandList->Close());
	ID3D12CommandList* cmdsLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	WaitForPreviousFrame();

	// For rasterazation.
	{
		// Update the viewport transform to cover the client area.
		/*m_viewport.TopLeftX = 0;
		m_viewport.TopLeftY = 0;
		m_viewport.Width = static_cast<float>(width);
		m_viewport.Height = static_cast<float>(height);
		m_viewport.MinDepth = 0.0f;
		m_viewport.MaxDepth = 1.0f;

		m_scissorRect.left = 0;
		m_scissorRect.top = 0;
		m_scissorRect.right = width;
		m_scissorRect.bottom = height;*/
	}

	// For raytracing.
	{
		m_outputResource.Reset();

		// Create the output resource. The dimensions and format should match the swap-chain.
		auto uavDesc = CD3DX12_RESOURCE_DESC::Tex2D(m_outPutFormat/*DXGI_FORMAT_R16G16B16A16_FLOAT*/, m_width, m_height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		ThrowIfFailed(m_device->CreateCommittedResource(
			&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &uavDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&m_outputResource)));

		D3D12_CPU_DESCRIPTOR_HANDLE uavDescriptorHandle = m_srvUavHeap->GetCPUDescriptorHandleForHeapStart();;

		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
		UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		m_device->CreateUnorderedAccessView(m_outputResource.Get(), nullptr, &UAVDesc, uavDescriptorHandle);
	}
}

void SceneEditor::PopulateCommandList()
{
	// Command list allocators can only be reset when the associated 
	// command lists have finished execution on the GPU; apps should use 
	// fences to determine GPU execution progress.
	ThrowIfFailed(m_commandAllocator->Reset());

	// However, when ExecuteCommandList() is called on a particular command 
	// list, that command list can then be reset at any time and must be before 
	// re-recording.
	ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));

	// Set necessary state for Rasterazation.
	//m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	//m_commandList->RSSetViewports(1, &m_viewport);
	//m_commandList->RSSetScissorRects(1, &m_scissorRect);
	// Indicate that the back buffer will be used as a render target.
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// Record commands 
	//Rasterazation.	
	/*
	if (m_raster)
	{
		const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
		m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
		m_commandList->DrawInstanced(6, 1, 0, 0);
	}*/
	// #DXR
	PopulateRaytracingCmdList();

	//define what will be displayed in imgui
	StartImgui();
	// Indicate that the back buffer will now be used to present.
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(m_commandList->Close());
}

void SceneEditor::PopulateRaytracingCmdList()
{
	// #DXR
		// Bind the descriptor heap giving access to the top-level acceleration
		// structure, as well as the raytracing output
	std::vector<ID3D12DescriptorHeap*> heaps = { m_srvUavHeap.Get() };
	m_commandList->SetDescriptorHeaps(static_cast<UINT>(heaps.size()),
		heaps.data());
	// On the last frame, the raytracing output was used as a copy source, to
	// copy its contents into the render target. Now we need to transition it to
	// a UAV so that the shaders can write in it.
	CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(
		m_outputResource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	m_commandList->ResourceBarrier(1, &transition);

	// Setup the raytracing task
	D3D12_DISPATCH_RAYS_DESC desc = {};
	// The layout of the SBT is as follows: ray generation shader, miss
	// shaders, hit groups. As described in the CreateShaderBindingTable method,
	// all SBT entries of a given type have the same size to allow a fixed stride.

	// The ray generation shaders are always at the beginning of the SBT. 
	uint32_t rayGenerationSectionSizeInBytes = m_sbtHelper.GetRayGenSectionSize();
	desc.RayGenerationShaderRecord.StartAddress = m_sbtStorage->GetGPUVirtualAddress();
	desc.RayGenerationShaderRecord.SizeInBytes = rayGenerationSectionSizeInBytes;

	// The miss shaders are in the second SBT section, right after the ray
	// generation shader. We have one miss shader for the camera rays and one
	// for the shadow rays, so this section has a size of 2*m_sbtEntrySize. We
	// also indicate the stride between the two miss shaders, which is the size
	// of a SBT entry
	uint32_t missSectionSizeInBytes = m_sbtHelper.GetMissSectionSize();
	desc.MissShaderTable.StartAddress =
		m_sbtStorage->GetGPUVirtualAddress() + rayGenerationSectionSizeInBytes;
	desc.MissShaderTable.SizeInBytes = missSectionSizeInBytes;
	desc.MissShaderTable.StrideInBytes = m_sbtHelper.GetMissEntrySize();

	// The hit groups section start after the miss shaders. In this sample we
	// have one 1 hit group for the triangle
	uint32_t hitGroupsSectionSize = m_sbtHelper.GetHitGroupSectionSize();
	desc.HitGroupTable.StartAddress = m_sbtStorage->GetGPUVirtualAddress() +
		rayGenerationSectionSizeInBytes +
		missSectionSizeInBytes;
	desc.HitGroupTable.SizeInBytes = hitGroupsSectionSize;
	desc.HitGroupTable.StrideInBytes = m_sbtHelper.GetHitGroupEntrySize();

	// Dimensions of the image to render, identical to a kernel launch dimension
	desc.Width = GetWidth();
	desc.Height = GetHeight();
	desc.Depth = 1;

	// Bind the raytracing pipeline
	m_commandList->SetPipelineState1(m_rtStateObject.Get());
	// Dispatch the rays and write to the raytracing output
	m_commandList->DispatchRays(&desc);

	// The raytracing output needs to be copied to the actual render target used for display.
	// For this, we need to transition the raytracing output from a	UAV to a copy source,
	// and the render target buffer to a copy destination.
	// We can then do the actual copy, before transitioning the render target
	// buffer into a render target, that will be then used to display the image
	transition = CD3DX12_RESOURCE_BARRIER::Transition(m_outputResource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_commandList->ResourceBarrier(1, &transition);
	transition = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
	m_commandList->ResourceBarrier(1, &transition);

	m_commandList->CopyResource(m_renderTargets[m_frameIndex].Get(), m_outputResource.Get());

	//transition = CD3DX12_RESOURCE_BARRIER::Transition(m_outputResource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	//m_commandList->ResourceBarrier(1, &transition);
	transition = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_commandList->ResourceBarrier(1, &transition);
}

void SceneEditor::WaitForPreviousFrame()
{
	// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
	// This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
	// sample illustrates how to use fences for efficient resource usage and to
	// maximize GPU utilization.

	// Signal and increment the fence value.
	const UINT64 fence = m_fenceValue;
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
	m_fenceValue++;

	// Wait until the previous frame is finished.
	if (m_fence->GetCompletedValue() < fence)
	{
		ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void SceneEditor::CheckRaytracingSupport()
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
	ThrowIfFailed(m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5,
		&options5, sizeof(options5)));
	if (options5.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
		throw std::runtime_error("Raytracing not supported on device");
}


//-----------------------------------------------------------------------------
//
// Create a bottom-level acceleration structure based on a list of vertex
// buffers in GPU memory along with their vertex count. The build is then done
// in 3 steps: gathering the geometry, computing the sizes of the required
// buffers, and building the actual AS
//
SceneEditor::AccelerationStructureBuffers
SceneEditor::CreateBottomLevelAS(
	std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vVertexBuffers) {
	nv_helpers_dx12::BottomLevelASGenerator bottomLevelAS;

	// Adding all vertex buffers and not transforming their position.
	for (const auto& buffer : vVertexBuffers) {
		bottomLevelAS.AddVertexBuffer(buffer.first.Get(), 0, buffer.second,
			sizeof(Vertex), 0, 0);
	}

	UINT64 scratchSizeInBytes = 0;
	UINT64 resultSizeInBytes = 0;

	bottomLevelAS.ComputeASBufferSizes(m_device.Get(), false, &scratchSizeInBytes,
		&resultSizeInBytes);

	AccelerationStructureBuffers buffers;
	buffers.pScratch = nv_helpers_dx12::CreateBuffer(
		m_device.Get(), scratchSizeInBytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON,
		nv_helpers_dx12::kDefaultHeapProps);
	buffers.pResult = nv_helpers_dx12::CreateBuffer(
		m_device.Get(), resultSizeInBytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		nv_helpers_dx12::kDefaultHeapProps);

	bottomLevelAS.Generate(m_commandList.Get(), buffers.pScratch.Get(),
		buffers.pResult.Get(), false, nullptr);

	return buffers;
}

SceneEditor::AccelerationStructureBuffers
SceneEditor::CreateBottomLevelAS(
	std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vVertexBuffers,
	std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vIndexBuffers) {
	nv_helpers_dx12::BottomLevelASGenerator bottomLevelAS;

	// Adding all vertex buffers and not transforming their position.
	for (size_t i = 0; i < vVertexBuffers.size(); i++) {
		// for (const auto &buffer : vVertexBuffers) {
		if (i < vIndexBuffers.size() && vIndexBuffers[i].second > 0)
			bottomLevelAS.AddVertexBuffer(vVertexBuffers[i].first.Get(), 0,
				vVertexBuffers[i].second, sizeof(Vertex),
				vIndexBuffers[i].first.Get(), 0,
				vIndexBuffers[i].second, nullptr, 0, true);

		else
			bottomLevelAS.AddVertexBuffer(vVertexBuffers[i].first.Get(), 0,
				vVertexBuffers[i].second, sizeof(Vertex), 0,
				0);
	}
	// The AS build requires some scratch space to store temporary information.
	// The amount of scratch memory is dependent on the scene complexity.
	UINT64 scratchSizeInBytes = 0;
	// The final AS also needs to be stored in addition to the existing vertex
	// buffers. It size is also dependent on the scene complexity.
	UINT64 resultSizeInBytes = 0;

	bottomLevelAS.ComputeASBufferSizes(m_device.Get(), false, &scratchSizeInBytes,
		&resultSizeInBytes);

	// Once the sizes are obtained, the application is responsible for allocating
	// the necessary buffers. Since the entire generation will be done on the GPU,
	// we can directly allocate those on the default heap
	AccelerationStructureBuffers buffers;
	buffers.pScratch = nv_helpers_dx12::CreateBuffer(
		m_device.Get(), scratchSizeInBytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON,
		nv_helpers_dx12::kDefaultHeapProps);
	buffers.pResult = nv_helpers_dx12::CreateBuffer(
		m_device.Get(), resultSizeInBytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		nv_helpers_dx12::kDefaultHeapProps);

	// Build the acceleration structure. Note that this call integrates a barrier
	// on the generated AS, so that it can be used to compute a top-level AS right
	// after this method.
	bottomLevelAS.Generate(m_commandList.Get(), buffers.pScratch.Get(),
		buffers.pResult.Get(), false, nullptr);

	return buffers;
}

//-----------------------------------------------------------------------------
// Create the main acceleration structure that holds all instances of the scene.
// Similarly to the bottom-level AS generation, it is done in 3 steps: gathering
// the instances, computing the memory requirements for the AS, and building the
// AS itself
//
void SceneEditor::CreateTopLevelAS(
	const std::vector<std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>>
	& instances // pair of bottom level AS and matrix of the instance
) {
	// Gather all the instances into the builder helper
	for (size_t i = 0; i < instances.size(); i++) {
		//AddInstance(instances1, matrix1, instanceId1, hitGroupIndex1);
		m_topLevelASGenerator.AddInstance(instances[i].first.Get(),
			instances[i].second, static_cast<UINT>(i),
			static_cast<UINT>(i));
	}

	// As for the bottom-level AS, the building the AS requires some scratch space
	// to store temporary data in addition to the actual AS. In the case of the
	// top-level AS, the instance descriptors also need to be stored in GPU
	// memory. This call outputs the memory requirements for each (scratch,
	// results, instance descriptors) so that the application can allocate the
	// corresponding memory
	UINT64 scratchSize, resultSize, instanceDescsSize;

	m_topLevelASGenerator.ComputeASBufferSizes(m_device.Get(), true, &scratchSize,
		&resultSize, &instanceDescsSize);

	// Create the scratch and result buffers. Since the build is all done on GPU,
	// those can be allocated on the default heap
	m_topLevelASBuffers.pScratch = nv_helpers_dx12::CreateBuffer(
		m_device.Get(), scratchSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nv_helpers_dx12::kDefaultHeapProps);
	m_topLevelASBuffers.pResult = nv_helpers_dx12::CreateBuffer(
		m_device.Get(), resultSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		nv_helpers_dx12::kDefaultHeapProps);

	// The buffer describing the instances: ID, shader binding information,
	// matrices ... Those will be copied into the buffer by the helper through
	// mapping, so the buffer has to be allocated on the upload heap.
	m_topLevelASBuffers.pInstanceDesc = nv_helpers_dx12::CreateBuffer(
		m_device.Get(), instanceDescsSize, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);

	// After all the buffers are allocated, or if only an update is required, we
	// can build the acceleration structure. Note that in the case of the update
	// we also pass the existing AS as the 'previous' AS, so that it can be
	// refitted in place.
	m_topLevelASGenerator.Generate(m_commandList.Get(),
		m_topLevelASBuffers.pScratch.Get(),
		m_topLevelASBuffers.pResult.Get(),
		m_topLevelASBuffers.pInstanceDesc.Get());
}

//-----------------------------------------------------------------------------
//
// Combine the BLAS and TLAS builds to construct the entire acceleration
// structure required to raytrace the scene
//
void SceneEditor::CreateAccelerationStructures() {
	// Build the bottom AS from the Triangle vertex buffer
	m_bottomLevelAS.resize(SceneObject::Count);
	std::vector<AccelerationStructureBuffers> bottomLevelBuffers;
	for (int i = 0; i < SceneObject::Count; ++i) {
		bottomLevelBuffers.emplace_back(
			CreateBottomLevelAS(
				{ {m_vertexBuffer[i].Get(), m_vertexCount[i]} },// VertexBuffers vector
				{ {m_indexBuffer[i].Get(), m_indexCount[i]} })// IndexBuffers vector
		);
		// Store the AS buffers. The rest of the buffers will be released once we exit
		// the function
		m_bottomLevelAS[i] = bottomLevelBuffers[i].pResult;
	}

	for (int i = 0; i < SceneObject::Count - 2; ++i) {
		m_instances.emplace_back(std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>(bottomLevelBuffers[i].pResult, XMMatrixIdentity()));
	}
	m_instances.emplace_back(std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>(bottomLevelBuffers[SceneObject::car].pResult, XMMatrixRotationY(-XM_PIDIV2-XM_PIDIV4) * XMMatrixScaling(2.f, 2.f, 2.f) * XMMatrixTranslation(200.f, 165.f, 160.f)));
	m_instances.emplace_back(std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>(bottomLevelBuffers[SceneObject::nanosuit].pResult, XMMatrixRotationY(XM_PI) * XMMatrixScaling(20.f, 20.f, 20.f) * XMMatrixTranslation(400.f, 0.f, 100.f)));
	/*// Just one instance for now
	m_instances = {
		{bottomLevelBuffers.pResult, XMMatrixIdentity()},
		{planebottomLevelBuffers.pResult, XMMatrixRotationY(XM_PI / 2)},
		{planebottomLevelBuffers.pResult, XMMatrixRotationX(XM_PI / 3)},
		{bottomLevelBuffers.pResult, XMMatrixRotationY(XM_PI / 4)},
		{planebottomLevelBuffers.pResult, XMMatrixRotationZ(XM_PI / 5)},
		{planebottomLevelBuffers.pResult, XMMatrixRotationY(XM_PI / 6)},
	};*/
	CreateTopLevelAS(m_instances);

	// Flush the command list and wait for it to finish
	m_commandList->Close();
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(1, ppCommandLists);
	m_fenceValue++;
	m_commandQueue->Signal(m_fence.Get(), m_fenceValue);

	m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent);
	WaitForSingleObject(m_fenceEvent, INFINITE);

	// Once the command list is finished executing, reset it to be reused for
	// rendering
	ThrowIfFailed(
		m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));

}

//-----------------------------------------------------------------------------
// The ray generation shader needs to access 2 resources: the raytracing output
// and the top-level acceleration structure
//
ComPtr<ID3D12RootSignature> SceneEditor::CreateRayGenSignature() {
	auto default = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	nv_helpers_dx12::RootSignatureGenerator rsc;
	rsc.AddHeapRangesParameter(
		{
			{0 /*u0*/, 1 , 0, D3D12_DESCRIPTOR_RANGE_TYPE_UAV ,default},//Raytracing output
			{0 /*t0*/, 1 , 0 ,D3D12_DESCRIPTOR_RANGE_TYPE_SRV ,default},//TLAS
			{0 /*b0*/, 1 , 0, D3D12_DESCRIPTOR_RANGE_TYPE_CBV ,default},//Scene parameters
		});

	return rsc.Generate(m_device.Get(), true);
}

//-----------------------------------------------------------------------------
// The hit shader communicates only through the ray payload, and therefore does
// not require any resources
//
ComPtr<ID3D12RootSignature> SceneEditor::CreateHitSignature() {
	nv_helpers_dx12::RootSignatureGenerator rsc;
	auto default = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	// but we want to access vertex buffer in hit shader, so add a parameter of SRV
	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV, 0);
	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV, 1);
	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV, 2);
	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_CBV, 0);
	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV, 3);
	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV, 4);
	return rsc.Generate(m_device.Get(), true);
}

//-----------------------------------------------------------------------------
// The miss shader communicates only through the ray payload, and therefore
// does not require any resources
//
ComPtr<ID3D12RootSignature> SceneEditor::CreateMissSignature() {
	nv_helpers_dx12::RootSignatureGenerator rsc;
	return rsc.Generate(m_device.Get(), true);
}

//-----------------------------------------------------------------------------
//
// The raytracing pipeline binds the shader code, root signatures and pipeline
// characteristics in a single structure used by DXR to invoke the shaders and
// manage temporary memory during raytracing
//
//
void SceneEditor::CreateRaytracingPipeline()
{
	nv_helpers_dx12::RayTracingPipelineGenerator pipeline(m_device.Get());

	// The pipeline contains the DXIL code of all the shaders potentially executed
	// during the raytracing process. This section compiles the HLSL code into a
	// set of DXIL libraries. We chose to separate the code in several libraries
	// by semantic (ray generation, hit, miss) for clarity. Any code layout can be
	// used.
	m_rayGenLibrary = nv_helpers_dx12::CompileShaderLibrary(L"RayGen.hlsl");
	m_missLibrary = nv_helpers_dx12::CompileShaderLibrary(L"Miss.hlsl");
	m_hitLibrary = nv_helpers_dx12::CompileShaderLibrary(L"Hit.hlsl");

	// In a way similar to DLLs, each library is associated with a number of
	// exported symbols. This
	// has to be done explicitly in the lines below. Note that a single library
	// can contain an arbitrary number of symbols, whose semantic is given in HLSL
	// using the [shader("xxx")] syntax
	pipeline.AddLibrary(m_rayGenLibrary.Get(), { ws_raygenShaderName });
	pipeline.AddLibrary(m_missLibrary.Get(), ws_missShaderNames);
	pipeline.AddLibrary(m_hitLibrary.Get(), ws_closestHitShaderNames);

	// To be used, each DX12 shader needs a root signature defining which
	// parameters and buffers will be accessed.
	m_rayGenSignature = CreateRayGenSignature();
	m_missSignature = CreateMissSignature();
	m_hitSignature = CreateHitSignature();

	// 3 different shaders can be invoked to obtain an intersection: an intersection shader is called
	// when hitting the bounding box of non-triangular geometry. This is beyond
	// the scope of this tutorial. An any-hit shader is called on potential
	// intersections. This shader can, for example, perform alpha-testing and
	// discard some intersections. Finally, the closest-hit program is invoked on
	// the intersection point closest to the ray origin. Those 3 shaders are bound
	// together into a hit group.

	// Note that for triangular geometry the intersection shader is built-in. An
	// empty any-hit shader is also defined by default, so in our simple case each
	// hit group contains only the closest hit shader. Note that since the
	// exported symbols are defined above the shaders can be simply referred to by
	// name.

	// Hit group for the triangles, with a shader simply interpolating vertex colors
	for (int i = 0; i < SceneObject::Count; ++i)
		pipeline.AddHitGroup(ws_hitGroupNames[i], ws_closestHitShaderNames[0]);

	// The following section associates the root signature to each shader. Note
	// that we can explicitly show that some shaders share the same root signature
	// (eg. Miss and ShadowMiss). Note that the hit shaders are now only referred
	// to as hit groups, meaning that the underlying intersection, any-hit and
	// closest-hit shaders share the same root signature.
	pipeline.AddRootSignatureAssociation(m_rayGenSignature.Get(), { ws_raygenShaderName });
	pipeline.AddRootSignatureAssociation(m_missSignature.Get(), { ws_missShaderNames });
	for (int i = 0; i < SceneObject::Count; ++i)
		pipeline.AddRootSignatureAssociation(m_hitSignature.Get(), { ws_hitGroupNames[i] });

	// The payload size defines the maximum size of the data carried by the rays,
	// ie. the the data
	// exchanged between shaders, such as the HitInfo structure in the HLSL code.
	// It is important to keep this value as low as possible as a too high value
	// would result in unnecessary memory consumption and cache trashing.
	pipeline.SetMaxPayloadSize(sizeof(PayLoad));
	//pipeline.SetMaxPayloadSize(4 * sizeof(float) ); // RGB + distance

	// Upon hitting a surface, DXR can provide several attributes to the hit. In
	// our sample we just use the barycentric coordinates defined by the weights
	// u,v of the last two vertices of the triangle. The actual barycentrics can
	// be obtained using float3 barycentrics = float3(1.f-u-v, u, v);
	pipeline.SetMaxAttributeSize(2 * sizeof(float)); // barycentric coordinates

	// The raytracing process can shoot rays from existing hit points, resulting
	// in nested TraceRay calls. Our sample code traces only primary rays, which
	// then requires a trace depth of 1. Note that this recursion depth should be
	// kept to a minimum for best performance. Path tracing algorithms can be
	// easily flattened into a simple loop in the ray generation.
	pipeline.SetMaxRecursionDepth(31);

	// Compile the pipeline for execution on the GPU
	m_rtStateObject = pipeline.Generate();

	// Cast the state object into a properties object, allowing to later access
	// the shader pointers by name
	ThrowIfFailed(
		m_rtStateObject->QueryInterface(IID_PPV_ARGS(&m_rtStateObjectProps)));
}

//-----------------------------------------------------------------------------
//
// Allocate the buffer holding the raytracing output, with the same size as the
// output image
//
void SceneEditor::CreateRaytracingOutputBuffer() {
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.DepthOrArraySize = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	// The backbuffer is actually DXGI_FORMAT_R16G16B16A16_FLOAT_SRGB, but sRGB
	// formats cannot be used with UAVs. For accuracy we should convert to sRGB
	// ourselves in the shader
	resDesc.Format = m_outPutFormat;// DXGI_FORMAT_R16G16B16A16_FLOAT;

	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	resDesc.Width = GetWidth();
	resDesc.Height = GetHeight();
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	ThrowIfFailed(m_device->CreateCommittedResource(
		&nv_helpers_dx12::kDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
		D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr,
		IID_PPV_ARGS(&m_outputResource)));

	// #DXR Extra: Perspective Camera
	// Create a buffer to store the modelview and perspective camera matrices
	CreateCameraBuffer();
}

void SceneEditor::CreateShaderResourceHeap() {
	// Create a SRV/UAV/CBV descriptor heap. We need 3 entries - 1 UAV for the
	// raytracing output, 1 SRV for the TLAS, and 1 CBV for the camera matrices
	m_srvUavHeap = nv_helpers_dx12::CreateDescriptorHeap(
		m_device.Get(), 1000, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

	// Get a handle to the heap memory on the CPU side, to be able to write the
	// descriptors directly
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle =
		m_srvUavHeap->GetCPUDescriptorHandleForHeapStart();

	// Create the UAV. Based on the root signature we created it is the first
	// entry. The Create*View methods write the view information directly into
	// srvHandle
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	m_device->CreateUnorderedAccessView(m_outputResource.Get(), nullptr, &uavDesc,
		srvHandle);

	// Add the Top Level AS SRV right after the raytracing output buffer
	srvHandle.ptr += m_device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.RaytracingAccelerationStructure.Location =
		m_topLevelASBuffers.pResult->GetGPUVirtualAddress();
	// Write the acceleration structure view in the heap
	m_device->CreateShaderResourceView(nullptr, &srvDesc, srvHandle);

	// #DXR Extra: Perspective Camera
	// Add the constant buffer for the camera after the TLAS
	{
		srvHandle.ptr += m_device->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// Describe and create a constant buffer view for the camera
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = m_cameraBuffer->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = m_cameraBufferSize;
		m_device->CreateConstantBufferView(&cbvDesc, srvHandle);
	}

	// Add the constant buffer for the material after the camera
	for (int i = 0; i < SceneObject::Count; ++i) {
		srvHandle.ptr += m_device->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = m_MaterialBuffer[i]->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = m_MaterialBufferSize;
		m_device->CreateConstantBufferView(&cbvDesc, srvHandle);
	}

}

//-----------------------------------------------------------------------------
//
// The Shader Binding Table (SBT) is the cornerstone of the raytracing setup:
// this is where the shader resources are bound to the shaders, in a way that
// can be interpreted by the raytracer on GPU. In terms of layout, the SBT
// contains a series of shader IDs with their resource pointers. The SBT
// contains the ray generation shader, the miss shaders, then the hit groups.
// Using the helper class, those can be specified in arbitrary order.
//
void SceneEditor::CreateShaderBindingTable() {
	// The SBT helper class collects calls to Add*Program.  If called several
	// times, the helper must be emptied before re-adding shaders.
	m_sbtHelper.Reset();

	// The pointer to the beginning of the heap is the only parameter required by
	// shaders without root parameters
	D3D12_GPU_DESCRIPTOR_HANDLE srvUavHeapHandle =
		m_srvUavHeap->GetGPUDescriptorHandleForHeapStart();

	// The helper treats both root parameter pointers and heap pointers as void*,
	// while DX12 uses the
	// D3D12_GPU_DESCRIPTOR_HANDLE to define heap pointers. The pointer in this
	// struct is a UINT64, which then has to be reinterpreted as a pointer.
	auto heapPointer = reinterpret_cast<UINT64*>(srvUavHeapHandle.ptr);

	// The ray generation only uses heap data
	m_sbtHelper.AddRayGenerationProgram(L"RayGen", { heapPointer });

	// The miss and hit shaders do not access any external resources: instead they
	// communicate their results through the ray payload
	m_sbtHelper.AddMissProgram(L"Miss", {});

	// Adding the triangle hit shader
	//m_sbtHelper.AddHitGroup(L"HitGroup", {});
	// Access vertexBuffer in hit shader
	for (int i = 0; i < SceneObject::Count; ++i)
		m_sbtHelper.AddHitGroup(ws_hitGroupNames[i], {
			(void*)(m_vertexBuffer[i]->GetGPUVirtualAddress()),
			(void*)(m_indexBuffer[i]->GetGPUVirtualAddress()),
			(void*)(m_topLevelASBuffers.pResult->GetGPUVirtualAddress()),
			(void*)(m_MaterialBuffer[i]->GetGPUVirtualAddress()),
			(void*)(m_vertexBuffer[SceneObject::light]->GetGPUVirtualAddress()),
			(void*)(m_indexBuffer[SceneObject::light]->GetGPUVirtualAddress()),
			});


	// Compute the size of the SBT given the number of shaders and their
	// parameters
	uint32_t sbtSize = m_sbtHelper.ComputeSBTSize();

	// Create the SBT on the upload heap. This is required as the helper will use
	// mapping to write the SBT contents. After the SBT compilation it could be
	// copied to the default heap for performance.
	m_sbtStorage = nv_helpers_dx12::CreateBuffer(
		m_device.Get(), sbtSize, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);
	if (!m_sbtStorage) {
		throw std::logic_error("Could not allocate the shader binding table");
	}

	// Compile the SBT from the shader and parameters info
	m_sbtHelper.Generate(m_sbtStorage.Get(), m_rtStateObjectProps.Get());
}


//----------------------------------------------------------------------------------
//
// The camera buffer is a constant buffer that stores the transform matrices of
// the camera, for use by both the rasterization and raytracing. This method
// allocates the buffer where the matrices will be copied. For the sake of code
// clarity, it also creates a heap containing only this buffer, to use in the
// rasterization path.
//
// #DXR Extra: Perspective Camera
void SceneEditor::CreateCameraBuffer()
{
	//uint32_t nbMatrix = 4; // view, perspective, viewInverse, perspectiveInverse
	//size must be multiple of 256
	m_cameraBufferSize = SizeOfIn256(SceneConstants);


	// Create the constant buffer for all matrices
	m_cameraBuffer = nv_helpers_dx12::CreateBuffer(
		m_device.Get(), m_cameraBufferSize, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);

	// Create a descriptor heap that will be used by the rasterization shaders
	m_constHeap = nv_helpers_dx12::CreateDescriptorHeap(
		m_device.Get(), 1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

	// Describe and create the constant buffer view.
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = m_cameraBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = m_cameraBufferSize;

	// Get a handle to the heap memory on the CPU side, to be able to write the
	// descriptors directly
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle =
		m_constHeap->GetCPUDescriptorHandleForHeapStart();
	m_device->CreateConstantBufferView(&cbvDesc, srvHandle);
}

void SceneEditor::UpadteMaterialParameter(int bufferIndex) {
	uint8_t* pData;
	ThrowIfFailed(m_MaterialBuffer[bufferIndex]->Map(0, nullptr, (void**)&pData));
	memcpy(pData, &(m_MaterialAttributes[bufferIndex]), sizeof(PrimitiveMaterialBuffer));
	m_MaterialBuffer[bufferIndex]->Unmap(0, nullptr);
}

// #DXR Extra: Perspective Camera
//--------------------------------------------------------------------------------
// Create and copies the viewmodel and perspective matrices of the camera
//
void SceneEditor::UpdateSceneParameterBuffer() {
	//SceneConstants matrices;

	// Initialize the view matrix, ideally this should be based on user
	// interactions The lookat and perspective matrices used for rasterization are
	// defined to transform world-space vertices into a [0,1]x[0,1]x[0,1] camera
	// space
	/*XMVECTOR Eye = XMVectorSet(0.0f, 0.0f, 2.0f, 0.0f);
	XMVECTOR At = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);*/
	//matrices.view = XMMatrixLookAtRH(Eye, At, Up);
	matrices.view = XMMatrixLookAtLH(m_camera.GetEye(), m_camera.GetAt(), m_camera.GetUp());

	//float fovAngleY = 10.0f * XM_PI / 180.0f;

	matrices.projection =
		XMMatrixPerspectiveFovLH(m_camera.GetFov(), m_aspectRatio, 0.1f, 1000.0f);

	// Raytracing has to do the contrary of rasterization: rays are defined in
	// camera space, and are transformed into world space. To do this, we need to
	// store the inverse matrices as well.
	XMVECTOR det;
	matrices.viewI = XMMatrixInverse(&det, matrices.view);
	matrices.projectionI = XMMatrixInverse(&det, matrices.projection);

	//set spp as 0 if some parameters change
	if (needRefreshScreen) {
		matrices.CurrSampleIdx = 0;
		needRefreshScreen = false;
	}
	else {
		matrices.CurrSampleIdx++;
		//matrices.CurrSampleIdx = min(matrices.CurrSampleIdx, 1000);
	}
	// Copy the matrix contents
	matrices.seed = XMFLOAT4(rand() / double(0xfff), rand() / double(0xfff), rand() / double(0xfff), rand() / double(0xfff));
	uint8_t* pData;
	ThrowIfFailed(m_cameraBuffer->Map(0, nullptr, (void**)&pData));
	memcpy(pData, &matrices, m_cameraBufferSize);
	m_cameraBuffer->Unmap(0, nullptr);


}

void SceneEditor::OnMouseDown(WPARAM btnState, int x, int y)
{
	m_lastMousePos.x = x;
	m_lastMousePos.y = y;
	SetCapture(m_mainWndHandle);
}

void SceneEditor::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void SceneEditor::OnMouseMove(WPARAM btnState, int x, int y)
{
	if (m_imguiManager.isHovered)
		return;

	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - m_lastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - m_lastMousePos.y));

		// Rotate camera.
		if (fabsf(dx) > fabsf(dy))m_camera.RotateAroundUp(dx);
		else m_camera.RotateAroundLeft(dy);
		OnResetSpp();
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to 0.5 unit in the scene.
		float dx = 0.5f * static_cast<float>(x - m_lastMousePos.x);
		float dy = 0.5f * static_cast<float>(y - m_lastMousePos.y);

		// Zoom in or out.
		m_camera.ScaleFov(dx - dy);
		OnResetSpp();
	}

	m_lastMousePos.x = x;
	m_lastMousePos.y = y;
}

void SceneEditor::OnKeyDown(UINT8 key)
{
	switch (key)
	{
	case 'W':
		m_camera.MoveEyeForward();
		break;
	case 'S':
		m_camera.MoveEyeBackward();
		break;
	case 'A':
		m_camera.MoveEyeLeft();
		break;
	case 'D':
		m_camera.MoveEyeRight();
		break;
	case 'Q':
		m_camera.MoveEyeUp();
		break;
	case 'E':
		m_camera.MoveEyeDown();
		break;
	default:
		break;
	}
}

void SceneEditor::StartImgui()
{
	// Start the Dear ImGui frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Parameters");
	ImGui::Combo("ObjectsName", &m_imguiManager.m_currentObjeectItem, m_ObjectName, SceneObject::Count );
	
	auto valueAddress = &m_MaterialAttributes[m_imguiManager.m_currentObjeectItem].smoothness;
	if (ImGui::SliderFloat("smoothness", valueAddress, 0.1f, 5.f))
		OnResetSpp();
	valueAddress = &m_MaterialAttributes[m_imguiManager.m_currentObjeectItem].index_of_refraction;
	if (ImGui::SliderFloat("refraction", valueAddress, 0.f, 5.f))
		OnResetSpp();

	ImGui::Text("Color:");
	if(ImGui::ColorEdit4("", reinterpret_cast<float*>(&m_MaterialAttributes[m_imguiManager.m_currentObjeectItem].Kd)))
		OnResetSpp();

	ImGui::Text("Material:");
	auto materialAddress = reinterpret_cast<int*>(&m_MaterialAttributes[m_imguiManager.m_currentObjeectItem].type);
	if(ImGui::Combo("", materialAddress, m_MaterialType, MaterialType::Count))
		OnResetSpp();

	if(ImGui::IsWindowHovered()||ImGui::IsAnyItemHovered()||ImGui::IsAnyItemActive())
		m_imguiManager.isHovered = true;
	else
		m_imguiManager.isHovered = false;

	ImGui::End();
	// set a srvheap for imgui to seperate it from raytracing
	m_commandList->SetDescriptorHeaps(1, m_imguiManager.GetSrvHeap4Imgui().GetAddressOf());
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_commandList.Get());
}

