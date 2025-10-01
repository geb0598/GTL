#pragma once
#include "Widget.h"
#include "Global/Types.h"
// #include "Actor/Public/Actor.h"
// #include "Component/Public/ActorComponent.h"
#include "Component/Public/TextRenderComponent.h"

class AActor;
class UActorComponent;
class USceneComponent;
class UStaticMeshComponent;
class UStaticMeshComponentWidget;

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

	FString GenerateUniqueComponentName(AActor* InActor, const FString& InBaseName);

	// Static mesh component widget helpers
	UStaticMeshComponentWidget* GetOrCreateStaticMeshWidget(UStaticMeshComponent* InComponent);
	void ResetStaticMeshWidgetCache();
	void PruneInvalidStaticMeshWidgets(const TArray<TObjectPtr<UActorComponent>>& InComponents);

	//액터 복제
	void DuplicateSelectedActor(TObjectPtr<AActor> InActor);

	// Static mesh widget cache
	TMap<TObjectPtr<UStaticMeshComponent>, UStaticMeshComponentWidget*> StaticMeshWidgetMap;
	TObjectPtr<AActor> StaticMeshWidgetOwner = nullptr;
};
