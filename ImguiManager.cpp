#include "stdafx.h"
#include "ImguiManager.h"
#include <DXSampleHelper.h>

void ImguiManager::CreateSRVHeap4Imgui()
{
	D3D12_DESCRIPTOR_HEAP_DESC SrvHeapDesc;
	SrvHeapDesc.NumDescriptors = 1;
	SrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	SrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	SrvHeapDesc.NodeMask = 0;
	ThrowIfFailed(m_device->CreateDescriptorHeap(
		&SrvHeapDesc, IID_PPV_ARGS(m_srvHeap4Imgui.GetAddressOf())));
}

void ImguiManager::InitImGui4RayTracing(HWND hwnd)
{
	//init imgui win32 impl
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	ImGui::StyleColorsDark();
	//init imgui dx12
	ImGui_ImplWin32_Init(hwnd);
	ID3D12DescriptorHeap* pSrvHeap4Imgui = m_srvHeap4Imgui.Get();
	ImGui_ImplDX12_Init(m_device.Get(), 2,
		m_imguiOutputFormat, pSrvHeap4Imgui,
		pSrvHeap4Imgui->GetCPUDescriptorHandleForHeapStart(),
		pSrvHeap4Imgui->GetGPUDescriptorHandleForHeapStart());
}
