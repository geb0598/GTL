#include "pch.h"
#include "Actor/Public/BillboardActor.h"

#include "Component/Public/BillboardComponent.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Manager/Path/Public/PathManager.h"

IMPLEMENT_CLASS(ABillboardActor, AActor)

ABillboardActor::ABillboardActor()
{
	auto BillboardComponent = CreateDefaultSubobject<UBillboardComponent>("BillboardComponent");
	SetRootComponent(BillboardComponent);
	if (UBillboardComponent* BillboardComponentRef = Cast<UBillboardComponent>(BillboardComponent))
	{
		path SpritePath = UPathManager::GetInstance().GetAssetPath() / "Icon/S_Pawn.PNG";


		UTexture* Sprite = UAssetManager::GetInstance().CreateTexture(SpritePath.generic_string());
		BillboardComponentRef->SetSprite(Sprite);
	}
}
