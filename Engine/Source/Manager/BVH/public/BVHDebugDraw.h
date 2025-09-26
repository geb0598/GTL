#pragma once

#include "Core/Public/Object.h"
#include "Global/CoreTypes.h"
#include "Global/Types.h"
#include "Editor/Public/EditorPrimitive.h"
#include "Physics/Public/AABB.h"

/**
 * @brief Builds and renders line-list primitives for a collection of axis-aligned bounding boxes.
 */
class UBVHDebugDraw : public UObject
{
public:
	UBVHDebugDraw();
	~UBVHDebugDraw() override;

	void SetBoxes(const TArray<FAABB>& InBoxes);
	void Clear();
	void Render(const TArray<FAABB>& InBoxes) const;

	void SetVisible(bool bInVisible) { bVisible = bInVisible; }
	bool IsVisible() const { return bVisible; }

private:
	void ReleaseBuffers();

	TArray<FVector> Vertices;
	TArray<uint32> Indices;

	FEditorPrimitive Primitive;

	bool bVisible = false;
	uint32 AllocatedVertexCapacity = 0;
	uint32 AllocatedIndexCapacity = 0;
};
