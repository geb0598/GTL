#pragma once
#include "Core/Public/Object.h"
#include "Editor/Public/Gizmo.h"
#include "Editor/Public/Grid.h"
#include "Editor/public/Axis.h"
#include "Editor/Public/ObjectPicker.h"
#include "Editor/Public/BatchLines.h"
#include "Editor/Public/SplitterWindow.h"

class UPrimitiveComponent;
class FViewportClient;
class UCamera;
class ULevel;
class USplitterWidget;
struct FRay;

enum class EViewModeIndex : uint32
{
	VMI_Lit,
	VMI_Unlit,
	VMI_Wireframe,
};

class UEditor : public UObject
{
public:
	UEditor();
	~UEditor();

	void Update();
	void RenderEditor(UCamera* InCamera);

	void SetViewMode(EViewModeIndex InNewViewMode) { CurrentViewMode = InNewViewMode; }
	EViewModeIndex GetViewMode() const { return CurrentViewMode; }

private:
	void InitializeLayout();
	void UpdateLayout();

	void ProcessMouseInput(ULevel* InLevel);
	TArray<UPrimitiveComponent*> FindCandidatePrimitives(ULevel* InLevel);

	// 모든 기즈모 드래그 함수가 ActiveCamera를 받도록 통일
	FVector GetGizmoDragLocation(UCamera* InActiveCamera, FRay& WorldRay);
	FVector GetGizmoDragRotation(UCamera* InActiveCamera, FRay& WorldRay);
	FVector GetGizmoDragScale(UCamera* InActiveCamera, FRay& WorldRay);

	UObjectPicker ObjectPicker;

	const float MinScale = 0.01f;
	UGizmo Gizmo;
	UAxis Axis;
	UBatchLines BatchLines;

	SSplitterV RootSplitter;
	SSplitterH LeftSplitter;
	SSplitterH RightSplitter;
	SWindow ViewportWindows[4]; // 최종 뷰포트 영역의 정보, 쉽게 참조하도록 선언했습니다.
	SSplitter* DraggedSplitter = nullptr; // 드래그 상태를 추적하는 포인터
	FViewportClient* InteractionViewport = nullptr; // 뷰포트의 상호작용을 고정하는 포인터

	EViewModeIndex CurrentViewMode = EViewModeIndex::VMI_Lit;
};
