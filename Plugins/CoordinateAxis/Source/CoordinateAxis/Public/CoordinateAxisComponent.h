// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HitProxies.h"
#include "DynamicMeshBuilder.h"
#include "Components/PrimitiveComponent.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "CoordinateAxisComponent.generated.h"

// ============================================================
// 前向声明（全局作用域，供命名空间函数声明使用）
// ============================================================
class UCoordinateAxisComponent;
class FSceneView;
class FPrimitiveDrawInterface;
class FMeshElementCollector;
struct FTriMeshCollisionData;

// ============================================================
// 公共枚举定义
// ============================================================

UENUM()
enum EAxisType
{
	EAT_None   = 0,
	EAT_X      = 1,
	EAT_Y      = 2,
	EAT_Z      = 4,
	EAT_Screen = 8,

	EAT_XY     = EAT_X | EAT_Y,
	EAT_XZ     = EAT_X | EAT_Z,
	EAT_YZ     = EAT_Y | EAT_Z,
	EAT_XYZ    = EAT_X | EAT_Y | EAT_Z,
	EAT_All    = EAT_XYZ | EAT_Screen,
};

/** 坐标轴功能类型：平移 / 旋转 / 缩放 */
UENUM()
enum ECoorAxisRenderType
{
	CART_None,
	CART_Translation,
	CART_Rotate,
	CART_Scale,
};

/** 坐标轴显示模式 */
UENUM()
enum ECoordinateAxisMode
{
	CAM_Normal,  // 正常（三轴各自可见）
	CAM_Mix,     // 混合（平移+旋转Z）
};

// ============================================================
// 拖拽平面信息（供 Movement 使用）
// ============================================================

struct FDragMovementInfo
{
	int32   CoordinateIndex = 0;
	FPlane  MovementPlane;
	FVector MovementDirVector;
	FVector DragStartPlanePosition;
};

struct FAbsoluteMovementParams
{
	FVector PlaneNormal;
	FVector NormalToRemove;
	FVector Position;
	FVector XAxis, YAxis, ZAxis;
	bool    bMovementLockedToCamera = false;
	FVector PixelDir;
	FVector CameraDir;
	FVector EyePos;
	bool    bPositionSnapping = false;
};

// ============================================================
// CoordinateAxisRender 命名空间 — 函数声明
// ============================================================
namespace CoordinateAxisRenderFuncs
{
	void RenderTranslation(UCoordinateAxisComponent* InComp, const FSceneView* InView, FPrimitiveDrawInterface* InPDI, FMeshElementCollector& InCollector, const FMatrix& InMatrix);
	void BuildTranslationCollision(UCoordinateAxisComponent* InComp, const FMatrix& InMatrix, FTriMeshCollisionData& CollisionData);

	void RenderRotate(UCoordinateAxisComponent* InComp, const FSceneView* InView, FPrimitiveDrawInterface* InPDI, FMeshElementCollector& InCollector, const FMatrix& InMatrix);
	void BuildRotateCollision(UCoordinateAxisComponent* InComp, const FMatrix& InMatrix, FTriMeshCollisionData& CollisionData);

	void RenderScale(UCoordinateAxisComponent* InComp, const FSceneView* InView, FPrimitiveDrawInterface* InPDI, FMeshElementCollector& InCollector, const FMatrix& InMatrix);
	void BuildScaleCollision(UCoordinateAxisComponent* InComp, const FMatrix& InMatrix, FTriMeshCollisionData& CollisionData);

	// 基础几何体构建（共用）
	void BuildTranslationAxisVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices, float CylinderRadius = 1.2f);
	void BuildCylinderVerts(const FVector& Base, const FVector& XAxis, const FVector& YAxis, const FVector& ZAxis, float Radius, float HalfHeight, uint32 Sides, TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices);
	void BuildConeVerts(float Angle1, float Angle2, float Scale, FVector Offset, uint32 NumSides, TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices);
	FVector CalcConeVert(float Angle1, float Angle2, float AzimuthAngle);
	void BuildSphereVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices);
	void BuildCornerVerts(float RenderBoxScale, TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices);
	void BuildSquareVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices);
	void BuildScaleAxisVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices);
	void BuildCubeVerts(float Scale, FVector Offset, TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices);
	void BuildTriangleVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices);
	void BuildThickArcVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices, const FVector& Axis0, const FVector& Axis1, float StartAngle, float EndAngle, float OuterRadius, float InnerRadius, const FColor& Color);
	void BuildStartStopMarkerVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices, const FVector& Axis0, const FVector& Axis1, float Angle, const FColor& Color);
	void BuildSnapMarkerVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices, const FVector& Axis0, const FVector& Axis1, float WidthPercent, float PercentSize, const FColor& Color);
}

