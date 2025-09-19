#pragma once

#include "ObjImporter.h"

struct FAABB;

/**
 * @brief 전역의 On-Memory Asset을 관리하는 매니저 클래스
 */
UCLASS()
class UAssetManager
	: public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UAssetManager, UObject)

public:
	void Initialize();
	void Release();

	// Vertex 관련 함수들
	TArray<FNormalVertex>* GetVertexData(EPrimitiveType InType);
	ID3D11Buffer* GetVertexbuffer(EPrimitiveType InType);
	uint32 GetNumVertices(EPrimitiveType InType);

	// Index 관련 함수들
	TArray<uint32>* GetIndexData(EPrimitiveType InType);
	ID3D11Buffer* GetIndexbuffer(EPrimitiveType InType);
	uint32 GetNumIndices(EPrimitiveType InType);

	// Shader 관련 함수들
	ID3D11VertexShader* GetVertexShader(EShaderType Type);
	ID3D11PixelShader* GetPixelShader(EShaderType Type);
	ID3D11InputLayout* GetIputLayout(EShaderType Type);

	// Texture 관련 함수들
	ComPtr<ID3D11ShaderResourceView> LoadTexture(const FString& InFilePath, const FName& InName = FName::None);
	ComPtr<ID3D11ShaderResourceView> GetTexture(const FString& InFilePath);
	void ReleaseTexture(const FString& InFilePath);
	bool HasTexture(const FString& InFilePath) const;

	// Create Texture
	static ID3D11ShaderResourceView* CreateTextureFromFile(const path& InFilePath);
	static ID3D11ShaderResourceView* CreateTextureFromMemory(const void* InData, size_t InDataSize);

	// Bounding Box
	const FAABB& GetAABB(EPrimitiveType InType);

private:
	// Vertex Resource
	TMap<EPrimitiveType, ID3D11Buffer*> VertexBuffers;
	TMap<EPrimitiveType, uint32> NumVertices;
	TMap<EPrimitiveType, TArray<FNormalVertex>*> VertexDatas;

	// 인덱스 리소스
	TMap<EPrimitiveType, ID3D11Buffer*> IndexBuffers;
	TMap<EPrimitiveType, uint32> NumIndices;
	TMap<EPrimitiveType, TArray<uint32>*> IndexDatas;

	// Shaser Resources
	TMap<EShaderType, ID3D11VertexShader*> VertexShaders;
	TMap<EShaderType, ID3D11InputLayout*> InputLayouts;
	TMap<EShaderType, ID3D11PixelShader*> PixelShaders;

	// Texture Resource
	TMap<FString, ID3D11ShaderResourceView*> TextureCache;

	// Release Functions
	void ReleaseAllTextures();

	// AABB Resource
	TMap<EPrimitiveType, FAABB> AABBs; // 각 타입별 AABB 저장
};
