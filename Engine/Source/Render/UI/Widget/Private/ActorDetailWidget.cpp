#include "pch.h"
#include "Render/UI/Widget/Public/ActorDetailWidget.h"
#include "Editor/Public/EditorEngine.h"
#include "Level/Public/Level.h"
#include "Actor/Public/Actor.h"
#include "Component/Public/ActorComponent.h"
#include "Component/Public/BillboardComponent.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Component/Public/SceneComponent.h"
#include "Component/Mesh/Public/MeshComponent.h"
#include "Component/Public/TextRenderComponent.h"
#include "Component/Public/LineComponent.h"
#include "Component/Mesh/Public/CubeComponent.h"
#include "Component/Mesh/Public/SphereComponent.h"
#include "Component/Mesh/Public/SquareComponent.h"
#include "Component/Mesh/Public/TriangleComponent.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Core/Public/ObjectIterator.h"
#include "Texture/Public/Texture.h"
#include "Manager/BVH/Public/BVHManager.h"
#include "Core/Public/Object.h"

UActorDetailWidget::UActorDetailWidget()
	: UWidget("Actor Detail Widget")
{
}

UActorDetailWidget::~UActorDetailWidget() = default;

void UActorDetailWidget::Initialize()
{
	UE_LOG("ActorDetailWidget: Initialized");
}

void UActorDetailWidget::Update()
{
	// 특별한 업데이트 로직이 필요하면 여기에 추가
}

void UActorDetailWidget::RenderWidget()
{
	TObjectPtr CurrentLevel = GEngine->GetCurrentLevel();

	if (!CurrentLevel)
	{
		ImGui::TextUnformatted("No Level Loaded");
		return;
	}

	TObjectPtr<AActor> SelectedActor = CurrentLevel->GetSelectedActor();
	if (!SelectedActor)
	{
		ImGui::TextUnformatted("No Object Selected");
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Detail 확인을 위해 Object를 선택해주세요");
		return;
	}

	// Actor 헤더 렌더링 (이름 + rename 기능)
	RenderActorHeader(SelectedActor);

	ImGui::Separator();

	// 컴포넌트 트리 렌더링
	RenderComponentTree(SelectedActor);
}

void UActorDetailWidget::RenderActorHeader(TObjectPtr<AActor> InSelectedActor)
{
	if (!InSelectedActor)
	{
		return;
	}

	FName ActorName = InSelectedActor->GetName();
	FString ActorDisplayName = ActorName.ToString();

	ImGui::Text("[A]");
	ImGui::SameLine();

	if (bIsRenamingActor)
	{
		// Rename 모드
		ImGui::SetKeyboardFocusHere();
		if (ImGui::InputText("##ActorRename", ActorNameBuffer, sizeof(ActorNameBuffer),
		                     ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
		{
			FinishRenamingActor(InSelectedActor);
		}

		// ESC로 취소, InputManager보다 일단 내부 API로 입력 받는 것으로 처리
		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			CancelRenamingActor();
		}
	}
	else
	{
		// 더블클릭으로 rename 시작
		ImGui::Text("%s", ActorDisplayName.data());

		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			StartRenamingActor(InSelectedActor);
		}

		// 툴팁 UI
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Double-Click to Rename");
		}

		// Duplicate 버튼 추가
		ImGui::SameLine();
		if (ImGui::Button("Duplicate"))
		{
			DuplicateSelectedActor(InSelectedActor);
		}
	}
}

/**
 * @brief 컴포넌트들을 트리 형태로 표시하는 함수
 * @param InSelectedActor 선택된 Actor
 */
