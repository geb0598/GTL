#include "pch.h"
#include "Render/UI/Widget/Public/SceneHierarchyWidget.h"

#include "Manager/Level/Public/LevelManager.h"
#include "Level/Public/Level.h"
#include "Actor/Public/Actor.h"
#include "Editor/Public/Camera.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Editor/Public/ViewportClient.h"
#include "Global/Quaternion.h"

USceneHierarchyWidget::USceneHierarchyWidget()
	: UWidget("Scene Hierarchy Widget")
{
}

USceneHierarchyWidget::~USceneHierarchyWidget() = default;

void USceneHierarchyWidget::Initialize()
{
	UE_LOG("SceneHierarchyWidget: Initialized");
}

void USceneHierarchyWidget::Update()
{
	// 카메라 애니메이션 업데이트
	if (bIsCameraAnimating)
	{
		UpdateCameraAnimation();
	}
}

void USceneHierarchyWidget::RenderWidget()
{
	ULevel* CurrentLevel = ULevelManager::GetInstance().GetCurrentLevel();

	if (!CurrentLevel)
	{
		ImGui::TextUnformatted("No Level Loaded");
		return;
	}

	// 헤더 정보
	ImGui::Text("Level: %s", CurrentLevel->GetName().ToString().c_str());
	ImGui::Separator();

	// 검색창 렌더링
	RenderSearchBar();

	const TArray<TObjectPtr<AActor>>& LevelActors = CurrentLevel->GetLevelActors();

	if (LevelActors.empty())
	{
		ImGui::TextUnformatted("No Actors in Level");
		return;
	}

	// 필터링 업데이트
	if (bNeedsFilterUpdate)
	{
		UE_LOG("SceneHierarchy: 필터 업데이트 실행 중...");
		UpdateFilteredActors(LevelActors);
		bNeedsFilterUpdate = false;
	}

	// Actor 개수 표시
	if (SearchFilter.empty())
	{
		ImGui::Text("Total Actors: %zu", LevelActors.size());
	}
	else
	{
		ImGui::Text("%d / %zu actors", static_cast<int32>(FilteredIndices.size()), LevelActors.size());
	}
	ImGui::Spacing();

	// Actor 리스트를 스크롤 가능한 영역으로 표시
	if (ImGui::BeginChild("ActorList", ImVec2(0, 0), true))
	{
		if (SearchFilter.empty())
		{
			// 검색어가 없으면 모든 Actor 표시
			for (int32 i = 0; i < static_cast<int32>(LevelActors.size()); ++i)
			{
				if (LevelActors[i])
				{
					RenderActorInfo(LevelActors[i], i);
				}
			}
		}
		else
		{
			// 필터링된 Actor들만 표시
			for (int32 FilteredIndex : FilteredIndices)
			{
				if (FilteredIndex < LevelActors.size() && LevelActors[FilteredIndex])
				{
					RenderActorInfo(LevelActors[FilteredIndex], FilteredIndex);
				}
			}

			// 검색 결과가 없으면 메시지 표시
			if (FilteredIndices.empty())
			{
				ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "검색 결과가 없습니다.");
			}
		}
	}
	ImGui::EndChild();

	// 하단 정보
	// AActor* SelectedActor = CurrentLevel->GetSelectedActor();
	// if (SelectedActor)
	// {
	// 	ImGui::Text("Selected: %s", SelectedActor->GetName().ToString().data());
	//
	// 	if (bShowDetails)
	// 	{
	// 		const FVector& Location = SelectedActor->GetActorLocation();
	// 		const FVector& Rotation = SelectedActor->GetActorRotation();
	// 		const FVector& Scale = SelectedActor->GetActorScale3D();
	//
	// 		ImGui::Text("Location: (%.2f, %.2f, %.2f)", Location.X, Location.Y, Location.Z);
	// 		ImGui::Text("Rotation: (%.2f, %.2f, %.2f)", Rotation.X, Rotation.Y, Rotation.Z);
	// 		ImGui::Text("Scale: (%.2f, %.2f, %.2f)", Scale.X, Scale.Y, Scale.Z);
	// 	}
	// }
	// else
	// {
	// 	ImGui::TextUnformatted("No Actor Selected");
	// }
}

