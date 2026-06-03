// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/CoordinateAxis/RotateComponent.h"
#include "CanvasTypes.h"
#include "RenderUtils.h"
#include "Engine/Canvas.h"
#include "Materials/Material.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Interfaces/Interface_CollisionDataProvider.h"

#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Components/CanvasPanel.h"
#include "SWidget/Common/SBackground.h"
#include "Public/GlobalObjects.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"

static const float INNER_AXIS_CIRCLE_RADIUS = 48.0f;
static const float OUTER_AXIS_CIRCLE_RADIUS = 56.0f;
static const float ROTATION_TEXT_RADIUS = 75.0f;
static const int32 AXIS_CIRCLE_SIDES = 24;

/** Util to find named canvas in transient package, and create if not found */
UCanvas* GetCanvasByName(FName CanvasName)
{
	// Cache to avoid FString/FName conversions/compares
	static TMap<FName, UCanvas*> CanvasMap;
	UCanvas** FoundCanvas = CanvasMap.Find(CanvasName);
	if (!FoundCanvas)
	{
		UCanvas* CanvasObject = FindObject<UCanvas>(GetTransientPackage(), *CanvasName.ToString());
		if (!CanvasObject)
		{
			CanvasObject = NewObject<UCanvas>(GetTransientPackage(), CanvasName);
			CanvasObject->AddToRoot();
		}

		CanvasMap.Add(CanvasName, CanvasObject);
		return CanvasObject;
	}

	return *FoundCanvas;
}

void URotateComponent::StartDragging()
{
	Super::StartDragging();
	CreatePopupString();
	DragStartPos = GetMousePosition();
}

void URotateComponent::StopDragging()
{
	Super::StopDragging();
	RemovePopupString();
	DragStartPos = FVector2D::ZeroVector;
}

void URotateComponent::UpdateDeltaRotation()
{
	TotalDeltaRotation += CurrentDeltaRotation;
	if ((TotalDeltaRotation <= -360.f) || (TotalDeltaRotation >= 360.f))
	{
		TotalDeltaRotation = FRotator::ClampAxis(TotalDeltaRotation);
	}
}

float URotateComponent::GetDeltaRotation() const
{
	return TotalDeltaRotation;
}

bool URotateComponent::IsRotationLocalSpace() const
{
	return false;
}

void URotateComponent::CreatePopupString()
{
	if (!CoordAxisPanel)
	{
		CoordAxisPanel = UGlobalObjects::GetGlobalObject()->CoordAxisPanel;
	}

	if (!RotateWidget.IsValid())
	{
		RotateWidget = NewObject<class URotateWidget>();

		UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(CoordAxisPanel->AddChild(RotateWidget.Get()));
		Slot->SetSize(FVector2D(50.f, 25.f));
	}
}

void URotateComponent::RemovePopupString()
{
	if (CoordAxisPanel && RotateWidget.IsValid())
	{
		CoordAxisPanel->RemoveChild(RotateWidget.Get());

		CoordAxisPanel = nullptr;
		RotateWidget = nullptr;
	}
}

void URotateComponent::UpdatePopupStringPos()
{
	if (HUDString.Len())
	{
		int32 StringPosX = FMath::FloorToInt(HUDInfoPos.X);
		int32 StringPosY = FMath::FloorToInt(HUDInfoPos.Y);

		if (CoordAxisPanel && RotateWidget.IsValid())
		{
			UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(RotateWidget.Get());
			if (CanvasSlot)
			{
				CanvasSlot->SetPosition(FVector2D(StringPosX, StringPosY));
			}
			RotateWidget.Get()->SetRotateValue(HUDString);
		}
	}
}

URotateComponent::URotateComponent()
{
    if(HasAnyFlags(RF_ClassDefaultObject))
        return;
    CoorAxisType = ECoorAxisRenderType::CART_Rotate;
}

void URotateComponent::Render(const FSceneView* InView, FPrimitiveDrawInterface* InPDI, FMeshElementCollector& InCollector)
{
	Super::Render(InView, InPDI, InCollector);

	RenderRotate(InView, InPDI, InCollector, (FScaleMatrix(GetComponentScale()) * FRotationMatrix(GetComponentRotation()) * FTranslationMatrix(GetComponentLocation())));

	FVector2D NewOrigin;
	if (InView->ScreenToPixel(InView->WorldToScreen(GetComponentLocation()), NewOrigin))
	{
		// Only update the viewport-space origin if the position was in front of the camera
		Origin = NewOrigin;
	}

	if (bIsDragging())
	{
		ConvertMouseMovementToAxisMovement(
			InView,
			CurrentPosition,
			InMovementDiff,
			OutMovementDrag,
			OutMovementRotation,
			OutMovementScale
		);
	}
}

