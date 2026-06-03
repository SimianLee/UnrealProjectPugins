// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HitProxies.h"
#include "Components/PrimitiveComponent.h"
#include "Blueprint/UserWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "CoordinateAxisComponent.generated.h"

USTRUCT()
struct AIRCITYPLUGIN_API FDragMovementInfo
{
	GENERATED_BODY()

public:

	int32 CoordinateIndex = 0;

	FPlane MovementPlane;

	FVector MovementDirVector;

	FVector DragStartPlanePosition;

};

UENUM()
enum EAxisType
{
	EAT_None = 0,
	EAT_X = 1,
	EAT_Y = 2,
	EAT_Z = 4,
	EAT_Screen = 8,

	EAT_XY = EAT_X | EAT_Y,
	EAT_XZ = EAT_X | EAT_Z,
	EAT_YZ = EAT_Y | EAT_Z,
	EAT_XYZ = EAT_X | EAT_Y | EAT_Z,
	EAT_All = EAT_XYZ | EAT_Screen,
};

UENUM()
enum ECoorAxisRenderType
{
	CART_None,
	CART_Translation,
	CART_Rotate,
	CART_Scale,
};

UENUM()
enum ECoordinateAxisMode
{
	CAM_Normal,		// 正常
	CAM_Mix,		// 混合
};

UCLASS()
class AIRCITYPLUGIN_API UCoordinateAxisComponent : public UPrimitiveComponent, public IInterface_CollisionDataProvider
{
	GENERATED_BODY()

public:	/* 外部调用函数 */

	// 开始拖拽
	UFUNCTION()
	virtual void StartDragging();

	// 停止拖拽
	UFUNCTION()
	virtual void StopDragging();

	// 是否拖拽
	UFUNCTION()
	bool bIsDragging();

	// 开始渲染
	UFUNCTION()
	void SetRenderComponent(bool bIsRender);

	// 是否渲染
	UFUNCTION()
	bool bIsRender() const;

	EAxisType GetFocusAxis();

	// 设置要渲染的轴
	void SetRenderCoordinateAxis(const EAxisType& InMode);

	EAxisType GetRenderAxis();

	UFUNCTION()
	void SetComponentMaterial(UMaterialInstanceDynamic* InOrigin, UMaterialInstanceDynamic* InX, UMaterialInstanceDynamic* InY, UMaterialInstanceDynamic* InZ, UMaterialInstanceDynamic* InFocus, UMaterialInstanceDynamic* InPlane, UMaterialInstanceDynamic* InFocusPlane, UMaterialInterface* InArcPlane);

	UFUNCTION()
	void SetCoorAixsMode(ECoordinateAxisMode InMode);

	ECoordinateAxisMode GetCoorAixsMode(){ return CoorAxisMode; };

	UFUNCTION()
	FVector GetTopLocation();

	float GetRenderScale();

	ECoorAxisRenderType GetCoorAxisType() { return CoorAxisType; };

	FVector GetDirectionToWidget() { return DirectionToWidget; };

	void SetTouchDragging(bool bTouch);

protected:

	// Sets default values for this component's properties
	UCoordinateAxisComponent();

	~UCoordinateAxisComponent();

	virtual void BeginPlay() override;

	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

protected:

	void InitParam();

	// void InitMaterial();

protected:/** PDI Render */

	FPrimitiveSceneProxy* CreateSceneProxy() override;

	FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

	class UBodySetup* GetBodySetup() override;

protected: /* Render */

	virtual void Render(const FSceneView* InView, FPrimitiveDrawInterface* InPDI, FMeshElementCollector& InCollector);

public:

	// Render
	float RenderBoxScale = 1.f;

	// Tick Component
	float RenderScale = 1.f;

protected:/** Render Collision */

	bool GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData) override;

	bool ContainsPhysicsTriMeshData(bool InUseAllTriData) const override;

	void GetMeshId(FString& OutMeshId) override;

	bool WantsNegXTriMesh() override;

protected:/** Collision */

	virtual void BuildCollision(FTriMeshCollisionData& InCollisionData);

	void UpdateCollision();

