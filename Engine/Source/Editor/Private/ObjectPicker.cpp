#include "pch.h"

#ifdef MULTI_THREADING
#include "cpp-thread-pool/thread_pool.h"
#endif

#include "Component/Public/PrimitiveComponent.h"
#include "Core/Public/AppWindow.h"
#include "Editor/Public/Camera.h"
#include "Editor/Public/Gizmo.h"
#include "Editor/Public/ObjectPicker.h"
#include "Global/Quaternion.h"
#include "ImGui/imgui.h"
#include "Level/Public/Level.h"
#include "Manager/Input/Public/InputManager.h"
#include "Physics/Public/AABB.h"
#include "Manager/BVH/public/BVHManager.h"

FRay UObjectPicker::GetModelRay(const FRay& Ray, UPrimitiveComponent* Primitive) const
{
	FMatrix ModelInverse = Primitive->GetWorldTransformMatrixInverse();

	FRay ModelRay;
	ModelRay.Origin = Ray.Origin * ModelInverse;
	ModelRay.Direction = Ray.Direction * ModelInverse;
	ModelRay.Direction.Normalize();
	return ModelRay;
}

UPrimitiveComponent* UObjectPicker::PickPrimitive(const FRay& WorldRay, TArray<TObjectPtr<UPrimitiveComponent>> Candidate, float* OutDistance)
{
	UPrimitiveComponent* ShortestPrimitive = nullptr;
	float PrimitiveDistance = D3D11_FLOAT32_MAX;

	UBVHManager::GetInstance().Raycast(WorldRay, ShortestPrimitive, PrimitiveDistance);
	*OutDistance = PrimitiveDistance;

	return ShortestPrimitive;
}

void UObjectPicker::PickGizmo(UCamera* InActiveCamera, const FRay& WorldRay, UGizmo& Gizmo, FVector& CollisionPoint)
{
	//Forward, Right, Up순으로 테스트할거임.
	//원기둥 위의 한 점 P, 축 위의 임의의 점 A에(기즈모 포지션) 대해, AP벡터와 축 벡터 V와 피타고라스 정리를 적용해서 점 P의 축부터의 거리 r을 구할 수 있음.
	//r이 원기둥의 반지름과 같다고 방정식을 세운 후 근의공식을 적용해서 충돌여부 파악하고 distance를 구할 수 있음.

	//FVector4 PointOnCylinder = WorldRay.Origin + WorldRay.Direction * X;
	//dot(PointOnCylinder - GizmoLocation)*Dot(PointOnCylinder - GizmoLocation) - Dot(PointOnCylinder - GizmoLocation, GizmoAxis)^2 = r^2 = radiusOfGizmo
	//이 t에 대한 방정식을 풀어서 근의공식 적용하면 됨.

	FVector GizmoLocation = Gizmo.GetGizmoLocation();
	FVector GizmoAxises[3] = { FVector{1, 0, 0}, FVector{0, 1, 0}, FVector{0, 0, 1} };

	if (Gizmo.GetGizmoMode() == EGizmoMode::Scale || !Gizmo.IsWorldMode())
	{
		FVector Rad = FVector::GetDegreeToRadian(Gizmo.GetActorRotation());
		FMatrix R = FMatrix::RotationMatrix(Rad);
		//FQuaternion q = FQuaternion::FromEuler(Rad);

		for (int i = 0; i < 3; i++)
		{
			//GizmoAxises[a] = FQuaternion::RotateVector(q, GizmoAxises[a]); // 쿼터니언으로 축 회전
			//GizmoAxises[a].Normalize();
			const FVector4 a4(GizmoAxises[i].X, GizmoAxises[i].Y, GizmoAxises[i].Z, 0.0f);
			FVector4 rotated4 = a4 * R;
			FVector V(rotated4.X, rotated4.Y, rotated4.Z);
			V.Normalize();
			GizmoAxises[i] = V;
		}
	}

	FVector WorldRayOrigin{ WorldRay.Origin.X,WorldRay.Origin.Y ,WorldRay.Origin.Z };
	FVector WorldRayDirection(WorldRay.Direction.X, WorldRay.Direction.Y, WorldRay.Direction.Z);
	WorldRayDirection.Normalize();

	switch (Gizmo.GetGizmoMode())
	{
	case EGizmoMode::Translate:
	case EGizmoMode::Scale:
	{
		FVector GizmoDistanceVector = WorldRayOrigin - GizmoLocation;
		bool bIsCollide = false;

		float GizmoRadius = Gizmo.GetTranslateRadius();
		float GizmoHeight = Gizmo.GetTranslateHeight();
		float A, B, C; //Ax^2 + Bx + C의 ABC
		float X; //해
		float Det; //판별식
		//0 = forward 1 = Right 2 = UP

		for (int a = 0; a < 3; a++)
		{
			FVector GizmoAxis = GizmoAxises[a];
			A = 1 - static_cast<float>(pow(WorldRay.Direction.Dot3(GizmoAxis), 2));
			B = WorldRay.Direction.Dot3(GizmoDistanceVector) - WorldRay.Direction.Dot3(GizmoAxis) * GizmoDistanceVector.
				Dot(GizmoAxis); //B가 2의 배수이므로 미리 약분
			C = static_cast<float>(GizmoDistanceVector.Dot(GizmoDistanceVector) -
				pow(GizmoDistanceVector.Dot(GizmoAxis), 2)) - GizmoRadius * GizmoRadius;

			Det = B * B - A * C;
			if (Det >= 0) //판별식 0이상 => 근 존재. 높이테스트만 통과하면 충돌
			{
				X = (-B + sqrtf(Det)) / A;
				FVector PointOnCylinder = WorldRayOrigin + WorldRayDirection * X;
				float Height = (PointOnCylinder - GizmoLocation).Dot(GizmoAxis);
				if (Height <= GizmoHeight && Height >= 0) //충돌
				{
					CollisionPoint = PointOnCylinder;
					bIsCollide = true;

				}
				X = (-B - sqrtf(Det)) / A;
				PointOnCylinder = WorldRay.Origin + WorldRay.Direction * X;
				Height = (PointOnCylinder - GizmoLocation).Dot(GizmoAxis);
				if (Height <= GizmoHeight && Height >= 0)
				{
					CollisionPoint = PointOnCylinder;
					bIsCollide = true;
				}
				if (bIsCollide)
				{
					switch (a)
					{
					case 0:	Gizmo.SetGizmoDirection(EGizmoDirection::Forward);	return;
					case 1:	Gizmo.SetGizmoDirection(EGizmoDirection::Right);	return;
					case 2:	Gizmo.SetGizmoDirection(EGizmoDirection::Up);		return;
					}
				}
			}
		}
	} break;
	case EGizmoMode::Rotate:
	{
		for (int a = 0; a < 3; a++)
		{
			if (DoesRayIntersectPlane(WorldRay, GizmoLocation, GizmoAxises[a], CollisionPoint))
			{
				FVector RadiusVector = CollisionPoint - GizmoLocation;
				if (Gizmo.IsInRadius(RadiusVector.Length()))
				{
					switch (a)
					{
					case 0:	Gizmo.SetGizmoDirection(EGizmoDirection::Forward);	return;
					case 1:	Gizmo.SetGizmoDirection(EGizmoDirection::Right);	return;
					case 2:	Gizmo.SetGizmoDirection(EGizmoDirection::Up);		return;
					}
				}
			}
		}
	} break;
	default: break;
	}

	Gizmo.SetGizmoDirection(EGizmoDirection::None);
}

