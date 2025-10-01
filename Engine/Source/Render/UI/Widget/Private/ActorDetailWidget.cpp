#include "pch.h"
#include "Render/UI/Widget/Public/ActorDetailWidget.h"

#include "Manager/Level/Public/LevelManager.h"
#include "Level/Public/Level.h"
#include "Actor/Public/Actor.h"
#include "Component/Public/ActorComponent.h"
#include "Component/Public/BillboardComponent.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Component/Public/SceneComponent.h"
#include "Core/Public/ObjectIterator.h"
#include "Texture/Public/Texture.h"
#include "Manager/Asset/Public/AssetManager.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <filesystem>

namespace
{
struct FBillboardSpriteOption
{
    FString DisplayName;
    FString FilePath;
    TObjectPtr<UTexture> Texture;
};

TArray<FBillboardSpriteOption>& GetBillboardSpriteOptions()
{
    static bool bInitialized = false;
    static TArray<FBillboardSpriteOption> Options;

    if (!bInitialized)
    {
    	bInitialized = true;

        const std::filesystem::path IconDirectory = std::filesystem::absolute(std::filesystem::path("Asset/Icon"));
        const FString IconDirectoryString = IconDirectory.generic_string();

        if (!std::filesystem::exists(IconDirectory))
        {
            UE_LOG_WARNING("ActorDetailWidget: Icon directory not found: %s", IconDirectoryString.c_str());
            return Options;
        }

        UAssetManager& AssetManager = UAssetManager::GetInstance();

        try
        {
            for (const auto& Entry : std::filesystem::directory_iterator(IconDirectory))
            {
                if (!Entry.is_regular_file())
                {
                    continue;
                }

                FString Extension = Entry.path().extension().string();
                std::transform(Extension.begin(), Extension.end(), Extension.begin(), [](unsigned char InChar)
                {
                    return static_cast<char>(std::tolower(InChar));
                });

                if (Extension != ".png")
                {
                    continue;
                }

                FString FilePath = Entry.path().generic_string();
                FString DisplayName = Entry.path().stem().string();

                UTexture* Texture = AssetManager.CreateTexture(FilePath, DisplayName);
                if (!Texture)
                {
                    UE_LOG_WARNING("ActorDetailWidget: Failed to load billboard icon texture %s", FilePath.c_str());
                    continue;
                }

                Options.push_back(FBillboardSpriteOption { DisplayName, FilePath, TObjectPtr(Texture) });
            }
        }
        catch (const std::exception& Exception)
        {
            UE_LOG_ERROR("ActorDetailWidget: Failed to enumerate billboard icons: %s", Exception.what());
            Options.clear();
        }

        std::sort(Options.begin(), Options.end(), [](const FBillboardSpriteOption& A, const FBillboardSpriteOption& B)
        {
            return A.DisplayName < B.DisplayName;
        });
    }

    return Options;
}
}
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
	}
}

/**
 * @brief 컴포넌트들을 트리 형태로 표시하는 함수
 * @param InSelectedActor 선택된 Actor
 */
