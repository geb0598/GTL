#include "pch.h"
#include "Component/Public/BillBoardComponent.h"
#include "Editor/Public/Editor.h"
#include "Actor/Public/Actor.h"

/**
 * @brief Level에서 각 Actor마다 가지고 있는 UUID를 출력해주기 위한 빌보드 클래스
 * Actor has a UBillBoardComponent
 */
UBillBoardComponent::UBillBoardComponent(AActor* InOwnerActor, float InYOffset)
	: ZOffset(InYOffset)
{
	SetOwner(InOwnerActor);
	Type = EPrimitiveType::BillBoard;
}

UBillBoardComponent::~UBillBoardComponent()
{
}

UObject* UBillBoardComponent::Duplicate(FObjectDuplicationParameters Parameters)
{
	auto DupObject = static_cast<UBillBoardComponent*>(Super::Duplicate(Parameters));

	DupObject->RTMatrix = RTMatrix;
	DupObject->ZOffset = ZOffset;

	return DupObject;
}

void UBillBoardComponent::UpdateRotationMatrix(const FVector& InCameraLocation)
{
	const FVector& OwnerActorLocation = GetOwner()->GetActorLocation();

	FVector ToCamera = FVector(
		InCameraLocation.X - OwnerActorLocation.X,
		InCameraLocation.Y - OwnerActorLocation.Y,
		InCameraLocation.Z - OwnerActorLocation.Z
	);

	const float len = ToCamera.Length();
	if (len > 1e-6f)
	{
		ToCamera.X /= len;
		ToCamera.Y /= len;
		ToCamera.Z /= len;
	}

	const FVector worldUp = FVector(0.0f, 0.0f, 1.0f);
	FVector Right = worldUp.Cross(ToCamera);

	const float rightLen = Right.Length();
	if (rightLen > 1e-6f)
	{
		Right.X /= rightLen;
		Right.Y /= rightLen;
		Right.Z /= rightLen;
	}

	FVector Up = ToCamera.Cross(Right);
	const float upLen = Up.Length();
	if (upLen > 1e-6f)
	{
		Up.X /= upLen;
		Up.Y /= upLen;
		Up.Z /= upLen;
	}

	RTMatrix = FMatrix(ToCamera, Right, Up);

	const FVector Translation = FVector(
		OwnerActorLocation.X,
		OwnerActorLocation.Y,
		OwnerActorLocation.Z + ZOffset
	);

	RTMatrix *= FMatrix::TranslationMatrix(Translation);
}
