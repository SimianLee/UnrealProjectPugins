// CoordinateAxisComponent.cpp
// 重构后版本 — 渲染逻辑委托给 CoordinateAxisRender.cpp，拖拽逻辑委托给 CoordinateAxisMovement.cpp

#include "CoordinateAxisComponent.h"
#include "Materials/Material.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicsEngine/BodySetup.h"
#include "Camera/PlayerCameraManager.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Interfaces/Interface_CollisionDataProvider.h"
#include "Framework/Application/SlateApplication.h"
#include "Blueprint/UserWidget.h"
#include "Widgets/SVirtualWindow.h"
#include "Slate/SGameLayerManager.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "SceneView.h"

#define COORD_AXIS TEXT("CoordAxisComponent")

// ============================================================
// 构造 & 生命周期
// ============================================================

UCoordinateAxisComponent::UCoordinateAxisComponent()
{
	if (HasAnyFlags(RF_ClassDefaultObject))
		return;

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = ETickingGroup::TG_PostPhysics;

	TranslucencySortPriority = TNumericLimits<int32>::Max();

	InitParam();

	CastShadow = false;
	bReceivesDecals = false;
	bAffectDistanceFieldLighting = false;
	bAffectDynamicIndirectLighting = false;
}

UCoordinateAxisComponent::~UCoordinateAxisComponent()
{
}

void UCoordinateAxisComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UCoordinateAxisComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	RemoveWidgetFromScreen();
}

void UCoordinateAxisComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveWidgetFromScreen();
	Super::EndPlay(EndPlayReason);
}

void UCoordinateAxisComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Controller)
	{
		ULocalPlayer* LP = Controller->GetLocalPlayer();
		if (LP && LP->ViewportClient)
		{
			FSceneViewProjectionData ProjectionData;
			if (LP->GetProjectionData(LP->ViewportClient->Viewport, eSSP_FULL, /*out*/ ProjectionData))
			{
				RenderScale =
					ProjectionData.ComputeViewProjectionMatrix().TransformFVector4(FVector4(GetComponentLocation(), 1)).W
					* (4.0f / ProjectionData.GetViewRect().Max.X - ProjectionData.GetViewRect().Min.X / ProjectionData.ProjectionMatrix.M[0][0]);
			}
		}
	}

	if (IsRenderVisible() && RenderBoxScale > 0)
	{
		UpdateCollision();

		UpdateBounds();
	}
	else
	{
		if (VertexMeshBodySetup)
		{
			UpdateBounds();

			VertexMeshBodySetup->InvalidatePhysicsData();
			DestroyPhysicsState();
		}
	}

	// 聚焦检测
	if (IsRenderVisible() && !IsDragging())
	{
		UpdateFocusAxis();
	}

	// 拖拽处理
	if (IsDragging())
	{
		CacluMouseDragOffset();
	}

	// 标记渲染脏
	if (IsRenderVisible())
	{
		MarkRenderStateDirty();
	}

	TickWidgetComponent(DeltaTime, TickType, ThisTickFunction);
}

// ============================================================
// 参数初始化
// ============================================================

void UCoordinateAxisComponent::InitParam()
{
	// 颜色
	AxisColorX = FLinearColor(0.60f, 0.02f, 0.0f, 1.f);
	AxisColorY = FLinearColor(0.15f, 0.40f, 0.0f, 1.f);
	AxisColorZ = FLinearColor(0.03f, 0.20f, 0.9f, 1.f);
	PlaneColor = FLinearColor(1.f, 1.f, 0.f, 0.3f);
	AxisOriginColor = FLinearColor(1.f, 1.f, 1.f, 1.f);
	FocusColor = FLinearColor(1.f, 1.f, 0.f, 1.f);
	FocusPlaneColor = FLinearColor(1.f, 1.f, 0.f, 0.5f);
	LucencyColor = FLinearColor(0.f, 0.f, 0.f, 0.f);

	// 拖拽状态
	bDragging = false;

	// 渲染状态
	bRenderVisible = false;
	SetHiddenInGame(!bRenderVisible);

	// 轴配置
	CoordinateFocusAxis = EAxisType::EAT_None;
	CoordinateRenderAxis = EAxisType::EAT_All;

	// 碰撞
	HitDistance = 100000000.f;

	// 面数计数器
	RenderAxisVertexNum = 0;
	RenderDualAxisVertexNum = 0;
	RenderSceneVertexNum = 0;

	// 控制器
	Controller = UGameplayStatics::GetPlayerController(this, 0);
}

