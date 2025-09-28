#pragma once

class UCamera;
struct  FAABB;

enum class EFrustumPlaneState : uint32
{
	Outside		= 0b00,
	Intersect	= 0b01,
	Inside		= 0b10,

	// 상태 추출용
	PlaneMask	= 0b11
};

// 평면의 비트 표현
enum class EFrustumPlaneIndex : uint32
{
	Left	= 0,
	Right	= 2,
	Bottom	= 4,
	Top		= 6,
	Near	= 8,
	Far 	= 10
};

enum class EFrustumTestResult : uint8
{
	Outside,
	Intersect,
	Inside
};

struct FPlane
{
	// Ax + By + Cz + D = 0
	// {A, B, C}
	FVector NormalVector;
	// D
	float ConstantD;
	FPlane() : NormalVector(FVector::Zero()), ConstantD(0.0f) {}
	FPlane(const FVector& Normal, const float Distance) : NormalVector(Normal), ConstantD(Distance) {}

	void Normalize()
	{
		float Magnitude = NormalVector.Length();
		if (Magnitude > 1.0e-6f)
		{
			NormalVector *=  (1.0f / Magnitude);
			ConstantD /= Magnitude;
		}
	}
};

struct FFrustum
{
	FPlane FarPlane;
	FPlane NearPlane;
	FPlane LeftPlane;
	FPlane RightPlane;
	FPlane TopPlane;
	FPlane BottomPlane;
};

class FFrustumCull : public UObject
{
	DECLARE_CLASS(FFrustumCull, UObject)

public:
	FFrustumCull();
	~FFrustumCull();

	void Update(UCamera* InCamera);
	EFrustumTestResult IsInFrustum(const FAABB& TargetAABB, uint32 Mask);

private:
	FPlane Planes[6];
};
