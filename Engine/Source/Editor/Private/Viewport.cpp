#include "pch.h"
#include "Editor/Public/Viewport.h"

void FViewport::Apply(ID3D11DeviceContext* InContext) const
{
	InContext->RSSetViewports(1, &Viewport);
}

void FViewport::ClearDepth(ID3D11DeviceContext* InContext, ID3D11DepthStencilView* InStencilView) const
{
	InContext->ClearDepthStencilView(InStencilView, D3D11_CLEAR_DEPTH, 1.f, 0);
}