// ============================================================
// 外部接口 — 拖拽
// ============================================================

void UCoordinateAxisComponent::StartDragging()
{
	if (CoordinateFocusAxis == EAT_None || !IsRenderVisible())
	{
		bDragging = false;
		return;
	}

	bDragging = true;
	bTranslationInitialOffsetCached = false;

	LastMousePos = CurrentPosition = GetMousePosition();

	// 委托给 Movement 命名空间
	CoordinateAxisMovementFuncs::PrepareStartDragMovement(this, CurrentPosition);
}

void UCoordinateAxisComponent::StopDragging()
{
	bDragging = false;
	SetTouchDragging(false);

	LastMousePos = CurrentPosition = FVector2D::ZeroVector;

	CoordinateAxisMovementFuncs::ResetMovementState(this);
}

bool UCoordinateAxisComponent::IsDragging() const
{
	return bDragging;
}

// ============================================================
// 外部接口 — 渲染可见性
// ============================================================

void UCoordinateAxisComponent::SetRenderComponent(bool bIsRender)
{
	bRenderVisible = bIsRender;
	SetHiddenInGame(!bIsRender);

	if (!bIsRender)
	{
		StopDragging();
	}
}

bool UCoordinateAxisComponent::IsRenderVisible() const
{
	return bRenderVisible;
}

// ============================================================
// 外部接口 — 轴选择
// ============================================================

EAxisType UCoordinateAxisComponent::GetFocusAxis() const
{
	return CoordinateFocusAxis;
}

void UCoordinateAxisComponent::SetRenderCoordinateAxis(const EAxisType& InMode)
{
	CoordinateRenderAxis = InMode;
}

EAxisType UCoordinateAxisComponent::GetRenderAxis() const
{
	return CoordinateRenderAxis;
}

// ============================================================
// 外部接口 — 材质
// ============================================================

void UCoordinateAxisComponent::SetComponentMaterial(
	UMaterialInstanceDynamic* InOrigin,
	UMaterialInstanceDynamic* InX,
	UMaterialInstanceDynamic* InY,
	UMaterialInstanceDynamic* InZ,
	UMaterialInstanceDynamic* InFocus,
	UMaterialInstanceDynamic* InPlane,
	UMaterialInstanceDynamic* InFocusPlane,
	UMaterialInterface* InArcPlane)
{
	if (InOrigin)
		AxisOriginMaterial = InOrigin;
	if (InX)
		AxisMaterialX = InX;
	if (InY)
		AxisMaterialY = InY;
	if (InZ)
		AxisMaterialZ = InZ;
	if (InFocus)
		FocusAxisMaterial = InFocus;
	if (InPlane)
		PlaneMaterial = InPlane;
	if (InFocusPlane)
		FocusPlaneMaterial = InFocusPlane;
	if (InArcPlane)
		ArcPlaneMaterial = InArcPlane;
}

// ============================================================
// 外部接口 — 模式 / 类型
// ============================================================

void UCoordinateAxisComponent::SetCoorAxisMode(ECoordinateAxisMode InMode)
{
	CoorAxisMode = InMode;
}

void UCoordinateAxisComponent::SetAxisType(ECoorAxisRenderType InType)
{
	if (CoorAxisType != InType)
	{
		StopDragging();
		CoorAxisType = InType;
		CoordinateFocusAxis = EAT_None;
	}
}

// ============================================================
// 外部接口 — 辅助
// ============================================================

FVector UCoordinateAxisComponent::GetTopLocation() const
{
	FVector XAxis = FVector(1, 0, 0);
	FVector YAxis = FVector(0, 1, 0);
	bool bMirrorAxis0 = ((XAxis | DirectionToWidget) <= 0.0f);
	bool bMirrorAxis1 = ((YAxis | DirectionToWidget) <= 0.0f);

	FVector Axis = FVector(0.f, 0.f, 45.f);
	if (bMirrorAxis0 && bMirrorAxis1)
		Axis = FVector(45.f, 0.f, 45.f);
	if (bMirrorAxis0 && !bMirrorAxis1)
		Axis = FVector(0.f, -45.f, 45.f);
	if (!bMirrorAxis0 && !bMirrorAxis1)
		Axis = FVector(-45.f, 0.f, 45.f);
	if (!bMirrorAxis0 && bMirrorAxis1)
		Axis = FVector(0.f, 45.f, 45.f);

	FMatrix WidgetMatrix = FTranslationMatrix(Axis) * FScaleMatrix(FVector(GetRenderScale()));
	return GetComponentLocation() + WidgetMatrix.GetOrigin();
}

