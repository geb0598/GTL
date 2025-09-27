#include "pch.h"


FMatrix FMatrix::UEToDx = FMatrix(
	{
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f,
	});

FMatrix FMatrix::DxToUE = FMatrix(
	{
		0.0f, 0.0f, 1.0f, 0.0f,
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f,
	});

/**
* @brief float 타입의 배열을 사용한 FMatrix의 기본 생성자
*/
FMatrix::FMatrix()
	: Data{ {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0} }
{
}


/**
* @brief float 타입의 param을 사용한 FMatrix의 기본 생성자
*/
FMatrix::FMatrix(
	float M00, float M01, float M02, float M03,
	float M10, float M11, float M12, float M13,
	float M20, float M21, float M22, float M23,
	float M30, float M31, float M32, float M33)
	: Data{ {M00,M01,M02,M03},
			{M10,M11,M12,M13},
			{M20,M21,M22,M23},
			{M30,M31,M32,M33} }
{
}

FMatrix::FMatrix(const FVector& x, const FVector& y, const FVector& z)
	:Data{ {x.X, x.Y, x.Z, 0.0f},
			{y.X, y.Y, y.Z, 0.0f},
			{z.X, z.Y, z.Z, 0.0f},
			{0.0f, 0.0f, 0.0f, 1.0f} }
{
}

FMatrix::FMatrix(const FVector4& x, const FVector4& y, const FVector4& z)
	:Data{ {x.X,x.Y,x.Z, x.W},
			{y.X,y.Y,y.Z,y.W},
			{z.X,z.Y,z.Z,z.W},
			{0.0f, 0.0f, 0.0f, 1.0f} }
{
}

/**
* @brief 항등행렬
*/
FMatrix FMatrix::Identity()
{
	return FMatrix(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
}


/**
* @brief 두 행렬곱을 진행한 행렬을 반환하는 연산자 함수
*/
FMatrix FMatrix::operator*(const FMatrix& InOtherMatrix)
{
	FMatrix Result;

	for (int i = 0; i < 4; ++i)
	{
		// Load the row from the left matrix (this)
		__m128 a = _mm_loadu_ps(Data[i]); // row i

		// Broadcast each element of row 'i'
		__m128 a0 = _mm_shuffle_ps(a, a, _MM_SHUFFLE(0,0,0,0));
		__m128 a1 = _mm_shuffle_ps(a, a, _MM_SHUFFLE(1,1,1,1));
		__m128 a2 = _mm_shuffle_ps(a, a, _MM_SHUFFLE(2,2,2,2));
		__m128 a3 = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3,3,3,3));

		// Multiply each broadcast with the corresponding row of the right matrix
		__m128 r0 = _mm_mul_ps(a0, _mm_loadu_ps(InOtherMatrix.Data[0]));
		__m128 r1 = _mm_mul_ps(a1, _mm_loadu_ps(InOtherMatrix.Data[1]));
		__m128 r2 = _mm_mul_ps(a2, _mm_loadu_ps(InOtherMatrix.Data[2]));
		__m128 r3 = _mm_mul_ps(a3, _mm_loadu_ps(InOtherMatrix.Data[3]));

		// Sum them together
		__m128 res = _mm_add_ps(_mm_add_ps(r0, r1), _mm_add_ps(r2, r3));

		// Store into Result row
		_mm_storeu_ps(Result.Data[i], res);
	}
	return Result;
}

void FMatrix::operator*=(const FMatrix& InOtherMatrix)
{
	*this = (*this) * InOtherMatrix;
}

/**
* @brief Position의 정보를 행렬로 변환하여 제공하는 함수
*/
FMatrix FMatrix::TranslationMatrix(const FVector& InOtherVector)
{
	FMatrix Result = FMatrix::Identity();
	Result.Data[3][0] = InOtherVector.X;
	Result.Data[3][1] = InOtherVector.Y;
	Result.Data[3][2] = InOtherVector.Z;
	Result.Data[3][3] = 1;

	return Result;
}