protected:

	float HitDistance;

	UPROPERTY()
	class UBodySetup* VertexMeshBodySetup = nullptr;

public:/* Focus */

	void UpdateFocusAxis();

	int32 GetSelectFaceIndex() const;

	EAxisType GetSelectedVertexAxis(int32 InFaceIndex) const;

protected:

	APlayerController* Controller;

protected: /* Move */

	virtual void CacluMouseDragOffset();

	FVector2D GetMousePosition();

	FVector2D GetTouchPosition();

	void PrepareStartDragMovement();

	virtual TArray<FVector> GetCoorAxisDirs();

	FDragMovementInfo GetStartDragMovement(int32 InCoordinateIndex);

	FVector GetDragLocation(const FDragMovementInfo& Info);

protected:

	FVector2D LastMousePos;

	FVector2D CurrentPosition;

	TArray<FDragMovementInfo> DragMovementInfos;

	/* End Move */

protected:

	// 拖动
	bool bDragging;

	//
	bool bTouchDragging = false;

	// 渲染
	bool bRender;

	// 聚焦轴
	EAxisType CoordinateFocusAxis;

	// 渲染轴
	EAxisType CoordinateRenderAxis;

	// 渲染类型
	ECoorAxisRenderType CoorAxisType;

	ECoordinateAxisMode CoorAxisMode;

	// 渲染轴面数
	int32 RenderAxisVertexNum;

	// 渲染拐角面数
	int32 RnederDualAxisVertexNum;

	// 渲染中心点面数
	int32 RenderSceneVertexNum;

protected:

	// Color
	FLinearColor AxisColorX, AxisColorY, AxisColorZ, PlaneClor, AxisOriginColor, FocusColor, FocusPlaneClor, LucencyColor;

	// Material Instance
	UPROPERTY()
	UMaterialInstanceDynamic* AxisMaterialX = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* AxisMaterialY = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* AxisMaterialZ = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* PlaneMaterial = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* AxisOriginMaterial = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* FocusAxisMaterial = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* FocusPlaneMaterial = nullptr;

	UPROPERTY()
	UMaterialInterface* ArcPlaneMaterial = nullptr;

protected:
	
	const float DefaultAxisLength = 35.0f;

	FVector DirectionToWidget = FVector(0);

public: /* Build Component */

	// Translation
	void BuildTranslationAxisVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices, float CylinderRadius = 1.2f);

	void BuildCylinderVerts(const FVector& Base, const FVector& XAxis, const FVector& YAxis, const FVector& ZAxis, float Radius, float HalfHeight, uint32 Sides, TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices);

	void BuildConeVerts(float Angle1, float Angle2, float Scale, FVector Offset, uint32 NumSides, TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices);

	FVector CalcConeVert(float Angle1, float Angle2, float AzimuthAngle);

	void BuildSphereVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices);

	void BuildCornerVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices);

	void BuildSquareVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices);

	// Scale
	void BuildScaleAxisVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices);

	void BuildCubeVerts(float Scale, FVector Offset, TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices);

	void BuildTriangleVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices);

	// Rotation
	void BuildThickArcVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices, const FVector& Axis0, const FVector& Axis1, const float StartAngle, const float EndAngle, float OuterRadius, float InnerRadius, const FColor& Color);

	void BuildStartStopMarkerVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices, const FVector& Axis0, const FVector& Axis1, const float Angle, const FColor& Color);

	void BuildSnapMarkerVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices, const FVector& Axis0, const FVector& Axis1, const float WidthPercent, const float PercentSize, const FColor& Color);

public: /* Build Widget */

	void TickWidgetComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction);

	ULocalPlayer* GetOwnerPlayer() const;

	void RemoveWidgetFromScreen();

protected:

	bool bAddedToScreen = false;

