#pragma once
#include "Render/UI/Widget/Public/Widget.h"

class UStaticMeshComponent;

UCLASS()
class UStaticMeshComponentWidget : public UWidget
{
	GENERATED_BODY()
	DECLARE_CLASS(UStaticMeshComponentWidget, UWidget)

public:
	void RenderWidget() override;

private:
	UStaticMeshComponent* Component{};
};