FMatrix FMatrix::TranslationMatrixInverse(const FVector& InOtherVector)
{
	FMatrix Result = FMatrix::Identity();
	Result.Data[3][0] = -InOtherVector.X;
	Result.Data[3][1] = -InOtherVector.Y;
	Result.Data[3][2] = -InOtherVector.Z;
	Result.Data[3][3] = 1;

	return Result;
}

/**
* @brief Scale의 정보를 행렬로 변환하여 제공하는 함수
*/
FMatrix FMatrix::ScaleMatrix(const FVector& InOtherVector)
{
	FMatrix Result = FMatrix::Identity();
	Result.Data[0][0] = InOtherVector.X;
	Result.Data[1][1] = InOtherVector.Y;
	Result.Data[2][2] = InOtherVector.Z;
	Result.Data[3][3] = 1;

	return Result;
}
FMatrix FMatrix::ScaleMatrixInverse(const FVector& InOtherVector)
{
	FMatrix Result = FMatrix::Identity();
	Result.Data[0][0] = 1 / InOtherVector.X;
	Result.Data[1][1] = 1 / InOtherVector.Y;
	Result.Data[2][2] = 1 / InOtherVector.Z;
	Result.Data[3][3] = 1;

	return Result;
}

/**
* @brief Rotation의 정보를 행렬로 변환하여 제공하는 함수
*/
FMatrix FMatrix::RotationMatrix(const FVector& InOtherVector)
{
	// Dx11 yaw(y), pitch(x), roll(z)
	// UE yaw(z), pitch(y), roll(x)
	// 회전 축이 바뀌어서 각 회전행렬 함수에 바뀐 값을 적용

	const float yaw = InOtherVector.Y;
	const float pitch = InOtherVector.X;
	const float roll = InOtherVector.Z;
	//return RotationZ(yaw) * RotationY(pitch) * RotationX(roll);
	//return RotationX(yaw) * RotationY(roll) * RotationZ(pitch);
	return RotationX(pitch) * RotationY(yaw) * RotationZ(roll);
}

FMatrix FMatrix::CreateFromYawPitchRoll(const float yaw, const float pitch, const float roll)
{
	//return RotationZ(yaw) * RotationY(pitch)* RotationX(roll);
	return RotationX(pitch) * RotationY(yaw) * RotationZ(roll);
}

FMatrix FMatrix::RotationMatrixInverse(const FVector& InOtherVector)
{
	const float yaw = InOtherVector.Y;
	const float pitch = InOtherVector.X;
	const float roll = InOtherVector.Z;

	return RotationZ(-roll) * RotationY(-yaw) * RotationX(-pitch);
}

/**
* @brief X의 회전 정보를 행렬로 변환
*/
FMatrix FMatrix::RotationX(float Radian)
{
	FMatrix Result = FMatrix::Identity();
	const float C = std::cosf(Radian);
	const float S = std::sinf(Radian);

	Result.Data[1][1] = C;
	Result.Data[1][2] = S;
	Result.Data[2][1] = -S;
	Result.Data[2][2] = C;

	return Result;
}

/**
* @brief Y의 회전 정보를 행렬로 변환
*/
FMatrix FMatrix::RotationY(float Radian)
{
	FMatrix Result = FMatrix::Identity();
	const float C = std::cosf(Radian);
	const float S = std::sinf(Radian);

	Result.Data[0][0] = C;
	Result.Data[0][2] = -S;
	Result.Data[2][0] = S;
	Result.Data[2][2] = C;

	return Result;
}

/**
* @brief Y의 회전 정보를 행렬로 변환
*/
FMatrix FMatrix::RotationZ(float Radian)
{
	FMatrix Result = FMatrix::Identity();
	const float C = std::cosf(Radian);
	const float S = std::sinf(Radian);

	Result.Data[0][0] = C;
	Result.Data[0][1] = S;
	Result.Data[1][0] = -S;
	Result.Data[1][1] = C;

	return Result;
}

