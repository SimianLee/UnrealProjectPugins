// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/CoordinateAxis/CoordinateAxisComponent.h"
#include "Materials/Material.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicsEngine/BodySetup.h"
#include "Components/ActorComponent.h"
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
#include "DTSWidgetStyles.h"
#include "SceneView.h"
#include "Tools/MultiViewportTool.h"

#define COORD_AXIS TEXT("CoordAxisComponent")

void UCoordinateAxisComponent::StartDragging()
{
	if (CoordinateFocusAxis == EAT_None || !bIsRender())
	{
		bDragging = false;
		return;
	}

	bDragging = true;
	bTranslationInitialOffsetCached = false;

	// Drag Data
	LastMousePos = CurrentPosition = GetMousePosition();
	PrepareStartDragMovement();
}

void UCoordinateAxisComponent::StopDragging()
{
	bDragging = false;
	SetTouchDragging(false);

	// Drag Data
	LastMousePos = CurrentPosition = FVector2D::ZeroVector;
	DragMovementInfos.Empty();
}

bool UCoordinateAxisComponent::bIsDragging()
{
	return bDragging;
}

void UCoordinateAxisComponent::SetRenderComponent(bool bIsRender)
{
	bRender = bIsRender;
	SetHiddenInGame(!bRender);

	if(!bRender)
	{
		StopDragging();
	}
}

bool UCoordinateAxisComponent::bIsRender()const
{
	return bRender;
}

EAxisType UCoordinateAxisComponent::GetFocusAxis()
{
	return CoordinateFocusAxis;
}

void UCoordinateAxisComponent::SetRenderCoordinateAxis(const EAxisType& InMode)
{
	CoordinateRenderAxis = InMode;
}

EAxisType UCoordinateAxisComponent::GetRenderAxis()
{
	return CoordinateRenderAxis;
}

void UCoordinateAxisComponent::SetComponentMaterial(UMaterialInstanceDynamic* InOrigin, UMaterialInstanceDynamic* InX, UMaterialInstanceDynamic* InY, UMaterialInstanceDynamic* InZ, UMaterialInstanceDynamic* InFocus, UMaterialInstanceDynamic* InPlane, UMaterialInstanceDynamic* InFocusPlane, UMaterialInterface* InArcPlane)
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

void UCoordinateAxisComponent::SetCoorAixsMode(ECoordinateAxisMode InMode)
{
	CoorAxisMode = InMode;
}

FVector UCoordinateAxisComponent::GetTopLocation()
{
	FVector XAxis = FVector(1, 0, 0);
	FVector YAxis = FVector(0, 1, 0);
	FVector ZAxis = FVector(0, 0, 1);
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
	FVector OutOrigin = GetComponentLocation() + WidgetMatrix.GetOrigin();

	return OutOrigin;
}

float UCoordinateAxisComponent::GetRenderScale()
{
	return RenderScale;
}

void UCoordinateAxisComponent::SetTouchDragging(bool bTouch)
{
	bTouchDragging = bTouch;
}

// Sets default values for this component's properties
UCoordinateAxisComponent::UCoordinateAxisComponent()
{
    if(HasAnyFlags(RF_ClassDefaultObject))
        return;

    PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = ETickingGroup::TG_PostPhysics;

	TranslucencySortPriority = TNumericLimits<int32>::Max();
	// DepthPriorityGroup = SDPG_MAX;

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

void UCoordinateAxisComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
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

	if (bIsRender() && RenderBoxScale > 0)
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

	// 聚焦
	if (bIsRender() && !bIsDragging())
	{
		UpdateFocusAxis();
	}

	// 拖拽
	if (bIsDragging())
	{
		CacluMouseDragOffset();
	}

	// 更新
	if (bIsRender())
	{
		MarkRenderStateDirty();
	}

	TickWidgetComponent(DeltaTime, TickType, ThisTickFunction);
}

void UCoordinateAxisComponent::InitParam()
{
	// Color
	AxisColorX = FLinearColor(0.60f, 0.02f, 0.0f, 1.f);
	AxisColorY = FLinearColor(0.15f, 0.40f, 0.0f, 1.f);
	AxisColorZ = FLinearColor(0.03f, 0.20f, 0.9f, 1.f);
	PlaneClor = FLinearColor(1.f, 1.f, 0.f, 0.3f);
	AxisOriginColor = FLinearColor(1.f, 1.f, 1.f, 1.f);
	FocusColor = FLinearColor(1.f, 1.f, 0.f, 1.f);
	FocusPlaneClor = FLinearColor(1.f, 1.f, 0.f, 0.5f);
	LucencyColor = FLinearColor(0.f, 0.f, 0.f, 0.f);

	// 拖拽
	bDragging = false;

	// 渲染
	bRender = false;
	SetHiddenInGame(!bRender);

	// Config Data
	CoordinateFocusAxis = EAxisType::EAT_None;

	CoordinateRenderAxis = EAxisType::EAT_All;

	// Hit Collision
	HitDistance = 100000000.f;

	// 渲染面数
	RenderAxisVertexNum = 0;

	RnederDualAxisVertexNum = 0;

	RenderSceneVertexNum = 0;

	// Foucs
	Controller = UGameplayStatics::GetPlayerController(this, 0);
}

