#include "pch.h"
#include "Component/Public/BillboardComponent.h"

UBillboardComponent::UBillboardComponent(AActor* InOwnerActor)
{
	SetName("BillboardComponent");
	Type = EPrimitiveType::Billboard;
}

UBillboardComponent::~UBillboardComponent()
{

}

void UBillboardComponent::UpdateRotationMatrix(const UCamera* InCamera)
{
	const FVector& OwnerActorLocation = POwnerActor->GetActorLocation();

	FVector ToCamera = InCamera->GetForward();
	ToCamera = FVector(-ToCamera.X, -ToCamera.Y, -ToCamera.Z);

	const FVector4 worldUp4 = FVector4(0, 0, 1, 1);
	const FVector worldUp = { worldUp4.X, worldUp4.Y, worldUp4.Z };
	FVector Right = worldUp.Cross(ToCamera);
	Right.Normalize();
	FVector Up = ToCamera.Cross(Right);
	Up.Normalize();

	RTMatrix = FMatrix(FVector4(0, 1, 0, 1), worldUp4, FVector4(1,0,0,1));
	RTMatrix = FMatrix(ToCamera, Right, Up);

	const FVector Translation = OwnerActorLocation + FVector(0.0f, 0.0f, ZOffset);
	RTMatrix *= FMatrix::TranslationMatrix(Translation);
}
