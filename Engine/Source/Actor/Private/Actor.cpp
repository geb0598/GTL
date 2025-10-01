#include "pch.h"
#include "Actor/Public/Actor.h"
#include "Component/Public/SceneComponent.h"
#include "Component/Public/TextRenderComponent.h"
#include "Level/Public/Level.h"
#include "Manager/Level/Public/LevelManager.h"

IMPLEMENT_CLASS(AActor, UObject)

AActor::AActor()
{
	// to do: primitive factory로 빌보드 생성
	// BillBoardComponent = new UBillboardComponent(this, 5.0f);
	// OwnedComponents.push_back(TObjectPtr<UBillboardComponent>(BillBoardComponent));
}

AActor::AActor(UObject* InOuter)
{
	SetOuter(InOuter);
}

AActor::~AActor()
{
	for (UActorComponent* Component : OwnedComponents)
	{
		SafeDelete(Component);
	}
	SetOuter(nullptr);
	OwnedComponents.clear();
}

void AActor::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (RootComponent)
	{
		RootComponent->Serialize(bInIsLoading, InOutHandle);
	}
}

void AActor::SetActorLocation(const FVector& InLocation) const
{
	if (RootComponent)
	{
		RootComponent->SetRelativeLocation(InLocation);
	}
}

void AActor::SetActorRotation(const FVector& InRotation) const
{
	if (RootComponent)
	{
		RootComponent->SetRelativeRotation(InRotation);
	}
}

void AActor::SetActorScale3D(const FVector& InScale) const
{
	if (RootComponent)
	{
		RootComponent->SetRelativeScale3D(InScale);
	}
}

void AActor::SetUniformScale(bool IsUniform)
{
	if (RootComponent)
	{
		RootComponent->SetUniformScale(IsUniform);
	}
}

bool AActor::IsUniformScale() const
{
	if (RootComponent)
	{
		return RootComponent->IsUniformScale();
	}
	return false;
}

const FVector& AActor::GetActorLocation() const
{
	assert(RootComponent);
	return RootComponent->GetRelativeLocation();
}

const FVector& AActor::GetActorRotation() const
{
	assert(RootComponent);
	return RootComponent->GetRelativeRotation();
}

const FVector& AActor::GetActorScale3D() const
{
	assert(RootComponent);
	return RootComponent->GetRelativeScale3D();
}

void AActor::AddComponent(TObjectPtr<UActorComponent> InComponent)
{
	if (!InComponent || !RootComponent)
	{
		return;
	}

	InComponent->SetOwner(this);
	OwnedComponents.push_back(InComponent);

	USceneComponent* InSceneComponent = Cast<USceneComponent>(InComponent);
	RootComponent->AddChild(InSceneComponent);
	InSceneComponent->SetParentAttachment(RootComponent);

	TObjectPtr<UPrimitiveComponent> PrimitiveComponent = Cast<UPrimitiveComponent>(InComponent);
	ULevelManager::GetInstance().GetCurrentLevel()->AddLevelPrimitiveComponent(PrimitiveComponent);
}

void AActor::Tick()
{
	for (auto& Component : OwnedComponents)
	{
		if (Component)
		{
			Component->TickComponent();
		}
	}
}

void AActor::BeginPlay()
{
}

void AActor::EndPlay()
{
}
