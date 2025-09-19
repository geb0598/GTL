#pragma once
#include "Global/Vector.h"
#include "Global/Matrix.h"
#include "Global/Types.h"
#include "Core/Public/Name.h"

//struct BatchLineContants
//{
//	float CellSize;
//	//FMatrix BoundingBoxModel;
//	uint32 ZGridStartIndex; // 인덱스 버퍼에서, z방향쪽 그리드가 시작되는 인덱스
//	uint32 BoundingBoxStartIndex; // 인덱스 버퍼에서, 바운딩박스가 시작되는 인덱스
//};

struct FViewProjConstants
{
	FViewProjConstants()
	{
		View = FMatrix::Identity();
		Projection = FMatrix::Identity();
	}

	FMatrix View;
	FMatrix Projection;
};

struct FVertex
{
	FVector Position;
	FVector4 Color;
};

struct FNormalVertex
{
	FVector Position;
	FVector Normal;
	FVector4 Color;
	FVector2 TexCoord;
};

struct FRay
{
	FVector4 Origin;
	FVector4 Direction;
};

/**
 * @brief Render State Settings for Actor's Component
 */
struct FRenderState
{
	ECullMode CullMode = ECullMode::None;
	EFillMode FillMode = EFillMode::Solid;
};

/**
 * @brief 변환 정보를 담는 구조체
 */
struct FTransform
{
	FVector Location = FVector(0.0f, 0.0f, 0.0f);
	FVector Rotation = FVector(0.0f, 0.0f, 0.0f);
	FVector Scale = FVector(1.0f, 1.0f, 1.0f);

	FTransform() = default;

	FTransform(const FVector& InLocation, const FVector& InRotation = FVector::ZeroVector(),
		const FVector& InScale = FVector::OneVector())
		: Location(InLocation), Rotation(InRotation), Scale(InScale)
	{
	}
};

struct FNormalVertex
{
	FVector Position;
	FVector Normal;
	FVector4 Color;
	FVector2 TexCoord;
};

// 렌더링에 필요한 표면 정보를 담는 구조체입니다.
// 실제로는 UMaterial과 같은 UObject 클래스로 만드는 것이 더 확장성 있습니다.
struct FMaterial
{
	//FName Name; // 재질 이름 (e.g., "SwordBlade_Metal")

	// 사용할 텍스처에 대한 참조 포인터
	//TObjectPtr<UTexture> BaseColorTexture;
	//TObjectPtr<UTexture> NormalTexture;
	//TObjectPtr<UTexture> SpecularTexture;
	// ... 기타 PBR 텍스처들 (Roughness, Metallic 등)

	// 텍스처가 없을 경우 사용할 기본 값들
	FVector4 BaseColorFactor = { 1.f, 1.f, 1.f, 1.f };
	float RoughnessFactor = 0.8f;
	float MetallicFactor = 0.f;
};

// FStaticMesh 내에서 특정 재질을 사용하는 인덱스 구간을 정의합니다.
struct FMeshSection
{
	// 이 섹션이 사용할 재질의 인덱스
	// (아래 FStaticMesh의 Materials 배열에 대한 인덱스)
	int32 MaterialIndex;

	// 이 섹션이 시작되는 전체 인덱스 버퍼의 위치
	uint32 FirstIndex;

	// 이 섹션을 구성하는 삼각형의 개수
	uint32 NumTriangles;
};

// Cooked Data
struct FStaticMesh
{
	FString PathFileName;

	TArray<FNormalVertex> Vertices;
	TArray<uint32> Indices;


	// GPU 메모리에 올라간 버퍼에 대한 포인터
	ID3D11Buffer* VertexBuffer = nullptr;
	ID3D11Buffer* IndexBuffer = nullptr;


	// --- 2. 재질 정보 (Materials) ---
	// 이 메시에 사용되는 모든 고유 재질의 목록 (페인트 팔레트)
	TArray<FMaterial> Materials;

	// --- 3. 연결 정보 (Sections) ---
	// 각 재질을 어떤 기하 구간에 칠할지에 대한 지시서
	TArray<FMeshSection> Sections;
};
