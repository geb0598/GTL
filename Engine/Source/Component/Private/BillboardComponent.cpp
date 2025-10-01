#include "pch.h"
#include "Component/Public/BillboardComponent.h"

UBillboardComponent::UBillboardComponent(AActor* InOwnerActor)
{
	SetName("BillboardComponent");
	Type = EPrimitiveType::Billboard;
}

UBillboardComponent::~UBillboardComponent()
{

}