protected: /* Movement */

	struct FAbsoluteMovementParams
	{
		/** The normal of the plane to project onto */
		FVector PlaneNormal;
		/** A vector that represent any displacement we want to mute (remove an axis if we're doing axis movement)*/
		FVector NormalToRemove;
		/** The current position of the widget */
		FVector Position;

		//Coordinate System Axes
		FVector XAxis;
		FVector YAxis;
		FVector ZAxis;

		//true if camera movement is locked to the object
		bool bMovementLockedToCamera;

		//Direction in world space to the current mouse location
		FVector PixelDir;
		//Direction in world space of the middle of the camera
		FVector CameraDir;
		FVector EyePos;

		//whether to snap the requested positionto the grid
		bool bPositionSnapping;
	};

	bool bTranslationInitialOffsetCached;
	FVector InitialTranslationPosition;
	FVector InitialTranslationOffset;

	FMatrix CustomCoordSystem = FMatrix::Identity;
	FVector2D DragStartPos = FVector2D::ZeroVector;
	FVector2D Origin = FVector2D::ZeroVector;
	FVector2D XAxisDir = FVector2D::ZeroVector;
	FVector2D YAxisDir = FVector2D::ZeroVector;
	FVector2D ZAxisDir = FVector2D::ZeroVector;
	bool bIsOrthoDrawingFullRing = false;
	float CurrentDeltaRotation = 0.f;

	void GetAxisPlaneNormalAndMask(
		const FMatrix& InCoordSystem,
		const FVector& InAxis,
		const FVector& InDirToPixel,
		FVector& OutPlaneNormal,
		FVector& NormalToRemove);

	void GetPlaneNormalAndMask(
		const FVector& InAxis,
		FVector& OutPlaneNormal,
		FVector& NormalToRemove);

	FVector GetTranslationInitialOffset(
		const FVector& InNewPosition,
		const FVector& InCurrentPosition);

	FVector GetTranslationDelta(
		const FAbsoluteMovementParams& InParams);

	void ConvertMouseToAxis_Translate(
		const FMatrix& InputCoordSystem,
		FAbsoluteMovementParams& InOutParams,
		FVector& OutDrag);

	void ConvertMouseToAxis_Translate(
		const FSceneView* InView,
		const FVector2D& InMousePosition,
		FVector& OutDrag);

	void ConvertMouseToAxis_Rotate(
		FVector2D DragDir,
		FVector& InOutDelta,
		FRotator& OutRotation);

	void ConvertMouseToAxis_Scale(
		FVector2D DragDir,
		FVector& InOutDelta,
		FVector& OutScale);

	void ConvertMouseMovementToAxisMovement(
		const FSceneView* InView, 
		const FVector2D& InMousePosition,
		FVector& InOutDelta,
		FVector& OutDrag,
		FRotator& OutRotation, 
		FVector& OutScale);

protected:

	FVector InMovementDiff = FVector::ZeroVector;
	FVector OutMovementDrag = FVector::ZeroVector;
	FRotator OutMovementRotation = FRotator::ZeroRotator;
	FVector OutMovementScale = FVector::OneVector;
};

class SCoordAxisWidgetScreenLayer : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SCoordAxisWidgetScreenLayer)
	{
		_Visibility = EVisibility::SelfHitTestInvisible;
	}
	SLATE_END_ARGS()

public:

	void Construct(const FArguments& InArgs, const FLocalPlayerContext& InPlayerContext);

	void AddComponent(UCoordinateAxisComponent* InComponent, const FString& InText, const FVector& InLocation, const FLinearColor& InColor);

	void RemoveComponent(UCoordinateAxisComponent* InComponent);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:

	FVector2D Pivot = FVector2D(0.5f, 0.5f);
	FVector2D DrawSize = FVector2D(32.f, 32.f);

	class FComponentData
	{
	public:

		TWeakObjectPtr<UCoordinateAxisComponent> Component;

		TArray<SConstraintCanvas::FSlot*> Slots;
		TArray<TSharedPtr<SWidget>> Widgets;
		TArray<FVector> SlotLocation;
	};

	TMap<UCoordinateAxisComponent*, FComponentData> ComponentMap;

	FSlateFontInfo FontInfo;

private:

	FLocalPlayerContext PlayerContext;

	TSharedPtr<SConstraintCanvas> Canvas;
};