void URotateComponent::BuildCollision(FTriMeshCollisionData& InCollisionData)
{
	Super::BuildCollision(InCollisionData);

	BuildRotateCollision((FScaleMatrix(FVector(1)) * FRotationMatrix(FRotator(0)) * FTranslationMatrix(FVector(0))), InCollisionData);
}

void URotateComponent::RenderRotate(const FSceneView* InView, FPrimitiveDrawInterface* InPDI, FMeshElementCollector& InCollector, const FMatrix& InMatrix)
{
	bIsOrthoDrawingFullRing = false;

	FVector XAxis = FVector(1, 0, 0);
	FVector YAxis = FVector(0, 1, 0);
	FVector ZAxis = FVector(0, 0, 1);

	FVector DirectionToWidget = InView->IsPerspectiveProjection() ? (GetComponentLocation() - InView->ViewMatrices.GetViewOrigin()) : -InView->GetViewDirection();
	DirectionToWidget.Normalize();
	// 绘制圆弧
	{
		if (CoordinateRenderAxis & EAxisType::EAT_X)
		{
			RenderRatationArc(EAxisType::EAT_X, InView, InPDI, InCollector, ZAxis, YAxis, DirectionToWidget, AxisColorX.ToFColor(true), InMatrix, XAxisDir);
		}

		if (CoordinateRenderAxis & EAxisType::EAT_Y)
		{
			RenderRatationArc(EAxisType::EAT_Y, InView, InPDI, InCollector, XAxis, ZAxis, DirectionToWidget, AxisColorY.ToFColor(true), InMatrix, YAxisDir);
		}

		if (CoordinateRenderAxis & EAxisType::EAT_Z)
		{
			RenderRatationArc(EAxisType::EAT_Z, InView, InPDI, InCollector, XAxis, YAxis, DirectionToWidget, AxisColorZ.ToFColor(true), InMatrix, ZAxisDir);
		}
	}
}

void URotateComponent::BuildRotateCollision(const FMatrix& InMatrix, FTriMeshCollisionData& CollisionData)
{
	bIsOrthoDrawingFullRing = false;

	FVector XAxis = FVector(1, 0, 0);
	FVector YAxis = FVector(0, 1, 0);
	FVector ZAxis = FVector(0, 0, 1);

	// 绘制圆弧
	{
		if (CoordinateRenderAxis & EAxisType::EAT_X)
		{
			BuildRatationArcCollision(EAxisType::EAT_X, ZAxis, YAxis, DirectionToWidget, CollisionData, InMatrix);
		}

		if (CoordinateRenderAxis & EAxisType::EAT_Y)
		{
			BuildRatationArcCollision(EAxisType::EAT_Y, XAxis, ZAxis, DirectionToWidget, CollisionData, InMatrix);
		}

		if (CoordinateRenderAxis & EAxisType::EAT_Z)
		{
			BuildRatationArcCollision(EAxisType::EAT_Z, XAxis, YAxis, DirectionToWidget, CollisionData, InMatrix);
		}
	}
}

