#include "pch.h"
#include "Render/UI/Widget/Public/StaticMeshComponentWidget.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Mesh/Public/StaticMesh.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Level/Public/Level.h"
#include "Core/Public/ObjectIterator.h"
#include "Texture/Public/Material.h"
#include "Texture/Public/Texture.h"

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

	RenderStaticMeshSelector();
	ImGui::Separator();
	RenderMaterialSections();
}

void UStaticMeshComponentWidget::RenderStaticMeshSelector()
{
	// 1. 현재 컴포넌트에 할당된 스태틱 메시를 가져옵니다.
	UStaticMesh* CurrentStaticMesh = StaticMeshComponent->GetStaticMesh();
	const FName PreviewName = CurrentStaticMesh ? CurrentStaticMesh->GetAssetPathFileName() : "None";

	// 2. ImGui::BeginCombo를 사용하여 드롭다운 메뉴를 시작합니다.
	// 첫 번째 인자는 라벨, 두 번째 인자는 닫혀 있을 때 표시될 텍스트입니다.
	if (ImGui::BeginCombo("Static Mesh", PreviewName.ToString().c_str()))
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
			if (ImGui::Selectable(MeshInList->GetAssetPathFileName().ToString().c_str(), bIsSelected))
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

void UStaticMeshComponentWidget::RenderMaterialSections()
{
	UStaticMesh* CurrentMesh = StaticMeshComponent->GetStaticMesh();
	FStaticMesh* MeshAsset = CurrentMesh->GetStaticMeshAsset();

	ImGui::Text("Material Slots (%d)", static_cast<int>(MeshAsset->MaterialInfo.size()));

	// 머티리얼 슬롯
	for (int32 SlotIndex = 0; SlotIndex < MeshAsset->MaterialInfo.size(); ++SlotIndex)
	{
		// 현재 할당된 Material 가져오기
		UMaterial* CurrentMaterial = StaticMeshComponent->GetStaticMesh()->GetMaterial(SlotIndex);
		FString PreviewName = CurrentMaterial ? ("Material_" + std::to_string(CurrentMaterial->GetUUID())) : "None";

		// Material 정보 표시
		ImGui::PushID(SlotIndex);

		std::string Label = "Element " + std::to_string(SlotIndex);
		if (ImGui::BeginCombo(Label.c_str(), PreviewName.c_str()))
		{
			if (ImGui::Selectable("None", CurrentMaterial == nullptr))
			{
				StaticMeshComponent->SetMaterial(SlotIndex, nullptr);
			}
			ImGui::Separator();

			RenderAvailableMaterials(SlotIndex);

			ImGui::EndCombo();
		}
		ImGui::PopID();
	}
}

void UStaticMeshComponentWidget::RenderAvailableMaterials(int32 TargetSlotIndex)
{
	// 모든 UMaterial을 직접 순회 - 훨씬 더 효율적
	TMap<FString, UMaterial*> UniqueMaterials;

	for (TObjectIterator<UMaterial> It; It; ++It)
	{
		UMaterial* Mat = *It;
		if (Mat)
		{
			// Material ID를 기반으로 고유 Key 생성
			FString Key = "Material_" + std::to_string(Mat->GetUUID());
			UniqueMaterials[Key] = Mat;
		}
	}

	// Material 목록 표시
	for (const auto& Pair : UniqueMaterials)
	{
		const FString& MaterialName = Pair.first;
		UMaterial* Material = Pair.second;

		bool bIsSelected = (StaticMeshComponent->GetStaticMesh()->GetMaterial(TargetSlotIndex) == Material);

		if (ImGui::Selectable(MaterialName.c_str(), bIsSelected))
		{
			// Material 복사 (수정 가능한 복사본 생성)
			UMaterial* CopiedMaterial = CopyMaterial(Material);
			StaticMeshComponent->SetMaterial(TargetSlotIndex, CopiedMaterial);
		}

		if (bIsSelected)
		{
			ImGui::SetItemDefaultFocus();
		}
	}
}

UMaterial* UStaticMeshComponentWidget::CopyMaterial(UMaterial* SourceMaterial)
{
	if (!SourceMaterial) return nullptr;

	// Material 복사본 생성
	UMaterial* CopiedMaterial = new UMaterial();

	// 텍스처 정보 복사 (포인터만 복사)
	CopiedMaterial->SetDiffuseTexture(SourceMaterial->GetDiffuseTexture());
	CopiedMaterial->SetAmbientTexture(SourceMaterial->GetAmbientTexture());
	CopiedMaterial->SetSpecularTexture(SourceMaterial->GetSpecularTexture());
	CopiedMaterial->SetNormalTexture(SourceMaterial->GetNormalTexture());
	CopiedMaterial->SetAlphaTexture(SourceMaterial->GetAlphaTexture());
	CopiedMaterial->SetBumpTexture(SourceMaterial->GetBumpTexture());

	return CopiedMaterial;
}
