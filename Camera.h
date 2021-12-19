#pragma once
#include <stdafx.h>
using namespace DirectX;

/// <summary>
/// This class used to operate camera.
/// To translate: press ¡®W¡¯¡¢¡®A¡¯¡¢¡®S¡¯¡¢¡®D¡¯¡¢¡®Q¡¯¡¢¡®E¡¯
/// To rotate: press mouse left button and move;
/// To scale: press mouse right button and move;
/// 
/// 
/// By: Lijin 2021.3.8
/// </summary>
/// 
class Camera
{
public:
	Camera(XMVECTOR eye = XMVectorSet(2.0f, 2.0f, -3.0f, 0.0f),
		XMVECTOR at = XMVectorSet(2.0f, 1.0f, -2.0f, 0.0f),
		XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)) :m_eye(eye), m_at(at)
	{
		m_up = XMVector3Normalize(up);
		m_direction = XMVector3Normalize(m_at - m_eye);
		m_left = XMVector3Normalize(XMVector3Cross(m_direction, m_up));
	}

	float m_movSpeed = 0.005f;
	void SetEye(float x, float y, float z, float w) { m_eye = XMVectorSet(x, y, z, w); }
	void SetEye(XMVECTOR eye) { m_eye = eye; }
	void SetAt(float x, float y, float z, float w) { m_at = XMVectorSet(x, y, z, w); }
	void SetAt(XMVECTOR at) { m_at = at; }
	void SetUp(float x, float y, float z, float w) { m_up = XMVectorSet(x, y, z, w); }
	void SetUp(XMVECTOR up) { m_up = XMVector3Normalize(up); }

	void Set(XMVECTOR eye, XMVECTOR at, XMVECTOR up)
	{
		m_eye = eye;
		m_at = at;
		m_up = up;
	}

	XMVECTOR GetEye() { return m_eye; }
	XMVECTOR GetAt() { return m_at; }
	XMVECTOR GetUp() { return m_up; }
	XMVECTOR GetRight() { return m_left; }
	XMVECTOR GetDirection() { return m_direction; }
	float GetFov() { return m_fovAngleY; }

	void MoveEyeForward(uint64_t span = 1);
	void MoveEyeBackward(uint64_t span = 1);
	void MoveEyeUp(uint64_t span = 1);
	void MoveEyeDown(uint64_t span = 1);
	void MoveEyeLeft(uint64_t span = 1);
	void MoveEyeRight(uint64_t span = 1);
	void SetMoveSpeed(float movSpeed);
	void RotateAroundUp(float dx);
	void RotateAroundLeft(float dy);
	void ScaleFov(float d);

private:
	XMVECTOR m_eye;
	XMVECTOR m_at;
	XMVECTOR m_up;
	XMVECTOR m_left;
	XMVECTOR m_direction;
	float m_fovAngleY = 45.0f * XM_PI / 180.0f;
};

