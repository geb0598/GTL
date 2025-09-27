#include "pch.h"
#include <immintrin.h>
#include "Editor/Public/FrustumCulling.h"
#include "Physics/Public/AABB.h"
#include "Editor/Public/Camera.h"


IMPLEMENT_CLASS(FFrustumCulling, UObject)

FFrustumCulling::FFrustumCulling()
{
}
FFrustumCulling::~FFrustumCulling()
{
}

void FFrustumCulling::Update(UCamera* InCamera)
{
	FMatrix ViewMatrix = InCamera->GetFViewProjConstants().View;
	FMatrix ProjMatrix =  InCamera->GetFViewProjConstants().Projection;
	FMatrix ViewProjMatrix = ViewMatrix * ProjMatrix;

	// left
	Planes[0].NormalVector.X = ViewProjMatrix.Data[0][3] + ViewProjMatrix.Data[0][0];
	Planes[0].NormalVector.Y = ViewProjMatrix.Data[1][3] + ViewProjMatrix.Data[1][0];
	Planes[0].NormalVector.Z = ViewProjMatrix.Data[2][3] + ViewProjMatrix.Data[2][0];
	Planes[0].ConstantD = ViewProjMatrix.Data[3][3] + ViewProjMatrix.Data[3][0];

	// right
	Planes[1].NormalVector.X = ViewProjMatrix.Data[0][3] - ViewProjMatrix.Data[0][0];
	Planes[1].NormalVector.Y = ViewProjMatrix.Data[1][3] - ViewProjMatrix.Data[1][0];
	Planes[1].NormalVector.Z = ViewProjMatrix.Data[2][3] - ViewProjMatrix.Data[2][0];
	Planes[1].ConstantD = ViewProjMatrix.Data[3][3] - ViewProjMatrix.Data[3][0];

	// bottom
	Planes[2].NormalVector.X = ViewProjMatrix.Data[0][3] + ViewProjMatrix.Data[0][1];
	Planes[2].NormalVector.Y = ViewProjMatrix.Data[1][3] + ViewProjMatrix.Data[1][1];
	Planes[2].NormalVector.Z = ViewProjMatrix.Data[2][3] + ViewProjMatrix.Data[2][1];
	Planes[2].ConstantD = ViewProjMatrix.Data[3][3] + ViewProjMatrix.Data[3][1];

	// top
	Planes[3].NormalVector.X = ViewProjMatrix.Data[0][3] - ViewProjMatrix.Data[0][1];
	Planes[3].NormalVector.Y = ViewProjMatrix.Data[1][3] - ViewProjMatrix.Data[1][1];
	Planes[3].NormalVector.Z = ViewProjMatrix.Data[2][3] - ViewProjMatrix.Data[2][1];
	Planes[3].ConstantD = ViewProjMatrix.Data[3][3] - ViewProjMatrix.Data[3][1];

	// near
	Planes[4].NormalVector.X = ViewProjMatrix.Data[0][2];
	Planes[4].NormalVector.Y = ViewProjMatrix.Data[1][2];
	Planes[4].NormalVector.Z = ViewProjMatrix.Data[2][2];
	Planes[4].ConstantD = ViewProjMatrix.Data[3][2];

	// far
	Planes[5].NormalVector.X = ViewProjMatrix.Data[0][3] - ViewProjMatrix.Data[0][2];
	Planes[5].NormalVector.Y = ViewProjMatrix.Data[1][3] - ViewProjMatrix.Data[1][2];
	Planes[5].NormalVector.Z = ViewProjMatrix.Data[2][3] - ViewProjMatrix.Data[2][2];
	Planes[5].ConstantD = ViewProjMatrix.Data[3][3] - ViewProjMatrix.Data[3][2];

	for (auto& Plane : Planes)
	{
		Plane.Normalize();
	}
}

EFrustumTestResult FFrustumCulling::IsInFrustum(const FAABB& TargetAABB)
{
	// TODO : BVH 적용 시 교차 검사 필요
	// 교차 시 leaf이면 rendering, 아니면 다음 노드 탐색
	for (int i =0; i < 6; i++)
	{
		const FPlane& Plane = Planes[i];

		FVector NegativeVector{};
		NegativeVector.X = (Plane.NormalVector.X >= 0.0f) ? TargetAABB.Min.X : TargetAABB.Max.X;
		NegativeVector.Y = (Plane.NormalVector.Y >= 0.0f) ? TargetAABB.Min.Y : TargetAABB.Max.Y;
		NegativeVector.Z = (Plane.NormalVector.Z >= 0.0f) ? TargetAABB.Min.Z : TargetAABB.Max.Z;

		float Distance = (Plane.NormalVector.X * NegativeVector.X)
						+ (Plane.NormalVector.Y * NegativeVector.Y)
						+ (Plane.NormalVector.Z * NegativeVector.Z)
						+ Plane.ConstantD;
		if (Distance < 0.0f)
		{
			// Invisible
			return EFrustumTestResult::Outside;
		}

	}
	// Visible
	return EFrustumTestResult::Inside;
}