bool UObjectPicker::DoesRayIntersectPrimitive(
    UCamera* InActiveCamera,
    const FRay& InWorldRay,
    UPrimitiveComponent* InPrimitive,
    const FMatrix& InModelMatrix,
    float* OutShortestDistance)
{
    // --- Broad phase: world-space AABB vs world ray ---
    FVector AabbMin, AabbMax;
    InPrimitive->GetWorldAABB(AabbMin, AabbMax);
    const FAABB worldAABB(AabbMin, AabbMax);

    float AabbDist;
    if (!worldAABB.RaycastHit(InWorldRay, &AabbDist))
        return false;

    // --- Narrow phase: only if AABB hit ---
    return DoesRayIntersectPrimitive_MollerTrumbore(InWorldRay, InPrimitive, OutShortestDistance);
}

//개별 primitive와 ray 충돌 검사
bool UObjectPicker::DoesRayIntersectPrimitive_MollerTrumbore(const FRay& InWorldRay,
	UPrimitiveComponent* InPrimitive, float* OutShortestDistance) const
{
	const uint32 NumVertices = InPrimitive->GetNumVertices();
	const uint32 NumIndices = InPrimitive->GetNumIndices();

	const TArray<FNormalVertex>* Vertices = InPrimitive->GetVerticesData();
	const TArray<uint32>* Indices = InPrimitive->GetIndicesData();

	float Distance = D3D11_FLOAT32_MAX; //Distance 초기화
	bool bIsHit = false;

	const int32 NumTriangles = (NumIndices > 0) ? (NumIndices / 3) : (NumVertices / 3);

	for (int32 TriIndex = 0; TriIndex < NumTriangles; TriIndex++) //삼각형 단위로 Vertex 위치정보 읽음
	{
		FVector TriangleVertices[3];

		// 인덱스 버퍼 사용 여부에 따라 정점 구성
		if (Indices)
		{
			TriangleVertices[0] = (*Vertices)[(*Indices)[TriIndex * 3 + 0]].Position;
			TriangleVertices[1] = (*Vertices)[(*Indices)[TriIndex * 3 + 1]].Position;
			TriangleVertices[2] = (*Vertices)[(*Indices)[TriIndex * 3 + 2]].Position;
		}
		else
		{
			TriangleVertices[0] = (*Vertices)[TriIndex * 3 + 0].Position;
			TriangleVertices[1] = (*Vertices)[TriIndex * 3 + 1].Position;
			TriangleVertices[2] = (*Vertices)[TriIndex * 3 + 2].Position;
		}

		if (DoesRayIntersectTriangle(InWorldRay, InPrimitive, TriangleVertices[0],
			TriangleVertices[1], TriangleVertices[2], &Distance))
			//Ray와 삼각형이 충돌하면 거리 비교 후 최단거리 갱신
		{
			bIsHit = true;
			*OutShortestDistance = std::min(Distance, *OutShortestDistance);
		}
	}

	return bIsHit;
}