void UActorDetailWidget::RenderComponentTree(TObjectPtr<AActor> InSelectedActor)
{
	if (!InSelectedActor)
	{
		return;
	}

	const TArray<TObjectPtr<UActorComponent>>& Components = InSelectedActor->GetOwnedComponents();
	USceneComponent* RootSceneComponentRaw = InSelectedActor->GetRootComponent();
	TObjectPtr<USceneComponent> RootSceneComponent = TObjectPtr(RootSceneComponentRaw);
	TObjectPtr<UActorComponent> RootAsActorComponent = Cast<UActorComponent>(RootSceneComponent);

	ImGui::Text("Components (%d)", static_cast<int>(Components.size()));
	ImGui::SameLine();

	if (ImGui::Button(" + Add "))
	{
		ImGui::OpenPopup("AddComponentPopup");
	}

	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Adds a new component to this actor");
	}

	if (ImGui::BeginPopup("AddComponentPopup"))
	{
		auto AddComponentToActor = [&](UActorComponent* NewComponent)
		{
			if (!NewComponent)
			{
				return;
			}

			TObjectPtr<UActorComponent> ComponentPtr(NewComponent);
			InSelectedActor->AddComponent(ComponentPtr);
		};

		if (ImGui::MenuItem("Text Render Component"))
		{
			AddComponentToActor(new UTextRenderComponent());
		}
		if (ImGui::MenuItem("Billboard Component"))
		{
			AddComponentToActor(new UBillboardComponent());
		}
		ImGui::Separator();
		if (ImGui::MenuItem("Cube Component"))
		{
			AddComponentToActor(new UCubeComponent());
		}
		if (ImGui::MenuItem("Sphere Component"))
		{
			AddComponentToActor(new USphereComponent());
		}
		if (ImGui::MenuItem("Square Component"))
		{
			AddComponentToActor(new USquareComponent());
		}
		if (ImGui::MenuItem("Triangle Component"))
		{
			AddComponentToActor(new UTriangleComponent());
		}
		if (ImGui::MenuItem("Static Mesh Component"))
		{
			AddComponentToActor(new UStaticMeshComponent());
		}

		ImGui::EndPopup();
	}

	ImGui::Separator();

	const bool bHasRootComponent = (RootSceneComponentRaw != nullptr);
	const bool bHasAnyComponent = bHasRootComponent || !Components.empty();

	if (!bHasAnyComponent)
	{
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No components");
		return;
	}

	if (bHasRootComponent && RootAsActorComponent)
	{
		RenderComponentNode(RootAsActorComponent, RootSceneComponentRaw);
	}


	for (int32 ComponentIndex = 0; ComponentIndex < static_cast<int32>(Components.size()); ++ComponentIndex)
	{
		const TObjectPtr<UActorComponent>& Component = Components[ComponentIndex];
		if (!Component)
		{
			continue;
		}

		if (bHasRootComponent && RootSceneComponentRaw && Component.Get() == RootSceneComponentRaw)
		{
			continue;
		}

		RenderComponentNode(Component, RootSceneComponentRaw);
	}

	ImGui::Separator();

	if (SelectedComponent)
	{
		RenderComponentDetails(SelectedComponent);
	}
}

/**
 * @brief 컴포넌트에 대한 정보를 표시하는 함수
 * 내부적으로 RTTI를 활용한 GetName 처리가 되어 있음
 * @param InComponent
 */
