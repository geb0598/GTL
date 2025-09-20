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
	std::ofstream ofs(EditorIniFileName.ToString());
	if (ofs.is_open())
	{
		ofs << "CellSize=" << CellSize << "\n";
		ofs << "CameraSensitivity=" << CameraSensitivity << "\n";
		ofs << "RootSplitterRatio=" << RootSplitterRatio << "\n";
		ofs << "LeftSplitterRatio=" << LeftSplitterRatio << "\n";
		ofs << "RightSplitterRatio=" << RightSplitterRatio << "\n";
	}
}

void UConfigManager::LoadEditorSetting()
{
	const FString& fileNameStr = EditorIniFileName.ToString();
	std::ifstream ifs(fileNameStr);
	if (!ifs.is_open())
	{
		CellSize = 1.0f;
		CameraSensitivity = UCamera::DEFAULT_SPEED;
		return; // 파일이 없으면 기본값 유지
	}

	std::string line;
	while (std::getline(ifs, line))
	{
		if (line.rfind("CellSize=", 0) == 0)
		{
			CellSize = std::stof(line.substr(9));
		}
		else if (line.rfind("CameraSensitivity=", 0) == 0)
		{
			CameraSensitivity = std::stof(line.substr(18));
		}
		else if (line.rfind("RootSplitterRatio=", 0) == 0)
		{
			RootSplitterRatio = std::stof(line.substr(18));
		}
		else if (line.rfind("LeftSplitterRatio=", 0) == 0)
		{
			LeftSplitterRatio = std::stof(line.substr(18));
		}
		else if (line.rfind("RightSplitterRatio=", 0) == 0)
		{
			RightSplitterRatio = std::stof(line.substr(19));
		}
	}
}