void URotateComponent::RenderRatationArc(EAxisType InAxis, const FSceneView* InView, FPrimitiveDrawInterface* InPDI, FMeshElementCollector& InCollector, const FVector& Axis0, const FVector& Axis1, const FVector& InDirectionToWidget, const FColor& InColor, const FMatrix& InMatrix, FVector2D& OutAxisDir)
{
	bool bIsPerspective = (InView->ViewMatrices.GetProjectionMatrix().M[3][3] < 1.0f);
	bool bIsOrtho = !bIsPerspective;

	//
	bIsOrthoDrawingFullRing |= bIsOrtho && (FMath::Abs(Axis0 | InDirectionToWidget) < KINDA_SMALL_NUMBER) && (FMath::Abs(Axis1 | InDirectionToWidget) < KINDA_SMALL_NUMBER);

	FColor ArcColor = InColor;
	ArcColor.A = LargeOuterAlpha;

	if (bIsDragging() || (bIsOrthoDrawingFullRing))
	{
		if (CoordinateFocusAxis & InAxis)
		{
			float DeltaRotation = GetDeltaRotation();
			float AdjustedDeltaRotation = IsRotationLocalSpace() ? -DeltaRotation : DeltaRotation;
			float AbsRotation = FRotator::ClampAxis(FMath::Abs(DeltaRotation));
			float AngleOfChangeRadians(AbsRotation * PI / 180.f);

			//
			float StartAngle = AdjustedDeltaRotation < 0.0f ? -AngleOfChangeRadians : 0.0f;
			float FilledAngle = AngleOfChangeRadians;

			//
			FVector ZAxis = Axis0 ^ Axis1;

			ArcColor.A = LargeOuterAlpha;
			{
				{
					TArray<uint32> MeshIndices;
					TArray<FDynamicMeshVertex> MeshVerts;
					BuildThickArcVerts(MeshVerts, MeshIndices, Axis0, Axis1, StartAngle, StartAngle + FilledAngle, 48.f, 56.f, ArcColor);

					FDynamicMeshBuilder MeshBuilder(InView->GetFeatureLevel());
					MeshBuilder.AddVertices(MeshVerts);
					MeshBuilder.AddTriangles(MeshIndices);
					MeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * InMatrix, ArcPlaneMaterial->GetRenderProxy(), DepthPriorityGroup, false, false, 0, InCollector);

					for (int32 Index = 0; Index < MeshVerts.Num() - 1; Index++)
					{
						if (Index != ((MeshVerts.Num() * 0.5) - 1))
							InPDI->DrawLine((FTranslationMatrix(MeshVerts[Index].Position) * (FScaleMatrix(FVector(RenderBoxScale)) * InMatrix)).GetOrigin(), (FTranslationMatrix(MeshVerts[Index + 1].Position) * (FScaleMatrix(FVector(RenderBoxScale)) * InMatrix)).GetOrigin(), ArcColor, SDPG_Foreground);
					}
				}

				{
					TArray<uint32> MeshIndices;
					TArray<FDynamicMeshVertex> MeshVerts;
					BuildThickArcVerts(MeshVerts, MeshIndices, Axis0, Axis1, StartAngle, StartAngle + FilledAngle, (CoorAxisMode == CAM_Mix) ? 20.f : 0.f, 48.f, ArcColor);

					FDynamicMeshBuilder MeshBuilder(InView->GetFeatureLevel());
					MeshBuilder.AddVertices(MeshVerts);
					MeshBuilder.AddTriangles(MeshIndices);
					MeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * InMatrix, ArcPlaneMaterial->GetRenderProxy(), DepthPriorityGroup, false, false, 0, InCollector);

					for (int32 Index = 0; Index < MeshVerts.Num() - 1; Index++)
					{
						if (Index != ((MeshVerts.Num() * 0.5) - 1))
							InPDI->DrawLine((FTranslationMatrix(MeshVerts[Index].Position) * (FScaleMatrix(FVector(RenderBoxScale)) * InMatrix)).GetOrigin(), (FTranslationMatrix(MeshVerts[Index + 1].Position) * (FScaleMatrix(FVector(RenderBoxScale)) * InMatrix)).GetOrigin(), ArcColor, SDPG_Foreground);
					}
				}
			}

			ArcColor.A = SmallOuterAlpha;
			{
				{
					TArray<uint32> MeshIndices;
					TArray<FDynamicMeshVertex> MeshVerts;
					BuildThickArcVerts(MeshVerts, MeshIndices, Axis0, Axis1, StartAngle + FilledAngle, StartAngle + 2 * PI, 48.f, 56.f, ArcColor);

					FDynamicMeshBuilder MeshBuilder(InView->GetFeatureLevel());
					MeshBuilder.AddVertices(MeshVerts);
					MeshBuilder.AddTriangles(MeshIndices);
					MeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * InMatrix, ArcPlaneMaterial->GetRenderProxy(), DepthPriorityGroup, false, false, 0, InCollector);

					for (int32 Index = 0; Index < MeshVerts.Num() - 1; Index++)
					{
						if (Index != ((MeshVerts.Num() * 0.5) - 1))
							InPDI->DrawLine((FTranslationMatrix(MeshVerts[Index].Position) * (FScaleMatrix(FVector(RenderBoxScale)) * InMatrix)).GetOrigin(), (FTranslationMatrix(MeshVerts[Index + 1].Position) * (FScaleMatrix(FVector(RenderBoxScale)) * InMatrix)).GetOrigin(), ArcColor, SDPG_Foreground);
					}
				}

				{
					TArray<uint32> MeshIndices;
					TArray<FDynamicMeshVertex> MeshVerts;
					BuildThickArcVerts(MeshVerts, MeshIndices, Axis0, Axis1, StartAngle + FilledAngle, StartAngle + 2 * PI, (CoorAxisMode == CAM_Mix) ? 20.f : 0.f, 48.f, ArcColor);

					FDynamicMeshBuilder MeshBuilder(InView->GetFeatureLevel());
					MeshBuilder.AddVertices(MeshVerts);
					MeshBuilder.AddTriangles(MeshIndices);
					MeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * InMatrix, ArcPlaneMaterial->GetRenderProxy(), DepthPriorityGroup, false, false, 0, InCollector);

					for (int32 Index = 0; Index < MeshVerts.Num() - 1; Index++)
					{
						if (Index != ((MeshVerts.Num() * 0.5) - 1))
							InPDI->DrawLine((FTranslationMatrix(MeshVerts[Index].Position) * (FScaleMatrix(FVector(RenderBoxScale)) * InMatrix)).GetOrigin(), (FTranslationMatrix(MeshVerts[Index + 1].Position) * (FScaleMatrix(FVector(RenderBoxScale)) * InMatrix)).GetOrigin(), ArcColor, SDPG_Foreground);
					}
				}
			}

			ArcColor = (CoordinateFocusAxis & InAxis) ? FocusColor.ToRGBE() : ArcColor;
			
			// Hallow Arrow
			ArcColor.A = 0;
			{
				TArray<uint32> MeshIndices;
				TArray<FDynamicMeshVertex> MeshVerts;
				BuildStartStopMarkerVerts(MeshVerts, MeshIndices, Axis0, Axis1, 0, ArcColor);

				InPDI->DrawLine((FTranslationMatrix(MeshVerts[0].Position) * (FScaleMatrix(FVector(RenderBoxScale)) * InMatrix)).GetOrigin(), (FTranslationMatrix(MeshVerts[1].Position) * (FScaleMatrix(FVector(RenderBoxScale)) * InMatrix)).GetOrigin(), ArcColor, SDPG_Foreground);
				InPDI->DrawLine((FTranslationMatrix(MeshVerts[1].Position) * (FScaleMatrix(FVector(RenderBoxScale)) * InMatrix)).GetOrigin(), (FTranslationMatrix(MeshVerts[2].Position) * (FScaleMatrix(FVector(RenderBoxScale)) * InMatrix)).GetOrigin(), ArcColor, SDPG_Foreground);
				InPDI->DrawLine((FTranslationMatrix(MeshVerts[0].Position) * (FScaleMatrix(FVector(RenderBoxScale)) * InMatrix)).GetOrigin(), (FTranslationMatrix(MeshVerts[2].Position) * (FScaleMatrix(FVector(RenderBoxScale)) * InMatrix)).GetOrigin(), ArcColor, SDPG_Foreground);
			}

			// Filled Arrow
			ArcColor.A = LargeOuterAlpha;
			{
				TArray<uint32> MeshIndices;
				TArray<FDynamicMeshVertex> MeshVerts;
				BuildStartStopMarkerVerts(MeshVerts, MeshIndices, Axis0, Axis1, AdjustedDeltaRotation, ArcColor);

				FDynamicMeshBuilder MeshBuilder(InView->GetFeatureLevel());
				MeshBuilder.AddVertices(MeshVerts);
				MeshBuilder.AddTriangles(MeshIndices);
				MeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * InMatrix, ArcPlaneMaterial->GetRenderProxy(), DepthPriorityGroup, false, false, 0, InCollector);
			}

			ArcColor.A = 255;

			//
			float DeltaAngle = 5.f;
			float TickMarker = 22.5f;
			for (float Angle = 0; Angle < 360.f; Angle += DeltaAngle)
			{
				FVector GridAxis0 = Axis0.RotateAngleAxis(Angle, ZAxis);
				FVector GridAxis1 = Axis1.RotateAngleAxis(Angle, ZAxis);
				float PercentSize = (FMath::Fmod(Angle, TickMarker) == 0) ? .75f : .25f;
				if (FMath::Fmod(Angle, 90.f) != 0)
				{
					TArray<uint32> MeshIndices;
					TArray<FDynamicMeshVertex> MeshVerts;
					BuildSnapMarkerVerts(MeshVerts, MeshIndices, GridAxis0, GridAxis1, 0.1f, PercentSize, ArcColor);

					FDynamicMeshBuilder MeshBuilder(InView->GetFeatureLevel());
					MeshBuilder.AddVertices(MeshVerts);
					MeshBuilder.AddTriangles(MeshIndices);
					MeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * InMatrix, ArcPlaneMaterial->GetRenderProxy(), DepthPriorityGroup, false, false, 0, InCollector);
				}
			}

			//
			FColor AxisColor = InColor;
			Swap(AxisColor.R, AxisColor.G);
			Swap(AxisColor.B, AxisColor.R);
			AxisColor.A = (AdjustedDeltaRotation == 0) ? MAX_uint8 : LargeOuterAlpha;
			{
				TArray<uint32> MeshIndices;
				TArray<FDynamicMeshVertex> MeshVerts;
				BuildSnapMarkerVerts(MeshVerts, MeshIndices, Axis0, Axis1, .25f, 1.0f, AxisColor);

				FDynamicMeshBuilder MeshBuilder(InView->GetFeatureLevel());
				MeshBuilder.AddVertices(MeshVerts);
				MeshBuilder.AddTriangles(MeshIndices);
				MeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * InMatrix, ArcPlaneMaterial->GetRenderProxy(), DepthPriorityGroup, false, false, 0, InCollector);
			}

			AxisColor.A = (AdjustedDeltaRotation == 180.f) ? MAX_uint8 : LargeOuterAlpha;
			{
				TArray<uint32> MeshIndices;
				TArray<FDynamicMeshVertex> MeshVerts;
				BuildSnapMarkerVerts(MeshVerts, MeshIndices, -Axis0, -Axis1, .25f, 1.0f, AxisColor);

				FDynamicMeshBuilder MeshBuilder(InView->GetFeatureLevel());
				MeshBuilder.AddVertices(MeshVerts);
				MeshBuilder.AddTriangles(MeshIndices);
				MeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * InMatrix, ArcPlaneMaterial->GetRenderProxy(), DepthPriorityGroup, false, false, 0, InCollector);
			}

			//
			Swap(AxisColor.R, AxisColor.G);
			Swap(AxisColor.B, AxisColor.R);
			AxisColor.A = (AdjustedDeltaRotation == 90.f) ? MAX_uint8 : LargeOuterAlpha;
			{
				TArray<uint32> MeshIndices;
				TArray<FDynamicMeshVertex> MeshVerts;
				BuildSnapMarkerVerts(MeshVerts, MeshIndices, Axis1, -Axis0, .25f, 1.0f, AxisColor);

				FDynamicMeshBuilder MeshBuilder(InView->GetFeatureLevel());
				MeshBuilder.AddVertices(MeshVerts);
				MeshBuilder.AddTriangles(MeshIndices);
				MeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * InMatrix, ArcPlaneMaterial->GetRenderProxy(), DepthPriorityGroup, false, false, 0, InCollector);
			}

			AxisColor.A = (AdjustedDeltaRotation == 270.f) ? MAX_uint8 : LargeOuterAlpha;
			{
				TArray<uint32> MeshIndices;
				TArray<FDynamicMeshVertex> MeshVerts;
				BuildSnapMarkerVerts(MeshVerts, MeshIndices, -Axis1, Axis0, .25f, 1.0f, AxisColor);

				FDynamicMeshBuilder MeshBuilder(InView->GetFeatureLevel());
				MeshBuilder.AddVertices(MeshVerts);
				MeshBuilder.AddTriangles(MeshIndices);
				MeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * InMatrix, ArcPlaneMaterial->GetRenderProxy(), DepthPriorityGroup, false, false, 0, InCollector);
			}

			// 旋转角度
			float OffsetAngle = IsRotationLocalSpace() ? 0 : AdjustedDeltaRotation;
			CacheRotationHUDText(InView, Axis0.RotateAngleAxis(OffsetAngle, ZAxis), Axis1.RotateAngleAxis(OffsetAngle, ZAxis), DeltaRotation, RenderBoxScale * (InMatrix.GetScaleVector().GetMax()));
		}
	}
	else
	{
		bool bMirrorAxis0 = true, bMirrorAxis1 = true;
		if (CoorAxisMode == CAM_Normal)
		{
			// 根据摄像机视图翻转坐标轴
			bMirrorAxis0 = ((Axis0 | InDirectionToWidget) <= 0.0f);
			bMirrorAxis1 = ((Axis1 | InDirectionToWidget) <= 0.0f);
		}
		if (CoorAxisMode == CAM_Mix)
		{
			bMirrorAxis0 = true;
			bMirrorAxis1 = false;
		}
		FVector RenderAxis0 = bMirrorAxis0 ? Axis0 : -Axis0;
		FVector RenderAxis1 = bMirrorAxis1 ? Axis1 : -Axis1;
		float Direction = (bMirrorAxis0 ^ bMirrorAxis1) ? -1.0f : 1.0f;

		{
			FColor OuterColor = (CoordinateFocusAxis & InAxis) ? FocusColor.ToRGBE() : ArcColor;
			OuterColor.A = InColor.A;

			TArray<uint32> MeshIndices;
			TArray<FDynamicMeshVertex> MeshVerts;
			BuildThickArcVerts(MeshVerts, MeshIndices, RenderAxis0, RenderAxis1, 0.f, PI / 2, 48.f, 56.f, OuterColor);

			FDynamicMeshBuilder MeshBuilder(InView->GetFeatureLevel());
			MeshBuilder.AddVertices(MeshVerts);
			MeshBuilder.AddTriangles(MeshIndices);
			MeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * InMatrix, ArcPlaneMaterial->GetRenderProxy(), DepthPriorityGroup, false, false, 0, InCollector);

			for (int32 Index = 0; Index < MeshVerts.Num() - 1; Index++)
			{
				if (Index != ((MeshVerts.Num() * 0.5) - 1))
					InPDI->DrawLine((FTranslationMatrix(MeshVerts[Index].Position) * (FScaleMatrix(FVector(RenderBoxScale)) * InMatrix)).GetOrigin(), (FTranslationMatrix(MeshVerts[Index + 1].Position) * (FScaleMatrix(FVector(RenderBoxScale)) * InMatrix)).GetOrigin(), ArcColor, SDPG_Foreground);
			}
		}

		{
			FColor InnerColor = (CoordinateFocusAxis & InAxis) ? FocusColor.ToRGBE() : ArcColor;
			InnerColor.A = 0x0f;

			TArray<uint32> MeshIndices;
			TArray<FDynamicMeshVertex> MeshVerts;
			BuildThickArcVerts(MeshVerts, MeshIndices, RenderAxis0, RenderAxis1, 0.f, PI / 2, (CoorAxisMode == CAM_Mix) ? 20.f : 0.f, 48.f, InnerColor);

			FDynamicMeshBuilder MeshBuilder(InView->GetFeatureLevel());
			MeshBuilder.AddVertices(MeshVerts);
			MeshBuilder.AddTriangles(MeshIndices);
			MeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * InMatrix, ArcPlaneMaterial->GetRenderProxy(), DepthPriorityGroup, false, false, 0, InCollector);

			for (int32 Index = 0; Index < MeshVerts.Num() - 1; Index++)
			{
				if (Index != ((MeshVerts.Num() * 0.5) - 1))
					InPDI->DrawLine((FTranslationMatrix(MeshVerts[Index].Position) * (FScaleMatrix(FVector(RenderBoxScale)) * InMatrix)).GetOrigin(), (FTranslationMatrix(MeshVerts[Index + 1].Position) * (FScaleMatrix(FVector(RenderBoxScale)) * InMatrix)).GetOrigin(), ArcColor, SDPG_Foreground);
			}
		}

		FVector2D Axis0ScreenLocation;
		if (!InView->ScreenToPixel(InView->WorldToScreen(GetComponentLocation() + RenderAxis0 * 64.0f), Axis0ScreenLocation))
		{
			Axis0ScreenLocation.X = Axis0ScreenLocation.Y = 0;
		}

		FVector2D Axis1ScreenLocation;
		if (!InView->ScreenToPixel(InView->WorldToScreen(GetComponentLocation() + RenderAxis1 * 64.0f), Axis1ScreenLocation))
		{
			Axis1ScreenLocation.X = Axis1ScreenLocation.Y = 0;
		}

		OutAxisDir = ((Axis1ScreenLocation - Axis0ScreenLocation) * Direction).GetSafeNormal();
	}
}

