#include "stdafx.h"
#include "Camera.h"

void Camera::MoveEyeForward(uint64_t span)
{
	m_eye += m_direction * m_movSpeed * span;
	m_at = m_eye + m_direction;
}

void Camera::MoveEyeBackward(uint64_t span)
{
	m_eye -= m_direction * m_movSpeed * span;
	m_at = m_eye + m_direction;
}

void Camera::MoveEyeUp(uint64_t span)
{
	m_eye += m_up * m_movSpeed * span;
	m_at = m_eye + m_direction;
}

void Camera::MoveEyeDown(uint64_t span)
{
	m_eye -= m_up * m_movSpeed * span;
	m_at = m_eye + m_direction;
}

void Camera::MoveEyeLeft(uint64_t span)
{
	m_eye += m_left * m_movSpeed * span;
	m_at = m_eye + m_direction;
}

void Camera::MoveEyeRight(uint64_t span)
{
	m_eye -= m_left * m_movSpeed * span;
	m_at = m_eye + m_direction;
}

void Camera::SetMoveSpeed(float movSpeed) {
	m_movSpeed = movSpeed;
}

void Camera::RotateAroundUp(float dx) {
	XMMATRIX rotateMat = XMMatrixRotationAxis(m_up, dx);
	m_direction = XMVector3Normalize(XMVector3TransformNormal(m_direction, rotateMat));
	m_at = m_eye + m_direction;
	XMVECTOR sup_up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR sup_right = XMVector3Normalize(XMVector3Cross(sup_up, m_direction));
	m_up = XMVector3Normalize(XMVector3Cross(m_direction, sup_right));
	m_left = XMVector3Normalize(XMVector3Cross(m_direction, m_up));
}
void Camera::RotateAroundLeft(float dy) {
	XMMATRIX rotateMat = XMMatrixRotationAxis(m_left, -dy);
	m_direction = XMVector3Normalize(XMVector3TransformNormal(m_direction, rotateMat));
	m_at = m_eye + m_direction;
	XMVECTOR sup_up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR sup_right = XMVector3Normalize(XMVector3Cross(sup_up, m_direction));
	m_up = XMVector3Normalize(XMVector3Cross(m_direction, sup_right));
}
void Camera::ScaleFov(float d) {
	m_fovAngleY += d * XM_PI / 180.0f;
	// Limit fov in [XM_PI/10, 2*XM_PI/3]
	m_fovAngleY = max(m_fovAngleY, XM_PI / 10);
	m_fovAngleY = min(m_fovAngleY, XM_PI / 1.5);
}