/*
void UCoordinateAxisComponent::InitMaterial()
{
	// Init Material
	UMaterial* AxisMaterialBase = LoadObject<UMaterial>(nullptr, TEXT("Material'/Game/AxisMaterial.AxisMaterial'"));
	check(AxisMaterialBase);

	AxisMaterialX = UMaterialInstanceDynamic::Create(AxisMaterialBase, NULL);
	AxisMaterialX->SetVectorParameterValue("Color", AxisColorX);

	AxisMaterialY = UMaterialInstanceDynamic::Create(AxisMaterialBase, NULL);
	AxisMaterialY->SetVectorParameterValue("Color", AxisColorY);

	AxisMaterialZ = UMaterialInstanceDynamic::Create(AxisMaterialBase, NULL);
	AxisMaterialZ->SetVectorParameterValue("Color", AxisColorZ);

	PlaneMaterial = UMaterialInstanceDynamic::Create(AxisMaterialBase, NULL);
	PlaneMaterial->SetVectorParameterValue("Color", PlaneClor);

	AxisOriginMaterial = UMaterialInstanceDynamic::Create(AxisMaterialBase, NULL);
	AxisOriginMaterial->SetVectorParameterValue("Color", AxisOriginColor);

	FocusAxisMaterial = UMaterialInstanceDynamic::Create(AxisMaterialBase, NULL);
	FocusAxisMaterial->SetVectorParameterValue("Color", FocusColor);

	FocusPlaneMaterial = UMaterialInstanceDynamic::Create(AxisMaterialBase, NULL);
	FocusPlaneMaterial->SetVectorParameterValue("Color", FocusPlaneClor);

	LucencyMaterial = UMaterialInstanceDynamic::Create(AxisMaterialBase, NULL);
	LucencyMaterial->SetVectorParameterValue("Color", LucencyColor);

	ArcPlaneMaterial = (UMaterial*)StaticLoadObject(UMaterial::StaticClass(), NULL, TEXT("/Engine/EditorMaterials/WidgetVertexColorMaterial.WidgetVertexColorMaterial"), NULL, LOAD_None, NULL);
	check(ArcPlaneMaterial);
}
*/

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
				if (VisibilityMap &(1 << ViewIndex))
				{
					const FSceneView* View = Views[ViewIndex];
					FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

					if (PDI && View)
					{
						if (CoordinateAxisBase && CoordinateAxisBase->bIsRender())
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
			return(sizeof(*this) + GetAllocatedSize());
		}

		uint32 GetAllocatedSize(void) const
		{
			return FPrimitiveSceneProxy::GetAllocatedSize();
		}

	private:

		UCoordinateAxisComponent* CoordinateAxisBase;
	};

	return  new FCustomRenderSceneProxy(this);
}

FBoxSphereBounds UCoordinateAxisComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	if (bRender)
	{
		return FBoxSphereBounds(FBox(GetComponentLocation() - FVector(50) * FMath::Max(RenderBoxScale, 1.f), GetComponentLocation() + FVector(50) * FMath::Max(RenderBoxScale, 1.f)));
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

void UCoordinateAxisComponent::Render(const FSceneView* InView, FPrimitiveDrawInterface* InPDI, FMeshElementCollector& InCollector)
{
	// Scale
	RenderBoxScale = InView->WorldToScreen(GetComponentLocation()).W * (4.0f / InView->UnscaledViewRect.Width() / InView->ViewMatrices.GetProjectionMatrix().M[0][0]);

	DirectionToWidget = InView->IsPerspectiveProjection() ? (GetComponentLocation() - InView->ViewMatrices.GetViewOrigin()) : -InView->GetViewDirection();
	DirectionToWidget.Normalize();

	//子类重写绘制
}

bool UCoordinateAxisComponent::GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData)
{
	if (bIsRender())
	{
		// 初始化
		RenderAxisVertexNum = 0;
		RnederDualAxisVertexNum = 0;
		RenderSceneVertexNum = 0;

		// 获取数据
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
	return bIsRender();
}

void UCoordinateAxisComponent::GetMeshId(FString& OutMeshId)
{
	OutMeshId = VertexMeshBodySetup->BodySetupGuid.ToString();
}

bool UCoordinateAxisComponent::WantsNegXTriMesh()
{
	return false;
}

void UCoordinateAxisComponent::BuildCollision(FTriMeshCollisionData& InCollisionData)
{
	// 子类重写
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
			int32 TempAxisVertexNum = RenderAxisVertexNum / RenderNum;
			if (CoordinateRenderAxis & EAxisType::EAT_X)
			{
				XAxisVertexNum = TempAxisVertexNum;
				if (XAxisVertexNum != 0 && InFaceIndex < (XAxisVertexNum))
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
		if (InFaceIndex < RenderAxisVertexNum + RnederDualAxisVertexNum)
		{
			int32 XYAxisVertexNum = 0;
			int32 XZAxisVertexNum = 0;
			int32 YZAxisVertexNum = 0;
			int32 TempAxisVertexNum = RnederDualAxisVertexNum / RenderNum;
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
		if (InFaceIndex < RenderAxisVertexNum + RnederDualAxisVertexNum + RenderSceneVertexNum)
		{
			return EAT_Screen;
		}
	}

	return EAT_None;
}

void UCoordinateAxisComponent::CacluMouseDragOffset()
{
	CurrentPosition = GetMousePosition();
}

FVector2D UCoordinateAxisComponent::GetMousePosition()
{
	if (bTouchDragging)
		return GetTouchPosition();

	FVector2D TempMousePostion;
	Controller->GetMousePosition(TempMousePostion.X, TempMousePostion.Y);
	UMultiViewportTool::GetMultiViewportTool()->GetMultiViewEnableAndMouseInfo(TempMousePostion);
	return TempMousePostion;
}

FVector2D UCoordinateAxisComponent::GetTouchPosition()
{
	bool bIsPressed = false;
	FVector2D TouchPosition = FVector2D::ZeroVector;
	Controller->GetInputTouchState(ETouchIndex::Type::Touch1, TouchPosition.X, TouchPosition.Y, bIsPressed);
	return TouchPosition;
}

void UCoordinateAxisComponent::PrepareStartDragMovement()
{
	DragMovementInfos.Empty();

	if (CoordinateFocusAxis & EAxisType::EAT_Screen)
	{
		// Location 选中中心点
		DragMovementInfos.Add(GetStartDragMovement(0));
		DragMovementInfos.Add(GetStartDragMovement(1));
		DragMovementInfos.Add(GetStartDragMovement(2));
	}
	else
	{
		if (CoordinateFocusAxis & EAxisType::EAT_X)
			DragMovementInfos.Add(GetStartDragMovement(0));
		if (CoordinateFocusAxis & EAxisType::EAT_Y)
			DragMovementInfos.Add(GetStartDragMovement(1));
		if (CoordinateFocusAxis & EAxisType::EAT_Z)
			DragMovementInfos.Add(GetStartDragMovement(2));
	}
}

TArray<FVector> UCoordinateAxisComponent::GetCoorAxisDirs()
{
	TArray<FVector> CoorAxisDirs;
	CoorAxisDirs.SetNum(3);

	CoorAxisDirs[0] = FVector(1.0f, 0, 0);
	CoorAxisDirs[1] = FVector(0, 1.0f, 0);
	CoorAxisDirs[2] = FVector(0, 0, 1.0f);

	return CoorAxisDirs;
}

FDragMovementInfo UCoordinateAxisComponent::GetStartDragMovement(int32 InCoordinateIndex)
{
	FDragMovementInfo OutInfo;
	OutInfo.CoordinateIndex = InCoordinateIndex;

	FVector FaceNormal;
	FaceNormal = -UGameplayStatics::GetPlayerCameraManager(Controller, 0)->GetCameraRotation().Vector().GetUnsafeNormal();

	int32 MaxIndex = 0;
	int32 CoorAxisIndex = 0;
	float MaxCosineValue = 0.0f;
	TArray<FVector> CandidatePlaneNormal;
	TArray<FVector> CoorAxisDirs = GetCoorAxisDirs();

	for (int32 i = 0; i < 3; ++i)
	{
		if (i != InCoordinateIndex)
		{
			CandidatePlaneNormal.Push(CoorAxisDirs[i]);
		}
	}

	for (CoorAxisIndex = 0; CoorAxisIndex < CandidatePlaneNormal.Num(); ++CoorAxisIndex)
	{
		float CosineValue = FaceNormal | CandidatePlaneNormal[CoorAxisIndex];
		CosineValue = FMath::Abs(CosineValue);
		if (CosineValue > MaxCosineValue)
		{
			MaxCosineValue = CosineValue;
			MaxIndex = CoorAxisIndex;
		}
	}
	FVector StartDragLocation = GetComponentLocation();
	OutInfo.MovementPlane = FPlane(StartDragLocation, CandidatePlaneNormal[MaxIndex]);
	OutInfo.MovementDirVector = CoorAxisDirs[InCoordinateIndex];

	FVector WorldOrigin;
	FVector WorldDirection;
	if (UGameplayStatics::DeprojectScreenToWorld(Controller, CurrentPosition, WorldOrigin, WorldDirection))
	{
		OutInfo.DragStartPlanePosition = FMath::LinePlaneIntersection(WorldOrigin - WorldDirection * HitDistance, WorldOrigin + WorldDirection * HitDistance, OutInfo.MovementPlane);
	}
	return OutInfo;
}

FVector UCoordinateAxisComponent::GetDragLocation(const FDragMovementInfo& Info)
{
	FVector2D DeltaMousePosition = CurrentPosition - LastMousePos;

	if (DeltaMousePosition.Size() > 0.1)
	{
		FVector WorldOrigin;
		FVector WorldDirection;
		FVector HitPlanePosition;
		float HitResultTraceDistance = 100000000.f;

		if (UGameplayStatics::DeprojectScreenToWorld(Controller, CurrentPosition, WorldOrigin, WorldDirection))
		{
			HitPlanePosition = FMath::LinePlaneIntersection(WorldOrigin - WorldDirection * HitResultTraceDistance, WorldOrigin + WorldDirection * HitResultTraceDistance, Info.MovementPlane);

			FVector PlaneDeltaPosition = (HitPlanePosition - Info.DragStartPlanePosition);
			FVector MoveDeltaFromStartDragLocation = Info.MovementDirVector * (PlaneDeltaPosition | Info.MovementDirVector);
			// FVector FinalLocation = (GetComponentLocation() + MoveDeltaFromStartDragLocation);
			return MoveDeltaFromStartDragLocation;
		}
	}
	return FVector(0);
}

void UCoordinateAxisComponent::BuildTranslationAxisVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices, float CylinderRadius /*= 1.2f*/)
{
	// Default Value
	const float AxisLength = 35.0f;
	const float HalfHeight = AxisLength * 0.5f;
	const FVector Offset(0, 0, HalfHeight);
	const float ConeHeadOffset = 12.0f;

	// Build Verts
	BuildCylinderVerts(Offset, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), CylinderRadius, HalfHeight, 16, OutVerts, OutIndices);

	BuildConeVerts(FMath::DegreesToRadians(PI * 5), FMath::DegreesToRadians(PI * 5), 13.f, FVector(0.f, 0.f, AxisLength + ConeHeadOffset), 32, OutVerts, OutIndices);
}


