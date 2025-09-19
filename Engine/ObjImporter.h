#pragma once

// C++ Standard Libraries
#include <filesystem>
#include <fstream>
#include <sstream>

// GTL Headers
#include "Global/Macro.h"
#include "Global/Types.h"
#include "Global/Vector.h"

struct FObjInfo
{
	TArray<FVector> Vertices;
	TArray<FVector> Normals;
	TArray<FVector2> TexCoords;

	TArray<size_t> VertexIndices;
	TArray<size_t> NormalIndices;
	TArray<size_t> TexCoordIndices;

	TArray<FString> Materials;
	TArray<FString> Textures;
};

struct FObjMaterialInfo
{
	/** @todo */
	//FString Name;
};

struct FObjImporter
{
	/** @todo: Parse texture and material information */
	static bool LoadObj(const std::filesystem::path& FileName, FObjInfo* ObjInfo, FObjMaterialInfo* ObjMaterialInfo)
	{
		if (!std::filesystem::exists(FileName))
		{
			UE_LOG_ERROR("파일을 찾지 못했습니다: %s", FileName.string().c_str());
			return false;
		}

		if (FileName.extension() != ".obj")
		{
			UE_LOG_ERROR("잘못된 파일 확장자입니다: %s", FileName.string().c_str());
			return false;
		}

		std::ifstream File(FileName);
		if (!File)
		{
			UE_LOG_ERROR("파일을 열지 못했습니다: %s", FileName.string().c_str());
			return false;
		}

		FString Buffer;
		while (std::getline(File, Buffer))
		{
			std::istringstream Tokenizer;
			FString Prefix;

			Tokenizer >> Prefix;

			/** Vertex Position */
			if (Prefix == "v")
			{
				/** Ignore data when ObjInfo is nullptr */
				if (!ObjInfo)
				{
					continue;
				}

				FVector Position;
				if (!(Tokenizer >> Position.X >> Position.Y >> Position.Z))
				{
					UE_LOG_ERROR("정점 위치 형식이 잘못되었습니다");
					return false;
				}

				ObjInfo->Vertices.emplace_back(Position);
			}
			/** Vertex Normal */
			else if (Prefix == "vn")
			{
				/** Ignore data when ObjInfo is nullptr */
				if (!ObjInfo)
				{
					continue;
				}

				FVector Normal;
				if (!(Tokenizer >> Normal.X >> Normal.Y >> Normal.Z))
				{
					UE_LOG_ERROR("정점 법선 형식이 잘못되었습니다");
					return false;
				}
				ObjInfo->Normals.emplace_back(Normal);
			}
			/** Texture Coordinate */
			else if (Prefix == "vt")
			{
				/** Ignore data when ObjInfo is nullptr */
				if (!ObjInfo)
				{
					continue;
				}

				/** @note: Ignore 3D Texture */
				FVector2 TexCoord;
				if (!(Tokenizer >> TexCoord.X >> TexCoord.Y))
				{
					UE_LOG_ERROR("정점 텍스쳐 좌표 형식이 잘못되었습니다");
					return false;
				}

				ObjInfo->TexCoords.emplace_back(TexCoord);
			}
			/** Face Information */
			else if (Prefix == "f")
			{
				/** Ignore data when ObjInfo is nullptr */
				if (!ObjInfo)
				{
					continue;
				}

				FString FaceBuffer;
				while (Tokenizer >> FaceBuffer)
				{
					std::istringstream FaceTokenizer(Buffer);
					FString IndexBuffer;
					
					/** #1. Parse Index of Vertex Position */
					if (!std::getline(FaceTokenizer, IndexBuffer, '/') || IndexBuffer.empty())
					{
						UE_LOG_ERROR("정점 위치 인덱스 형식이 잘못되었습니다");
						return false;
					}

					try
					{
						ObjInfo->VertexIndices.push_back(std::stoull(IndexBuffer) - 1);
					}
					catch ([[maybe_unused]] const std::invalid_argument& exception)
					{
						UE_LOG_ERROR("정점 위치 인덱스 형식이 잘못되었습니다");
						return false;
					}

					/** #2. Parse Index of Vertex Normal */
					if (!std::getline(FaceTokenizer, IndexBuffer, '/') || IndexBuffer.empty())
					{
						UE_LOG_ERROR("정점 법선 인덱스 형식이 잘못되었습니다");
						return false;
					}

					try
					{
						ObjInfo->NormalIndices.push_back(std::stoull(IndexBuffer) - 1);
					}
					catch ([[maybe_unused]] const std::invalid_argument& exception)
					{
						UE_LOG_ERROR("정점 법선 인덱스 형식이 잘못되었습니다");
						return false;
					}

					/** #3. Parse Index of Texture Coordinate */
					if (!std::getline(FaceTokenizer, IndexBuffer, '/') || IndexBuffer.empty())
					{
						UE_LOG_ERROR("정점 텍스쳐 좌표 인덱스 형식이 잘못되었습니다");
						return false;
					}

					try
					{
						ObjInfo->TexCoordIndices.push_back(std::stoull(IndexBuffer) - 1);
					}
					catch ([[maybe_unused]] const std::invalid_argument& exception)
					{
						UE_LOG_ERROR("정점 텍스쳐 좌표 인덱스 형식이 잘못되었습니다");
						return false;
					}
				}
			}
			/** Others... */
		}
	}
};