void UActorDetailWidget::RenderComponentNode(TObjectPtr<UActorComponent> InComponent, USceneComponent* InRootComponent)
{
	if (!InComponent)
	{
		return;
	}

	// 컴포넌트 타입에 따른 아이콘
	FName ComponentTypeName = InComponent.Get()->GetClass()->GetClassTypeName();
	FString ComponentIcon = "[C]"; // 기본 컴포넌트 아이콘

	bool bIsPrimitiveComponent = false;

	if (Cast<UPrimitiveComponent>(InComponent))
	{
		ComponentIcon = "[P]";
		bIsPrimitiveComponent = true;
	}
	else if (Cast<USceneComponent>(InComponent))
	{
		ComponentIcon = "[S]";
	}

	TObjectPtr<USceneComponent> AsSceneComponent = Cast<USceneComponent>(InComponent);
	const bool bIsRootComponent = (InRootComponent != nullptr) && AsSceneComponent && (AsSceneComponent.Get() == InRootComponent);

	FString NodeLabel;
	if (bIsRootComponent)
	{
		NodeLabel = "[Root] ";
	}
	else
	{
		NodeLabel = ComponentIcon + " ";
	}
	NodeLabel += InComponent->GetName().ToString();

	if (bIsRootComponent)
	{
		NodeLabel += " (Root Component)";
	}

	ImGuiTreeNodeFlags NodeFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	if (SelectedComponent == InComponent)
	{
		NodeFlags |= ImGuiTreeNodeFlags_Selected;
	}

	if (bIsRootComponent)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.82f, 0.25f, 1.0f));
	}

	ImGui::TreeNodeEx(NodeLabel.c_str(), NodeFlags);

	if (bIsRootComponent)
	{
		ImGui::PopStyleColor();
	}

	if (ImGui::IsItemClicked())
	{
		SelectedComponent = InComponent;
	}

	if (ImGui::IsItemHovered())
	{
		if (bIsRootComponent)
		{
			ImGui::SetTooltip("Root Component\nType: %s", ComponentTypeName.ToString().data());
		}
		else
		{
			ImGui::SetTooltip("Component Type: %s", ComponentTypeName.ToString().data());
		}
	}

	if (bIsPrimitiveComponent)
	{
		if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(InComponent))
		{
			ImGui::SameLine();
			ImGui::TextColored(
				PrimitiveComponent->IsVisible() ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
				PrimitiveComponent->IsVisible() ? "[Visible]" : "[Hidden]"
			);
		}
	}
}

void UActorDetailWidget::RenderComponentDetails(TObjectPtr<UActorComponent> InComponent)
{
	if (!InComponent) return;

	FName TypeName = InComponent->GetClass()->GetClassTypeName();
	ImGui::Text("Details for: %s", TypeName.ToString().data());

	if (InComponent->IsA(UTextRenderComponent::StaticClass()))
	{
		UTextRenderComponent* TextComp = Cast<UTextRenderComponent>(InComponent);
		static char TextBuffer[256];
		strncpy_s(TextBuffer, TextComp->GetText().c_str(), sizeof(TextBuffer)-1);

		bool bShowUUID = TextComp->bIsUUIDText;
		if (ImGui::Checkbox("Show UUID", &bShowUUID))
		{
			TextComp->bIsUUIDText = bShowUUID;
		}

		if (!bShowUUID)
		{
			if (ImGui::InputText("Text", TextBuffer, sizeof(TextBuffer)))
			{
				TextComp->SetText(TextBuffer);
			}
		}
	}
	else if (InComponent->IsA(UBillboardComponent::StaticClass()))
	{
		UBillboardComponent* Billboard = Cast<UBillboardComponent>(InComponent);
		// // Texture field (for now, string path or ID)
		// static char TexturePath[256];
		// strncpy_s(TexturePath, Billboard->GetTexturePath().c_str(), sizeof(TexturePath)-1);
		//
		// if (ImGui::InputText("Texture Path", TexturePath, sizeof(TexturePath)))
		// {
		// 	Billboard->SetTexturePath(TexturePath);
		// }
		//
		// // Relative offset transform
		// FVector Offset = Billboard->GetRelativeLocation();
		// if (ImGui::DragFloat3("Offset", &Offset.X, 0.1f))
		// {
		// 	Billboard->SetRelativeLocation(Offset);
		// }
	}
	else
	{
		ImGui::TextColored(ImVec4(0.6f,0.6f,0.6f,1.0f), "No detail view for this component type.");
	}

	ImGui::Separator();
	ImGui::Text("Component Transform");

	TObjectPtr<USceneComponent> SceneComponent = Cast<USceneComponent>(InComponent);
	if (!SceneComponent)
	{
		return;
	}
	bool bTransformChanged = false;

	FVector RelativeLocation = SceneComponent->GetRelativeLocation();
	float LocationArr[3] = { RelativeLocation.X, RelativeLocation.Y, RelativeLocation.Z };

	if (ImGui::DragFloat3("Relative Location", LocationArr, 0.1f))
	{
		SceneComponent->SetRelativeLocation(FVector(LocationArr[0], LocationArr[1], LocationArr[2]));
		bTransformChanged = true;
	}

	FVector RelativeRotation = SceneComponent->GetRelativeRotation();
	float RotationArr[3] = { RelativeRotation.X, RelativeRotation.Y, RelativeRotation.Z };
	if (ImGui::DragFloat3("Relative Rotation", RotationArr, 0.1f))
	{
		SceneComponent->SetRelativeRotation(FVector(RotationArr[0], RotationArr[1], RotationArr[2]));
		bTransformChanged = true;
	}

	bool bUniformScale = SceneComponent->IsUniformScale();
	FVector RelativeScale = SceneComponent->GetRelativeScale3D();

	if (bUniformScale)
	{
		float UniformScale = RelativeScale.X;
		if (ImGui::DragFloat("Relative Scale", &UniformScale, 0.01f, 0.01f, 10.0f))
		{
			SceneComponent->SetRelativeScale3D(FVector(UniformScale, UniformScale, UniformScale));
			bTransformChanged = true;
		}
	}
	else
	{
		float ScaleArr[3] = { RelativeScale.X, RelativeScale.Y, RelativeScale.Z };
		if (ImGui::DragFloat3("Relative Scale", ScaleArr, 0.01f))
		{
			SceneComponent->SetRelativeScale3D(FVector(ScaleArr[0], ScaleArr[1], ScaleArr[2]));
			bTransformChanged = true;
		}
	}

	if (ImGui::Checkbox("Relative Uniform Scale", &bUniformScale))
	{
		SceneComponent->SetUniformScale(bUniformScale);
		bTransformChanged = true;

		if (bUniformScale)
		{
			float UniformScale = SceneComponent->GetRelativeScale3D().X;
			SceneComponent->SetRelativeScale3D(FVector(UniformScale, UniformScale, UniformScale));
		}
	}

	if (bTransformChanged && InComponent->IsA(UPrimitiveComponent::StaticClass()))
	{
		UBVHManager::GetInstance().Refit();
	}
}