/**
 * @brief Actor 정보를 렌더링하는 헬퍼 함수
 * @param InActor 렌더링할 Actor
 * @param InIndex Actor의 인덱스
 */
void USceneHierarchyWidget::RenderActorInfo(TObjectPtr<AActor> InActor, int32 InIndex)
{
	// TODO(KHJ): 컴포넌트 정보 탐색을 위한 트리 노드를 작업 후 남겨두었음, 필요하다면 사용할 것

	if (!InActor)
	{
		return;
	}

	ImGui::PushID(InIndex);

	// 현재 선택된 Actor인지 확인
	ULevel* CurrentLevel = ULevelManager::GetInstance().GetCurrentLevel();
	bool bIsSelected = (CurrentLevel && CurrentLevel->GetSelectedActor() == InActor);

	// 선택된 Actor는 하이라이트
	if (bIsSelected)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f)); // 노란색
	}

	FName ActorName = InActor->GetName();
	FString ActorDisplayName = ActorName.ToString();

	// Actor의 PrimitiveComponent들의 Visibility 체크
	bool bHasPrimitive = false;
	bool bAllVisible = true;
	TObjectPtr<UPrimitiveComponent> FirstPrimitive = nullptr;

	// Actor의 모든 Component 중에서 PrimitiveComponent 찾기
	for (auto& Component : InActor->GetOwnedComponents())
	{
		if (TObjectPtr<UPrimitiveComponent> PrimitiveComponent = Cast<UPrimitiveComponent>(Component))
		{
			bHasPrimitive = true;

			if (!FirstPrimitive)
			{
				FirstPrimitive = PrimitiveComponent;
			}

			if (!PrimitiveComponent->IsVisible())
			{
				bAllVisible = false;
			}
		}
	}

	// PrimitiveComponent가 있는 경우에만 Visibility 버튼 표시
	if (bHasPrimitive)
	{
		if (ImGui::SmallButton(bAllVisible ? "[O]" : "[X]"))
		{
			// 모든 PrimitiveComponent의 Visibility 토글
			bool bNewVisibility = !bAllVisible;
			for (auto& Component : InActor->GetOwnedComponents())
			{
				if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Component))
				{
					PrimComp->SetVisibility(bNewVisibility);
				}
			}
			UE_LOG_INFO("SceneHierarchy: %s의 가시성이 %s로 변경되었습니다",
			            ActorName.ToString().data(),
			            bNewVisibility ? "Visible" : "Hidden");
		}
	}
	else
	{
		// PrimitiveComponent가 없는 경우 비활성화된 버튼 표시
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
		ImGui::SmallButton("[-]");
		ImGui::PopStyleVar();
	}

	// 이름 클릭 감지 (오른쪽)
	ImGui::SameLine();

	// 이름 변경 모드인지 확인
	if (RenamingActor == InActor)
	{
		// 이름 변경 입력창
		ImGui::PushItemWidth(-1.0f);
		bool bEnterPressed = ImGui::InputText("##Rename", RenameBuffer, sizeof(RenameBuffer),
		                                      ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
		ImGui::PopItemWidth();

		// Enter 키로 확인
		if (bEnterPressed)
		{
			FinishRenaming(true);
		}
		// ESC 키로 취소
		else if (ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			FinishRenaming(false);
		}
		// 다른 곳 클릭으로 취소
		else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsItemHovered())
		{
			FinishRenaming(false);
		}

		// 포커스 설정 (첫 렌더링에서만)
		if (ImGui::IsWindowFocused() && !ImGui::IsAnyItemActive())
		{
			ImGui::SetKeyboardFocusHere(-1);
		}
	}
	else
	{
		// 일반 선택 모드
		bool bClicked = ImGui::Selectable(ActorDisplayName.data(), bIsSelected, ImGuiSelectableFlags_SpanAllColumns);

		if (bClicked)
		{
			double CurrentTime = ImGui::GetTime();

			// 이미 선택된 Actor를 2초 이내 다시 클릭한 경우
			if (bIsSelected && LastClickedActor == InActor &&
				(CurrentTime - LastClickTime) > RENAME_CLICK_DELAY &&
				(CurrentTime - LastClickTime) < 2.0f)
			{
				// 이름 변경 모드 시작
				StartRenaming(InActor);
			}
			else
			{
				// 일반 선택
				SelectActor(InActor, false);
			}

			LastClickTime = CurrentTime;
			LastClickedActor = InActor;
		}

		// 더블 클릭 감지: 카메라 이동 수행
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			SelectActor(InActor, true);
			// 더블클릭 시 이름변경 모드 비활성화
			FinishRenaming(false);
		}
	}

	if (bIsSelected)
	{
		ImGui::PopStyleColor();
	}

	ImGui::PopID();
}

