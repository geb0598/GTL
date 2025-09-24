#include "pch.h"
#include "Level/Public/Level.h"

#include "Actor/Public/Actor.h"
#include "Component/Public/BillBoardComponent.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Manager/UI/Public/UIManager.h"
#include "Utility/Public/JsonSerializer.h"
#include "Actor/Public/CubeActor.h"
#include "Actor/Public/SphereActor.h"
#include "Actor/Public/TriangleActor.h"
#include "Actor/Public/SquareActor.h"
#include "Factory/Public/NewObject.h"
#include "Core/Public/Object.h"
#include "Factory/Public/FactorySystem.h"
#include "Manager/Config/Public/ConfigManager.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Editor/Public/Viewport.h"

#include <json.hpp>

ULevel::ULevel() = default;

ULevel::ULevel(const FName& InName)
	: UObject(InName)
{
}

ULevel::~ULevel()
{
	for (auto Actor : LevelActors)
	{
		SafeDelete(Actor);
	}
}

void ULevel::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	// 불러오기
	if (bInIsLoading)
	{
		uint32 Version = 0;
		FJsonSerializer::ReadUint32(InOutHandle, "Version", Version);

		uint32 NextUUID = 0;
		FJsonSerializer::ReadUint32(InOutHandle, "NextUUID", Version);

		JSON PerspectiveCameraData;
		if (FJsonSerializer::ReadObject(InOutHandle, "PerspectiveCamera", PerspectiveCameraData))
		{
			UConfigManager::GetInstance().SetCameraSettingsFromJson(PerspectiveCameraData);
			URenderer::GetInstance().GetViewportClient()->ApplyAllCameraDataToViewportClients();
		}

		JSON PrimitivesJson;
		if (FJsonSerializer::ReadObject(InOutHandle, "Primitives", PrimitivesJson))
		{
			// ObjectRange()를 사용하여 Primitives 객체의 모든 키-값 쌍을 순회
			for (auto& Pair : PrimitivesJson.ObjectRange())
			{
				// Pair.first는 ID 문자열, Pair.second는 단일 프리미티브의 JSON 데이터입니다.
				const FString& IdString = Pair.first;
				JSON& PrimitiveDataJson = Pair.second;

				FString TypeString;
				FJsonSerializer::ReadString(PrimitiveDataJson, "Type", TypeString);

				UClass* NewClass = UClass::FindClass(TypeString);

				AActor* NewActor = SpawnActor(NewClass);
				if (NewActor)
				{
					NewActor->Serialize(bInIsLoading, PrimitiveDataJson);
				}
			}
		}
	}
	// 저장
	else
	{
		InOutHandle["Version"] = 1;
		InOutHandle["NextUUID"] = GetNextUUID();	// Todo: 액터 하나당 UUID 가 여러개라서 +1 씩 늘어나지 않는다

		// GetCameraSetting 호출 전에 뷰포트 클라이언트의 최신 데이터를 ConfigManager로 동기화합니다.
		URenderer::GetInstance().GetViewportClient()->UpdateCameraSettingsToConfig();
		InOutHandle["PerspectiveCamera"] = UConfigManager::GetInstance().GetCameraSettingsAsJson();

		JSON PrimitivesJson = json::Object();
		for (const TObjectPtr<AActor>& Actor : LevelActors)
		{
			JSON PrimitiveJson;
			PrimitiveJson["Type"] = Actor->GetClass()->GetClassTypeName().ToString();
			Actor->Serialize(bInIsLoading, PrimitiveJson);

			PrimitivesJson[std::to_string(Actor->GetUUID())] = PrimitiveJson;
		}
		InOutHandle["Primitives"] = PrimitivesJson;
	}
}

void ULevel::Init()
{
	// TEST CODE
}

void ULevel::Update()
{
	// Process Delayed Task
	ProcessPendingDeletions();

	uint64 AllocatedByte = GetAllocatedBytes();
	uint32 AllocatedCount = GetAllocatedCount();

	LevelPrimitiveComponents.clear();
	//Deprecated : EditorPrimitive는 에디터에서 처리
	//EditorPrimitiveComponents.clear();

	//AActor* pOldActor = nullptr;
	for (auto& Actor : LevelActors)
	{
		if (Actor)
		{
			Actor->Tick();
			AddLevelPrimitiveComponent(Actor);
		}
	}
}

void ULevel::Render()
{
}

void ULevel::Cleanup()
{
}

uint32 ULevel::GetNextUUID()
{
	uint32 NextUUID = 1;
	return NextUUID;
}

AActor* ULevel::SpawnActor(const UClass* InActorClass, const FName& InName)
{
	if (!InActorClass)
	{
		return nullptr;
	}

	AActor* NewActor = Cast<AActor>(InActorClass->CreateDefaultObject());
	if (NewActor)
	{
		if (InName != FName::GetNone())
		{
			NewActor->SetName(InName);
		}
		LevelActors.push_back(TObjectPtr(NewActor));
		NewActor->BeginPlay();
		return NewActor;
	}

	return nullptr;
}