void UCoordinateAxisComponent::BuildCylinderVerts(const FVector& Base, const FVector& XAxis, const FVector& YAxis, const FVector& ZAxis, float Radius, float HalfHeight, uint32 Sides, TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices)
{
	const float	AngleDelta = 2.0f * PI / Sides;
	FVector	LastVertex = Base + XAxis * Radius;

	FVector2D TC = FVector2D(0.0f, 0.0f);
	float TCStep = 1.0f / Sides;

	FVector TopOffset = HalfHeight * ZAxis;

	int32 BaseVertIndex = OutVerts.Num();

	//Compute vertices for base circle.
	for (uint32 SideIndex = 0; SideIndex < Sides; SideIndex++)
	{
		const FVector Vertex = Base + (XAxis * FMath::Cos(AngleDelta * (SideIndex + 1)) + YAxis * FMath::Sin(AngleDelta * (SideIndex + 1))) * Radius;
		FVector Normal = Vertex - Base;
		Normal.Normalize();

		FDynamicMeshVertex MeshVertex;

		MeshVertex.Position = Vertex - TopOffset;
		MeshVertex.TextureCoordinate[0] = TC;

		MeshVertex.SetTangents(
			-ZAxis,
			(-ZAxis) ^ Normal,
			Normal
		);

		OutVerts.Add(MeshVertex); //Add bottom vertex

		LastVertex = Vertex;
		TC.X += TCStep;
	}

	LastVertex = Base + XAxis * Radius;
	TC = FVector2D(0.0f, 1.0f);

	//Compute vertices for the top circle
	for (uint32 SideIndex = 0; SideIndex < Sides; SideIndex++)
	{
		const FVector Vertex = Base + (XAxis * FMath::Cos(AngleDelta * (SideIndex + 1)) + YAxis * FMath::Sin(AngleDelta * (SideIndex + 1))) * Radius;
		FVector Normal = Vertex - Base;
		Normal.Normalize();

		FDynamicMeshVertex MeshVertex;

		MeshVertex.Position = Vertex + TopOffset;
		MeshVertex.TextureCoordinate[0] = TC;

		MeshVertex.SetTangents(
			-ZAxis,
			(-ZAxis) ^ Normal,
			Normal
		);

		OutVerts.Add(MeshVertex); //Add top vertex

		LastVertex = Vertex;
		TC.X += TCStep;
	}

	//Add top/bottom triangles, in the style of a fan.
	//Note if we wanted nice rendering of the caps then we need to duplicate the vertices and modify
	//texture/tangent coordinates.
	for (uint32 SideIndex = 1; SideIndex < Sides; SideIndex++)
	{
		int32 V0 = BaseVertIndex;
		int32 V1 = BaseVertIndex + SideIndex;
		int32 V2 = BaseVertIndex + ((SideIndex + 1) % Sides);

		FTriIndices TriIndices;

		//bottom
		OutIndices.Add(V0);
		OutIndices.Add(V1);
		OutIndices.Add(V2);

		TriIndices.v0 = V0;
		TriIndices.v1 = V1;
		TriIndices.v2 = V2;

		// top
		OutIndices.Add(Sides + V2);
		OutIndices.Add(Sides + V1);
		OutIndices.Add(Sides + V0);

		TriIndices.v0 = Sides + V2;
		TriIndices.v1 = Sides + V1;
		TriIndices.v2 = Sides + V0;
	}

	//Add sides.

	for (uint32 SideIndex = 0; SideIndex < Sides; SideIndex++)
	{
		int32 V0 = BaseVertIndex + SideIndex;
		int32 V1 = BaseVertIndex + ((SideIndex + 1) % Sides);
		int32 V2 = V0 + Sides;
		int32 V3 = V1 + Sides;

		FTriIndices TriIndices;

		OutIndices.Add(V0);
		OutIndices.Add(V2);
		OutIndices.Add(V1);

		TriIndices.v0 = V0;
		TriIndices.v1 = V2;
		TriIndices.v2 = V1;

		OutIndices.Add(V2);
		OutIndices.Add(V3);
		OutIndices.Add(V1);

		TriIndices.v0 = V2;
		TriIndices.v1 = V3;
		TriIndices.v2 = V1;
	}
}