float UCoordinateAxisComponent::GetRenderScale() const
{
	return RenderScale;
}

// GetDirectionToWidget 已在头文件中内联定义

void UCoordinateAxisComponent::SetTouchDragging(bool bTouch)
{
	bTouchDragging = bTouch;
}

TArray<FVector> UCoordinateAxisComponent::GetCoorAxisDirs() const
{
	TArray<FVector> CoorAxisDirs;
	CoorAxisDirs.SetNum(3);
	CoorAxisDirs[0] = FVector(1.0f, 0, 0);
	CoorAxisDirs[1] = FVector(0, 1.0f, 0);
	CoorAxisDirs[2] = FVector(0, 0, 1.0f);
	return CoorAxisDirs;
}

// ============================================================
// 旋转专属
// ============================================================

void UCoordinateAxisComponent::UpdateDeltaRotation()
{
	TotalDeltaRotation += CurrentDeltaRotation;
}

void UCoordinateAxisComponent::CreateRotatePopupString()
{
	// 旋转角度 HUD 弹窗创建（如需自定义实现，在此扩展）
}

void UCoordinateAxisComponent::RemoveRotatePopupString()
{
	// 移除旋转角度 HUD 弹窗（由 Slate 层管理）
}

void UCoordinateAxisComponent::UpdateRotatePopupStringPos()
{
	// 更新旋转角度 HUD 弹窗位置
}

// ============================================================
// SceneProxy
// ============================================================

FPrimitiveSceneProxy* UCoordinateAxisComponent::CreateSceneProxy()
{
	class FCustomRenderSceneProxy final : public FPrimitiveSceneProxy
	{
	public:
		SIZE_T GetTypeHash() const override
		{
			static size_t UniquePointer;
			return reinterpret_cast<size_t>(&UniquePointer);
		}

		FCustomRenderSceneProxy(UCoordinateAxisComponent* InComponent)
			: FPrimitiveSceneProxy(InComponent)
			, CoordinateAxisBase(InComponent)
		{
			bVerifyUsedMaterials = false;
			bAffectDistanceFieldLighting = false;
			bAffectDynamicIndirectLighting = false;
		}

		bool CanBeOccluded() const override
		{
			return false;
		}

		virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
		{
			QUICK_SCOPE_CYCLE_COUNTER(STAT_RenderComponentSceneProxy_GetDynamicMeshElements);

			for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
			{
				if (VisibilityMap & (1 << ViewIndex))
				{
					const FSceneView* View = Views[ViewIndex];
					FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

					if (PDI && View)
					{
						if (CoordinateAxisBase && CoordinateAxisBase->IsRenderVisible())
						{
							CoordinateAxisBase->Render(View, PDI, Collector);
						}
					}
				}
			}
		}

		virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
		{
			FPrimitiveViewRelevance ViewRelevance;
			ViewRelevance.bDynamicRelevance = true;
			ViewRelevance.bDrawRelevance = IsShown(View);
			ViewRelevance.bSeparateTranslucency = true;
			return ViewRelevance;
		}

		uint32 GetMemoryFootprint(void) const
		{
			return (sizeof(*this) + GetAllocatedSize());
		}

		uint32 GetAllocatedSize(void) const
		{
			return FPrimitiveSceneProxy::GetAllocatedSize();
		}

	private:
		UCoordinateAxisComponent* CoordinateAxisBase;
	};

	return new FCustomRenderSceneProxy(this);
}

// ============================================================
// 边界 & 碰撞基础设施
// ============================================================

FBoxSphereBounds UCoordinateAxisComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	if (bRenderVisible)
	{
		return FBoxSphereBounds(FBox(
			GetComponentLocation() - FVector(50) * FMath::Max(RenderBoxScale, 1.f),
			GetComponentLocation() + FVector(50) * FMath::Max(RenderBoxScale, 1.f)));
	}
	return FBoxSphereBounds(FVector::ZeroVector, FVector::ZeroVector, 0.f);
}

class UBodySetup* UCoordinateAxisComponent::GetBodySetup()
{
	if (VertexMeshBodySetup == nullptr)
	{
		VertexMeshBodySetup = NewObject<UBodySetup>(this, NAME_None, (IsTemplate() ? RF_Public : RF_NoFlags));
		VertexMeshBodySetup->BodySetupGuid = FGuid::NewGuid();

		VertexMeshBodySetup->bGenerateMirroredCollision = false;
		VertexMeshBodySetup->bDoubleSidedGeometry = true;
		VertexMeshBodySetup->bHasCookedCollisionData = false;
		VertexMeshBodySetup->CollisionTraceFlag = CTF_UseComplexAsSimple;
		VertexMeshBodySetup->SetFlags(RF_Public);
	}
	return VertexMeshBodySetup;
}