void URotateComponent::BuildRatationArcCollision(EAxisType InAxis, const FVector& Axis0, const FVector& Axis1, const FVector& InDirectionToWidget, FTriMeshCollisionData& CollisionData, const FMatrix& InMatrix)
{
	bool bIsOrtho = !bIsPerspective;

	if (bIsDragging() )
	{
		float DeltaRotation = GetDeltaRotation();
		float AdjustedDeltaRotation = IsRotationLocalSpace() ? -DeltaRotation : DeltaRotation;
		float AbsRotation = FRotator::ClampAxis(FMath::Abs(DeltaRotation));
		float AngleOfChangeRadians(AbsRotation * PI / 180.f);

		//
		float StartAngle = AdjustedDeltaRotation < 0.0f ? -AngleOfChangeRadians : 0.0f;
		float FilledAngle = AngleOfChangeRadians;

		{
			TArray<uint32> MeshIndices;
			TArray<FDynamicMeshVertex> MeshVerts;
			BuildThickArcVerts(MeshVerts, MeshIndices, Axis0, Axis1, StartAngle, StartAngle + FilledAngle, 48.f, 56.f, FColor::Blue);

			int32 CollisionNum = CollisionData.Vertices.Num();

			for (const FDynamicMeshVertex& Vertex : MeshVerts)
			{
				CollisionData.Vertices.Add((FTranslationMatrix(Vertex.Position) * FScaleMatrix(FVector(RenderBoxScale)) * InMatrix).GetOrigin());
			}

			for (int32 i = 0; i < MeshIndices.Num(); i += 3)
			{
				FTriIndices TriIndices;

				TriIndices.v0 = CollisionNum + MeshIndices[i + 0];
				TriIndices.v1 = CollisionNum + MeshIndices[i + 1];
				TriIndices.v2 = CollisionNum + MeshIndices[i + 2];

				CollisionData.Indices.Add(TriIndices);

				// 面数
				RenderAxisVertexNum += 1;
			}
		}

		{
			TArray<uint32> MeshIndices;
			TArray<FDynamicMeshVertex> MeshVerts;
			BuildThickArcVerts(MeshVerts, MeshIndices, Axis0, Axis1, StartAngle, StartAngle + FilledAngle, (CoorAxisMode == CAM_Mix) ? 20.f : 0.f, 48.f, FColor::Blue);
				
			int32 CollisionNum = CollisionData.Vertices.Num();

			for (const FDynamicMeshVertex& Vertex : MeshVerts)
			{
				CollisionData.Vertices.Add((FTranslationMatrix(Vertex.Position) * FScaleMatrix(FVector(RenderBoxScale)) * InMatrix).GetOrigin());
			}

			for (int32 i = 0; i < MeshIndices.Num(); i += 3)
			{
				FTriIndices TriIndices;

				TriIndices.v0 = CollisionNum + MeshIndices[i + 0];
				TriIndices.v1 = CollisionNum + MeshIndices[i + 1];
				TriIndices.v2 = CollisionNum + MeshIndices[i + 2];

				CollisionData.Indices.Add(TriIndices);

				// 面数
				RenderAxisVertexNum += 1;
			}
		}

		{
			TArray<uint32> MeshIndices;
			TArray<FDynamicMeshVertex> MeshVerts;
			BuildThickArcVerts(MeshVerts, MeshIndices, Axis0, Axis1, StartAngle + FilledAngle, StartAngle + 2 * PI, 48.f, 56.f, FColor::Blue);

			int32 CollisionNum = CollisionData.Vertices.Num();

			for (const FDynamicMeshVertex& Vertex : MeshVerts)
			{
				CollisionData.Vertices.Add((FTranslationMatrix(Vertex.Position) * FScaleMatrix(FVector(RenderBoxScale)) * InMatrix).GetOrigin());
			}

			for (int32 i = 0; i < MeshIndices.Num(); i += 3)
			{
				FTriIndices TriIndices;

				TriIndices.v0 = CollisionNum + MeshIndices[i + 0];
				TriIndices.v1 = CollisionNum + MeshIndices[i + 1];
				TriIndices.v2 = CollisionNum + MeshIndices[i + 2];

				CollisionData.Indices.Add(TriIndices);

				// 面数
				RenderAxisVertexNum += 1;
			}
		}

		{
			TArray<uint32> MeshIndices;
			TArray<FDynamicMeshVertex> MeshVerts;
			BuildThickArcVerts(MeshVerts, MeshIndices, Axis0, Axis1, StartAngle + FilledAngle, StartAngle + 2 * PI, (CoorAxisMode == CAM_Mix) ? 20.f : 0.f, 48.f, FColor::Blue);

			int32 CollisionNum = CollisionData.Vertices.Num();

			for (const FDynamicMeshVertex& Vertex : MeshVerts)
			{
				CollisionData.Vertices.Add((FTranslationMatrix(Vertex.Position) * FScaleMatrix(FVector(RenderBoxScale)) * InMatrix).GetOrigin());
			}

			for (int32 i = 0; i < MeshIndices.Num(); i += 3)
			{
				FTriIndices TriIndices;

				TriIndices.v0 = CollisionNum + MeshIndices[i + 0];
				TriIndices.v1 = CollisionNum + MeshIndices[i + 1];
				TriIndices.v2 = CollisionNum + MeshIndices[i + 2];

				CollisionData.Indices.Add(TriIndices);

				// 面数
				RenderAxisVertexNum += 1;
			}
		}
	}
	else
	{
		bool bMirrorAxis0 = true, bMirrorAxis1 = true;
		if (CoorAxisMode == CAM_Normal)
		{
			// 根据摄像机视图翻转坐标轴
			bMirrorAxis0 = ((Axis0 | InDirectionToWidget) <= 0.0f);
			bMirrorAxis1 = ((Axis1 | InDirectionToWidget) <= 0.0f);
		}
		if (CoorAxisMode == CAM_Mix)
		{
			bMirrorAxis0 = true;
			bMirrorAxis1 = false;
		}
		FVector RenderAxis0 = bMirrorAxis0 ? Axis0 : -Axis0;
		FVector RenderAxis1 = bMirrorAxis1 ? Axis1 : -Axis1;

		{
			TArray<uint32> MeshIndices;
			TArray<FDynamicMeshVertex> MeshVerts;
			BuildThickArcVerts(MeshVerts, MeshIndices, RenderAxis0, RenderAxis1, 0.f, PI / 2, 48.f, 56.f, FColor::Blue);

			int32 CollisionNum = CollisionData.Vertices.Num();

			for (const FDynamicMeshVertex& Vertex : MeshVerts)
			{
				CollisionData.Vertices.Add((FTranslationMatrix(Vertex.Position) * FScaleMatrix(FVector(RenderBoxScale)) * InMatrix).GetOrigin());
			}

			for (int32 i = 0; i < MeshIndices.Num(); i += 3)
			{
				FTriIndices TriIndices;

				TriIndices.v0 = CollisionNum + MeshIndices[i + 0];
				TriIndices.v1 = CollisionNum + MeshIndices[i + 1];
				TriIndices.v2 = CollisionNum + MeshIndices[i + 2];

				CollisionData.Indices.Add(TriIndices);

				// 面数
				RenderAxisVertexNum += 1;
			}
		}

		{
			TArray<uint32> MeshIndices;
			TArray<FDynamicMeshVertex> MeshVerts;
			BuildThickArcVerts(MeshVerts, MeshIndices, RenderAxis0, RenderAxis1, 0.f, PI / 2, (CoorAxisMode == CAM_Mix) ? 20.f : 0.f, 48.f, FColor::Blue);

			int32 CollisionNum = CollisionData.Vertices.Num();

			for (const FDynamicMeshVertex& Vertex : MeshVerts)
			{
				CollisionData.Vertices.Add((FTranslationMatrix(Vertex.Position) * FScaleMatrix(FVector(RenderBoxScale)) * InMatrix).GetOrigin());
			}

			for (int32 i = 0; i < MeshIndices.Num(); i += 3)
			{
				FTriIndices TriIndices;

				TriIndices.v0 = CollisionNum + MeshIndices[i + 0];
				TriIndices.v1 = CollisionNum + MeshIndices[i + 1];
				TriIndices.v2 = CollisionNum + MeshIndices[i + 2];

				CollisionData.Indices.Add(TriIndices);

				// 面数
				RenderAxisVertexNum += 1;
			}
		}

		TotalDeltaRotation = 0.f;
	}
}

