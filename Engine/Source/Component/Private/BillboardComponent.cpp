#include "pch.h"
#include "Component/Public/BillboardComponent.h"

IMPLEMENT_CLASS(UBillboardComponent, UPrimitiveComponent)

#include "Actor/Public/Actor.h"


UBillboardComponent::UBillboardComponent()
    : Sprite(nullptr)
    , POwnerActor(nullptr)
{
    SetName("BillboardComponent");
    Type = EPrimitiveType::Billboard;
}

UBillboardComponent::UBillboardComponent(AActor* InOwnerActor)
    : Sprite(nullptr)
    , POwnerActor(InOwnerActor)
{
    SetName("BillboardComponent");
    Type = EPrimitiveType::Billboard;
}

UBillboardComponent::~UBillboardComponent()
{
    Sprite = nullptr;
    POwnerActor = nullptr;
}

void UBillboardComponent::SetSprite(UTexture* InTexture)
{
    Sprite = InTexture;
}

void UBillboardComponent::UpdateRotationMatrix(const UCamera* InCamera)
{
	const FVector& OwnerActorLocation = GetOwner()->GetActorLocation();
    if (!InCamera)
    {
        return;
    }

    AActor* OwnerActor = POwnerActor ? POwnerActor : GetOwner();
    if (!OwnerActor)
    {
        return;
    }


    FVector ToCamera = InCamera->GetForward();
    ToCamera = FVector(-ToCamera.X, -ToCamera.Y, -ToCamera.Z);

    const FVector4 WorldUp4 = FVector4(0, 0, 1, 1);
    const FVector WorldUp = { WorldUp4.X, WorldUp4.Y, WorldUp4.Z };
    FVector Right = WorldUp.Cross(ToCamera);
    Right.Normalize();
    FVector Up = ToCamera.Cross(Right);
    Up.Normalize();

    RTMatrix = FMatrix(ToCamera, Right, Up);

    const FVector Translation = OwnerActorLocation + GetRelativeLocation();
    RTMatrix *= FMatrix::TranslationMatrix(Translation);
}