// ============================================================
// 渲染入口 — 分发到 CoordinateAxisRenderFuncs
// ============================================================

void UCoordinateAxisComponent::Render(const FSceneView* InView, FPrimitiveDrawInterface* InPDI, FMeshElementCollector& InCollector)
{
	// 计算帧内渲染缩放
	RenderBoxScale = InView->WorldToScreen(GetComponentLocation()).W
		* (4.0f / InView->UnscaledViewRect.Width() / InView->ViewMatrices.GetProjectionMatrix().M[0][0]);

	DirectionToWidget = InView->IsPerspectiveProjection()
		? (GetComponentLocation() - InView->ViewMatrices.GetViewOrigin())
		: -InView->GetViewDirection();
	DirectionToWidget.Normalize();

	// 构建 LocalToWorld 矩阵（无缩放）
	FMatrix Matrix = GetComponentTransform().ToMatrixNoScale();

	// 根据坐标轴类型分发
	switch (CoorAxisType)
	{
	case CART_Translation:
		CoordinateAxisRenderFuncs::RenderTranslation(this, InView, InPDI, InCollector, Matrix);
		break;
	case CART_Rotate:
		CoordinateAxisRenderFuncs::RenderRotate(this, InView, InPDI, InCollector, Matrix);
		break;
	case CART_Scale:
		CoordinateAxisRenderFuncs::RenderScale(this, InView, InPDI, InCollector, Matrix);
		break;
	default:
		break;
	}
}

// ============================================================
// 碰撞构建 — 分发到 CoordinateAxisRenderFuncs
// ============================================================

void UCoordinateAxisComponent::BuildCollision(FTriMeshCollisionData& InCollisionData)
{
	FMatrix Matrix = GetComponentTransform().ToMatrixNoScale();

	switch (CoorAxisType)
	{
	case CART_Translation:
		CoordinateAxisRenderFuncs::BuildTranslationCollision(this, Matrix, InCollisionData);
		break;
	case CART_Rotate:
		CoordinateAxisRenderFuncs::BuildRotateCollision(this, Matrix, InCollisionData);
		break;
	case CART_Scale:
		CoordinateAxisRenderFuncs::BuildScaleCollision(this, Matrix, InCollisionData);
		break;
	default:
		break;
	}
}

// ============================================================
// 碰撞数据接口 (IInterface_CollisionDataProvider)
// ============================================================

bool UCoordinateAxisComponent::GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData)
{
	if (IsRenderVisible())
	{
		RenderAxisVertexNum = 0;
		RenderDualAxisVertexNum = 0;
		RenderSceneVertexNum = 0;

		FTriMeshCollisionData TempCollisionData;
		BuildCollision(TempCollisionData);
		CollisionData->Vertices = TempCollisionData.Vertices;
		CollisionData->Indices = TempCollisionData.Indices;
		return true;
	}
	return false;
}

bool UCoordinateAxisComponent::ContainsPhysicsTriMeshData(bool InUseAllTriData) const
{
	return IsRenderVisible();
}

void UCoordinateAxisComponent::GetMeshId(FString& OutMeshId)
{
	OutMeshId = VertexMeshBodySetup->BodySetupGuid.ToString();
}

bool UCoordinateAxisComponent::WantsNegXTriMesh()
{
	return false;
}

void UCoordinateAxisComponent::UpdateCollision()
{
	if (!VertexMeshBodySetup)
		return;
	VertexMeshBodySetup->BodySetupGuid = FGuid::NewGuid();
	VertexMeshBodySetup->InvalidatePhysicsData();
	VertexMeshBodySetup->CreatePhysicsMeshes();
	RecreatePhysicsState();
}

// ============================================================
// FocusAxis 检测
// ============================================================

void UCoordinateAxisComponent::UpdateFocusAxis()
{
	int32 SelectFaceIndex = GetSelectFaceIndex();
	CoordinateFocusAxis = GetSelectedVertexAxis(SelectFaceIndex);
}