void UCoordinateAxisComponent::BuildConeVerts(float Angle1, float Angle2, float Scale, FVector Offset, uint32 NumSides, TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices)
{
	TArray<FVector> ConeVerts;
	ConeVerts.AddUninitialized(NumSides);

	for (uint32 i = 0; i < NumSides; i++)
	{
		float Fraction = (float)i / (float)(NumSides);
		float Azi = 2.f * PI * Fraction;
		ConeVerts[i] = (FTranslationMatrix(CalcConeVert(Angle1, Angle2, Azi)) * FRotationMatrix(FRotator(-90.f, 0.f, 0.f))).GetOrigin() * Scale + Offset;
	}

	for (uint32 i = 0; i < NumSides; i++)
	{
		// Normal of the current face 
		FVector TriTangentZ = ConeVerts[(i + 1) % NumSides] ^ ConeVerts[i]; // aka triangle normal
		FVector TriTangentY = ConeVerts[i];
		FVector TriTangentX = TriTangentZ ^ TriTangentY;


		FDynamicMeshVertex V0, V1, V2;

		V0.Position = Offset;
		V0.TextureCoordinate[0].X = 0.0f;
		V0.TextureCoordinate[0].Y = (float)i / NumSides;
		V0.SetTangents(TriTangentX, TriTangentY, FVector(-1, 0, 0));
		int32 I0 = OutVerts.Add(V0);

		V1.Position = ConeVerts[i];
		V1.TextureCoordinate[0].X = 1.0f;
		V1.TextureCoordinate[0].Y = (float)i / NumSides;
		FVector TriTangentZPrev = ConeVerts[i] ^ ConeVerts[i == 0 ? NumSides - 1 : i - 1]; // Normal of the previous face connected to this face
		V1.SetTangents(TriTangentX, TriTangentY, (TriTangentZPrev + TriTangentZ).GetSafeNormal());
		int32 I1 = OutVerts.Add(V1);

		V2.Position = ConeVerts[(i + 1) % NumSides];
		V2.TextureCoordinate[0].X = 1.0f;
		V2.TextureCoordinate[0].Y = (float)((i + 1) % NumSides) / NumSides;
		FVector TriTangentZNext = ConeVerts[(i + 2) % NumSides] ^ ConeVerts[(i + 1) % NumSides]; // Normal of the next face connected to this face
		V2.SetTangents(TriTangentX, TriTangentY, (TriTangentZNext + TriTangentZ).GetSafeNormal());
		int32 I2 = OutVerts.Add(V2);

		// Flip winding for negative scale
		if (Scale >= 0.f)
		{
			OutIndices.Add(I0);
			OutIndices.Add(I1);
			OutIndices.Add(I2);
		}
		else
		{
			OutIndices.Add(I0);
			OutIndices.Add(I2);
			OutIndices.Add(I1);
		}
	}
}

FVector UCoordinateAxisComponent::CalcConeVert(float Angle1, float Angle2, float AzimuthAngle)
{
	float ang1 = FMath::Clamp<float>(Angle1, 0.01f, (float)PI - 0.01f);
	float ang2 = FMath::Clamp<float>(Angle2, 0.01f, (float)PI - 0.01f);

	float sinX_2 = FMath::Sin(0.5f * ang1);
	float sinY_2 = FMath::Sin(0.5f * ang2);

	float sinSqX_2 = sinX_2 * sinX_2;
	float sinSqY_2 = sinY_2 * sinY_2;

	float tanX_2 = FMath::Tan(0.5f * ang1);
	float tanY_2 = FMath::Tan(0.5f * ang2);


	float phi = FMath::Atan2(FMath::Sin(AzimuthAngle)*sinY_2, FMath::Cos(AzimuthAngle)*sinX_2);
	float sinPhi = FMath::Sin(phi);
	float cosPhi = FMath::Cos(phi);
	float sinSqPhi = sinPhi * sinPhi;
	float cosSqPhi = cosPhi * cosPhi;

	float rSq, r, Sqr, alpha, beta;

	rSq = sinSqX_2 * sinSqY_2 / (sinSqX_2*sinSqPhi + sinSqY_2 * cosSqPhi);
	r = FMath::Sqrt(rSq);
	Sqr = FMath::Sqrt(1 - rSq);
	alpha = r * cosPhi;
	beta = r * sinPhi;

	FVector ConeVert;

	ConeVert.X = (1 - 2 * rSq);
	ConeVert.Y = 2 * Sqr * alpha;
	ConeVert.Z = 2 * Sqr * beta;

	return ConeVert;
}

