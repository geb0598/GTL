#pragma once
#include "Widget.h"
#include "Global/Types.h"
#include "Component/Public/TextRenderComponent.h"

class AActor;
class UActorComponent;
class USceneComponent;
class UStaticMeshComponent;
class UStaticMeshComponentWidget;
class UTexture;

struct FBillboardSpriteOption
{
	FString DisplayName;
	FString FilePath;
	TObjectPtr<UTexture> Texture;
};

struct FTextureOption
{
	FString DisplayName;
	FString FilePath;
	TObjectPtr<UTexture> Texture;
};

/**
 * @brief 선택된 Actor의 이름과 컴포넌트 트리를 표시하는 Widget
 * Rename 기능이 추가되어 있음
 */
class UActorDetailWidget :
	public UWidget
{
public:
	// Special Member Function
	UActorDetailWidget();
	~UActorDetailWidget() override;

	void Initialize() override;
	void Update() override;
	void RenderWidget() override;

	// 캐시 데이터 로드 및 해제
	static void LoadAssets();
	static void ReleaseAssets();

private:
	TObjectPtr<UActorComponent> SelectedComponent;
	bool bIsRenamingActor = false;
	char ActorNameBuffer[256] = {};

	// Static mesh widget cache
	TMap<TObjectPtr<UStaticMeshComponent>, UStaticMeshComponentWidget*> StaticMeshWidgetMap;
	TObjectPtr<AActor> StaticMeshWidgetOwner = nullptr;

	// billboard cache
	static TArray<FBillboardSpriteOption> BillboardSpriteOptions;

	// decal cache
	static TArray<FTextureOption> DecalTextureOptions;

	static bool bAssetsLoaded;

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
};
