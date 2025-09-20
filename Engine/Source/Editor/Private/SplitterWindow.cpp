#include "pch.h"
#include "Editor/Public/SplitterWindow.h"

bool SWindow::IsHovered(const FPoint& InMousePosition) const
{
	return (InMousePosition.X >= Rect.Left &&
		InMousePosition.X <= Rect.GetRight() &&
		InMousePosition.Y >= Rect.Top &&
		InMousePosition.Y <= Rect.GetBottom());
}

void SSplitter::SetChildren(SWindow* InSideLT, SWindow* InSideRB)
{
	SideLT = InSideLT;
	SideRB = InSideRB;
}

void SSplitter::SetRatio(float NewRatio)
{
	// 뷰포트가 너무 작아지거나 사라지는 것을 방지합니다.
	Ratio = std::clamp(NewRatio, 0.05f, 0.95f);
}

void SSplitterH::Resize(const FRect& ParentRect)
{
	// 1. 부모 영역을 기반으로 자신의 위치(비율)를 먼저 계산합니다.
	Rect.Left = ParentRect.Left;
	Rect.Width = ParentRect.Width;
	Rect.Top = ParentRect.Top + (ParentRect.Height * Ratio) - (Thickness / 2.0f);
	Rect.Height = Thickness;

	// 2. 자식들이 존재한다면, 자신의 위치를 기준으로 자식들의 영역을 재계산합니다.
	if (SideLT && SideRB)
	{
		// 위쪽 자식(SideLT)의 영역
		FRect RectTop;
		RectTop.Left = ParentRect.Left;
		RectTop.Top = ParentRect.Top;
		RectTop.Width = ParentRect.Width;
		RectTop.Height = Rect.Top - ParentRect.Top;
		SideLT->Resize(RectTop);

		// 아래쪽 자식(SideRB)의 영역
		FRect RectBottom;
		RectBottom.Left = ParentRect.Left;
		RectBottom.Top = Rect.GetBottom();
		RectBottom.Width = ParentRect.Width;
		RectBottom.Height = ParentRect.GetBottom() - Rect.GetBottom();
		SideRB->Resize(RectBottom);
	}
}

void SSplitterV::Resize(const FRect& ParentRect)
{
	// 1. 부모 영역을 기반으로 자신의 위치(비율)를 먼저 계산합니다.
	Rect.Top = ParentRect.Top;
	Rect.Height = ParentRect.Height;
	Rect.Left = ParentRect.Left + (ParentRect.Width * Ratio) - (Thickness / 2.0f);
	Rect.Width = Thickness;

	// 2. 자식들이 존재한다면, 자신의 위치를 기준으로 자식들의 영역을 재계산합니다.
	if (SideLT && SideRB)
	{
		// 왼쪽 자식(SideLT)의 영역
		FRect RectLeft;
		RectLeft.Left = ParentRect.Left;
		RectLeft.Top = ParentRect.Top;
		RectLeft.Width = Rect.Left - ParentRect.Left;
		RectLeft.Height = ParentRect.Height;
		SideLT->Resize(RectLeft);

		// 오른쪽 자식(SideRB)의 영역
		FRect RectRight;
		RectRight.Left = Rect.GetRight();
		RectRight.Top = ParentRect.Top;
		RectRight.Width = ParentRect.GetRight() - Rect.GetRight();
		RectRight.Height = ParentRect.Height;
		SideRB->Resize(RectRight);
	}
}
