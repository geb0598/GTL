#include "pch.h"

#include "Manager/Asset/Public/ObjImporter.h"

bool FObjImporter::LoadObj(const std::filesystem::path& FilePath, FObjInfo* OutObjInfo, Configuration Config)
{
	if (!OutObjInfo)
	{
		return false;
	}

	if (!std::filesystem::exists(FilePath))
	{
		UE_LOG_ERROR("파일을 찾지 못했습니다: %s", FilePath.string().c_str());
		return false;
	}

	if (FilePath.extension() != ".obj")
	{
		UE_LOG_ERROR("잘못된 파일 확장자입니다: %s", FilePath.string().c_str());
		return false;
	}

	std::ifstream File(FilePath);
	if (!File)
	{
		UE_LOG_ERROR("파일을 열지 못했습니다: %s", FilePath.string().c_str());
		return false;
	}

	size_t FaceCount = 0;

	TOptional<FObjectInfo> OptObjectInfo;

	FString Buffer;
	while (std::getline(File, Buffer))
	{
		std::istringstream Tokenizer(Buffer);
		FString Prefix;

		Tokenizer >> Prefix;

		// ========================== Vertex Information ============================ //

		/** Vertex Position */
		if (Prefix == "v")
		{
			FVector Position;
			if (!(Tokenizer >> Position.X >> Position.Y >> Position.Z))
			{
				UE_LOG_ERROR("정점 위치 형식이 잘못되었습니다");
				return false;
			}

			OutObjInfo->VertexList.emplace_back(Position);
		}
		/** Vertex Normal */
		else if (Prefix == "vn")
		{
			FVector Normal;
			if (!(Tokenizer >> Normal.X >> Normal.Y >> Normal.Z))
			{
				UE_LOG_ERROR("정점 법선 형식이 잘못되었습니다");
				return false;
			}

			OutObjInfo->NormalList.emplace_back(Normal);
		}
		/** Texture Coordinate */
		else if (Prefix == "vt")
		{
			/** @note: Ignore 3D Texture */
			FVector2 TexCoord;
			if (!(Tokenizer >> TexCoord.X >> TexCoord.Y))
			{
				UE_LOG_ERROR("정점 텍스쳐 좌표 형식이 잘못되었습니다");
				return false;
			}

			OutObjInfo->TexCoordList.emplace_back(TexCoord);
		}

		// =========================== Group Information ============================ //

		/** Object Information */
		else if (Prefix == "o")
		{
			if (OptObjectInfo)
			{
				OutObjInfo->ObjectInfoList.emplace_back(std::move(*OptObjectInfo));
			}

			FString ObjectName;
			if (!(Tokenizer >> ObjectName))
			{
				UE_LOG_ERROR("오브젝트 이름 형식이 잘못되었습니다");
				return false;
			}
			OptObjectInfo.emplace();
			OptObjectInfo->Name = std::move(ObjectName);

			FaceCount = 0;
		}

		/** Group Information */
		else if (Prefix == "g")
		{
			if (!OptObjectInfo)
			{
				OptObjectInfo.emplace();
				OptObjectInfo->Name = "DefaultObject";
			}

			FString GroupName;
			if (!(Tokenizer >> GroupName))
			{
				UE_LOG_ERROR("잘못된 그룹 이름 형식입니다");
				return false;
			}

			OptObjectInfo->GroupNameList.emplace_back(std::move(GroupName));
			OptObjectInfo->GroupIndexList.emplace_back(FaceCount);
		}

		// ============================ Face Information ============================ //

		/** Face Information */
		else if (Prefix == "f")
		{
			if (!OptObjectInfo)
			{
				OptObjectInfo.emplace();
				OptObjectInfo->Name = "DefaultObject";
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
				UE_LOG_ERROR("삼각형 메쉬만 지원합니다");
				return false;
			}

			for (size_t i = 0; i < FaceBuffers.size(); ++i)
			{
				if (!ParseFaceBuffer(FaceBuffers[i], &(*OptObjectInfo)))
				{
					return false;
				}
			}

			++FaceCount;
		}

		// ============================ Material Information ============================ //

		else if (Prefix == "mtllib")
		{
			/** @todo: Parse material data */
			/** @todo: Support relative path from .obj file to find .mtl file */
			FString MaterialFileName;
			Tokenizer >> MaterialFileName;

			std::filesystem::path MaterialFilePath = FilePath.parent_path() / MaterialFileName;

			MaterialFilePath = std::filesystem::weakly_canonical(MaterialFilePath);

			if (!LoadMaterial(MaterialFilePath, OutObjInfo))
			{
				UE_LOG_ERROR("머티리얼을 불러오는데 실패했습니다: %s", MaterialFilePath.string().c_str());
				return false;
			}
		}

		else if (Prefix == "usemtl")
		{
			FString MaterialName;
			Tokenizer >> MaterialName;

			if (!OptObjectInfo)
			{
				OptObjectInfo.emplace();
				OptObjectInfo->Name = "DefaultObject";
			}

			OptObjectInfo->MaterialNameList.emplace_back(std::move(MaterialName));
			OptObjectInfo->MaterialIndexList.emplace_back(FaceCount);
		}
	}

	if (OptObjectInfo)
	{
		OutObjInfo->ObjectInfoList.emplace_back(std::move(*OptObjectInfo));
	}

	return true;
}

bool FObjImporter::LoadMaterial(const std::filesystem::path& FilePath, FObjInfo* OutObjInfo)
{
	if (!OutObjInfo)
	{
		return false;
	}

	if (!std::filesystem::exists(FilePath))
	{
		UE_LOG_ERROR("파일을 찾지 못했습니다: %s", FilePath.string().c_str());
		return false;
	}

	if (FilePath.extension() != ".mtl")
	{
		UE_LOG_ERROR("잘못된 파일 확장자입니다: %s", FilePath.string().c_str());
		return false;
	}

	std::ifstream File(FilePath);
	if (!File)
	{
		UE_LOG_ERROR("파일을 열지 못했습니다: %s", FilePath.string().c_str());
		return false;
	}

	// TODO

	return true;
}

bool FObjImporter::ParseFaceBuffer(const FString& FaceBuffer, FObjectInfo* OutObjectInfo)
{
	/** Ignore data when ObjInfo is nullptr */
	if (!OutObjectInfo)
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
		OutObjectInfo->VertexIndexList.push_back(std::stoull(IndexBuffers[0]) - 1);
	}
	catch ([[maybe_unused]] const std::invalid_argument& Exception)
	{
		UE_LOG_ERROR("정점 위치 인덱스 형식이 잘못되었습니다");
		return false;
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
			OutObjectInfo->TexCoordIndexList.push_back(std::stoull(IndexBuffers[1]) - 1);
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
				OutObjectInfo->NormalIndexList.push_back(std::stoull(IndexBuffers[2]) - 1);
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
				OutObjectInfo->TexCoordIndexList.push_back(std::stoull(IndexBuffers[1]) - 1);
				OutObjectInfo->NormalIndexList.push_back(std::stoull(IndexBuffers[2]) - 1);
			}
			catch ([[maybe_unused]] const std::invalid_argument& Exception)
			{
				UE_LOG_ERROR("정점 텍스쳐 좌표 또는 법선 인덱스 형식이 잘못되었습니다");
				return false;
			}
		}
		break;
	}

	return true;
}
