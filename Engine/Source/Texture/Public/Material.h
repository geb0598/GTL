#pragma once
#include "Core/Public/Object.h";

class UTexture;

/**
 * @note: This struct is exactly same as the one defined in ObjImporter.h.
 * This is intentionally introduced for abstractional layer of material object.
 */
struct FMaterial
{
	FString Name;

	/** Ambient color (Ka). */
	FVector Ka;

	/** Diffuse color (Kd). */
	FVector Kd;

	/** Specular color (Ks). */
	FVector Ks;

	/** Emissive color (Ke) */
	FVector Ke;

	/** Specular exponent (Ns). Defines the size of the specular highlight. */
	float Ns;

	/** Optical density or index of refraction (Ni). */
	float Ni;

	/** Dissolve factor (d). 1.0 is fully opaque. */
	float D;

	/** Illumination model (illum). */
	int32 Illumination;

	/** Ambient texture map (map_Ka). */
	FString KaMap;

	/** Diffuse texture map (map_Kd). */
	FString KdMap;

	/** Specular texture map (map_Ks). */
	FString KsMap;

	/** Specular highlight map (map_Ns). */
	FString NsMap;

	/** Alpha texture map (map_d). */
	FString DMap;

	/** Bump map (map_bump or bump). */
	FString BumpMap;
};

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
