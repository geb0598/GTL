#include "pch.h"
#include "Render/UI/Widget/Public/StaticMeshComponentWidget.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Mesh/Public/StaticMesh.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Level/Public/Level.h"
#include "Core/Public/ObjectIterator.h"

IMPLEMENT_CLASS(UStaticMeshComponentWidget, UWidget)

void UStaticMeshComponentWidget::RenderWidget()
{
	TObjectPtr<ULevel> CurrentLevel = ULevelManager::GetInstance().GetCurrentLevel();

	if (!CurrentLevel)
	{
		ImGui::TextUnformatted("No Level Loaded");
		return;
	}

	TObjectPtr<AActor> SelectedActor = CurrentLevel->GetSelectedActor();
	if (!SelectedActor)
	{
		ImGui::TextUnformatted("No Object Selected");
		return;
	}

	for (const TObjectPtr<UActorComponent>& Component : SelectedActor->GetOwnedComponents())
	{
		StaticMeshComponent = Cast<UStaticMeshComponent>(Component);

		// 위젯이 편집해야 할 대상 컴포넌트가 유효한지 확인합니다.
		if (StaticMeshComponent)
		{
			break;
		}
	}

	if (!StaticMeshComponent)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Target Component is not valid.");
		return;
	}

	// 1. 현재 컴포넌트에 할당된 스태틱 메시를 가져옵니다.
	UStaticMesh* CurrentStaticMesh = StaticMeshComponent->GetStaticMesh();
	const FString PreviewName = CurrentStaticMesh ? CurrentStaticMesh->GetAssetPathFileName().data() : "None";

	// 2. ImGui::BeginCombo를 사용하여 드롭다운 메뉴를 시작합니다.
	// 첫 번째 인자는 라벨, 두 번째 인자는 닫혀 있을 때 표시될 텍스트입니다.
	if (ImGui::BeginCombo("Static Mesh", PreviewName.c_str()))
	{
		// 3. TObjectIterator로 모든 UStaticMesh 에셋을 순회합니다.
		for (TObjectIterator<UStaticMesh> It; It; ++It)
		{
			UStaticMesh* MeshInList = *It;
			if (!MeshInList) continue;

			// 현재 선택된 항목인지 확인합니다.
			const bool bIsSelected = (CurrentStaticMesh == MeshInList);

			// 4. ImGui::Selectable로 각 항목을 만듭니다.
			// 사용자가 이 항목을 클릭하면 if문이 true가 됩니다.
			if (ImGui::Selectable(MeshInList->GetAssetPathFileName().data(), bIsSelected))
			{
				// 5. 항목이 선택되면, 컴포넌트의 스태틱 메시를 교체합니다.
				StaticMeshComponent->SetStaticMesh(MeshInList->GetAssetPathFileName());
			}

			// 현재 선택된 항목에 포커스를 맞춰서 드롭다운이 열렸을 때 바로 보이게 합니다.
			if (bIsSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}

		// 6. 드롭다운 메뉴를 닫습니다.
		ImGui::EndCombo();
	}
}