// ============================================================
// CoordinateAxisMovement 命名空间 — 函数声明
// ============================================================
namespace CoordinateAxisMovementFuncs
{
	void ConvertMouseMovementToAxisMovement(UCoordinateAxisComponent* InComp, const FSceneView* InView, const FVector2D& InMousePos, FVector& InOutDelta, FVector& OutDrag, FRotator& OutRotation, FVector& OutScale);
	void PrepareStartDragMovement(UCoordinateAxisComponent* InComp, const FVector2D& InCurrentPos);
	void CalcTranslationDragDelta(UCoordinateAxisComponent* InComp, const FVector2D& InCurrentPos, FVector& OutMoveDelta);
	void ResetMovementState(UCoordinateAxisComponent* InComp);

	FDragMovementInfo GetStartDragMovement(UCoordinateAxisComponent* InComp, int32 InCoordinateIndex, const FVector2D& InCurrentPos);
	FVector GetDragLocation(UCoordinateAxisComponent* InComp, const FDragMovementInfo& Info, const FVector2D& InCurrentPos);
}

// ============================================================
// UCoordinateAxisComponent — 完整坐标轴功能组件
// ============================================================

/**
 * UCoordinateAxisComponent
 *
 * 完整的坐标轴功能组件。通过 SetAxisType() 切换为 Translation / Rotation / Scale 三种形态。
 * 绘制逻辑委托给 CoordinateAxisRender.cpp，拖拽逻辑委托给 CoordinateAxisMovement.cpp。
 *
 * 外部使用者通过以下接口控制：
 *   - SetRenderComponent()   控制可见性
 *   - StartDragging()        开始拖拽
 *   - StopDragging()         停止拖拽
 *   - SetAxisType()          设置坐标轴类型（Translation/Rotation/Scale）
 *   - SetCoorAxisMode()      设置显示模式（Normal/Mix）
 *   - SetComponentMaterial() 设置材质
 *
 * 拖拽回调：
 *   - OnTranslationOffsetDelegate  平移偏移
 *   - OnRotationOffsetDelegate     旋转偏移
 *   - OnScaleOffsetDelegate        缩放偏移
 */