/**
 * @brief Actor를 선택하는 함수
 * @param InActor 선택할 Actor
 * @param bInFocusCamera 카메라 포커싱 여부 (더블 클릭 시 true)
 */
void USceneHierarchyWidget::SelectActor(TObjectPtr<AActor> InActor, bool bInFocusCamera)
{
	TObjectPtr<ULevel> CurrentLevel = ULevelManager::GetInstance().GetCurrentLevel();
	if (CurrentLevel)
	{
		CurrentLevel->SetSelectedActor(InActor);
		UE_LOG("SceneHierarchy: %s를 선택했습니다", InActor->GetName().ToString().data());

		// 카메라 포커싱은 더블 클릭에서만 수행
		if (InActor && bInFocusCamera)
		{
			FocusOnActor(InActor);
			UE_LOG_SUCCESS("SceneHierarchy: %s에 카메라 포커싱 완료", InActor->GetName().ToString().data());
		}
	}
}

/**
 * @brief 카메라를 특정 Actor에 포커스하는 함수
 * @param InActor 포커스할 Actor
 */
void USceneHierarchyWidget::FocusOnActor(TObjectPtr<AActor> InActor)
{
	if (!InActor) { return; }

	// 렌더러로부터 활성화된 카메라를 요청
	UCamera* ActiveCamera = URenderer::GetInstance().GetViewportClient()->GetActiveCamera();
	if (!ActiveCamera)
	{
		UE_LOG_WARNING("SceneHierarchy: 포커싱할 활성 카메라를 찾을 수 없습니다.");
		return;
	}

	// 현재 카메라의 위치와 회전을 저장
	CameraStartLocation = ActiveCamera->GetLocation();
	CameraStartRotation = ActiveCamera->GetRotation();

	// Actor의 월드 위치를 얻음
	FVector ActorLocation = InActor->GetActorLocation();

	// 카메라의 정확한 Forward 벡터를 사용하여 화면 중앙 배치 보정
	// Camera 클래스에서 이미 계산된 정확한 Forward 벡터 사용
	FVector CameraForward = ActiveCamera->GetForward();
	CameraForward.Z = 0.0f;
	CameraForward.Normalize();

	// Actor를 정확히 화면 중앙에 놓기 위해 Forward 방향의 반대로 거리를 둔 위치에 카메라 배치
	CameraTargetLocation = ActorLocation - (CameraForward * FOCUS_DISTANCE);

	// 카메라가 Actor를 정확히 바라보도록 조정
	FVector LookAtDir = ActorLocation - CameraTargetLocation;
	LookAtDir.Normalize();
	FVector Axis = CameraForward.Cross(LookAtDir);
	float Angle = acosf(CameraForward.Dot(LookAtDir));
	CameraTargetRotation = FQuaternion::FromAxisAngle(Axis, Angle).ToEuler();
		
	// 카메라 애니메이션 시작
	bIsCameraAnimating = true;
	CameraAnimationTime = 0.0f;
}

/**
 * @brief 카메라 애니메이션을 업데이트하는 함수
 * 선형 보간을 활용한 부드러운 움직임을 구현함
 */
