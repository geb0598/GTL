#include "pch.h"
#include "Render/UI/Widget/Public/ViewportMenuBarWidget.h"
#include "Editor/Public/Viewport.h"
#include "Editor/Public/ViewportClient.h"
#include "Editor/Public/Editor.h"

/* *
* @brief UI에 적용할 색상을 정의합니다.
*/
// 카메라 모드 제어 위젯
const ImVec4 MenuBarBg = ImVec4(pow(0.12f, 2.2f), pow(0.12f, 2.2f), pow(0.12f, 2.2f), 1.00f);
const ImVec4 PopupBg = ImVec4(pow(0.09f, 2.2f), pow(0.09f, 2.2f), pow(0.09f, 2.2f), 1.00f);
const ImVec4 Header = ImVec4(pow(0.20f, 2.2f), pow(0.20f, 2.2f), pow(0.20f, 2.2f), 1.00f);
const ImVec4 HeaderHovered = ImVec4(pow(0.35f, 2.2f), pow(0.35f, 2.2f), pow(0.35f, 2.2f), 1.00f);
const ImVec4 HeaderActive = ImVec4(pow(0.50f, 2.2f), pow(0.50f, 2.2f), pow(0.50f, 2.2f), 1.00f);
const ImVec4 TextColor = ImVec4(pow(0.92f, 2.2f), pow(0.92f, 2.2f), pow(0.92f, 2.2f), 1.00f);
// 카메라 변수 제어 위젯
const ImVec4 FrameBg = ImVec4(pow(0.1f, 2.2f), pow(0.1f, 2.2f), pow(0.1f, 2.2f), 1.00f);
const ImVec4 FrameBgHovered = ImVec4(pow(0.25f, 2.2f), pow(0.25f, 2.2f), pow(0.25f, 2.2f), 1.00f);
const ImVec4 SliderGrab = ImVec4(pow(0.6f, 2.2f), pow(0.6f, 2.2f), pow(0.6f, 2.2f), 1.00f);
const ImVec4 SliderGrabActive = ImVec4(pow(0.8f, 2.2f), pow(0.8f, 2.2f), pow(0.8f, 2.2f), 1.00f);

UViewportMenuBarWidget::~UViewportMenuBarWidget()
{
	Viewport = nullptr;
}

void UViewportMenuBarWidget::RenderWidget()
{
	if (!Viewport) { return; }

	TArray<FViewportClient>& ViewportClients = Viewport->GetViewports();

	for (int Index = 0; Index < ViewportClients.size(); ++Index)
	{
		FViewportClient& ViewportClient = ViewportClients[Index];
		const D3D11_VIEWPORT& ViewportInfo = ViewportClient.GetViewportInfo();

		// 뷰포트 영역이 너무 작으면 렌더링하지 않음
		if (ViewportInfo.Width < 1.0f || ViewportInfo.Height < 1.0f) { continue; }

		// 0. 고유 ID 부여 및 스타일 적용
		ImGui::PushID(Index);
		ImGui::PushStyleColor(ImGuiCol_MenuBarBg, MenuBarBg);
		ImGui::PushStyleColor(ImGuiCol_PopupBg, PopupBg);
		ImGui::PushStyleColor(ImGuiCol_Header, Header);
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, HeaderHovered);
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, HeaderActive);
		ImGui::PushStyleColor(ImGuiCol_Button, Header);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, HeaderHovered);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, HeaderActive);
		ImGui::PushStyleColor(ImGuiCol_Text, TextColor);

		ImGui::PushStyleColor(ImGuiCol_FrameBg, FrameBg);
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, FrameBgHovered);
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, FrameBg);
		ImGui::PushStyleColor(ImGuiCol_SliderGrab, SliderGrab);
		ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, SliderGrabActive);

		// 1. UI를 그릴 위치를 뷰포트의 좌측 상단으로 지정
		ImGui::SetCursorScreenPos(ImVec2(ViewportInfo.TopLeftX, ViewportInfo.TopLeftY));

		// 2. 메뉴바를 담을 투명한 자식 창(컨테이너) 생성
		ImGui::BeginChild("ViewportMenuBarContainer",
			ImVec2(ViewportInfo.Width, 22.0f), // 높이를 살짝 늘려줍니다.
			false, ImGuiWindowFlags_MenuBar |
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse);

		if (ImGui::BeginMenuBar())
		{
			// 3. 기존의 뷰포트 타입 메뉴 (Perspective, Ortho 등)
			if (ImGui::BeginMenu(ClientCameraTypeToString(ViewportClient.GetCameraType())))
			{
				if (ImGui::MenuItem("Perspective"))
				{
					ViewportClient.SetCameraType(EViewportCameraType::Perspective);
					Viewport->UpdateAllViewportClientCameras();
				}

				if (ImGui::BeginMenu("Orthographic"))
				{
					if (ImGui::MenuItem("Top"))
					{
						ViewportClient.SetCameraType(EViewportCameraType::Ortho_Top);
						Viewport->UpdateAllViewportClientCameras();
					}
					if (ImGui::MenuItem("Bottom"))
					{
						ViewportClient.SetCameraType(EViewportCameraType::Ortho_Bottom);
						Viewport->UpdateAllViewportClientCameras();
					}
					if (ImGui::MenuItem("Left"))
					{
						ViewportClient.SetCameraType(EViewportCameraType::Ortho_Left);
						Viewport->UpdateAllViewportClientCameras();
					}
					if (ImGui::MenuItem("Right"))
					{
						ViewportClient.SetCameraType(EViewportCameraType::Ortho_Right);
						Viewport->UpdateAllViewportClientCameras();
					}
					if (ImGui::MenuItem("Front"))
					{
						ViewportClient.SetCameraType(EViewportCameraType::Ortho_Front);
						Viewport->UpdateAllViewportClientCameras();
					}
					if (ImGui::MenuItem("Back"))
					{
						ViewportClient.SetCameraType(EViewportCameraType::Ortho_Back);
						Viewport->UpdateAllViewportClientCameras();
					}
					ImGui::EndMenu();
				}
				ImGui::EndMenu();
			}

			// 4. 각 메뉴의 영역이 구분이 되도록 만듭니다.
			ImGui::Separator();

			// 카메라 설정을 위한 버튼 추가
			if (ImGui::Button("Camera Settings"))
			{
				ImGui::OpenPopup("CameraSettingsPopup");
			}

			// "Camera Settings" 버튼을 눌렀을 때 나타날 팝업 정의
			if (ImGui::BeginPopup("CameraSettingsPopup"))
			{
				RenderCameraControls(ViewportClient.Camera); 
				ImGui::EndPopup();
			}

			ImGui::Separator();

			// 5. 뷰포트 레이아웃 전환 버튼
			{
				const char* LayoutIcon = bIsSingleViewportClient ? "[+]" : "□";
				const char* TooltipText = bIsSingleViewportClient ? "Switch to 4 Viewport" : "Switch to Single Viewport";
				const float ButtonWidth = 25.0f;

				// 메뉴바의 가용 공간 내에서 커서를 오른쪽 끝으로 이동
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - ButtonWidth);

				// Bar의 우측 말단에 배치
				if (ImGui::Button(LayoutIcon, ImVec2(ButtonWidth, 0)))
				{
					// FViewport가 UEditor에 대한 포인터를 가지고 있다고 가정합니다.
					if (Editor)
					{
						bIsSingleViewportClient = !bIsSingleViewportClient;

						if (bIsSingleViewportClient) // 싱글 뷰포트로 전환
						{
							// 현재 활성화된(마우스가 위에 있는) 뷰포트를 찾아 전체 화면으로 만듭니다.
							FViewportClient* ActiveClient = Viewport->GetActiveViewportClient();
							int ActiveIndex = Index; // 활성 뷰포트가 없으면 현재 메뉴바의 뷰포트를 기준으로 합니다.

							if (ActiveClient)
							{
								for (int i = 0; i < ViewportClients.size(); ++i)
								{
									if (&ViewportClients[i] == ActiveClient)
									{
										ActiveIndex = i;
										break;
									}
								}
							}
							Editor->SetSingleViewportLayout(ActiveIndex);
						}
						else // 4분할 뷰포트로 복원
						{
							Editor->RestoreMultiViewportLayout();
						}
					}

				}

				// 호버링 시, 문구 출력
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip(TooltipText);
				}
			}

			ImGui::EndMenuBar();
		}

		ImGui::EndChild();

		ImGui::PopStyleColor(14);

		ImGui::PopID();
	}
}

