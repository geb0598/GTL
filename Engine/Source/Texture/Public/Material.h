#pragma once
#include "Core/Public/Object.h";

class UTexture;

UCLASS()
class UMaterial : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UMaterial, UObject)

public:
	UMaterial() {};
	UMaterial(const FName& InName) {};
	~UMaterial() override {};
	
	// Texture access functions
	UTexture* GetDiffuseTexture() const { return DiffuseTexture; }
	void SetDiffuseTexture(UTexture* InTexture) { DiffuseTexture = InTexture; }

private:
	UTexture* DiffuseTexture = nullptr;
};
