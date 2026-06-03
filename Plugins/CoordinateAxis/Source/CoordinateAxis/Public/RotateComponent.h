// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CanvasTypes.h"
#include "Widgets/SCompoundWidget.h"
#include "Components/Widget.h"
#include "Components/CoordinateAxis/CoordinateAxisComponent.h"
#include "Components/Widget.h"
#include "RotateComponent.generated.h"

/**
 * 
 */

UCLASS()
class AIRCITYPLUGIN_API URotateComponent : public UCoordinateAxisComponent
{
	GENERATED_BODY()

public:

	DECLARE_DELEGATE_OneParam(FRotationOffsetDelegate, const FRotator&);

public:

	void StartDragging() override;

	void StopDragging() override;

public:

	void UpdateDeltaRotation();

	float GetDeltaRotation() const;

	bool IsRotationLocalSpace() const;

	float TotalDeltaRotation = 0.f;

public:/* 显示旋转度数 */

	void CreatePopupString();

	void RemovePopupString();

	void UpdatePopupStringPos();

protected:

	FString HUDString;

	FVector2D HUDInfoPos;

	UPROPERTY()
	class UCanvasPanel* CoordAxisPanel = nullptr;

	TWeakObjectPtr<class URotateWidget> RotateWidget = nullptr;

protected:

	URotateComponent();

	void Render(const FSceneView* InView, FPrimitiveDrawInterface* InPDI, FMeshElementCollector& InCollector) override;

	void BuildCollision(FTriMeshCollisionData& InCollisionData) override;

	void RenderRotate(const FSceneView* InView, FPrimitiveDrawInterface* InPDI, FMeshElementCollector& InCollector, const FMatrix& InMatrix);

	void BuildRotateCollision(const FMatrix& InMatrix, FTriMeshCollisionData& InCollisionData);

	// 绘制弧
	void RenderRatationArc(EAxisType InAxis, const FSceneView* InView, FPrimitiveDrawInterface* InPDI, FMeshElementCollector& InCollector, const FVector& Axis0, const FVector& Axis1, const FVector& InDirectionToWidget, const FColor& InColor, const FMatrix& InMatrix, FVector2D& OutAxisDir);

	void BuildRatationArcCollision(EAxisType InAxis, const FVector& Axis0, const FVector& Axis1, const FVector& InDirectionToWidget, FTriMeshCollisionData& CollisionData, const FMatrix& InMatrix);

	//
	void CacheRotationHUDText(const FSceneView* InView, const FVector& Axis0, const FVector& Axis1, const float AngleOfChange, const float InScale);

protected:

	uint8 LargeOuterAlpha = 0x7f;

	uint8 SmallOuterAlpha = 0x0f;

	// 绘制碰撞使用
	bool bIsPerspective = false;

	bool bIsMirrorAxis = true;

public:/* Move */

	FRotationOffsetDelegate OnRotationOffsetDelegate = FRotationOffsetDelegate();

protected:

	UPROPERTY()
	float RotatorChangeSpeed = 0.5f;

protected:

	void CacluMouseDragOffset() override;
};


UCLASS()
class AIRCITYPLUGIN_API URotateWidget : public UWidget
{
	GENERATED_BODY()

protected:

	//~ Begin UVisual Interface
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	//~ End UVisual Interface

	//~ Begin UWidget Interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget

#if WITH_EDITOR

	// UWidget interface
	virtual const FText GetPaletteCategory() override;
	// End UWidget interface

#endif

public:

	void SetRotateValue(const FString& Value);

protected:

	TSharedPtr<class SRotateWidget> RotateWidget;

};

class SRotateWidget : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SRotateWidget)
	{}
	SLATE_END_ARGS()

public:

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	void SetRotateValue(const FString& Value);

protected:

	TSharedPtr<class STextBlock> RotateValueText;
};