void UCoordinateAxisComponent::BuildSphereVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices)
{
	// Use a mesh builder to draw the sphere.
	int32 NumSides = 10;
	int32 NumRings = 5;

	// The first/last arc are on top of each other.
	int32 NumVerts = (NumSides + 1) * (NumRings + 1);
	FDynamicMeshVertex* Verts = (FDynamicMeshVertex*)FMemory::Malloc(NumVerts * sizeof(FDynamicMeshVertex));

	// Calculate verts for one arc
	FDynamicMeshVertex* ArcVerts = (FDynamicMeshVertex*)FMemory::Malloc((NumRings + 1) * sizeof(FDynamicMeshVertex));

	for (int32 i = 0; i < NumRings + 1; i++)
	{
		FDynamicMeshVertex* ArcVert = &ArcVerts[i];

		float angle = ((float)i / NumRings) * PI;

		// Note- unit sphere, so position always has mag of one. We can just use it for normal!			
		ArcVert->Position.X = 0.0f;
		ArcVert->Position.Y = FMath::Sin(angle);
		ArcVert->Position.Z = FMath::Cos(angle);

		ArcVert->SetTangents(
			FVector(1, 0, 0),
			FVector(0.0f, -ArcVert->Position.Z, ArcVert->Position.Y),
			ArcVert->Position
		);

		ArcVert->TextureCoordinate[0].X = 0.0f;
		ArcVert->TextureCoordinate[0].Y = ((float)i / NumRings);
	}

	// Then rotate this arc InNumSides+1 times.
	for (int32 s = 0; s < NumSides + 1; s++)
	{
		FRotator ArcRotator(0, 360.f * (float)s / NumSides, 0);
		FRotationMatrix ArcRot(ArcRotator);
		float XTexCoord = ((float)s / NumSides);

		for (int32 v = 0; v < NumRings + 1; v++)
		{
			int32 VIx = (NumRings + 1)*s + v;

			Verts[VIx].Position = ArcRot.TransformPosition(ArcVerts[v].Position);

			Verts[VIx].SetTangents(
				ArcRot.TransformVector(ArcVerts[v].TangentX.ToFVector()),
				ArcRot.TransformVector(ArcVerts[v].GetTangentY()),
				ArcRot.TransformVector(ArcVerts[v].TangentZ.ToFVector())
			);

			Verts[VIx].TextureCoordinate[0].X = XTexCoord;
			Verts[VIx].TextureCoordinate[0].Y = ArcVerts[v].TextureCoordinate[0].Y;
		}
	}

	// Add all of the vertices we generated to the mesh builder.
	for (int32 VertIdx = 0; VertIdx < NumVerts; VertIdx++)
	{
		OutVerts.Add(Verts[VertIdx]);
	}

	// Add all of the triangles we generated to the mesh builder.
	for (int32 s = 0; s < NumSides; s++)
	{
		int32 a0start = (s + 0) * (NumRings + 1);
		int32 a1start = (s + 1) * (NumRings + 1);

		for (int32 r = 0; r < NumRings; r++)
		{
			OutIndices.Add(a0start + r + 0);
			OutIndices.Add(a1start + r + 0);
			OutIndices.Add(a0start + r + 1);

			OutIndices.Add(a1start + r + 0);
			OutIndices.Add(a1start + r + 1);
			OutIndices.Add(a0start + r + 1);
		}
	}

	// Free our local copy of verts and arc verts
	FMemory::Free(Verts);
	FMemory::Free(ArcVerts);
}

FDynamicMeshVertex AddVertex
(
	const FVector& InPosition,
	const FVector2D& InTextureCoordinate,
	const FVector& InTangentX,
	const FVector& InTangentY,
	const FVector& InTangentZ,
	const FColor& InColor
)
{
	FDynamicMeshVertex Vertex;

	Vertex.Position = InPosition;
	Vertex.TextureCoordinate[0] = InTextureCoordinate;
	Vertex.TangentX = InTangentX;
	Vertex.TangentZ = InTangentZ;
	Vertex.TangentZ.Vector.W = GetBasisDeterminantSignByte(InTangentX, InTangentY, InTangentZ);
	Vertex.Color = InColor;

	return Vertex;
}

