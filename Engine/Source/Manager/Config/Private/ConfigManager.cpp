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

		for (int Index = 0; Index < 4; ++Index)
		{
			const auto& Data = ViewportCameraSettings[Index];
			std::string Prefix = "Viewport" + std::to_string(Index);
			Ofs << Prefix + "_CameraType=" << static_cast<int>(Data.ViewportCameraType) << "\n";
			Ofs << Prefix + "_Location=" << VectorToString(Data.Location) << "\n";
			Ofs << Prefix + "_Rotation=" << VectorToString(Data.Rotation) << "\n";
			Ofs << Prefix + "_FocusLocation=" << VectorToString(Data.FocusLocation) << "\n";
			Ofs << Prefix + "_FovY=" << Data.FovY << "\n";
			Ofs << Prefix + "_OrthoWidth=" << Data.OrthoWidth << "\n";
			Ofs << "\n";
		}
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
		size_t DelimiterPos = Line.find('=');
		if (DelimiterPos == std::string::npos) continue;

		std::string Key = Line.substr(0, DelimiterPos);
		std::string Value = Line.substr(DelimiterPos + 1);

		// --- 기본 설정 파싱 ---
		if (Key == "CellSize") CellSize = std::stof(Value);
		else if (Key == "CameraSensitivity") CameraSensitivity = std::stof(Value);
		else if (Key == "RootSplitterRatio") RootSplitterRatio = std::stof(Value);
		else if (Key == "LeftSplitterRatio") LeftSplitterRatio = std::stof(Value);
		else if (Key == "RightSplitterRatio") RightSplitterRatio = std::stof(Value);

		// --- 뷰포트 카메라 설정 파싱 ---
		else if (Key.rfind("Viewport", 0) == 0)
		{
			// "Viewport" 다음의 숫자(인덱스)를 추출
			int Index = std::stoi(Key.substr(8, 1));
			if (Index >= 0 && Index < 4)
			{
				// 키의 나머지 부분 (e.g., "_Location")을 확인
				std::string Suffix = Key.substr(9);
				if (Suffix == "_CameraType") ViewportCameraSettings[Index].ViewportCameraType = static_cast<EViewportCameraType>(std::stoi(Value));
				else if (Suffix == "_Location") ViewportCameraSettings[Index].Location = StringToVector(Value);
				else if (Suffix == "_Rotation") ViewportCameraSettings[Index].Rotation = StringToVector(Value);
				else if (Suffix == "_FocusLocation") ViewportCameraSettings[Index].FocusLocation = StringToVector(Value);
				else if (Suffix == "_FovY") ViewportCameraSettings[Index].FovY = std::stof(Value);
				else if (Suffix == "_OrthoWidth") ViewportCameraSettings[Index].OrthoWidth = std::stof(Value);
			}
		}
	}
}

FVector UConfigManager::StringToVector(const std::string& InString)
{
	std::stringstream StringStream(InString);
	std::string Item;
	std::vector<float> Components;
	while (std::getline(StringStream, Item, ','))
	{
		Components.push_back(std::stof(Item));
	}

	if (Components.size() == 3) { return FVector(Components[0], Components[1], Components[2]); }

	return FVector::Zero(); // 파싱 실패 시 0 벡터 반환
}

// FVector를 "x,y,z" 형태의 문자열로 변환
std::string UConfigManager::VectorToString(const FVector& InVector)
{
	return std::to_string(InVector.X) + "," + std::to_string(InVector.Y) + "," + std::to_string(InVector.Z);
}
