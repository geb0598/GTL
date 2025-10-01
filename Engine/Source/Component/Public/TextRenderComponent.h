#pragma once
#include "Component/Public/PrimitiveComponent.h"
#include "Editor/Public/Camera.h"
#include "Global/Matrix.h"

class AActor;

UCLASS()

class UTextRenderComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UTextRenderComponent, UPrimitiveComponent)

public:
	UTextRenderComponent();
	UTextRenderComponent(AActor* InOwnerActor, float InYOffset);
	~UTextRenderComponent();

	void UpdateRotationMatrix(const FVector& InCameraLocation, const UCamera* InCamera);
	FMatrix GetRTMatrix() const { return RTMatrix; }

	FString GetText() const;
	void SetText(const FString& InText);

	bool bIsUUIDText = false;

private:
	FString Text;

	FMatrix RTMatrix;
	AActor* POwnerActor;
	float ZOffset;
};