void UCoordinateAxisComponent::BuildCornerVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices)
{
	float TH = RenderBoxScale * 1.2f;
	float TX = RenderBoxScale * 12.f / 2;
	float TY = RenderBoxScale * 1.2f / 2;
	float TZ = RenderBoxScale * 12.f / 2;

	// Top
	{
		FDynamicMeshVertex Vertex;
		Vertex.Position = FVector(-TX, -TY, +TZ);
		Vertex.TextureCoordinate[0] = FVector2D::ZeroVector;
		Vertex.TangentX = FVector(1, 0, 0);
		Vertex.TangentZ = FVector(0, 0, 1);
		Vertex.TangentZ.Vector.W = GetBasisDeterminantSignByte(FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1));
		Vertex.Color = FColor::White;

		int32 VertexIndices[4];
		VertexIndices[0] = OutVerts.Add(AddVertex(FVector(-TX, -TY, +TZ), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FColor::White));
		VertexIndices[1] = OutVerts.Add(AddVertex(FVector(-TX, +TY, +TZ), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FColor::White));
		VertexIndices[2] = OutVerts.Add(AddVertex(FVector(+TX, +TY, +TZ), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FColor::White));
		VertexIndices[3] = OutVerts.Add(AddVertex(FVector(+TX, -TY, +TZ), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FColor::White));

		OutIndices.Add(VertexIndices[0]);
		OutIndices.Add(VertexIndices[1]);
		OutIndices.Add(VertexIndices[2]);

		OutIndices.Add(VertexIndices[0]);
		OutIndices.Add(VertexIndices[2]);
		OutIndices.Add(VertexIndices[3]);
	}

	//Left
	{
		int32 VertexIndices[4];
		VertexIndices[0] = OutVerts.Add(AddVertex(FVector(-TX, -TY, TZ - TH), FVector2D::ZeroVector, FVector(0, 0, 1), FVector(0, 1, 0), FVector(-1, 0, 0), FColor::White));
		VertexIndices[1] = OutVerts.Add(AddVertex(FVector(-TX, -TY, TZ), FVector2D::ZeroVector, FVector(0, 0, 1), FVector(0, 1, 0), FVector(-1, 0, 0), FColor::White));
		VertexIndices[2] = OutVerts.Add(AddVertex(FVector(-TX, +TY, TZ), FVector2D::ZeroVector, FVector(0, 0, 1), FVector(0, 1, 0), FVector(-1, 0, 0), FColor::White));
		VertexIndices[3] = OutVerts.Add(AddVertex(FVector(-TX, +TY, TZ - TH), FVector2D::ZeroVector, FVector(0, 0, 1), FVector(0, 1, 0), FVector(-1, 0, 0), FColor::White));

		OutIndices.Add(VertexIndices[0]);
		OutIndices.Add(VertexIndices[1]);
		OutIndices.Add(VertexIndices[2]);

		OutIndices.Add(VertexIndices[0]);
		OutIndices.Add(VertexIndices[2]);
		OutIndices.Add(VertexIndices[3]);
	}

	// Front
	{
		int32 VertexIndices[5];
		VertexIndices[0] = OutVerts.Add(AddVertex(FVector(-TX, +TY, TZ - TH), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 0, -1), FVector(0, 1, 0), FColor::White));
		VertexIndices[1] = OutVerts.Add(AddVertex(FVector(-TX, +TY, +TZ), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 0, -1), FVector(0, 1, 0), FColor::White));
		VertexIndices[2] = OutVerts.Add(AddVertex(FVector(+TX - TH, +TY, +TX), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 0, -1), FVector(0, 1, 0), FColor::White));
		VertexIndices[3] = OutVerts.Add(AddVertex(FVector(+TX, +TY, +TZ), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 0, -1), FVector(0, 1, 0), FColor::White));
		VertexIndices[4] = OutVerts.Add(AddVertex(FVector(+TX - TH, +TY, TZ - TH), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 0, -1), FVector(0, 1, 0), FColor::White));

		OutIndices.Add(VertexIndices[0]);
		OutIndices.Add(VertexIndices[1]);
		OutIndices.Add(VertexIndices[2]);

		OutIndices.Add(VertexIndices[0]);
		OutIndices.Add(VertexIndices[2]);
		OutIndices.Add(VertexIndices[4]);

		OutIndices.Add(VertexIndices[4]);
		OutIndices.Add(VertexIndices[2]);
		OutIndices.Add(VertexIndices[3]);
	}

	// Back
	{
		int32 VertexIndices[5];
		VertexIndices[0] = OutVerts.Add(AddVertex(FVector(-TX, -TY, TZ - TH), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 0, 1), FVector(0, -1, 0), FColor::White));
		VertexIndices[1] = OutVerts.Add(AddVertex(FVector(-TX, -TY, +TZ), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 0, 1), FVector(0, -1, 0), FColor::White));
		VertexIndices[2] = OutVerts.Add(AddVertex(FVector(+TX - TH, -TY, +TX), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 0, 1), FVector(0, -1, 0), FColor::White));
		VertexIndices[3] = OutVerts.Add(AddVertex(FVector(+TX, -TY, +TZ), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 0, 1), FVector(0, -1, 0), FColor::White));
		VertexIndices[4] = OutVerts.Add(AddVertex(FVector(+TX - TH, -TY, TZ - TH), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 0, 1), FVector(0, -1, 0), FColor::White));

		OutIndices.Add(VertexIndices[0]);
		OutIndices.Add(VertexIndices[1]);
		OutIndices.Add(VertexIndices[2]);

		OutIndices.Add(VertexIndices[0]);
		OutIndices.Add(VertexIndices[2]);
		OutIndices.Add(VertexIndices[4]);

		OutIndices.Add(VertexIndices[4]);
		OutIndices.Add(VertexIndices[2]);
		OutIndices.Add(VertexIndices[3]);
	}

	// Bottom
	{
		int32 VertexIndices[4];
		VertexIndices[0] = OutVerts.Add(AddVertex(FVector(-TX, -TY, TZ - TH), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 0, -1), FVector(0, 0, 1), FColor::White));
		VertexIndices[1] = OutVerts.Add(AddVertex(FVector(-TX, +TY, TZ - TH), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 0, -1), FVector(0, 0, 1), FColor::White));
		VertexIndices[2] = OutVerts.Add(AddVertex(FVector(+TX - TH, +TY, TZ - TH), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 0, -1), FVector(0, 0, 1), FColor::White));
		VertexIndices[3] = OutVerts.Add(AddVertex(FVector(+TX - TH, -TY, TZ - TH), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 0, -1), FVector(0, 0, 1), FColor::White));

		OutIndices.Add(VertexIndices[0]);
		OutIndices.Add(VertexIndices[1]);
		OutIndices.Add(VertexIndices[2]);

		OutIndices.Add(VertexIndices[0]);
		OutIndices.Add(VertexIndices[2]);
		OutIndices.Add(VertexIndices[3]);
	}
}

void UCoordinateAxisComponent::BuildSquareVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices)
{
	FVector Positions[4] =
	{
		FVector(0, 0, 0),
		FVector(0, 0, 1),
		FVector(1, 0, 1),
		FVector(1, 0, 0)
	};

	int32 VertexIndices[4];
	for (int32 VertexIndex = 0; VertexIndex < 4; VertexIndex++)
	{
		VertexIndices[VertexIndex] = OutVerts.Add(AddVertex((Positions[VertexIndex] * 12.f), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FColor::White));
	}

	OutIndices.Add(VertexIndices[0]);
	OutIndices.Add(VertexIndices[1]);
	OutIndices.Add(VertexIndices[2]);

	OutIndices.Add(VertexIndices[0]);
	OutIndices.Add(VertexIndices[2]);
	OutIndices.Add(VertexIndices[3]);
}