int32 UCoordinateAxisComponent::GetSelectFaceIndex() const
{
	if (Controller)
	{
		FHitResult HitResult;
		FVector WorldLocation, WorldDirection;

		if (bTouchDragging)
		{
			bool bIsPressed = false;
			FVector2D TouchPosition = FVector2D::ZeroVector;
			Controller->GetInputTouchState(ETouchIndex::Type::Touch1, TouchPosition.X, TouchPosition.Y, bIsPressed);
			UGameplayStatics::DeprojectScreenToWorld(Controller, TouchPosition, WorldLocation, WorldDirection);

			FVector StartLocation = WorldLocation;
			FVector EndLocation = StartLocation + WorldDirection * 100000000.f;
			FCollisionQueryParams CollisionQueryParams;
			CollisionQueryParams.bTraceComplex = true;
			CollisionQueryParams.bReturnFaceIndex = true;
			GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_GameTraceChannel2, CollisionQueryParams);

			if (HitResult.bBlockingHit && HitResult.GetComponent() == this)
			{
				return HitResult.FaceIndex;
			}
		}
		else
		{
			Controller->DeprojectMousePositionToWorld(WorldLocation, WorldDirection);
			FVector StartLocation = WorldLocation;
			FVector EndLocation = StartLocation + WorldDirection * HitDistance;
			FCollisionQueryParams CollisionQueryParams;
			CollisionQueryParams.bTraceComplex = true;
			CollisionQueryParams.bReturnFaceIndex = true;
			GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_GameTraceChannel2, CollisionQueryParams);

			if (HitResult.bBlockingHit && HitResult.GetComponent() == this)
			{
				return HitResult.FaceIndex;
			}
		}
	}
	return -1;
}

EAxisType UCoordinateAxisComponent::GetSelectedVertexAxis(int32 InFaceIndex) const
{
	if (InFaceIndex < 0)
		return EAT_None;

	{
		int32 RenderNum = 0;
		if (CoordinateRenderAxis & EAxisType::EAT_X)
			RenderNum += 1;
		if (CoordinateRenderAxis & EAxisType::EAT_Y)
			RenderNum += 1;
		if (CoordinateRenderAxis & EAxisType::EAT_Z)
			RenderNum += 1;

		// 轴
		if (InFaceIndex < RenderAxisVertexNum)
		{
			int32 XAxisVertexNum = 0;
			int32 YAxisVertexNum = 0;
			int32 ZAxisVertexNum = 0;
			int32 TempAxisVertexNum = (RenderNum > 0) ? (RenderAxisVertexNum / RenderNum) : 0;

			if (CoordinateRenderAxis & EAxisType::EAT_X)
			{
				XAxisVertexNum = TempAxisVertexNum;
				if (XAxisVertexNum != 0 && InFaceIndex < XAxisVertexNum)
					return EAT_X;
			}
			if (CoordinateRenderAxis & EAxisType::EAT_Y)
			{
				YAxisVertexNum = TempAxisVertexNum;
				if (YAxisVertexNum != 0 && InFaceIndex < (XAxisVertexNum + YAxisVertexNum))
					return EAT_Y;
			}
			if (CoordinateRenderAxis & EAxisType::EAT_Z)
			{
				ZAxisVertexNum = TempAxisVertexNum;
				if (ZAxisVertexNum != 0 && InFaceIndex < (XAxisVertexNum + YAxisVertexNum + ZAxisVertexNum))
					return EAT_Z;
			}
		}

		// 拐角
		if (InFaceIndex < RenderAxisVertexNum + RenderDualAxisVertexNum)
		{
			int32 XYAxisVertexNum = 0;
			int32 XZAxisVertexNum = 0;
			int32 YZAxisVertexNum = 0;
			int32 TempAxisVertexNum = (RenderNum > 0) ? (RenderDualAxisVertexNum / RenderNum) : 0;

			if ((CoordinateRenderAxis & EAxisType::EAT_XY) == EAxisType::EAT_XY)
			{
				XYAxisVertexNum = TempAxisVertexNum;
				if (XYAxisVertexNum != 0 && InFaceIndex < (XYAxisVertexNum + RenderAxisVertexNum))
					return EAT_XY;
			}
			if ((CoordinateRenderAxis & EAxisType::EAT_XZ) == EAxisType::EAT_XZ)
			{
				XZAxisVertexNum = TempAxisVertexNum;
				if (XZAxisVertexNum != 0 && InFaceIndex < (XYAxisVertexNum + XZAxisVertexNum + RenderAxisVertexNum))
					return EAT_XZ;
			}
			if ((CoordinateRenderAxis & EAxisType::EAT_YZ) == EAxisType::EAT_YZ)
			{
				YZAxisVertexNum = TempAxisVertexNum;
				if (YZAxisVertexNum != 0 && InFaceIndex < (XYAxisVertexNum + XZAxisVertexNum + YZAxisVertexNum + RenderAxisVertexNum))
					return EAT_YZ;
			}
		}

		// 原点
		if (InFaceIndex < RenderAxisVertexNum + RenderDualAxisVertexNum + RenderSceneVertexNum)
		{
			return EAT_Screen;
		}
	}

	return EAT_None;
}