//
FMatrix FMatrix::GetModelMatrix(const FVector& Location, const FVector& Rotation, const FVector& Scale)
{
	FMatrix T = TranslationMatrix(Location);
	FMatrix R = RotationMatrix(Rotation);
	FMatrix S = ScaleMatrix(Scale);
	FMatrix modelMatrix = S * R * T;

	// Dx11 y-up 왼손좌표계에서 정의된 물체의 정점을 UE z-up 왼손좌표계로 변환
	return  FMatrix::UEToDx * modelMatrix;
}

FMatrix FMatrix::GetModelMatrixInverse(const FVector& Location, const FVector& Rotation, const FVector& Scale)
{
	FMatrix T = TranslationMatrixInverse(Location);
	FMatrix R = RotationMatrixInverse(Rotation);
	FMatrix S = ScaleMatrixInverse(Scale);
	FMatrix modelMatrixInverse = T * R * S;

	// UE 좌표계로 변환된 물체의 정점을 원래의 Dx 11 왼손좌표계 정점으로 변환
	return modelMatrixInverse * FMatrix::DxToUE;
}

FVector4 FMatrix::VectorMultiply(const FVector4& v, const FMatrix& m)
{
	FVector4 out;

	// Load the vector [X,Y,Z,W]
	__m128 vec = _mm_loadu_ps(&v.X);

	// Broadcast each component
	__m128 vx = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(0,0,0,0)); // X
	__m128 vy = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(1,1,1,1)); // Y
	__m128 vz = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(2,2,2,2)); // Z
	__m128 vw = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(3,3,3,3)); // W

	// Multiply with each row of the matrix
	__m128 r0 = _mm_mul_ps(vx, _mm_loadu_ps(m.Data[0])); // X * row0
	__m128 r1 = _mm_mul_ps(vy, _mm_loadu_ps(m.Data[1])); // Y * row1
	__m128 r2 = _mm_mul_ps(vz, _mm_loadu_ps(m.Data[2])); // Z * row2
	__m128 r3 = _mm_mul_ps(vw, _mm_loadu_ps(m.Data[3])); // W * row3

	// Sum them together
	__m128 res = _mm_add_ps(_mm_add_ps(r0, r1), _mm_add_ps(r2, r3));

	// Store result
	_mm_storeu_ps(&out.X, res);

	return out;
}

FVector FMatrix::VectorMultiply(const FVector& v, const FMatrix& m)
{
	FVector out;

	// Load vector [X, Y, Z, 0]
	__m128 vec = _mm_set_ps(0.0f, v.Z, v.Y, v.X);

	// Broadcast each component
	__m128 vx = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(0,0,0,0)); // X
	__m128 vy = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(1,1,1,1)); // Y
	__m128 vz = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(2,2,2,2)); // Z

	// Multiply with the first 3 rows of the matrix
	__m128 r0 = _mm_mul_ps(vx, _mm_loadu_ps(m.Data[0])); // X * row0
	__m128 r1 = _mm_mul_ps(vy, _mm_loadu_ps(m.Data[1])); // Y * row1
	__m128 r2 = _mm_mul_ps(vz, _mm_loadu_ps(m.Data[2])); // Z * row2

	// Sum them
	__m128 res = _mm_add_ps(_mm_add_ps(r0, r1), r2);

	// Store result (only XYZ matter)
	_mm_storeu_ps(&out.X, res);

	return out;
}

FMatrix FMatrix::Transpose() const
{
	FMatrix out;

	__m128 row0 = _mm_loadu_ps(Data[0]);
	__m128 row1 = _mm_loadu_ps(Data[1]);
	__m128 row2 = _mm_loadu_ps(Data[2]);
	__m128 row3 = _mm_loadu_ps(Data[3]);

	_MM_TRANSPOSE4_PS(row0, row1, row2, row3);

	_mm_storeu_ps(out.Data[0], row0);
	_mm_storeu_ps(out.Data[1], row1);
	_mm_storeu_ps(out.Data[2], row2);
	_mm_storeu_ps(out.Data[3], row3);

	return out;
}

