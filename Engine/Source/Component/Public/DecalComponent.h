#pragma once
#include "PrimitiveComponent.h"

namespace json { class JSON; }
using JSON = json::JSON;

class UMaterial;

UCLASS()
class UDecalComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UDecalComponent, UPrimitiveComponent)

public:
	UDecalComponent();
	virtual ~UDecalComponent();

	void SetDecalMaterial(UMaterial* InMaterial);
	UMaterial* GetDecalMaterial() const;

	UObject* Duplicate(FObjectDuplicationParameters Parameters) override;
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

private:
	UMaterial* DecalMaterial;
};
