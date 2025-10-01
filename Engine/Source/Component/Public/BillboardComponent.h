#pragma once
#include "PrimitiveComponent.h"
#include "Editor/Public/Camera.h"

class UBillboardComponent : public UPrimitiveComponent
{
public:
	UBillboardComponent(AActor* InOwnerActor);
	~UBillboardComponent();

	FMatrix GetRTMatrix() const { return RTMatrix; }

	void UpdateRotationMatrix(const UCamera* InCamera);

private:
	FMatrix RTMatrix;
	AActor* POwnerActor;
	float ZOffset;
};