UCLASS()
class COORDINATEAXIS_API UCoordinateAxisComponent
	: public UPrimitiveComponent
	, public IInterface_CollisionDataProvider
{
	GENERATED_BODY()

	// --------------------------------------------------------
	// 构造 / 生命周期
	// --------------------------------------------------------
public:
	UCoordinateAxisComponent();
	~UCoordinateAxisComponent();

protected:
	virtual void BeginPlay() override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --------------------------------------------------------
	// 外部调用接口
	// --------------------------------------------------------
public:

	/** 开始拖拽（需先有 FocusAxis） */
	UFUNCTION()
	void StartDragging();

	/** 停止拖拽 */
	UFUNCTION()
	void StopDragging();

	/** 是否正在拖拽 */
	UFUNCTION()
	bool IsDragging() const;

	/** 设置可见性（false 时同时停止拖拽） */
	UFUNCTION()
	void SetRenderComponent(bool bIsRender);

	/** 是否可见 */
	UFUNCTION()
	bool IsRenderVisible() const;

	/** 获取当前高亮轴 */
	EAxisType GetFocusAxis() const;

	/** 设置要渲染的轴标志 */
	void SetRenderCoordinateAxis(const EAxisType& InMode);

	/** 获取当前渲染轴标志 */
	EAxisType GetRenderAxis() const;

	/** 批量设置材质 */
	UFUNCTION()
	void SetComponentMaterial(
		UMaterialInstanceDynamic* InOrigin,
		UMaterialInstanceDynamic* InX,
		UMaterialInstanceDynamic* InY,
		UMaterialInstanceDynamic* InZ,
		UMaterialInstanceDynamic* InFocus,
		UMaterialInstanceDynamic* InPlane,
		UMaterialInstanceDynamic* InFocusPlane,
		UMaterialInterface*        InArcPlane);

	/** 设置坐标轴显示模式 */
	UFUNCTION()
	void SetCoorAxisMode(ECoordinateAxisMode InMode);
	ECoordinateAxisMode GetCoorAxisMode() const { return CoorAxisMode; }

	/** 设置坐标轴功能类型（Translation / Rotation / Scale） */
	void SetAxisType(ECoorAxisRenderType InType);
	ECoorAxisRenderType GetAxisType() const { return CoorAxisType; }

	/** 获取顶部 UI 吸附锚点（世界坐标） */
	UFUNCTION()
	FVector GetTopLocation() const;

	/** 获取当前渲染缩放比例 */
	float GetRenderScale() const;

	/** 获取相机到 Widget 的方向 */
	FVector GetDirectionToWidget() const { return DirectionToWidget; }

	/** 触屏拖拽模式开关 */
	void SetTouchDragging(bool bTouch);

	/** 手动触发 FocusAxis 更新（触屏时由外部调用） */
	void UpdateFocusAxis();

	/** 获取坐标轴方向（旋转后） */
	TArray<FVector> GetCoorAxisDirs() const;

	// --------------------------------------------------------
	// 拖拽回调 Delegate
	// --------------------------------------------------------
public:

	DECLARE_DELEGATE_OneParam(FTranslationOffsetDelegate, const FVector&);
	DECLARE_DELEGATE_OneParam(FRotationOffsetDelegate,    const FRotator&);
	DECLARE_DELEGATE_OneParam(FScaleOffsetDelegate,       const FVector&);

	FTranslationOffsetDelegate OnTranslationOffsetDelegate;
	FTranslationOffsetDelegate OnTranslationFinalDelegate;
	FRotationOffsetDelegate    OnRotationOffsetDelegate;
	FScaleOffsetDelegate       OnScaleOffsetDelegate;

	// --------------------------------------------------------
	// 旋转专属
	// --------------------------------------------------------
public:

	/** 当前累计旋转量（供圆弧可视化） */
	float TotalDeltaRotation = 0.f;

	/** 更新累计旋转量 */
	void UpdateDeltaRotation();

	// --------------------------------------------------------
	// 旋转角度 HUD 弹窗
	// --------------------------------------------------------
public:
	void CreateRotatePopupString();
	void RemoveRotatePopupString();
	void UpdateRotatePopupStringPos();

	// --------------------------------------------------------
	// 渲染相关公开数据
	// --------------------------------------------------------
public:

	/** 当前帧渲染缩放（由 SceneProxy 在渲染线程写入） */
	float RenderBoxScale = 1.f;

	/** Tick 端渲染缩放 */
	float RenderScale = 1.f;

	// --------------------------------------------------------
	// 材质
	// --------------------------------------------------------
public:

	UPROPERTY()
	UMaterialInstanceDynamic* AxisMaterialX      = nullptr;
	UPROPERTY()
	UMaterialInstanceDynamic* AxisMaterialY      = nullptr;
	UPROPERTY()
	UMaterialInstanceDynamic* AxisMaterialZ      = nullptr;
	UPROPERTY()
	UMaterialInstanceDynamic* PlaneMaterial      = nullptr;
	UPROPERTY()
	UMaterialInstanceDynamic* AxisOriginMaterial = nullptr;
	UPROPERTY()
	UMaterialInstanceDynamic* FocusAxisMaterial  = nullptr;
	UPROPERTY()
	UMaterialInstanceDynamic* FocusPlaneMaterial = nullptr;
	UPROPERTY()
	UMaterialInterface*        ArcPlaneMaterial   = nullptr;

	FLinearColor AxisColorX, AxisColorY, AxisColorZ;
	FLinearColor PlaneColor, AxisOriginColor, FocusColor, FocusPlaneColor, LucencyColor;

	// --------------------------------------------------------
	// FocusAxis / 碰撞检测
	// --------------------------------------------------------
public:

	int32     GetSelectFaceIndex() const;
	EAxisType GetSelectedVertexAxis(int32 InFaceIndex) const;

	/** 碰撞三角面数计数器 */
	int32 RenderAxisVertexNum    = 0;
	int32 RenderDualAxisVertexNum = 0;
	int32 RenderSceneVertexNum   = 0;

protected: /* PDI Render & Collision */

	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual class UBodySetup* GetBodySetup() override;

	virtual bool GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData) override;
	virtual bool ContainsPhysicsTriMeshData(bool InUseAllTriData) const override;
	virtual void GetMeshId(FString& OutMeshId) override;
	virtual bool WantsNegXTriMesh() override;

	void UpdateCollision();

	UPROPERTY()
	class UBodySetup* VertexMeshBodySetup = nullptr;

