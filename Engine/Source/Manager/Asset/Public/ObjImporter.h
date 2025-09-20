#pragma once

// C++ Standard Libraries
#include <filesystem>
#include <fstream>
#include <sstream>

// GTL Headers
#include "Global/Macro.h"
#include "Global/Types.h"
#include "Global/Vector.h"


struct FObjectInfo
{
	FString ObjectName;
	FString MaterialLibraryName;

	/** Vertex Information */
	TArray<FVector> Vertices;
	TArray<FVector> Normals;
	TArray<FVector2> TexCoords;

	/** Face Information */
	TArray<size_t> VertexIndices;
	TArray<size_t> NormalIndices;
	TArray<size_t> TexCoordIndices;

	/** Group Information */
	TArray<FString> GroupNames;
	TArray<size_t> GroupIndices; 

	/** Material Information */
	TArray<FString> MaterialNames;
	TArray<size_t> MaterialIndices; 

	TArray<FString> TextureNames;
};

struct FObjMaterialInfo
{
	/** @todo */
	FString Name;

	/** Ambient Color */
	FVector Ka;

	/** Diffuse Color */
	FVector Kd;

	/** Specular Color */
	FVector Ks;

	/** Specualr Exponent */
	float Ns;

	/** Optical Density */
	float Ni;

	/** Dissolve */
	float D;

	/** Illumination */
	int32 Illumination;

	/** Ambient Texture Map */
	FString KaMap;

	/** Diffuse Texture Map */
	FString KdMap;

	/** Specular Texture Map */
	FString KsMap;

	/** Specular Highlight Map */
	FString NsMap;

	/** Alpha Texture Map */
	FString DMap;

	/** Bump Map */
	FString BumpMap;
};

struct FObjImporter
{
	/** @todo: Parse texture and material information */
	static bool LoadObj(const std::filesystem::path& FileName, FObjectInfo* ObjInfo, FObjMaterialInfo* ObjMaterialInfo)
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

		size_t FaceCount = 0;

		bool bHasNormal = false;
		bool bHasTexCoord = false;

		FString Buffer;
		while (std::getline(File, Buffer))
		{
			std::istringstream Tokenizer;
			FString Prefix;

			Tokenizer >> Prefix;

			// ========================== Vertex Information ============================ //

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

			// =========================== Group Information ============================ //

			/** Object Information */
			else if (Prefix == "o")
			{
				if (!ObjInfo)
				{
					continue;
				}

				FString ObjectName;
				if (!(Tokenizer >> ObjectName))
				{
					UE_LOG_ERROR("오브젝트 이름 형식이 잘못되었습니다");
					return false;
				}

				ObjInfo->ObjectName = std::move(ObjectName);
				// For now, there is no support for multiple objects.
			}

			/** Group Information */
			else if (Prefix == "g")
			{
				if (!ObjInfo)
				{
					continue;
				}

				FString GroupName;
				if (!(Tokenizer >> GroupName))
				{
					UE_LOG_ERROR("잘못된 그룹 이름 형식입니다");
					return false;
				}

				ObjInfo->GroupIndices.emplace_back(FaceCount);
				ObjInfo->GroupNames.emplace_back(GroupName);
			}

			// ============================ Face Information ============================ //

			/** Face Information */
			else if (Prefix == "f")
			{
				/** Ignore data when ObjInfo is nullptr */
				if (!ObjInfo)
				{
					continue;
				}

				TArray<FString> FaceBuffers;
				FString FaceBuffer;
				while (Tokenizer >> FaceBuffer)
				{
					FaceBuffers.emplace_back(FaceBuffer);
				}

				if (FaceBuffers.size() < 2)
				{
					UE_LOG_ERROR("면 형식이 잘못되었습니다");
					return false;
				}

				/** @todo: Support other types of meshes in later */
				if (FaceBuffers.size() > 3)
				{
					UE_LOG_ERROR("삼각형 메쉬만을 지원합니다");
					return false;
				}

				for (size_t i = 0; i < FaceBuffers.size(); ++i)
				{
					if (!ParseFaceBuffer(FaceBuffers[i], ObjInfo))
					{
						return false;
					}
				}

				++FaceCount;
			}

			// ============================ Material Information ============================ //

			/** relative path */
			else if (Prefix == "mtllib")
			{

			}

			else if (Prefix == "usemtl")
			{

			}
		}


