#pragma once

class UCamera;
struct  FAABB;

// 평면의 비트 표현
enum class EFrustumPlane : uint32
{
	Left	= 1 << 0, // 0b000001
	Right	= 1 << 1, // 0b000010
	Bottom	= 1 << 2, // 0b000100
	Top		= 1 << 3, // 0b001000
	Near	= 1 << 4, // 0b010000
	Far 	= 1 << 5, // 0b100000

	// 모든 평면 검사용 마스크 값
	All = 0x3F		  // 0b111111
};

enum class EPlaneIndex : uint8
{
	Left,
	Right,
	Bottom,
	Top,
	Near,
	Far
};

enum class EFrustumTestResult : uint8
{
	Outside,
	Inside,
	CompletelyOutside,
	Intersect,
	CompletelyInside
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
	EFrustumTestResult IsInFrustum(const FAABB& TargetAABB);
	const EFrustumTestResult TestAABBWithPlane(const FAABB& TargetAABB, const EPlaneIndex Index);

	FPlane& GetPlane(EPlaneIndex Index) { return Planes[static_cast<uint8>(Index)]; }

private:
	EFrustumTestResult CheckPlane(const FAABB& TargetAABB,  EPlaneIndex Index);

private:
	// 순서대로 left, right, bottom, top, near, far
	FPlane Planes[6];
};