void UViewportMenuBarWidget::RenderCameraControls(UCamera& InCamera)
{
	// --- UI를 그리기 직전에 항상 카메라로부터 최신 값을 가져옵니다 ---
	auto& Location = InCamera.GetLocation();
	auto& Rotation = InCamera.GetRotation();
	float FovY = InCamera.GetFovY();
	float NearZ = InCamera.GetNearZ();
	float FarZ = InCamera.GetFarZ();
	float OrthoWidth = InCamera.GetOrthoWidth();
	float MoveSpeed = InCamera.GetMoveSpeed();
	int ModeIndex = (InCamera.GetCameraType() == ECameraType::ECT_Perspective) ? 0 : 1;
	static const char* CameraMode[] = { "Perspective", "Orthographic" };


	ImGui::Text("Camera Properties");
	ImGui::Separator();

	// --- UI 렌더링 및 상호작용 ---
	if (ImGui::SliderFloat("Move Speed", &MoveSpeed, UCamera::MIN_SPEED, UCamera::MAX_SPEED, "%.1f"))
	{
		InCamera.SetMoveSpeed(MoveSpeed); // 변경 시 즉시 적용
	}

	ImGui::DragFloat3("Location", &Location.X, 0.05f);
	ImGui::DragFloat3("Rotation", &Rotation.X, 0.1f);

	bool bOpticsChanged = false;
	if (ModeIndex == 0) // 원근 투영 
	{
		bOpticsChanged |= ImGui::SliderFloat("FOV", &FovY, 1.0f, 170.0f, "%.1f");
		bOpticsChanged |= ImGui::DragFloat("Z Near", &NearZ, 0.01f, 0.0001f, 1e6f, "%.4f");
		bOpticsChanged |= ImGui::DragFloat("Z Far", &FarZ, 0.1f, 0.001f, 1e7f, "%.3f");
	}
	else if (ModeIndex == 1) // 직교 투영
	{
		bOpticsChanged |= ImGui::SliderFloat("OrthoWidth", &OrthoWidth, 1.0f, 150.0f, "%.1f");
	}

	// 변경된 값을 카메라에 다시 적용
	if (bOpticsChanged)
	{
		InCamera.SetFovY(FovY);
		InCamera.SetNearZ(NearZ);
		InCamera.SetFarZ(FarZ);
		InCamera.SetOrthoWidth(OrthoWidth);
	}
}
