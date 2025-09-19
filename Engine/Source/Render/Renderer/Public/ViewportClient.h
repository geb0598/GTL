#pragma once
#include "Render/Renderer/Public/Viewport.h"

class URenderer;

class FViewportClient
{
public:
	FViewportClient() = default;
	~FViewportClient() = default;

	/**
	* @brief ViewportClient가 보유한 Viewport들의 정보를 갱신합니다.
	* 이 함수는 현재 2x2 기준으로 작성되어 있습니다.
	*/
	void UpdateLayout(const D3D11_VIEWPORT& InViewport);

	/**
	* @brief 각각의 Viewport를 적용하여 최종적으로 화면에 출력합니다.
	*/
	void Render(URenderer& Renderer, ID3D11DeviceContext* DeviceContext, ID3D11DepthStencilView* DepthStencilView);

	TArray<FViewport>& GetViewports() { return Viewports; }

private:
	TArray<FViewport> Viewports = {};
};

