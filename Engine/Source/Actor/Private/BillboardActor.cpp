#include "pch.h"
#include "Actor/Public/BillboardActor.h"

#include "Component/Public/BillboardComponent.h"

IMPLEMENT_CLASS(ABillboardActor, AActor)

ABillboardActor::ABillboardActor()
{
	auto BillboardComponent = CreateDefaultSubobject<UBillboardComponent>("BillboardComponent");
	SetRootComponent(BillboardComponent);
}
