#include "pch.h"
#include "Render/UI/Widget/Public/StaticMeshComponentWidget.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Mesh/Public/StaticMesh.h"
#include "Manager/Level/Public/LevelManager.h"
#include "Level/Public/Level.h"

IMPLEMENT_CLASS(UStaticMeshComponentWidget, UWidget)

void UStaticMeshComponentWidget::RenderWidget()
{
	ImGui::TextUnformatted("Static Mesh Component Detail Widget");
}