void USceneHierarchyWidget::UpdateCameraAnimation()
{
	if (!bIsCameraAnimating) { return; }

	// 애니메이션 도중 활성 카메라가 사라지면 애니메이션을 즉시 중단
	UCamera* ActiveCamera = URenderer::GetInstance().GetViewportClient()->GetActiveCamera();
	const int CameraCount = URenderer::GetInstance().GetViewportClient()->GetViewports().size();

	if (!ActiveCamera)
	{
		bIsCameraAnimating = false;
		return;
	}

	CameraAnimationTime += DT;

	// 애니메이션 진행 비율 계산
	float Progress = CameraAnimationTime / CAMERA_ANIMATION_DURATION;

	if (Progress >= 1.0f)
	{
		// 애니메이션 완료
		Progress = 1.0f;
		bIsCameraAnimating = false;
	}

	// Easing 함수를 사용하여 부드러운 움직임 확보 (easeInOutQuart)
	float SmoothProgress;
	if (Progress < 0.5f)
	{
		// 움직인 전반에서는 아주 천천히 가속
		SmoothProgress = 8.0f * Progress * Progress * Progress * Progress;
	}
	else
	{
		// 움직인 후반에서는 아주 천천히 감속
		float ProgressFromEnd = Progress - 1.0f;
		SmoothProgress = 1.0f - 8.0f * ProgressFromEnd * ProgressFromEnd * ProgressFromEnd * ProgressFromEnd;
	}

	// Linear interpolation으로 위치 및 회전 보간
	FVector CurrentLocation = CameraStartLocation + (CameraTargetLocation - CameraStartLocation) * SmoothProgress;
	FVector CurrentRotation = CameraStartRotation + (CameraTargetRotation - CameraStartRotation) * SmoothProgress;

	for (int Index = 0; Index < 4; ++Index)
	{
		FViewportClient* ViewportClient = URenderer::GetInstance().GetViewportClient();
		UCamera& Camera = ViewportClient->GetViewports()[Index].Camera;

		Camera.SetLocation(CurrentLocation);
		ViewportClient->SetFocusPoint(CurrentLocation);

		// 원근 투영 카메라에만 회전 변환을 합니다.
		if (Camera.GetCameraType() == ECameraType::ECT_Perspective)
		{
			Camera.SetRotation(CurrentRotation);
		}
	}

	if (!bIsCameraAnimating)
	{
		UE_LOG_SUCCESS("SceneHierarchy: 카메라 포커싱 애니메이션 완료");
	}
}

/**
 * @brief 검색창을 렌더링하는 함수
 */
void USceneHierarchyWidget::RenderSearchBar()
{
	// 검색 지우기 버튼
	if (ImGui::SmallButton("X"))
	{
		memset(SearchBuffer, 0, sizeof(SearchBuffer));
		SearchFilter.clear();
		bNeedsFilterUpdate = true;
	}

	// 검색창
	ImGui::SameLine();
	ImGui::PushItemWidth(-1.0f); // 나머지 너비 모두 사용
	bool bTextChanged = ImGui::InputTextWithHint("##Search", "검색...", SearchBuffer, sizeof(SearchBuffer));
	ImGui::PopItemWidth();

	// 검색어가 변경되면 필터 업데이트 플래그 설정
	if (bTextChanged)
	{
		FString NewSearchFilter = FString(SearchBuffer);
		if (NewSearchFilter != SearchFilter)
		{
			UE_LOG("SceneHierarchy: 검색어 변경: '%s' -> '%s'", SearchFilter.data(), NewSearchFilter.data());
			SearchFilter = NewSearchFilter;
			bNeedsFilterUpdate = true;
		}
	}
}

/**
 * @brief 필터링된 Actor 인덱스 리스트를 업데이트하는 함수
 * @param InLevelActors 레벨의 모든 Actor 리스트
 */
