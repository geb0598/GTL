#pragma once
#include "Editor/Public/Gizmo.h"

class UPrimitiveComponent;
class AActor;
class ULevel;
class UCamera;
class UGizmo;
struct FRay;

class UObjectPicker : public UObject
{
public:
	UObjectPicker() = default;
	UPrimitiveComponent* PickPrimitive(UCamera* InActiveCamera, const FRay& WorldRay, TArray<UPrimitiveComponent*> Candidate, float* OutDistance);
	void PickGizmo(UCamera* InActiveCamera, const FRay& WorldRay, UGizmo& Gizmo, FVector& CollisionPoint);
	bool DoesRayIntersectPlane(const FRay& WorldRay, FVector PlanePoint, FVector Normal, FVector& PointOnPlane);

private:
	bool DoesRayIntersectPrimitive(UCamera* InActiveCamera, const FRay& InWorldRay, UPrimitiveComponent* InPrimitive, const FMatrix& InModelMatrix, float* OutShortestDistance);
	bool DoesRayIntersectPrimitive_MollerTrumbore(UCamera* InActiveCamera, const FRay& InModelRay, UPrimitiveComponent* InPrimitive, const FMatrix& InModelMatrix, float* OutShortestDistance);
	FRay GetModelRay(const FRay& Ray, UPrimitiveComponent* Primitive);
	bool DoesRayIntersectTriangle(UCamera* InActiveCamera, const FRay& InRay, const FVector& Vertex1, const FVector& Vertex2, const FVector& Vertex3,
		const FMatrix& ModelMatrix, float* OutDistance);
};
