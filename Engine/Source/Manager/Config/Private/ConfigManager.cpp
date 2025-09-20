#include "pch.h"
#include "Manager/Config/Public/ConfigManager.h"
#include "Core/Public/Class.h"
#include "Editor/Public/Camera.h"

IMPLEMENT_SINGLETON_CLASS_BASE(UConfigManager)

UConfigManager::UConfigManager()
	: EditorIniFileName("editor.ini")
{
	LoadEditorSetting();
}

UConfigManager::~UConfigManager()
{
	SaveEditorSetting();
}

void UConfigManager::SaveEditorSetting()
{
	std::ofstream Ofs(EditorIniFileName.ToString());
	if (Ofs.is_open())
	{
		Ofs << "CellSize=" << CellSize << "\n";
		Ofs << "CameraSensitivity=" << CameraSensitivity << "\n";
		Ofs << "RootSplitterRatio=" << RootSplitterRatio << "\n";
		Ofs << "LeftSplitterRatio=" << LeftSplitterRatio << "\n";
		Ofs << "RightSplitterRatio=" << RightSplitterRatio << "\n";
	}
}

void UConfigManager::LoadEditorSetting()
{
	const FString& FileNameStr = EditorIniFileName.ToString();
	std::ifstream Ifs(FileNameStr);
	if (!Ifs.is_open())
	{
		CellSize = 1.0f;
		CameraSensitivity = UCamera::DEFAULT_SPEED;
		return; // 파일이 없으면 기본값 유지
	}

	std::string Line;
	while (std::getline(Ifs, Line))
	{
		if (Line.rfind("CellSize=", 0) == 0)
		{
			CellSize = std::stof(Line.substr(9));
		}
		else if (Line.rfind("CameraSensitivity=", 0) == 0)
		{
			CameraSensitivity = std::stof(Line.substr(18));
		}
		else if (Line.rfind("RootSplitterRatio=", 0) == 0)
		{
			RootSplitterRatio = std::stof(Line.substr(18));
		}
		else if (Line.rfind("LeftSplitterRatio=", 0) == 0)
		{
			LeftSplitterRatio = std::stof(Line.substr(18));
		}
		else if (Line.rfind("RightSplitterRatio=", 0) == 0)
		{
			RightSplitterRatio = std::stof(Line.substr(19));
		}
	}
}