void USceneHierarchyWidget::UpdateFilteredActors(const TArray<TObjectPtr<AActor>>& InLevelActors)
{
	FilteredIndices.clear();

	if (SearchFilter.empty())
	{
		return; // 검색어가 없으면 모든 Actor 표시
	}

	// 검색 성능 최적화: 대소문자 변환을 한 번만 수행
	FString SearchLower = SearchFilter;
	std::transform(SearchLower.begin(), SearchLower.end(), SearchLower.begin(), ::tolower);

	// UE_LOG("SceneHierarchy: 검색어 = '%s', 변환된 검색어 = '%s'", SearchFilter.data(), SearchLower.data());
	// UE_LOG("SceneHierarchy: Level에 %zu개의 Actor가 있습니다", InLevelActors.size());

	for (int32 i = 0; i < InLevelActors.size(); ++i)
	{
		if (InLevelActors[i])
		{
			FString ActorName = InLevelActors[i]->GetName().ToString();
			bool bMatches = IsActorMatchingSearch(ActorName, SearchLower);
			// UE_LOG("SceneHierarchy: Actor[%d] = '%s', 매치 = %s", i, ActorName.c_str(), bMatches ? "Yes" : "No");

			if (bMatches)
			{
				FilteredIndices.push_back(i);
			}
		}
	}

	UE_LOG("SceneHierarchy: 필터링 결과: %zu개 찾음", FilteredIndices.size());
}

/**
 * @brief Actor 이름이 검색어와 일치하는지 확인
 * @param InActorName Actor 이름
 * @param InSearchTerm 검색어 (대소문자를 무시)
 * @return 일치하면 true
 */
bool USceneHierarchyWidget::IsActorMatchingSearch(const FString& InActorName, const FString& InSearchTerm)
{
	if (InSearchTerm.empty())
	{
		return true;
	}

	FString ActorNameLower = InActorName;
	std::transform(ActorNameLower.begin(), ActorNameLower.end(), ActorNameLower.begin(), ::tolower);

	bool bResult = ActorNameLower.find(InSearchTerm) != std::string::npos;

	return bResult;
}

/**
 * @brief 이름 변경 모드를 시작하는 함수
 * @param InActor 이름을 변경할 Actor
 */
void USceneHierarchyWidget::StartRenaming(TObjectPtr<AActor> InActor)
{
	if (!InActor)
	{
		return;
	}

	RenamingActor = InActor;
	FString CurrentName = InActor->GetName().ToString();

	// 현재 이름을 버퍼에 복사 - Detail 패널과 동일한 방식 사용
	strncpy_s(RenameBuffer, CurrentName.data(), sizeof(RenameBuffer) - 1);
	RenameBuffer[sizeof(RenameBuffer) - 1] = '\0';

	UE_LOG("SceneHierarchy: '%s' 에 대한 이름 변경 시작", CurrentName.data());
}

/**
 * @brief 이름 변경을 완료하는 함수
 * @param bInConfirm true면 적용, false면 취소
 */
void USceneHierarchyWidget::FinishRenaming(bool bInConfirm)
{
	if (!RenamingActor)
	{
		return;
	}

	if (bInConfirm)
	{
		FString NewName = FString(RenameBuffer);
		// 빈 이름 방지 및 이름 변경 여부 확인
		if (!NewName.empty() && NewName != RenamingActor->GetName().ToString())
		{
			// Detail 패널과 동일한 방식 사용
			RenamingActor->SetDisplayName(NewName);
			UE_LOG_SUCCESS("SceneHierarchy: Actor의 이름을 '%s' (으)로 변경하였습니다", NewName.c_str());

			// 검색 필터를 업데이트해야 할 수도 있음
			bNeedsFilterUpdate = true;
		}
		else if (NewName.empty())
		{
			UE_LOG_WARNING("SceneHierarchy: 빈 이름으로 인해 이름 변경 취소됨");
		}
	}
	else
	{
		UE_LOG_WARNING("SceneHierarchy: 이름 변경 취소");
	}

	// 상태 초기화
	RenamingActor = nullptr;
	RenameBuffer[0] = '\0';
}
