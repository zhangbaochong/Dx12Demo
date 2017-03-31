#include "DxBase.h"
#include "Win32App.h"
using namespace Microsoft::WRL;

#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"D3D12.lib")
#pragma comment(lib,"dxgi.lib")

DxBase::DxBase(UINT width, UINT height, std::wstring name):
	m_width(width),
	m_height(height),
	m_title(name),
	m_isUseWarpDevice(false)
{
	m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
}

DxBase::~DxBase()
{

}

void DxBase::GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter)
{
	ComPtr<IDXGIAdapter1> adapter;
	*ppAdapter = nullptr;

	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			continue;
		}

		// 检测是否支持dx12
		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
		{
			break;
		}
	}

	*ppAdapter = adapter.Detach();
}

//设置标题
void DxBase::SetCustomWindowText(LPCWSTR text)
{
	std::wstring windowText = m_title + L":" + text;
	SetWindowText(Win32App::GetHwnd(), windowText.c_str());
}

