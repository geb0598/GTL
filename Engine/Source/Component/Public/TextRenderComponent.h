#pragma once
#include "Component/Public/PrimitiveComponent.h"
#include "Editor/Public/Camera.h"
#include "Global/Matrix.h"

class AActor;

class UTextRenderComponent : public UPrimitiveComponent
{
public:
	UTextRenderComponent(AActor* InOwnerActor, float InYOffset);
	~UTextRenderComponent();

	void UpdateRotationMatrix(const FVector& InCameraLocation);
	FMatrix GetRTMatrix() const { return RTMatrix; }

	FString GetText() const;
	void SetText(const FString& InText);

	bool bIsUUIDText = true;

private:
	FString Text;

	FMatrix RTMatrix;
	AActor* POwnerActor;
	float ZOffset;
};