void URotateComponent::CacheRotationHUDText(const FSceneView* InView, const FVector& Axis0, const FVector& Axis1, const float AngleOfChange, const float InScale)
{
	const float TextDistance = (ROTATION_TEXT_RADIUS * InScale);

	FVector AxisVectors[4] = { Axis0, Axis1, -Axis0, -Axis1 };

	for (int i = 0; i < 4; ++i)
	{
		FVector PotentialTextPosition = GetComponentLocation() + (TextDistance) * AxisVectors[i];
		if (CoordAxisPanel && InView->ScreenToPixel(InView->WorldToScreen(PotentialTextPosition), HUDInfoPos))
		{
			FGeometry Geometry = CoordAxisPanel->GetCachedGeometry();
			HUDInfoPos = Geometry.AbsoluteToLocal(HUDInfoPos + Geometry.GetAbsolutePosition());

			if (FMath::IsWithin<float>(HUDInfoPos.X, 0, InView->UnscaledViewRect.Width()) && FMath::IsWithin<float>(HUDInfoPos.Y, 0, InView->UnscaledViewRect.Height()))
			{
				//only valid screen locations get a valid string
				HUDString = FString::Printf(TEXT("%3.2f"), AngleOfChange);
				// UpdatePopupStringPos();
				break;
			}
		}
	}
}

