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
	//int m_currentObjeectItem = 0;
	
	bool isHovered = 0;
	char** m_texNamesChar; // display Tex Names need char*[]
	void ConvertString2Char(char** &dest, const std::vector<std::string>& src);// convert vector<string> to char*[]

	char** m_objectsName;
	int m_selectObjIdx = 0;
	std::string m_selObjName;
private:
	ComPtr<ID3D12Device5> m_device;	
	ComPtr<ID3D12DescriptorHeap> m_srvHeap4Imgui;
	//DXGI_FORMAT m_imguiOutputFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT m_imguiOutputFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	bool show_demo_window = true;

};