void UCoordinateAxisComponent::BuildScaleAxisVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices)
{
	// Default Value
	const float AxisLength = 35.0f;
	const float HalfHeight = AxisLength * 0.5f;
	const float CylinderRadius = 1.2f;
	const FVector Offset(0, 0, HalfHeight);
	const float ConeHeadOffset = 1.0f;

	// Build Verts
	BuildCylinderVerts(Offset, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), 1.2f, HalfHeight, 16, OutVerts, OutIndices);

	BuildCubeVerts(4.f, FVector(0.f, 0.f, AxisLength + ConeHeadOffset), OutVerts, OutIndices);
}

void UCoordinateAxisComponent::BuildCubeVerts(float Scale, FVector Offset, TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices)
{
	// 
	FVector Positions[4] =
	{
		FVector(-1, -1, +1),
		FVector(-1, +1, +1),
		FVector(+1, +1, +1),
		FVector(+1, -1, +1)
	};
	FVector2D UVs[4] =
	{
		FVector2D(0,0),
		FVector2D(0,1),
		FVector2D(1,1),
		FVector2D(1,0),
	};

	//
	FRotator FaceRotations[6];
	FaceRotations[0] = FRotator(0, 0, 0);
	FaceRotations[1] = FRotator(90.f, 0, 0);
	FaceRotations[2] = FRotator(-90.f, 0, 0);
	FaceRotations[3] = FRotator(0, 0, 90.f);
	FaceRotations[4] = FRotator(0, 0, -90.f);
	FaceRotations[5] = FRotator(180.f, 0, 0);

	for (int32 f = 0; f < 6; f++)
	{
		FMatrix FaceTransform = FRotationMatrix(FaceRotations[f]);

		int32 VertexIndices[4];
		for (int32 VertexIndex = 0; VertexIndex < 4; VertexIndex++)
		{
			VertexIndices[VertexIndex] = OutVerts.Add(AddVertex(
				FaceTransform.TransformPosition(Positions[VertexIndex] * Scale) + Offset,
				UVs[VertexIndex],
				FaceTransform.TransformVector(FVector(1, 0, 0)),
				FaceTransform.TransformVector(FVector(0, 1, 0)),
				FaceTransform.TransformVector(FVector(0, 0, 1)),
				FColor::White
			));
		}

		OutIndices.Add(VertexIndices[0]);
		OutIndices.Add(VertexIndices[1]);
		OutIndices.Add(VertexIndices[2]);

		OutIndices.Add(VertexIndices[0]);
		OutIndices.Add(VertexIndices[2]);
		OutIndices.Add(VertexIndices[3]);
	}
}

void UCoordinateAxisComponent::BuildTriangleVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices)
{
	FVector Positions[3] =
	{
		FVector(0, 0, 0),
		FVector(0, 0, 2),
		FVector(2, 0, 0),
	};

	int32 VertexIndices[3];
	for (int32 VertexIndex = 0; VertexIndex < 3; VertexIndex++)
	{
		VertexIndices[VertexIndex] = OutVerts.Add(AddVertex((Positions[VertexIndex] * 12.f), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FColor::White));
	}

	OutIndices.Add(VertexIndices[0]);
	OutIndices.Add(VertexIndices[1]);
	OutIndices.Add(VertexIndices[2]);
}

void UCoordinateAxisComponent::BuildThickArcVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices, const FVector& Axis0, const FVector& Axis1, const float StartAngle, const float EndAngle, float OuterRadius, float InnerRadius, const FColor& Color)
{
	// Add more sides to the circle if we've been scaled up to keep the circle looking circular
	// An extra side for every 5 extra unreal units seems to produce a nice result
	const int32 CircleSides = 24.f;
	const int32 NumPoints = FMath::TruncToInt(CircleSides * (EndAngle - StartAngle) / (PI / 2)) + 1;

	FColor TriangleColor = Color;
	FColor RingColor = Color;
	RingColor.A = MAX_uint8;

	FVector ZAxis = Axis0 ^ Axis1;

	for (int32 RadiusIndex = 0; RadiusIndex < 2; ++RadiusIndex)
	{
		float Radius = (RadiusIndex == 0) ? OuterRadius : InnerRadius;
		float TCRadius = Radius / (float)OuterRadius;
		//Compute vertices for base circle.
		for (int32 VertexIndex = 0; VertexIndex <= NumPoints; VertexIndex++)
		{
			float Percent = VertexIndex / (float)NumPoints;
			float Angle = FMath::Lerp(StartAngle, EndAngle, Percent);
			float AngleDeg = FRotator::ClampAxis(Angle * 180.f / PI);

			FVector VertexDir = Axis0.RotateAngleAxis(AngleDeg, ZAxis);
			VertexDir.Normalize();

			float TCAngle = Percent * (PI / 2);
			FVector2D TC(TCRadius*FMath::Cos(Angle), TCRadius*FMath::Sin(Angle));

			const FVector VertexPosition = VertexDir * Radius;
			FVector Normal = VertexPosition;
			Normal.Normalize();

			FDynamicMeshVertex MeshVertex;
			MeshVertex.Position = VertexPosition;
			MeshVertex.Color = TriangleColor;
			MeshVertex.TextureCoordinate[0] = TC;

			MeshVertex.SetTangents(
				-ZAxis,
				(-ZAxis) ^ Normal,
				Normal
			);

			OutVerts.Add(MeshVertex);

// 			FVector StartLinePos = (FTranslationMatrix(LastVertex) * Matrix).GetOrigin();
// 			FVector EndLinePos = (FTranslationMatrix(VertexPosition) * Matrix).GetOrigin();
// 			if (VertexIndex != 0)
// 			{
// 				PDI->DrawLine(StartLinePos, EndLinePos, RingColor, SDPG_Foreground);
// 			}
		}
	}

	//
	int32 InnerVertexStartIndex = NumPoints + 1;
	for (int32 VertexIndex = 0; VertexIndex < NumPoints; VertexIndex++)
	{
		OutIndices.Add(VertexIndex);
		OutIndices.Add(VertexIndex + 1);
		OutIndices.Add(InnerVertexStartIndex + VertexIndex);

		OutIndices.Add(VertexIndex + 1);
		OutIndices.Add(InnerVertexStartIndex + VertexIndex + 1);
		OutIndices.Add(InnerVertexStartIndex + VertexIndex);
	}
}

