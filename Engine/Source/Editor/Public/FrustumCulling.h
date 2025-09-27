#pragma once

class UCamera;
struct  FAABB;

enum class EFrustumTestResult : uint8
{
	Outside,
	Inside,
	Intersection
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

class FFrustumCulling : public UObject
{
	DECLARE_CLASS(FFrustumCulling, UObject)

public:
	FFrustumCulling();
	~FFrustumCulling();

	void Update(UCamera* InViewProperties);
	EFrustumTestResult IsInFrustum(const FAABB& TargetAABB);

private:
	FPlane Planes[6];
};
