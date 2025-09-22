#pragma once
#include "Global/Types.h"
#include "Core/Public/Object.h"
#include "Core/Public/Name.h"
#include "Editor/Public/Viewport.h"

/**
 * @brief 뷰포트 클라이언트의 카메라 정보
 * @param Location 위치
 * @param Rotation 회전
 * @param FocusLotation 직교 투영 카메라가 바라볼 위치
 * @param FovY 원근 투영 시야각
 * @param FarClip  Far Z 값
 * @param NearClip Near Z 값
 * @param OrthoWidth 직교 투영 시야각
 * @param CameraType 카메라 모드
 */
struct FViewportCameraData
{
	FVector Location;
	FVector Rotation;
	FVector FocusLocation;
	float FovY;
	float FarClip;
	float NearClip;
	float OrthoWidth;
	EViewportCameraType ViewportCameraType;

	FViewportCameraData()
		:Location(-15, 0, 0), Rotation(0, 0, 0), FocusLocation(0, 0, 0), FovY(90.f),
		NearClip(0.1f), FarClip(100.0f), OrthoWidth(90.0f), ViewportCameraType(EViewportCameraType::Perspective) {}
};

UCLASS()
class UConfigManager : public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UConfigManager, UObject)

public:
	void SaveEditorSetting();
	void LoadEditorSetting();

	float GetCellSize() const
	{
		return CellSize;
	}

	float GetCameraSensitivity() const
	{
		return CameraSensitivity;
	}

	TArray<float> GetSplitterRatio() const
	{
		return TArray<float>({ RootSplitterRatio, LeftSplitterRatio, RightSplitterRatio });
	}

	const FViewportCameraData& GetViewportCameraData(int InIndex) const { return ViewportCameraSettings[InIndex]; }


	void SetCellSize(const float cellSize)
	{
		CellSize = cellSize;
	}

	void SetCameraSensitivity(const float cameraSensitivity)
	{
		CameraSensitivity = cameraSensitivity;
	}

	void SetSplitterRatio(const float RootRatio, const float LeftRatio, const float RightRatio)
	{
		RootSplitterRatio = RootRatio;
		LeftSplitterRatio = LeftRatio;
		RightSplitterRatio = RightRatio;
	}

	void SetViewportCameraData(int InIndex, const FViewportCameraData& InData) { ViewportCameraSettings[InIndex] = InData; }

private:
	// INI 파일 파싱을 위한 헬퍼 함수
	FVector StringToVector(const std::string& InString);
	std::string VectorToString(const FVector& InVector);

	FName EditorIniFileName;
	float CellSize;
	float CameraSensitivity;
	float RootSplitterRatio;
	float LeftSplitterRatio;
	float RightSplitterRatio;
	FViewportCameraData ViewportCameraSettings[4];
};