void UCoordinateAxisComponent::BuildStartStopMarkerVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices, const FVector& Axis0, const FVector& Axis1, const float Angle, const FColor& Color)
{
	const float ArrowHeightPercent = 0.8f;
	const float InnerDistance = 48.0f;
	const float OuterDistance = 56.0f;
	const float RingHeight = OuterDistance - InnerDistance;
	const float ArrowHeight = RingHeight * ArrowHeightPercent;
	const float ThirtyDegrees = PI / 6.0f;
	const float HalfArrowidth = ArrowHeight * FMath::Tan(ThirtyDegrees);

	FVector ZAxis = Axis0 ^ Axis1;
	FVector RotatedAxis0 = Axis0.RotateAngleAxis(Angle, ZAxis);
	FVector RotatedAxis1 = Axis1.RotateAngleAxis(Angle, ZAxis);

	FVector Vertices[3];
	Vertices[0] = OuterDistance * RotatedAxis0;
	Vertices[1] = Vertices[0] + (ArrowHeight)* RotatedAxis0 - HalfArrowidth * RotatedAxis1;
	Vertices[2] = Vertices[1] + (2 * HalfArrowidth) * RotatedAxis1;

	//
	for (int32 VertexIndex = 0; VertexIndex < 3; VertexIndex++)
	{
		FDynamicMeshVertex MeshVertex;
		MeshVertex.Position = Vertices[VertexIndex];
		MeshVertex.Color = Color;
		MeshVertex.TextureCoordinate[0] = FVector2D(0.0f, 0.0f);
		MeshVertex.SetTangents(
			RotatedAxis0,
			RotatedAxis1,
			(RotatedAxis0) ^ RotatedAxis1
		);

		OutVerts.Add(MeshVertex);
	}

	OutIndices.Add(0);
	OutIndices.Add(1);
	OutIndices.Add(2);
}

void UCoordinateAxisComponent::BuildSnapMarkerVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices, const FVector& Axis0, const FVector& Axis1, const float WidthPercent, const float PercentSize, const FColor& Color)
{
	const float InnerDistance = 48.f;
	const float OuterDistance = 56.f;
	const float MaxMarkerHeight = OuterDistance - InnerDistance;
	const float MarkerWidth = MaxMarkerHeight * WidthPercent;
	const float MarkerHeight = MaxMarkerHeight * PercentSize;

	FVector Vertices[4];
	Vertices[0] = (OuterDistance) * Axis0 - (MarkerWidth * 0.5f) * Axis1;
	Vertices[1] = Vertices[0] + (MarkerWidth) * Axis1;
	Vertices[2] = (OuterDistance - MarkerHeight) * Axis0 - (MarkerWidth * 0.5f) * Axis1;
	Vertices[3] = Vertices[2] + (MarkerWidth) * Axis1;

	//
	if (WidthPercent > 0.0f)
	{
		//
		for (int32 VertexIndex = 0; VertexIndex < 4; VertexIndex++)
		{
			FDynamicMeshVertex MeshVertex;
			MeshVertex.Position = Vertices[VertexIndex];
			MeshVertex.Color = Color;
			MeshVertex.TextureCoordinate[0] = FVector2D(0.0f, 0.0f);
			MeshVertex.SetTangents(
				Axis0,
				Axis1,
				(Axis0) ^ Axis1
			);
			OutVerts.Add(MeshVertex); 
		}

		OutIndices.Add(0);
		OutIndices.Add(1);
		OutIndices.Add(2);

		OutIndices.Add(1);
		OutIndices.Add(3);
		OutIndices.Add(2);
	}
}

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

	class FComponentLayerData
	{
	public:
		TWeakObjectPtr<UCoordinateAxisComponent> Component;

		TArray<FString> Texts;
		TArray<FVector> Locations;
		TArray<FLinearColor> Colors;
	};

	TMap<UCoordinateAxisComponent*, FComponentLayerData> ComponentMap;
};

void UCoordinateAxisComponent::TickWidgetComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	if (CoorAxisMode == CAM_Mix && CoorAxisType == CART_Rotate)
	{
		if (bAddedToScreen)
			RemoveWidgetFromScreen();
		return;
	}

	ULocalPlayer* TargetPlayer = GetOwnerPlayer();
	APlayerController* PlayerController = TargetPlayer ? TargetPlayer->PlayerController : nullptr;

	if (TargetPlayer && PlayerController && IsVisible() && bIsRender())
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

	FontInfo = FDTSWidgetStyles::GetDefaultFontStyle(12);
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
		TSharedPtr<SWidget > Widget;
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
						if (CoorAxisComponentBase->GetCoorAixsMode() == CAM_Normal && CoorAxisComponentBase->GetCoorAxisType() == CART_Rotate)
						{
							FVector XAxis = FVector(1, 0, 0);							FVector YAxis = FVector(0, 1, 0);							FVector ZAxis = FVector(0, 0, 1);
							bool bMirrorAxisX = ((XAxis | CoorAxisComponentBase->GetDirectionToWidget()) <= 0.0f);							bool bMirrorAxisY = ((YAxis | CoorAxisComponentBase->GetDirectionToWidget()) <= 0.0f);							bool bMirrorAxisZ = ((ZAxis | CoorAxisComponentBase->GetDirectionToWidget()) <= 0.0f);
							Data.SlotLocation[Index].X = bMirrorAxisX ? FMath::Abs(Data.SlotLocation[Index].X) : -FMath::Abs(Data.SlotLocation[Index].X);							Data.SlotLocation[Index].Y = bMirrorAxisY ? FMath::Abs(Data.SlotLocation[Index].Y) : -FMath::Abs(Data.SlotLocation[Index].Y);							Data.SlotLocation[Index].Z = bMirrorAxisZ ? FMath::Abs(Data.SlotLocation[Index].Z) : -FMath::Abs(Data.SlotLocation[Index].Z);						}

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
