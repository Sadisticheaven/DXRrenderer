#pragma once
#include <stdafx.h>
#include <vector>

using Microsoft::WRL::ComPtr;
/// <summary>
/// This class used to manage ImGui of current device.
/// </summary>

class ImguiManager {
public:	
	ImguiManager(ComPtr<ID3D12Device5> m_device): m_device(m_device){}
	void CreateSRVHeap4Imgui();
	void InitImGui4RayTracing(HWND hwnd);
	ComPtr<ID3D12DescriptorHeap> GetSrvHeap4Imgui() { return m_srvHeap4Imgui; }
	
	bool isHovered = 0;
	char** m_texNamesChar; // display Tex Names need char*[]
	void ConvertString2Char(char** &dest, const std::vector<std::string>& src);// convert vector<string> to char*[]

	char** m_objectsName;
	int m_selectObjIdx = 0;
	int m_selectLightIdx = 0;
	std::string m_selObjName;

	float m_translation[3] = { 0.f , 0.f, 0.f};
	float m_rotation[3] = { 0.f , 0.f, 0.f};
	float m_scale[4] = { 1.f , 1.f, 1.f, 1.f};
	float m_lightPos[3] = { -10.f , 1.f, 1.f};

private:
	ComPtr<ID3D12Device5> m_device;	
	ComPtr<ID3D12DescriptorHeap> m_srvHeap4Imgui;
	//DXGI_FORMAT m_imguiOutputFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT m_imguiOutputFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	bool show_demo_window = true;

};