void URotateComponent::CacluMouseDragOffset()
{
	if (!bIsDragging())
		return;

	Super::CacluMouseDragOffset();
	InMovementDiff = FVector(CurrentPosition - LastMousePos, 0);
	LastMousePos = CurrentPosition;
	if (OnRotationOffsetDelegate.IsBound())
	{
		OnRotationOffsetDelegate.Execute((OutMovementRotation));
	}
}

void URotateWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	RotateWidget.Reset();
}

TSharedRef<SWidget> URotateWidget::RebuildWidget()
{
	RotateWidget = SNew(SRotateWidget);
	return RotateWidget.ToSharedRef();
}

#if WITH_EDITOR
const FText URotateWidget::GetPaletteCategory()
{
	return NSLOCTEXT("URotateWidget", "StringWidget", "StringWidget");
}
#endif

void URotateWidget::SetRotateValue(const FString& Value)
{
	if (RotateWidget.IsValid())
	{
		RotateWidget->SetRotateValue(Value);
	}
}

void SRotateWidget::Construct(const FArguments& InArgs)
{
	ChildSlot
	.VAlign(VAlign_Fill)
	.HAlign(HAlign_Fill)
	[
		SNew(SBackground)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
        .Visibility(EVisibility::SelfHitTestInvisible)
        .BlurStrength(0.f)
        .PixelUnit(true)
        .BackgroundColor(FColor(0.f, 0.f, 0.f, (int)0.6f))
		[
			SAssignNew(RotateValueText, STextBlock)
			.Text(FText::FromString(TEXT("")))
			.Visibility(EVisibility::SelfHitTestInvisible)
			.Font(FDTSWidgetStyles::GetDefaultFontStyle(10))
		]
	];
}

void SRotateWidget::SetRotateValue(const FString& Value)
{
	if (RotateValueText.IsValid())
	{
		RotateValueText->SetText(FText::FromString(Value));
	}
}