// ============================================================
// 鼠标 / 触屏输入
// ============================================================

void UCoordinateAxisComponent::CacluMouseDragOffset()
{
	CurrentPosition = GetMousePosition();
}

FVector2D UCoordinateAxisComponent::GetMousePosition() const
{
	if (bTouchDragging)
		return GetTouchPosition();

	FVector2D TempMousePosition;
	Controller->GetMousePosition(TempMousePosition.X, TempMousePosition.Y);
	return TempMousePosition;
}

FVector2D UCoordinateAxisComponent::GetTouchPosition() const
{
	bool bIsPressed = false;
	FVector2D TouchPosition = FVector2D::ZeroVector;
	Controller->GetInputTouchState(ETouchIndex::Type::Touch1, TouchPosition.X, TouchPosition.Y, bIsPressed);
	return TouchPosition;
}

// ============================================================
// Widget Screen Layer — IGameLayer 适配器
// ============================================================

class FCoordWorldWidgetScreenLayer : public IGameLayer
{
public:
	FCoordWorldWidgetScreenLayer(const FLocalPlayerContext& PlayerContext)
	{
		OwningPlayer = PlayerContext;
	}

	~FCoordWorldWidgetScreenLayer()
	{
	}

	void AddComponent(UCoordinateAxisComponent* InComponent, const FString& InText, const FVector& InLocation, const FLinearColor& InColor)
	{
		if (ScreenLayer.IsValid())
		{
			ScreenLayer.Pin()->AddComponent(InComponent, InText, InLocation, InColor);
		}
		else
		{
			FComponentLayerData& Data = ComponentMap.FindOrAdd(InComponent);
			Data.Component = InComponent;
			Data.Texts.Add(InText);
			Data.Locations.Add(InLocation);
			Data.Colors.Add(InColor);
		}
	}

	void RemoveComponent(UCoordinateAxisComponent* InComponent)
	{
		if (ScreenLayer.IsValid())
		{
			ScreenLayer.Pin()->RemoveComponent(InComponent);
		}
	}

	virtual TSharedRef<SWidget> AsWidget() override
	{
		if (ScreenLayer.IsValid())
		{
			return ScreenLayer.Pin().ToSharedRef();
		}
		else
		{
			TSharedRef<SCoordAxisWidgetScreenLayer> NewScreenLayer = SNew(SCoordAxisWidgetScreenLayer, OwningPlayer);

			for (auto It = ComponentMap.CreateIterator(); It; ++It)
			{
				FComponentLayerData& Data = It.Value();
				for (int32 Index = 0; Index < Data.Texts.Num(); Index++)
				{
					NewScreenLayer->AddComponent(Data.Component.Get(), Data.Texts[Index], Data.Locations[Index], Data.Colors[Index]);
				}
			}

			ComponentMap.Empty();
			ScreenLayer = NewScreenLayer;
			return NewScreenLayer;
		}
	}

private:
	FLocalPlayerContext OwningPlayer;
	TWeakPtr<SCoordAxisWidgetScreenLayer> ScreenLayer;

protected:
	struct FComponentLayerData
	{
		TWeakObjectPtr<UCoordinateAxisComponent> Component;
		TArray<FString> Texts;
		TArray<FVector> Locations;
		TArray<FLinearColor> Colors;
	};

	TMap<UCoordinateAxisComponent*, FComponentLayerData> ComponentMap;
};

// ============================================================
// Widget Screen Layer — Tick 管理
// ============================================================

