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

void ImguiManager::StartImgui(ComPtr<ID3D12GraphicsCommandList4> m_commandList)
{
	// Start the Dear ImGui frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	bool show_demo_window = true;
	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

	{
		static int counter = 0;
		ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.
		ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
		ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
		if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
			counter++;
		ImGui::SameLine();
		ImGui::Text("counter = %d", counter);
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
	}
	// set a srvheap for imgui to seperate it from raytracing
	m_commandList->SetDescriptorHeaps(1, m_srvHeap4Imgui.GetAddressOf());
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_commandList.Get());
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
		m_imguiOutputFormat/*DXGI_FORMAT_R8G8B8A8_UNORM*/, pSrvHeap4Imgui,
		pSrvHeap4Imgui->GetCPUDescriptorHandleForHeapStart(),
		pSrvHeap4Imgui->GetGPUDescriptorHandleForHeapStart());
}
