#pragma once
#include "Editor/Public/Camera.h"

class FViewport
{
public:
	FViewport() = default;
	~FViewport() = default;

	/* *
	* @brief 출력될 화면의 너비, 높이, 깊이 등을 적용합니다.
	*/
	void Apply(ID3D11DeviceContext* InContext) const;

	/* *
	* @brief 현재는 사용하지 않지만, 추후 사용될 여지가 있음
	*/
	void ClearDepth(ID3D11DeviceContext* InContext, ID3D11DepthStencilView* InStencilView) const;

	// Getter
	D3D11_VIEWPORT GetViewport() { return ViewportInfo; }

	// Setter
	void SetViewport(const D3D11_VIEWPORT& InViewport) { ViewportInfo = InViewport; }

	D3D11_VIEWPORT ViewportInfo = {};
	UCamera Camera;
	bool bIsActive = false;
};

