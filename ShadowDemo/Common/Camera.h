#pragma once

#include <Windows.h>
#include <DirectXMath.h>
#include <cmath>
class Camera
{
public:
	Camera();

	//设置摄像机位置
	void SetPosition(float x, float y, float z) { m_position = DirectX::XMFLOAT3(x, y, z); }
	void SetPosition(DirectX::FXMVECTOR pos) { DirectX::XMStoreFloat3(&m_position, pos); }

	//获得摄像机位置方向等相关参数
	DirectX::XMFLOAT3 GetPosition() const { return m_position; }
	DirectX::XMFLOAT3 GetRight() { return m_right; }
	DirectX::XMFLOAT3 GetLook() { return m_look; }
	DirectX::XMFLOAT3 GetUp() { return m_up; }

	DirectX::XMVECTOR GetPosotionXM() const { return DirectX::XMLoadFloat3(&m_position); }
	DirectX::XMVECTOR GetRightXM() const { return DirectX::XMLoadFloat3(&m_right); }
	DirectX::XMVECTOR GetLookXM() const { return DirectX::XMLoadFloat3(&m_look); }
	DirectX::XMVECTOR GetUpXM() const { return DirectX::XMLoadFloat3(&m_up); }

	//获得投影相关参数
	float GetNearZ() const { return m_nearZ; }
	float GetFarZ() const { return m_farZ; }
	float GetFovY() const { return m_fovY; }
	float GetFovX() const { return atan(m_aspect * tan(m_fovY * 0.5f)) * 2.f; }
	float GetAspect() const { return m_aspect; }

	//获得视图投影矩阵
	DirectX::XMMATRIX GetView() const { return DirectX::XMLoadFloat4x4(&m_view); }
	DirectX::XMMATRIX GetProj() const { return DirectX::XMLoadFloat4x4(&m_proj); }
	DirectX::XMMATRIX GetViewProj() const { return XMLoadFloat4x4(&m_view) * XMLoadFloat4x4(&m_proj); }

	//设置投影矩阵
	void SetLens(float fovY, float aspect, float nearz, float farz);

	//设置视角矩阵
	void LookAtXM(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR lookAt, DirectX::FXMVECTOR worldUp);
	void LookAt(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& lookAt, 
		const DirectX::XMFLOAT3& worldUp);

	//基本操作
	void Walk(float dist);			//前后行走
	void Strafe(float dist);			//左右平移
	void Pitch(float angle);			//上下点头
	void RotateY(float angle);		//左右摇头

	//更新矩阵
	void UpdateViewMatrix();
private:
	DirectX::XMFLOAT3 m_right;		//位置和三个坐标轴
	DirectX::XMFLOAT3 m_up;
	DirectX::XMFLOAT3 m_look;
	DirectX::XMFLOAT3 m_position;

	float m_aspect;					//投影相关参数
	float m_fovY;
	float m_nearZ;
	float m_farZ;

	DirectX::XMFLOAT4X4 m_view;		//视角矩阵
	DirectX::XMFLOAT4X4 m_proj;		//投影矩阵
};