void ULevel::AddLevelPrimitiveComponent(AActor* Actor)
{
	if (!Actor) return;

	for (auto& Component : Actor->GetOwnedComponents())
	{
		if (Component->GetComponentType() >= EComponentType::Primitive)
		{
			TObjectPtr<UPrimitiveComponent> PrimitiveComponent = Cast<UPrimitiveComponent>(Component);

			if (!PrimitiveComponent)
			{
				continue;
			}

			/* 3가지 경우 존재.
			1: primitive show flag가 꺼져 있으면, 도형, 빌보드 모두 렌더링 안함.
			2: primitive show flag가 켜져 있고, billboard show flag가 켜져 있으면, 도형, 빌보드 모두 렌더링
			3: primitive show flag가 켜져 있고, billboard show flag가 꺼져 있으면, 도형은 렌더링 하지만, 빌보드는 렌더링 안함. */
			// 빌보드는 무조건 피킹이 된 actor의 빌보드여야 렌더링 가능
			if (PrimitiveComponent->IsVisible() && (ShowFlags & EEngineShowFlags::SF_Primitives))
			{
				if (PrimitiveComponent->GetPrimitiveType() != EPrimitiveType::BillBoard)
				{
					LevelPrimitiveComponents.push_back(PrimitiveComponent);
				}
				else if (PrimitiveComponent->GetPrimitiveType() == EPrimitiveType::BillBoard && (ShowFlags & EEngineShowFlags::SF_BillboardText) && (ULevelManager::GetInstance().GetCurrentLevel()->GetSelectedActor() == Actor))
				{
					//TObjectPtr<UBillBoardComponent> BillBoard = Cast<UBillBoardComponent>(PrimitiveComponent);
					//BillBoard->UpdateRotationMatrix();
					LevelPrimitiveComponents.push_back(PrimitiveComponent);
				}
			}
		}
	}
}

void ULevel::SetSelectedActor(AActor* InActor)
{
	// Set Selected Actor
	if (SelectedActor)
	{
		for (auto& Component : SelectedActor->GetOwnedComponents())
		{
			if (Component->GetComponentType() >= EComponentType::Primitive)
			{
				TObjectPtr<UPrimitiveComponent> PrimitiveComponent = Cast<UPrimitiveComponent>(Component);
				if (PrimitiveComponent->IsVisible())
				{
					PrimitiveComponent->SetColor({ 0.f, 0.f, 0.f, 0.f });
				}
			}
		}
	}

	if (InActor != SelectedActor)
	{
		UUIManager::GetInstance().OnSelectedActorChanged(InActor);
	}
	SelectedActor = InActor;

	if (SelectedActor)
	{
		for (auto& Component : SelectedActor->GetOwnedComponents())
		{
			if (Component->GetComponentType() >= EComponentType::Primitive)
			{
				TObjectPtr<UPrimitiveComponent> PrimitiveComponent = Cast<UPrimitiveComponent>(Component);
				if (PrimitiveComponent->IsVisible())
				{
					PrimitiveComponent->SetColor({ 1.f, 0.8f, 0.2f, 0.4f });
				}
			}
		}
	}
}

/**
 * @brief Level에서 Actor 제거하는 함수
 */
bool ULevel::DestroyActor(AActor* InActor)
{
	if (!InActor)
	{
		return false;
	}

	// LevelActors 리스트에서 제거
	for (auto Iterator = LevelActors.begin(); Iterator != LevelActors.end(); ++Iterator)
	{
		if (*Iterator == InActor)
		{
			LevelActors.erase(Iterator);
			break;
		}
	}

	// Remove Actor Selection
	if (SelectedActor == InActor)
	{
		SelectedActor = nullptr;

		//Deprecated : Gizmo는 에디터에서 처리
		// Gizmo Target Release
		/*if (Gizmo)
		{
			Gizmo->SetTargetActor(nullptr);
		}*/
	}

	// Remove
	delete InActor;

	UE_LOG("Level: Actor Destroyed Successfully");
	return true;
}

/**
 * @brief Delete In Next Tick
 */
void ULevel::MarkActorForDeletion(AActor* InActor)
{
	if (!InActor)
	{
		UE_LOG("Level: MarkActorForDeletion: InActor Is Null");
		return;
	}

	// 이미 삭제 대기 중인지 확인
	for (AActor* PendingActor : ActorsToDelete)
	{
		if (PendingActor == InActor)
		{
			UE_LOG("Level: Actor Already Marked For Deletion");
			return;
		}
	}

	// 삭제 대기 리스트에 추가
	ActorsToDelete.push_back(InActor);
	UE_LOG("Level: 다음 Tick에 Actor를 제거하기 위한 마킹 처리: %s", InActor->GetName().ToString().data());

	// 선택 해제는 바로 처리
	if (SelectedActor == InActor)
	{
		SelectedActor = nullptr;
	}
}

/**
 * @brief Level에서 Actor를 실질적으로 제거하는 함수
 * 이전 Tick에서 마킹된 Actor를 제거한다
 */
void ULevel::ProcessPendingDeletions()
{
	if (ActorsToDelete.empty())
	{
		return;
	}

	UE_LOG("Level: %zu개의 객체 지연 삭제 프로세스 처리 시작", ActorsToDelete.size());

	// 대기 중인 액터들을 삭제
	for (AActor* ActorToDelete : ActorsToDelete)
	{
		if (!ActorToDelete)
			continue;

		// 혹시 남아있을 수 있는 참조 정리
		if (SelectedActor == ActorToDelete)
		{
			SelectedActor = nullptr;
		}

		// LevelActors 리스트에서 제거
		for (auto Iterator = LevelActors.begin(); Iterator != LevelActors.end(); ++Iterator)
		{
			if (*Iterator == ActorToDelete)
			{
				LevelActors.erase(Iterator);
				break;
			}
		}

		FName DeletedActorName = ActorToDelete->GetName();

		// Release Memory
		delete ActorToDelete;
		UE_LOG("Level: Actor 제거: %s (%p)", DeletedActorName.ToString().data(), static_cast<void*>(ActorToDelete));
	}

	// Clear TArray
	ActorsToDelete.clear();
	UE_LOG("Level: 모든 지연 삭제 프로세스 완료");
}
