#pragma once

// --- Standard Library Includes ---
#include <string>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <cstdint>

// --- Project Includes ---
// FString, FVector, TArray, TMap 등의 타입이 정의된 헤더 파일을 포함해야 합니다.
// #include "Core/Public/CoreTypes.h" 
// #include "Core/Public/Object.h" // UE_LOG 등
#include "json.hpp" // 사용하는 JSON 라이브러리

namespace json { class JSON; }
using JSON = json::JSON;

/**
 * @brief Level 직렬화에 관여하는 클래스
 * JSON 기반으로 레벨의 데이터를 Save / Load 처리
 */
class FJsonSerializer
{
public:
	//====================================================================================
	// Reading from JSON
	//====================================================================================

	/**
	 * @brief JSON 객체에서 키를 찾아 FString 값을 읽어옵니다.
	 * 키가 없거나 타입이 문자열이 아니면 기본값을 사용합니다.
	 */
	static void ReadString(const JSON& InJson, const std::string& InKey, FString& OutValue, const FString& InDefaultValue = "")
	{
		if (InJson.hasKey(InKey))
		{
			const json::JSON& Value = InJson.at(InKey);
			if (Value.JSONType() == json::JSON::Class::String)
			{
				// json.hpp의 ToString()이 std::string을 반환한다고 가정합니다.
				// FString이 std::string으로부터 생성 가능해야 합니다.
				OutValue = Value.ToString();
				return;
			}
		}

		UE_LOG_ERROR("[JsonSerializer] %s String 파싱에 실패했습니다", InKey.c_str());
		OutValue = InDefaultValue;
	}

	/**
	 * @brief JSON 객체에서 키를 찾아 FVector 값을 안전하게 읽어옵니다.
	 * 키가 없거나, 유효한 Vector 배열이 아니면 기본값을 사용합니다.
	 */
	static void ReadVector(const JSON& InJson, const std::string& InKey, FVector& OutValue, const FVector& InDefaultValue = FVector::Zero())
	{
		if (InJson.hasKey(InKey))
		{
			const JSON& VectorJson = InJson.at(InKey);
			if (VectorJson.JSONType() == JSON::Class::Array && VectorJson.size() == 3)
			{
				try
				{
					OutValue = {
						static_cast<float>(VectorJson.at(0).ToFloat()),
						static_cast<float>(VectorJson.at(1).ToFloat()),
						static_cast<float>(VectorJson.at(2).ToFloat())
					};
					return;
				}
				catch (const std::exception&)
				{
				}
			}
		}

		UE_LOG_ERROR("[JsonSerializer] %s Vector 파싱에 실패했습니다", InKey.c_str());
		OutValue = InDefaultValue;
	}

	/**
	 * @brief 부호 없는 32비트 정수(uint32_t)를 안전하게 읽어옵니다.
	 * 음수 값이거나 uint32_t의 표현 범위를 벗어나면 기본값을 사용합니다.
	 */
	static void ReadUint32(const JSON& InJson, const std::string& InKey, uint32_t& OutValue, uint32_t InDefaultValue = 0)
	{
		int64_t value_i64;
		if (ReadInt64(InJson, InKey, value_i64))
		{
			if (value_i64 >= 0 && value_i64 <= UINT32_MAX)
			{
				OutValue = static_cast<uint32_t>(value_i64);
				return;
			}

		}

		UE_LOG_ERROR("[JsonSerializer] %s uint32 파싱에 실패했습니다", InKey.c_str());
		OutValue = InDefaultValue;
	}

	//====================================================================================
	// Converting To JSON
	//====================================================================================

	static JSON VectorToJson(const FVector& InVector)
	{
		JSON VectorArray = JSON::Make(JSON::Class::Array);
		VectorArray.append(InVector.X, InVector.Y, InVector.Z);
		return VectorArray;
	}

	//====================================================================================
	// File I/O
	//====================================================================================

	static bool SaveJsonToFile(const JSON& InJsonData, const FString& InFilePath)
	{
		try
		{
			std::ofstream File(InFilePath);
			if (!File.is_open())
			{
				return false;
			}
			File << std::setw(2) << InJsonData << "\n";
			File.close();
			return true;
		}
		catch (const std::exception&)
		{
			return false;
		}
	}

	static bool LoadJsonFromFile(JSON& OutJson, const FString& InFilePath)
	{
		try
		{
			std::ifstream File(InFilePath);
			if (!File.is_open())
			{
				return false;
			}

			std::string FileContent((std::istreambuf_iterator<char>(File)), std::istreambuf_iterator<char>());
			File.close();

			std::cout << "[JsonSerializer] File Content Length: " << FileContent.length() << "\n";
			OutJson = JSON::Load(FileContent);
			return true;
		}
		catch (const std::exception&)
		{
			return false;
		}
	}

	//====================================================================================
	// Utility & Analysis Functions
	//====================================================================================

	static FString FormatJsonString(const JSON& JsonData, int Indent = 2)
	{
		return JsonData.dump(Indent);
	}

	struct FLevelStats
	{
		uint32 TotalPrimitives = 0;
		TMap<EPrimitiveType, uint32> PrimitiveCountByType;
	};

private:
	static bool HandleJsonError(const std::exception& InException, const FString& InContext, FString& OutErrorMessage)
	{
		OutErrorMessage = InContext + ": " + InException.what();
		return false;
	}

	static bool ReadInt64(const json::JSON& InJson, const std::string& InKey, int64_t& OutValue)
	{
		if (InJson.hasKey(InKey))
		{
			const json::JSON& Value = InJson.at(InKey);
			if (Value.JSONType() == json::JSON::Class::Integral)
			{
				OutValue = Value.ToInt();
				return true;
			}
		}
		return false;
	}
};