void UCoordinateAxisComponent::TickWidgetComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (CoorAxisMode == CAM_Mix && CoorAxisType == CART_Rotate)
	{
		if (bAddedToScreen)
			RemoveWidgetFromScreen();
		return;
	}

	ULocalPlayer* TargetPlayer = GetOwnerPlayer();
	APlayerController* PlayerController = TargetPlayer ? TargetPlayer->PlayerController : nullptr;

	if (TargetPlayer && PlayerController && IsVisible() && IsRenderVisible())
	{
		if (!bAddedToScreen)
		{
			if (GetWorld()->IsGameWorld())
			{
				if (UGameViewportClient* ViewportClient = GetWorld()->GetGameViewport())
				{
					TSharedPtr<IGameLayerManager> LayerManager = ViewportClient->GetGameLayerManager();
					if (LayerManager.IsValid())
					{
						TSharedPtr<FCoordWorldWidgetScreenLayer> ScreenLayer;
						FLocalPlayerContext PlayerContext(TargetPlayer, GetWorld());

						TSharedPtr<IGameLayer> Layer = LayerManager->FindLayerForPlayer(TargetPlayer, COORD_AXIS);
						if (!Layer.IsValid())
						{
							TSharedRef<FCoordWorldWidgetScreenLayer> NewScreenLayer = MakeShareable(new FCoordWorldWidgetScreenLayer(PlayerContext));
							LayerManager->AddLayerForPlayer(TargetPlayer, COORD_AXIS, NewScreenLayer, -100);
							ScreenLayer = NewScreenLayer;
						}
						else
						{
							ScreenLayer = StaticCastSharedPtr<FCoordWorldWidgetScreenLayer>(Layer);
						}

						bAddedToScreen = true;

						if (CoorAxisType == CART_Translation || CoorAxisType == CART_Scale)
						{
							ScreenLayer->AddComponent(this, TEXT("X"), FVector(60.f, 0.f, 0.f), AxisColorX);
							ScreenLayer->AddComponent(this, TEXT("Y"), FVector(0.f, -60.f, 0.f), AxisColorY);
							ScreenLayer->AddComponent(this, TEXT("Z"), FVector(0.f, 0.f, 60.f), AxisColorZ);
						}
						else if (CoorAxisType == CART_Rotate && CoorAxisMode == CAM_Normal)
						{
							ScreenLayer->AddComponent(this, TEXT("X"), FVector(0.f, 60.f, 60.f), AxisColorX);
							ScreenLayer->AddComponent(this, TEXT("Y"), FVector(60.f, 0.f, 60.f), AxisColorY);
							ScreenLayer->AddComponent(this, TEXT("Z"), FVector(60.f, 60.f, 0.f), AxisColorZ);
						}
					}
				}
			}
		}
	}
	else if (bAddedToScreen)
	{
		RemoveWidgetFromScreen();
	}
}

ULocalPlayer* UCoordinateAxisComponent::GetOwnerPlayer() const
{
	if (UWorld* LocalWorld = GetWorld())
	{
		UGameInstance* GameInstance = LocalWorld->GetGameInstance();
		check(GameInstance);
		return GameInstance->GetFirstGamePlayer();
	}
	return nullptr;
}

void UCoordinateAxisComponent::RemoveWidgetFromScreen()
{
	bAddedToScreen = false;

	if (UGameViewportClient* ViewportClient = GetWorld()->GetGameViewport())
	{
		TSharedPtr<IGameLayerManager> LayerManager = ViewportClient->GetGameLayerManager();
		if (LayerManager.IsValid())
		{
			if (ULocalPlayer* TargetPlayer = GetOwnerPlayer())
			{
				TSharedPtr<IGameLayer> Layer = LayerManager->FindLayerForPlayer(TargetPlayer, COORD_AXIS);
				if (Layer.IsValid())
				{
					TSharedPtr<FCoordWorldWidgetScreenLayer> ScreenLayer = StaticCastSharedPtr<FCoordWorldWidgetScreenLayer>(Layer);
					ScreenLayer->RemoveComponent(this);
				}
			}
		}
	}
}

// ============================================================
// SCoordAxisWidgetScreenLayer — 坐标轴标签 Slate Widget
// ============================================================

void SCoordAxisWidgetScreenLayer::Construct(const FArguments& InArgs, const FLocalPlayerContext& InPlayerContext)
{
	PlayerContext = InPlayerContext;

	bCanSupportFocus = false;
	DrawSize = FVector2D(0, 0);
	Pivot = FVector2D(0.5f, 0.5f);

	ChildSlot
	[
		SAssignNew(Canvas, SConstraintCanvas)
	];

	FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 12);
	FontInfo.OutlineSettings.OutlineSize = 1.f;
	FontInfo.OutlineSettings.OutlineColor = FLinearColor(0, 0, 0, 1);
}