void UActorDetailWidget::StartRenamingActor(TObjectPtr<AActor> InActor)
{
	if (!InActor)
	{
		return;
	}

	bIsRenamingActor = true;
	FString CurrentName = InActor->GetName().ToString();
	strncpy_s(ActorNameBuffer, CurrentName.data(), sizeof(ActorNameBuffer) - 1);
	ActorNameBuffer[sizeof(ActorNameBuffer) - 1] = '\0';

	UE_LOG("ActorDetailWidget: '%s' 에 대한 이름 변경 시작", CurrentName.data());
}

void UActorDetailWidget::FinishRenamingActor(TObjectPtr<AActor> InActor)
{
	if (!InActor || !bIsRenamingActor)
	{
		return;
	}

	FString NewName = ActorNameBuffer;
	if (!NewName.empty() && NewName != InActor->GetName().ToString())
	{
		// Actor 이름 변경
		InActor->SetDisplayName(NewName);
		UE_LOG_SUCCESS("ActorDetailWidget: Actor의 이름을 '%s' (으)로 변경하였습니다", NewName.data());
	}

	bIsRenamingActor = false;
	ActorNameBuffer[0] = '\0';
}

void UActorDetailWidget::CancelRenamingActor()
{
	bIsRenamingActor = false;
	ActorNameBuffer[0] = '\0';
	UE_LOG_WARNING("ActorDetailWidget: 이름 변경 취소");
}

void UActorDetailWidget::DuplicateSelectedActor(TObjectPtr<AActor> InActor)
{
	if (!InActor)
	{
		return;
	}

	ULevel* CurrentLevel = GEngine->GetCurrentLevel();
	if (!CurrentLevel)
	{
		return;
	}

	AActor* NewActor = DuplicateObject(InActor, CurrentLevel, FName::GetNone());

	if (NewActor)
	{
		//FVector Location = NewActor->GetActorLocation();
		//NewActor->SetActorLocation(Location + FVector(1.0f, 0.0f, 0.0f)); // Offset by 100 on X

		CurrentLevel->RegisterDuplicatedActor(NewActor);
	}
}
