#pragma once
#include <stdafx.h>
using Microsoft::WRL::ComPtr;
/// <summary>
/// This class used to manage ImGui of current device.
/// </summary>

class ImguiManager {
public:	
	ImguiManager(ComPtr<ID3D12Device5> m_device): m_device(m_device) {}
	void CreateSRVHeap4Imgui();
	void StartImgui(ComPtr<ID3D12GraphicsCommandList4> m_commandList);
	void InitImGui4RayTracing(HWND hwnd);
	float smoothness = 1.f;
private:
	ComPtr<ID3D12Device5> m_device;
	ComPtr<ID3D12DescriptorHeap> m_srvHeap4Imgui;
	//DXGI_FORMAT m_imguiOutputFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT m_imguiOutputFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	bool show_demo_window = true;

};