bool UObjectPicker::DoesRayIntersectTriangle(const FRay& InRay, UPrimitiveComponent* InPrimitive,
	const FVector& Vertex1, const FVector& Vertex2, const FVector& Vertex3, float* OutDistance) const
{
	FRay ModelRay = GetModelRay(InRay, InPrimitive);

    // Pack ray origin and dir
    __m128 rayO = _mm_setr_ps(ModelRay.Origin.X, ModelRay.Origin.Y, ModelRay.Origin.Z, 0.0f);
    __m128 rayD = _mm_setr_ps(ModelRay.Direction.X, ModelRay.Direction.Y, ModelRay.Direction.Z, 0.0f);

    // Pack vertices
    __m128 v0 = _mm_setr_ps(Vertex1.X, Vertex1.Y, Vertex1.Z, 0.0f);
    __m128 v1 = _mm_setr_ps(Vertex2.X, Vertex2.Y, Vertex2.Z, 0.0f);
    __m128 v2 = _mm_setr_ps(Vertex3.X, Vertex3.Y, Vertex3.Z, 0.0f);

    // E1 = v1 - v0
    __m128 e1 = _mm_sub_ps(v1, v0);
    // E2 = v2 - v0
    __m128 e2 = _mm_sub_ps(v2, v0);
    // Result = rayO - v0
    __m128 res = _mm_sub_ps(rayO, v0);

    // CrossE2Ray = E2 × RayDir
    __m128 e2_yzx = _mm_shuffle_ps(e2, e2, _MM_SHUFFLE(3,0,2,1));
    __m128 d_yzx  = _mm_shuffle_ps(rayD, rayD, _MM_SHUFFLE(3,0,2,1));
    __m128 crossE2Ray = _mm_sub_ps(_mm_mul_ps(e2, d_yzx), _mm_mul_ps(e2_yzx, rayD));
    crossE2Ray = _mm_shuffle_ps(crossE2Ray, crossE2Ray, _MM_SHUFFLE(3,0,2,1));

    // CrossE1Result = E1 × Result
    __m128 e1_yzx = _mm_shuffle_ps(e1, e1, _MM_SHUFFLE(3,0,2,1));
    __m128 res_yzx = _mm_shuffle_ps(res, res, _MM_SHUFFLE(3,0,2,1));
    __m128 crossE1Res = _mm_sub_ps(_mm_mul_ps(e1, res_yzx), _mm_mul_ps(e1_yzx, res));
    crossE1Res = _mm_shuffle_ps(crossE1Res, crossE1Res, _MM_SHUFFLE(3,0,2,1));

    // Determinant = dot(E1, CrossE2Ray)
    __m128 det4 = _mm_mul_ps(e1, crossE2Ray);
    __m128 det2 = _mm_hadd_ps(det4, det4);
    __m128 det1 = _mm_hadd_ps(det2, det2);
    float det = _mm_cvtss_f32(det1);

    if (fabsf(det) <= 1e-4f)
        return false;

    float invDet = 1.0f / det;

    // V = dot(Result, CrossE2Ray) / det
    __m128 v4 = _mm_mul_ps(res, crossE2Ray);
    __m128 v2_ = _mm_hadd_ps(v4, v4);
    __m128 v1_ = _mm_hadd_ps(v2_, v2_);
    float V = _mm_cvtss_f32(v1_) * invDet;
    if (V < 0.0f || V > 1.0f)
        return false;

    // U = dot(RayDir, CrossE1Result) / det
    __m128 u4 = _mm_mul_ps(rayD, crossE1Res);
    __m128 u2 = _mm_hadd_ps(u4, u4);
    __m128 u1 = _mm_hadd_ps(u2, u2);
    float U = _mm_cvtss_f32(u1) * invDet;
    if (U < 0.0f || U + V > 1.0f)
        return false;

    // T = dot(E2, CrossE1Result) / det
    __m128 t4 = _mm_mul_ps(e2, crossE1Res);
    __m128 t2 = _mm_hadd_ps(t4, t4);
    __m128 t1 = _mm_hadd_ps(t2, t2);
    float T = _mm_cvtss_f32(t1) * invDet;
    if (T < 0.0f)
        return false;

    *OutDistance = T;
    return true;
}

bool UObjectPicker::DoesRayIntersectPlane(const FRay& WorldRay, FVector PlanePoint, FVector Normal, FVector& PointOnPlane)
{
	FVector WorldRayOrigin{ WorldRay.Origin.X, WorldRay.Origin.Y ,WorldRay.Origin.Z };

	if (abs(WorldRay.Direction.Dot3(Normal)) < 0.01f)
		return false;

	float Distance = (PlanePoint - WorldRayOrigin).Dot(Normal) / WorldRay.Direction.Dot3(Normal);

	if (Distance < 0)
		return false;
	PointOnPlane = WorldRay.Origin + WorldRay.Direction * Distance;


	return true;
}