protected: /* Screen Widget Layer */

	void TickWidgetComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction);
	ULocalPlayer* GetOwnerPlayer() const;
	void RemoveWidgetFromScreen();

	bool bAddedToScreen = false;

	// --------------------------------------------------------
	// 内部渲染 & 拖拽入口（由 SceneProxy / Tick 调用）
	// --------------------------------------------------------
public:

	/** 渲染入口 */
	void Render(const FSceneView* InView, FPrimitiveDrawInterface* InPDI, FMeshElementCollector& InCollector);

	/** 碰撞构建入口 */
	void BuildCollision(FTriMeshCollisionData& InCollisionData);

	// --------------------------------------------------------
	// Movement 用公开成员（供 CoordinateAxisMovement.cpp 访问）
	// --------------------------------------------------------
public:

	/** 拖拽平面缓存 */
	TArray<FDragMovementInfo> DragMovementInfos;

	/** 屏幕空间坐标轴方向 */
	FVector2D ScreenXAxisDir = FVector2D::ZeroVector;
	FVector2D ScreenYAxisDir = FVector2D::ZeroVector;
	FVector2D ScreenZAxisDir = FVector2D::ZeroVector;
	FVector2D ScreenOrigin   = FVector2D::ZeroVector;

	/** 当前帧旋转增量 */
	float CurrentDeltaRotation = 0.f;

	/** 自定义坐标系 */
	FMatrix CustomCoordSystem = FMatrix::Identity;

	/** 平移初始偏移缓存 */
	bool    bTranslationInitialOffsetCached = false;
	FVector InitialTranslationPosition      = FVector::ZeroVector;
	FVector InitialTranslationOffset        = FVector::ZeroVector;

	// --------------------------------------------------------
	// 私有成员
	// --------------------------------------------------------
private:

	void InitParam();
	FVector2D GetMousePosition() const;
	FVector2D GetTouchPosition() const;
	void CacluMouseDragOffset();

	// 状态
	bool   bDragging      = false;
	bool   bTouchDragging = false;
	bool   bRenderVisible = false;

	// 轴选择
	EAxisType CoordinateFocusAxis  = EAT_None;
	EAxisType CoordinateRenderAxis = EAT_All;

	// 类型与模式
	ECoorAxisRenderType CoorAxisType = CART_None;
	ECoordinateAxisMode CoorAxisMode = CAM_Normal;

	// 射线检测最大距离
	float HitDistance = 100000000.f;

	// 方向缓存
	FVector DirectionToWidget = FVector::ZeroVector;

	// 帧间运动缓存
	FVector2D LastMousePos    = FVector2D::ZeroVector;
	FVector2D CurrentPosition = FVector2D::ZeroVector;

	// 运动输出缓存
	FVector  InMovementDiff    = FVector::ZeroVector;
	FVector  OutMovementDrag   = FVector::ZeroVector;
	FRotator OutMovementRotation = FRotator::ZeroRotator;
	FVector  OutMovementScale  = FVector::OneVector;

	// Controller
	APlayerController* Controller = nullptr;

	static constexpr float DefaultAxisLength = 35.0f;
};

// ============================================================
// SCoordAxisWidgetScreenLayer — 坐标轴标签 Slate 层
// ============================================================

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
	FVector2D Pivot    = FVector2D(0.5f, 0.5f);
	FVector2D DrawSize = FVector2D(32.f, 32.f);

	struct FComponentData
	{
		TWeakObjectPtr<UCoordinateAxisComponent> Component;
		TArray<SConstraintCanvas::FSlot*>        Slots;
		TArray<TSharedPtr<SWidget>>              Widgets;
		TArray<FVector>                          SlotLocation;
	};

	TMap<UCoordinateAxisComponent*, FComponentData> ComponentMap;
	FSlateFontInfo                                   FontInfo;
	FLocalPlayerContext                              PlayerContext;
	TSharedPtr<SConstraintCanvas>                    Canvas;
};
