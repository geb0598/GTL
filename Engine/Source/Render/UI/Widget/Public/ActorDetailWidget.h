#pragma once
#include "Widget.h"
// #include "Actor/Public/Actor.h"
// #include "Component/Public/ActorComponent.h"
#include "Component/Public/TextRenderComponent.h"

class AActor;
class UActorComponent;
class USceneComponent;

/**
 * @brief 선택된 Actor의 이름과 컴포넌트 트리를 표시하는 Widget
 * Rename 기능이 추가되어 있음
 */
class UActorDetailWidget :
	public UWidget
{
public:
	void Initialize() override;
	void Update() override;
	void RenderWidget() override;

	// Special Member Function
	UActorDetailWidget();
	~UActorDetailWidget() override;

private:
	TObjectPtr<UActorComponent> SelectedComponent;
	bool bIsRenamingActor = false;
	char ActorNameBuffer[256] = {};

	// Helper functions
	void RenderActorHeader(TObjectPtr<AActor> InSelectedActor);
	void RenderComponentTree(TObjectPtr<AActor> InSelectedActor);
	void RenderComponentNode(TObjectPtr<UActorComponent> InComponent, USceneComponent* InRootComponent);
	void RenderComponentDetails(TObjectPtr<UActorComponent> InComponent);

	// 이름 변경 함수
	void StartRenamingActor(TObjectPtr<AActor> InActor);
	void FinishRenamingActor(TObjectPtr<AActor> InActor);
	void CancelRenamingActor();

	//액터 복제
	void DuplicateSelectedActor(TObjectPtr<AActor> InActor);
};

