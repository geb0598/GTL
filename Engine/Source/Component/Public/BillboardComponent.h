#pragma once
#include "PrimitiveComponent.h"
#include "Editor/Public/Camera.h"

UCLASS()

class UBillboardComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UBillboardComponent, UPrimitiveComponent)

public:
	UBillboardComponent();
	~UBillboardComponent();

	FMatrix GetRTMatrix() const { return RTMatrix; }

	void UpdateRotationMatrix(const UCamera* InCamera);

private:
	FMatrix RTMatrix;
	float ZOffset;
};
