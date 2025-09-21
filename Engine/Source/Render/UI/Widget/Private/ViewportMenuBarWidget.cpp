#include "pch.h"
#include "Render/UI/Widget/Public/ViewportMenuBarWidget.h"
#include "Editor/Public/ViewportClient.h"

UViewportMenuBarWidget::~UViewportMenuBarWidget()
{
	ViewportClient = nullptr;
}

void UViewportMenuBarWidget::RenderWidget()
{
	if (!ViewportClient) { return; }

	TArray<FViewport>& Viewports = ViewportClient->GetViewports();

	for (int i = 0; i < Viewports.size(); ++i)
	{
		FViewport& Viewport = Viewports[i];
		const D3D11_VIEWPORT& ViewportInfo = Viewport.GetViewport();

		// 뷰포트 영역이 접힘 상태라면 렌더링 비활성화
		if (ViewportInfo.Width < 1.0f || ViewportInfo.Height < 1.0f) { continue; }

		// 0. 고유 ID 부여
		ImGui::PushID(i);

		// 1. UI를 그릴 위치를 뷰포트의 좌측 상단으로 지정
		ImGui::SetCursorScreenPos(ImVec2(ViewportInfo.TopLeftX, ViewportInfo.TopLeftY));

		// 2. 메뉴바를 담을 투명한 자식 창(컨테이너)을 만듭니다.
		ImGui::BeginChild("ViewportMenuBarContainer",
			ImVec2(ViewportInfo.Width, 20.0f),
			false, ImGuiWindowFlags_MenuBar |
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse);

		// 3. 자식 창 내부에 실제 메뉴바를 생성합니다. 이 메뉴바는 자동으로 배경색을 가집니다.
		if (ImGui::BeginMenuBar())
		{
			// 4. 버튼 대신 BeginMenu를 사용하여 메뉴 항목을 만듭니다.
			if (ImGui::BeginMenu(ViewportTypeToString(Viewport.GetViewportCameraType())))
			{
				// 메뉴를 클릭하면 아래 항목들이 드롭다운으로 나타납니다.
				if (ImGui::MenuItem("Perspective")) { Viewport.SetViewportCameraType(EViewportCameraType::Perspective); }
				if (ImGui::BeginMenu("Orthographic"))
				{
					if (ImGui::MenuItem("Top")) { Viewport.SetViewportCameraType(EViewportCameraType::Ortho_Top); }
					if (ImGui::MenuItem("Bottom")) { Viewport.SetViewportCameraType(EViewportCameraType::Ortho_Bottom); }
					if (ImGui::MenuItem("Left")) { Viewport.SetViewportCameraType(EViewportCameraType::Ortho_Left); }
					if (ImGui::MenuItem("Right")) { Viewport.SetViewportCameraType(EViewportCameraType::Ortho_Right); }
					if (ImGui::MenuItem("Front")) { Viewport.SetViewportCameraType(EViewportCameraType::Ortho_Front); }
					if (ImGui::MenuItem("Back")) { Viewport.SetViewportCameraType(EViewportCameraType::Ortho_Back); }
					ImGui::EndMenu();
				}
				ImGui::EndMenu();

			}

			ImGui::EndMenuBar();
		}

		ImGui::EndChild();
		ImGui::PopID();
	}
}
