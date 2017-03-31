#pragma once

#include "Common\d3dUtil.h"
#include "Common\MathHelper.h"
#include "Common\UploadBuffer.h"

struct ObjectConstants
{
	//世界矩阵和纹理变换矩阵
    DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
};

struct PassConstants
{
    DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
    float cbPerObjectPad1 = 0.0f;
    DirectX::XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
    DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
    float NearZ = 0.0f;
    float FarZ = 0.0f;
    float TotalTime = 0.0f;
    float DeltaTime = 0.0f;

    DirectX::XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

    // Indices [0, NUM_DIR_LIGHTS) are directional lights;
    // indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
    // indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
    // are spot lights for a maximum of MaxLights per object.
    Light Lights[MaxLights];
};

//顶点结构
struct Vertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 TexC;
};

//每一帧需要的resources.  
struct FrameResource
{
public:
    
    FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount);
    FrameResource(const FrameResource& rhs) = delete;
    FrameResource& operator=(const FrameResource& rhs) = delete;
    ~FrameResource();

	//每一帧需要它们的allocator
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

    // We cannot update a cbuffer until the GPU is done processing the commands
    // that reference it.  So each frame needs their own cbuffers.
    std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
    std::unique_ptr<UploadBuffer<MaterialConstants>> MaterialCB = nullptr;
    std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;

    // Fence value to mark commands up to this fence point.  This lets us
    // check if these frame resources are still in use by the GPU.
    UINT64 Fence = 0;
};