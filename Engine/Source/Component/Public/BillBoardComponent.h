#pragma once
#include "Component/Public/PrimitiveComponent.h"
#include "Global/Matrix.h"

class AActor;

class UBillBoardComponent : public UPrimitiveComponent
{
public:
	UBillBoardComponent(AActor* InOwnerActor, float InYOffset);
	~UBillBoardComponent();

	virtual UObject* Duplicate(FObjectDuplicationParameters Parameters) override;

	void UpdateRotationMatrix(const FVector& InCameraLocation);

	FMatrix GetRTMatrix() const { return RTMatrix; }
private:
	FMatrix RTMatrix;
	/** @deprecated SetOwner, GetOwner 사용으로 대체 */
	[[maybe_unused]] [[deprecated]] AActor* POwnerActor;
	float ZOffset;
};
