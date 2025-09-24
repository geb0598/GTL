#pragma once
#include "Core/Public/Object.h"
//#include "Editor/Public/Editor.h"

class UEditor;
class ULevel;

UCLASS()
class ULevelManager :
	public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(ULevelManager, UObject)

public:
	void Update() const;
	void CreateDefaultLevel();
	void RegisterLevel(const FName& InName, TObjectPtr<ULevel> InLevel);
	void LoadLevel(const FName& InName);
	void Shutdown();

	// Getter
	TObjectPtr<ULevel> GetCurrentLevel() const { return CurrentLevel; }

	// Save & Load System
	bool SaveCurrentLevel(const FString& InFilePath) const;
	bool LoadLevel(const FString& InLevelName, const FString& InFilePath);
	bool CreateNewLevel(const FString& InLevelName);
	static path GetLevelDirectory();
	static path GenerateLevelFilePath(const FString& InLevelName);

	TObjectPtr<UEditor> GetEditor() const { return Editor; }

private:
	TObjectPtr<ULevel> CurrentLevel;
	TMap<FName, TObjectPtr<ULevel>> Levels;
	TObjectPtr<UEditor> Editor;
};
