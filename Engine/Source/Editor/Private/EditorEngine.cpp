#include "pch.h"
#include "Editor/Public/EditorEngine.h"
#include "Editor/Public/Editor.h"
#include "Level/Public/Level.h"
#include "Manager/Config/Public/ConfigManager.h"

IMPLEMENT_CLASS(UEditorEngine, UObject);

UEditorEngine* GEngine = nullptr;

UEditorEngine::UEditorEngine()
{
}

UEditorEngine::~UEditorEngine()
{
	Shutdown();
}

void UEditorEngine::Initialize()
{
	// Editor World 생성
	FWorldContext EditorContext;
	EditorContext.ContextHandle = FName("EditorWorld");
	EditorContext.WorldType = EWorldType::Editor;
	EditorContext.WorldPtr = NewObject<UWorld>();
	EditorContext.WorldPtr->SetWorldType(EWorldType::Editor);

	WorldContexts.push_back(EditorContext);

	// Editor 생성
	Editor = new UEditor();

	// 마지막 저장된 레벨 로드 (World에 위임)
	FString LastSavedLevelPath = UConfigManager::GetInstance().GetLastSavedLevelPath();
	if (!LoadLevel(LastSavedLevelPath))
	{
		CreateNewLevel();
	}
}

void UEditorEngine::Shutdown()
{
	// World 정리 (World 소멸자가 Level 정리)
	for (auto& Context : WorldContexts)
	{
		if (Context.WorldPtr)
		{
			delete Context.WorldPtr;
		}
	}
	WorldContexts.clear();

	// Editor 정리
	if (Editor)
	{
		delete Editor;
		Editor = nullptr;
	}
}

void UEditorEngine::BeginPlay()
{
}

void UEditorEngine::Tick(float DeltaTime)
{
	if (bPIEActive)
	{
		// PIE 중에는 PIEWorld만 Tick
		if (UWorld* PIEWorld = GetPIEWorld())
		{
			PIEWorld->Tick(DeltaTime);
		}
	}
	else
	{
		// 일반 에디터 모드에서는 EditorWorld만 Tick
		if (UWorld* EditorWorld = GetEditorWorld())
		{
			EditorWorld->Tick(DeltaTime);
		}
	}

	// Editor는 항상 Tick (Gizmo, Viewport 등)
	if (Editor)
	{
		Editor->Tick(DeltaTime);
	}
}

UWorld* UEditorEngine::GetEditorWorld() const
{
	for (const auto& Context : WorldContexts)
	{
		if (Context.WorldType == EWorldType::Editor)
		{
			return Context.WorldPtr;
		}
	}
	return nullptr;
}

UWorld* UEditorEngine::GetPIEWorld() const
{
	for (const auto& Context : WorldContexts)
	{
		if (Context.WorldType == EWorldType::PIE)
		{
			return Context.WorldPtr;
		}
	}
	return nullptr;
}

UWorld* UEditorEngine::GetActiveWorld() const
{
	return bPIEActive ? GetPIEWorld() : GetEditorWorld();
}

bool UEditorEngine::LoadLevel(const FString& InFilePath)
{
	UWorld* EditorWorld = GetEditorWorld();
	if (!EditorWorld)
	{
		UE_LOG("EditorEngine: No Editor World available");
		return false;
	}

	bool bSuccess = EditorWorld->LoadLevel(InFilePath);
	if (bSuccess)
	{
		UConfigManager::GetInstance().SetLastUsedLevelPath(InFilePath);
	}
	return bSuccess;
}

bool UEditorEngine::SaveCurrentLevel(const FString& InFilePath)
{
	UWorld* EditorWorld = GetEditorWorld();
	if (!EditorWorld)
	{
		UE_LOG("EditorEngine: No Editor World available");
		return false;
	}

	bool bSuccess = EditorWorld->SaveLevel(InFilePath);
	if (bSuccess)
	{
		UConfigManager::GetInstance().SetLastUsedLevelPath(InFilePath);
	}
	return bSuccess;
}

bool UEditorEngine::CreateNewLevel(const FString& InLevelName)
{
	UWorld* EditorWorld = GetEditorWorld();
	if (!EditorWorld)
	{
		UE_LOG("EditorEngine: No Editor World available");
		return false;
	}

	return EditorWorld->CreateNewLevel(InLevelName);
}

ULevel* UEditorEngine::GetCurrentLevel() const
{
	if (UWorld* EditorWorld = GetEditorWorld())
	{
		return EditorWorld->GetLevel();
	}
	return nullptr;
}

void UEditorEngine::StartPIE()
{
	if (bPIEActive)
	{
		UE_LOG("EditorEngine: PIE is already running");
		return;
	}

	UE_LOG("EditorEngine: Starting Play In Editor (PIE) Mode");
	bPIEActive = true;

	// TODO: PIEWorld 생성 및 EditorWorld 복제
	// UWorld* EditorWorld = GetEditorWorld();
	// UWorld* PIEWorld = DuplicateWorldForPIE(EditorWorld);
}

void UEditorEngine::EndPIE()
{
	if (!bPIEActive)
	{
		UE_LOG("EditorEngine: PIE is not running");
		return;
	}

	UE_LOG("EditorEngine: Stop Play In Editor (PIE) Mode");
	bPIEActive = false;

	// TODO: PIEWorld 정리
}

UWorld* UEditorEngine::DuplicateWorldForPIE(UWorld* SourceWorld)
{

}

FWorldContext* UEditorEngine::GetEditorWorldContext()
{
	for (auto& Context : WorldContexts)
	{
		if (Context.WorldType == EWorldType::Editor)
		{
			return &Context;
		}
	}
	return nullptr;
}

FWorldContext* UEditorEngine::GetPIEWorldContext()
{
	for (auto& Context : WorldContexts)
	{
		if (Context.WorldType == EWorldType::PIE)
		{
			return &Context;
		}
	}
	return nullptr;
}