void SCoordAxisWidgetScreenLayer::AddComponent(UCoordinateAxisComponent* InComponent, const FString& InText, const FVector& InLocation, const FLinearColor& InColor)
{
	if (InComponent)
	{
		FComponentData& Data = ComponentMap.FindOrAdd(InComponent);
		Data.Component = InComponent;

		SConstraintCanvas::FSlot* Slot;
		TSharedPtr<SWidget> Widget;
		Canvas->AddSlot()
		.Expose(Slot)
		[
			SAssignNew(Widget, SBox)
			[
				SNew(STextBlock)
				.ColorAndOpacity(InColor)
				.Text(FText::FromString(InText))
				.Font(FontInfo)
			]
		];

		Data.Slots.Add(Slot);
		Data.Widgets.Add(Widget);
		Data.SlotLocation.Add(InLocation);
	}
}

void SCoordAxisWidgetScreenLayer::RemoveComponent(UCoordinateAxisComponent* InComponent)
{
	if (InComponent)
	{
		if (FComponentData* Data = ComponentMap.Find(InComponent))
		{
			for (TSharedPtr<SWidget> Widget : Data->Widgets)
			{
				Canvas->RemoveSlot(Widget.ToSharedRef());
			}
			ComponentMap.Remove(InComponent);
		}
	}
}

void SCoordAxisWidgetScreenLayer::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	if (APlayerController* PlayerController = PlayerContext.GetPlayerController())
	{
		if (UGameViewportClient* ViewportClient = PlayerController->GetWorld()->GetGameViewport())
		{
			const FGeometry& ViewportGeometry = ViewportClient->GetGameLayerManager()->GetViewportWidgetHostGeometry();

			for (auto It = ComponentMap.CreateIterator(); It; ++It)
			{
				FComponentData& Data = It.Value();

				if (UCoordinateAxisComponent* CoorAxisComponentBase = Data.Component.Get())
				{
					for (int32 Index = 0; Index < Data.Slots.Num(); Index++)
					{
						if (CoorAxisComponentBase->GetCoorAxisMode() == CAM_Normal && CoorAxisComponentBase->GetAxisType() == CART_Rotate)
						{
							FVector XAxis = FVector(1, 0, 0);
							FVector YAxis = FVector(0, 1, 0);
							FVector ZAxis = FVector(0, 0, 1);
							bool bMirrorAxisX = ((XAxis | CoorAxisComponentBase->GetDirectionToWidget()) <= 0.0f);
							bool bMirrorAxisY = ((YAxis | CoorAxisComponentBase->GetDirectionToWidget()) <= 0.0f);
							bool bMirrorAxisZ = ((ZAxis | CoorAxisComponentBase->GetDirectionToWidget()) <= 0.0f);
							Data.SlotLocation[Index].X = bMirrorAxisX ? FMath::Abs(Data.SlotLocation[Index].X) : -FMath::Abs(Data.SlotLocation[Index].X);
							Data.SlotLocation[Index].Y = bMirrorAxisY ? FMath::Abs(Data.SlotLocation[Index].Y) : -FMath::Abs(Data.SlotLocation[Index].Y);
							Data.SlotLocation[Index].Z = bMirrorAxisZ ? FMath::Abs(Data.SlotLocation[Index].Z) : -FMath::Abs(Data.SlotLocation[Index].Z);
						}

						FVector WorldLocation = CoorAxisComponentBase->GetComponentLocation() + (FTranslationMatrix(Data.SlotLocation[Index]) * FRotationMatrix(CoorAxisComponentBase->GetComponentRotation()) * FScaleMatrix(FVector(CoorAxisComponentBase->GetRenderScale()))).GetOrigin();
						FVector ViewportPosition = FVector::ZeroVector;
						const bool bProjected = UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPositionWithDistance(PlayerController, WorldLocation, ViewportPosition, false);

						if (bProjected)
						{
							Data.Widgets[Index]->SetVisibility(EVisibility::SelfHitTestInvisible);

							SConstraintCanvas::FSlot* CanvasSlot = Data.Slots[Index];
							if (CanvasSlot)
							{
								FVector2D AbsoluteProjectedLocation = ViewportGeometry.LocalToAbsolute(FVector2D(ViewportPosition.X, ViewportPosition.Y));
								FVector2D LocalPosition = AllottedGeometry.AbsoluteToLocal(AbsoluteProjectedLocation);

								CanvasSlot->AutoSize(DrawSize.IsZero());
								CanvasSlot->Offset(FMargin(LocalPosition.X, LocalPosition.Y, DrawSize.X, DrawSize.Y));
								CanvasSlot->Anchors(FAnchors(0, 0, 0, 0));
								CanvasSlot->Alignment(Pivot);
								CanvasSlot->ZOrder(-ViewportPosition.Z);
							}
						}
						else
						{
							Data.Widgets[Index]->SetVisibility(EVisibility::Collapsed);
						}
					}
				}
			}
		}
	}
}
