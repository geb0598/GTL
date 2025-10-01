#pragma once
#include "PrimitiveComponent.h"
#include "Editor/Public/Camera.h"
#include "Texture/Public/Texture.h"

class AActor;

UCLASS()

class UBillboardComponent : public UPrimitiveComponent
{
    GENERATED_BODY()
    DECLARE_CLASS(UBillboardComponent, UPrimitiveComponent)

public:
    UBillboardComponent();
    explicit UBillboardComponent(AActor* InOwnerActor);
    ~UBillboardComponent();

    FMatrix GetRTMatrix() const { return RTMatrix; }

    void UpdateRotationMatrix(const UCamera* InCamera);

    UTexture* GetSprite() const { return Sprite.Get(); }
    void SetSprite(UTexture* InTexture);

private:
    FMatrix RTMatrix;
    TObjectPtr<UTexture> Sprite;
    AActor* POwnerActor;
};
