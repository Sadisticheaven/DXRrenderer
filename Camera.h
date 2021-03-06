#pragma once
#include <stdafx.h>
using namespace DirectX;
class Camera
{
public:
	Camera(XMFLOAT4 eye = XMFLOAT4(1.5f, 1.5f, 1.5f, 0.0f),
		XMFLOAT4 at = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f),
		XMFLOAT4 up = XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f)) :Eye(eye), At(at), Up(up) {}
	~Camera();

	void SetEye(float x, float y, float z, float w) { Eye = XMFLOAT4(x, y, z, w); }
	void SetAt(float x, float y, float z, float w) { At = XMFLOAT4(x, y, z, w); }
	void SetUp(float x, float y, float z, float w) { Up = XMFLOAT4(x, y, z, w); }

	void MoveUp() { Eye.y += 0.5f; }
	void MoveDown() { Eye.y -= 0.5f; }
	void MoveLeft();
	void MoveRight();

private:
	XMFLOAT4 Eye;
	XMFLOAT4 At;
	XMFLOAT4 Up;
};