void UActorDetailWidget::RenderComponentTree(TObjectPtr<AActor> InSelectedActor)
{
	if (!InSelectedActor) return;

	const TArray<TObjectPtr<UActorComponent>>& Components = InSelectedActor->GetOwnedComponents();

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
		// Example menu items (replace with your actual component list)
		if (ImGui::MenuItem("Text Render Component"))
		{
			UTextRenderComponent* TextRender = new UTextRenderComponent(InSelectedActor, 5.0f);
			TObjectPtr TextRenderPtr(TextRender);
			InSelectedActor->AddComponent(Cast<UActorComponent>(TextRenderPtr));
		}
		if (ImGui::MenuItem("Billboard Component"))
		{
			UBillboardComponent* Billboard = new UBillboardComponent(InSelectedActor);
			TObjectPtr BillboardPtr(Billboard);
			InSelectedActor->AddComponent(Cast<UActorComponent>(BillboardPtr));
		}
		// if (ImGui::MenuItem("Light Component"))
		// {
		// }

		ImGui::EndPopup();
	}

	ImGui::Separator();

	if (Components.empty())
	{
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No components");
		return;
	}

	for (int32 i = 0; i < static_cast<int32>(Components.size()); ++i)
	{
		if (Components[i])
		{
			RenderComponentNode(Components[i]);
		}
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
void UActorDetailWidget::RenderComponentNode(TObjectPtr<UActorComponent> InComponent)
{
	if (!InComponent)
	{
		return;
	}

	// 컴포넌트 타입에 따른 아이콘
	FName ComponentTypeName = InComponent.Get()->GetClass()->GetClassTypeName();
	FString ComponentIcon = "[C]"; // 기본 컴포넌트 아이콘

	if (Cast<UPrimitiveComponent>(InComponent))
	{
		ComponentIcon = "[P]"; // PrimitiveComponent 아이콘
	}
	else if (Cast<USceneComponent>(InComponent))
	{
		ComponentIcon = "[S]"; // SceneComponent 아이콘
	}

	// 트리 노드 생성
	ImGuiTreeNodeFlags NodeFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

	FString NodeLabel = ComponentIcon + " " + InComponent->GetName().ToString();
	if (SelectedComponent == InComponent)
		NodeFlags |= ImGuiTreeNodeFlags_Selected;

	ImGui::TreeNodeEx(NodeLabel.data(), NodeFlags);

	if (ImGui::IsItemClicked())
	{
		SelectedComponent = InComponent;
	}

	// 컴포넌트 세부 정보를 추가로 표시할 수 있음
	if (ImGui::IsItemHovered())
	{
		// 컴포넌트 타입 정보를 툴팁으로 표시
		ImGui::SetTooltip("Component Type: %s", ComponentTypeName.ToString().data());
	}

	// PrimitiveComponent인 경우 추가 정보
	if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(InComponent))
	{
		ImGui::SameLine();
		ImGui::TextColored(
			PrimitiveComponent->IsVisible() ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
			PrimitiveComponent->IsVisible() ? "[Visible]" : "[Hidden]"
		);
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

		// --- Offset (Relative Location) ---
		FVector Offset = TextComp->GetRelativeLocation();
		float OffsetArr[3] = { Offset.X, Offset.Y, Offset.Z };
		if (ImGui::SliderFloat3("Offset", OffsetArr, -10.0f, 10.0f))
		{
			TextComp->SetRelativeLocation(FVector(OffsetArr[0], OffsetArr[1], OffsetArr[2]));
		}

		// --- Rotation (Relative Rotation) ---
		FVector Rotation = TextComp->GetRelativeRotation();
		float RotArr[3] = { Rotation.X, Rotation.Y, Rotation.Z };
		if (ImGui::DragFloat3("Rotation", RotArr, 0.1f, -360.0f, 360.0f, "%.3f"))
		{
			TextComp->SetRelativeRotation(FVector(RotArr[0], RotArr[1], RotArr[2]));
		}

		// --- Size (Relative Scale) ---
		FVector Scale = TextComp->GetRelativeScale3D();
		float ScaleArr[3] = { Scale.X, Scale.Y, Scale.Z };
		if (Scale.X < 0.5f && Scale.Y < 0.5f && Scale.Z < 0.5f)
		{
			ScaleArr[0] = ScaleArr[1] = ScaleArr[2] = 1.0f;
			TextComp->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
		}
		if (ImGui::SliderFloat3("Size", ScaleArr, 1.0f, 10.0f))
		{
			TextComp->SetRelativeScale3D(FVector(ScaleArr[0], ScaleArr[1], ScaleArr[2]));
		}
	}
	else if (InComponent->IsA(UBillboardComponent::StaticClass()))
	{
		UBillboardComponent* Billboard = Cast<UBillboardComponent>(InComponent);

		auto GetTextureDisplayName = [](UTexture* InTexture) -> FString
		{
			if (!InTexture)
			{
				return "None";
			}

			FString DisplayName = InTexture->GetName().ToString();
			if (!DisplayName.empty() && DisplayName.rfind("Object_", 0) != 0)
			{
				return DisplayName;
			}

			FString FilePath = InTexture->GetFilePath().ToString();
			if (!FilePath.empty())
			{
				size_t LastSlash = FilePath.find_last_of("/\\");
				FString FileName = (LastSlash != FString::npos) ? FilePath.substr(LastSlash + 1) : FilePath;

				size_t LastDot = FileName.find_last_of('.');
				if (LastDot != FString::npos)
				{
					FileName = FileName.substr(0, LastDot);
				}

				if (!FileName.empty())
				{
					return FileName;
				}
			}

			return "Texture_" + std::to_string(InTexture->GetUUID());
		};

		UTexture* CurrentSprite = Billboard->GetSprite();
		auto& SpriteOptions = GetBillboardSpriteOptions();
		FString PreviewName = GetTextureDisplayName(CurrentSprite);

		if (CurrentSprite)
		{
			const auto Found = std::find_if(SpriteOptions.begin(), SpriteOptions.end(), [CurrentSprite](const FBillboardSpriteOption& Option)
			{
				return Option.Texture.Get() == CurrentSprite;
			});
			if (Found != SpriteOptions.end())
			{
				PreviewName = Found->DisplayName;
			}
		}

		if (SpriteOptions.empty())
		{
			ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "No icon textures found under Engine/Asset/Icon");
		}

		if (ImGui::BeginCombo("Sprite", PreviewName.c_str()))
		{
			bool bNoneSelected = (CurrentSprite == nullptr);
			if (ImGui::Selectable("None", bNoneSelected))
			{
				Billboard->SetSprite(nullptr);
				CurrentSprite = nullptr;
			}

			if (bNoneSelected)
			{
				ImGui::SetItemDefaultFocus();
			}

			for (const FBillboardSpriteOption& Option : SpriteOptions)
			{
				bool bIsSelected = (Option.Texture.Get() == CurrentSprite);

				if (ImGui::Selectable(Option.DisplayName.c_str(), bIsSelected))
				{
					Billboard->SetSprite(Option.Texture.Get());
					CurrentSprite = Option.Texture.Get();
				}

				if (bIsSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}

			ImGui::EndCombo();
		}
	}
	else
	{
		ImGui::TextColored(ImVec4(0.6f,0.6f,0.6f,1.0f), "No detail view for this component type.");
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