		return true;
	}

	static bool LoadMaterial(const std::filesystem::path& FileName, FObjMaterialInfo* ObjMaterialInfo)
	{
		if (!std::filesystem::exists(FileName))
		{
			UE_LOG_ERROR("파일을 찾지 못했습니다: %s", FileName.string().c_str());
			return false;
		}

		if (FileName.extension() != ".mtl")
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

		// TODO

		return true;
	}

private:
	/**
	 * @brief 
	 * @param FaceBuffer 
	 * @param ObjInfo 
	 * @return 
	 * @note: This function doesn't care about difference between faces.
	 * So, it would be problematic to use different face formats at same file.
	 * (e.g., 'f 1//2 2//3 2//4' and 'f 1/1/2 2/2/3 3/2/3' in the same file)
	 */
	static bool ParseFaceBuffer(const FString& FaceBuffer, FObjectInfo* ObjInfo)
	{
		/** Ignore data when ObjInfo is nullptr */
		if (!ObjInfo)
		{
			return false;
		}

		TArray<FString> IndexBuffers;
		std::istringstream Tokenizer(FaceBuffer);
		FString IndexBuffer;
		while (std::getline(Tokenizer, IndexBuffer, '/'))
		{
			IndexBuffers.emplace_back(IndexBuffer);
		}

		if (IndexBuffers.empty())
		{
			UE_LOG_ERROR("면 형식이 잘못되었습니다");
			return false;
		}

		if (IndexBuffers[0].empty())
		{
			UE_LOG_ERROR("정점 위치 형식이 잘못되었습니다");
			return false;
		}

		try
		{
			ObjInfo->VertexIndices.push_back(std::stoull(IndexBuffers[0]) - 1);
		}
		catch ([[maybe_unused]] const std::invalid_argument& Exception)
		{
			UE_LOG_ERROR("정점 위치 인덱스 형식이 잘못되었습니다");
		}

		switch (IndexBuffers.size())
		{
		case 1:
			/** @brief: Only position data (e.g., 'f 1 2 3') */
			break;
		case 2:
			/** @brief: Position and texture coordinate data (e.g., 'f 1/1 2/1') */
			if (IndexBuffers[1].empty())
			{
				UE_LOG_ERROR("정점 텍스쳐 좌표 인덱스 형식이 잘못되었습니다");
				return false;
			}

			try
			{
				ObjInfo->TexCoordIndices.push_back(std::stoull(IndexBuffers[1]) - 1);
			}
			catch ([[maybe_unused]] const std::invalid_argument& Exception)
			{
				UE_LOG_ERROR("정점 텍스쳐 좌표 인덱스 형식이 잘못되었습니다");
				return false;
			}
			break;
		case 3:
			/** @brief: Position, texture coordinate and vertex normal data (e.g., 'f 1/1/1 2/2/1' or 'f 1//1 2//1') */
			if (IndexBuffers[1].empty()) /** Position and vertex normal */
			{
				if (IndexBuffers[2].empty())
				{
					UE_LOG_ERROR("정점 법선 인덱스 형식이 잘못되었습니다");
					return false;
				}

				try
				{
					ObjInfo->NormalIndices.push_back(std::stoull(IndexBuffers[2]) - 1);
				}
				catch ([[maybe_unused]] const std::invalid_argument& Exception)
				{
					UE_LOG_ERROR("정점 법선 인덱스 형식이 잘못되었습니다");
					return false;
				}
			}
			else /** Position, texture coordinate, and vertex normal */
			{
				if (IndexBuffers[1].empty() || IndexBuffers[2].empty())
				{
					UE_LOG_ERROR("정점 텍스쳐 좌표 또는 법선 인덱스 형식이 잘못되었습니다");
					return false;
				}

				try
				{
					ObjInfo->TexCoordIndices.push_back(std::stoull(IndexBuffers[1]) - 1);
					ObjInfo->NormalIndices.push_back(std::stoull(IndexBuffers[2]) - 1);
				}
				catch ([[maybe_unused]] const std::invalid_argument& Exception)
				{
					UE_LOG_ERROR("정점 텍스쳐 좌표 또는 법선 인덱스 형식이 잘못되었습니다");
					return false;
				}
			}
			break;
		}
	}
};
