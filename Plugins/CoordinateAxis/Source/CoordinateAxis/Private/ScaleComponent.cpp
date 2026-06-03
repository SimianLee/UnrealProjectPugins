// Fill out your copyright notice in the Description page of Project Settings.

#include "ScaleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Interfaces/Interface_CollisionDataProvider.h"

UScaleComponent::UScaleComponent()
{
    if(HasAnyFlags(RF_ClassDefaultObject))
        return;
    CoorAxisType = ECoorAxisRenderType::CART_Scale;
}

void UScaleComponent::Render(const FSceneView* InView, FPrimitiveDrawInterface* InPDI, FMeshElementCollector& InCollector)
{
	Super::Render(InView, InPDI, InCollector);

	RenderScale(InView, InPDI, InCollector, (FScaleMatrix(GetComponentScale()) * FRotationMatrix(GetComponentRotation()) * FTranslationMatrix(GetComponentLocation())));

	FMatrix WidgetMatrix = CustomCoordSystem * FTranslationMatrix(GetComponentLocation());
	FMatrix AxisRotationX = FMatrix::Identity;
	FMatrix AxisRotationY = FMatrix::Identity;
	FMatrix AxisRotationZ = FMatrix::Identity;

	AxisRotationY = FRotationMatrix::MakeFromXZ(FVector(0, 1, 0), FVector(0, 0, 1));
	AxisRotationZ = FRotationMatrix::MakeFromXY(FVector(0, 0, 1), FVector(0, 1, 0));

	FMatrix ArrowToWorldX = AxisRotationX * WidgetMatrix;
	FMatrix ArrowToWorldY = AxisRotationY * WidgetMatrix;
	FMatrix ArrowToWorldZ = AxisRotationZ * WidgetMatrix;

	FScaleMatrix Scale(RenderBoxScale);
	ArrowToWorldX = Scale * ArrowToWorldX;
	ArrowToWorldY = Scale * ArrowToWorldY;
	ArrowToWorldZ = Scale * ArrowToWorldZ;

	FVector2D NewOrigin;
	FVector2D AxisEnd;
	const FVector AxisEndWorldX = ArrowToWorldX.TransformPosition(FVector(64, 0, 0));
	const FVector AxisEndWorldY = ArrowToWorldY.TransformPosition(FVector(64, 0, 0));
	const FVector AxisEndWorldZ = ArrowToWorldZ.TransformPosition(FVector(64, 0, 0));
	const FVector WidgetOrigin = WidgetMatrix.GetOrigin();

	if (InView->ScreenToPixel(InView->WorldToScreen(WidgetOrigin), NewOrigin) &&
		InView->ScreenToPixel(InView->WorldToScreen(AxisEndWorldX), AxisEnd))
	{
		// If both the origin and the axis endpoint are in front of the camera, trivially calculate the viewport space axis direction
		XAxisDir = (AxisEnd - NewOrigin).GetSafeNormal();
	}
	if (InView->ScreenToPixel(InView->WorldToScreen(WidgetOrigin), NewOrigin) &&
		InView->ScreenToPixel(InView->WorldToScreen(AxisEndWorldY), AxisEnd))
	{
		// If both the origin and the axis endpoint are in front of the camera, trivially calculate the viewport space axis direction
		YAxisDir = (AxisEnd - NewOrigin).GetSafeNormal();
	}
	if (InView->ScreenToPixel(InView->WorldToScreen(WidgetOrigin), NewOrigin) &&
		InView->ScreenToPixel(InView->WorldToScreen(AxisEndWorldZ), AxisEnd))
	{
		// If both the origin and the axis endpoint are in front of the camera, trivially calculate the viewport space axis direction
		ZAxisDir = (AxisEnd - NewOrigin).GetSafeNormal();
	}

	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Blue, XAxisDir.ToString());
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Blue, YAxisDir.ToString());
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Blue, ZAxisDir.ToString());

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

void UScaleComponent::BuildCollision(FTriMeshCollisionData& InCollisionData)
{
	Super::BuildCollision(InCollisionData);
	
	BuildScaleCollision((FScaleMatrix(FVector(1)) * FRotationMatrix(FRotator(0)) * FTranslationMatrix(FVector(0))), InCollisionData);
}

void UScaleComponent::RenderScale(const FSceneView* InView, FPrimitiveDrawInterface* InPDI, FMeshElementCollector& InCollector, const FMatrix& InMatrix)
{
	{
		TArray<uint32> MeshIndices;
		TArray<FDynamicMeshVertex> MeshVerts;
		BuildScaleAxisVerts(MeshVerts, MeshIndices);
		
		// X
		{
			UMaterialInstanceDynamic* XMaterial = (CoordinateFocusAxis & EAxisType::EAT_X ? FocusAxisMaterial : AxisMaterialX);

			FDynamicMeshBuilder MeshBuilder(InView->GetFeatureLevel());
			MeshBuilder.AddVertices(MeshVerts);
			MeshBuilder.AddTriangles(MeshIndices);
			MeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(-90.f, 0.f, 0.f)) * InMatrix, XMaterial->GetRenderProxy(), DepthPriorityGroup, false, false, 0, InCollector);
		}

		// Y
		{
			UMaterialInstanceDynamic* YMaterial = (CoordinateFocusAxis & EAxisType::EAT_Y ? FocusAxisMaterial : AxisMaterialY);

			FDynamicMeshBuilder MeshBuilder(InView->GetFeatureLevel());
			MeshBuilder.AddVertices(MeshVerts);
			MeshBuilder.AddTriangles(MeshIndices);
			MeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(0.f, 0.f, -90.f)) * InMatrix, YMaterial->GetRenderProxy(), DepthPriorityGroup, false, false, 0, InCollector);
		}

		// Z
		{
			UMaterialInstanceDynamic* ZMaterial = (CoordinateFocusAxis & EAxisType::EAT_Z ? FocusAxisMaterial : AxisMaterialZ);

			FDynamicMeshBuilder MeshBuilder(InView->GetFeatureLevel());
			MeshBuilder.AddVertices(MeshVerts);
			MeshBuilder.AddTriangles(MeshIndices);
			MeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(0.f, 0.f, 0.f)) * InMatrix, ZMaterial->GetRenderProxy(), DepthPriorityGroup, false, false, 0, InCollector);
		}
	}

	{
		//
		{
			const FLinearColor& XColor = ((CoordinateFocusAxis & EAxisType::EAT_XY) == EAxisType::EAT_XY ? FocusColor : AxisColorX);
			const FLinearColor& YColor = ((CoordinateFocusAxis & EAxisType::EAT_XY) == EAxisType::EAT_XY ? FocusColor : AxisColorY);
			InPDI->DrawLine(InMatrix.TransformPosition(FVector(24, 0, 0) * FVector(RenderBoxScale)), InMatrix.TransformPosition(FVector(12, -12, 0) * FVector(RenderBoxScale)), XColor, SDPG_Foreground, RenderBoxScale);
			InPDI->DrawLine(InMatrix.TransformPosition(FVector(12, -12, 0) * FVector(RenderBoxScale)), InMatrix.TransformPosition(FVector(0, -24, 0) * FVector(RenderBoxScale)), YColor, SDPG_Foreground, RenderBoxScale);
		}

		//
		{
			const FLinearColor& XColor = ((CoordinateFocusAxis & EAxisType::EAT_XZ) == EAxisType::EAT_XZ ? FocusColor : AxisColorX);
			const FLinearColor& ZColor = ((CoordinateFocusAxis & EAxisType::EAT_XZ) == EAxisType::EAT_XZ ? FocusColor : AxisColorZ);
			InPDI->DrawLine(InMatrix.TransformPosition(FVector(24, 0, 0) * FVector(RenderBoxScale)), InMatrix.TransformPosition(FVector(12, 0, 12) * FVector(RenderBoxScale)), XColor, SDPG_Foreground, RenderBoxScale);
			InPDI->DrawLine(InMatrix.TransformPosition(FVector(12, 0, 12) * FVector(RenderBoxScale)), InMatrix.TransformPosition(FVector(0, 0, 24) * FVector(RenderBoxScale)), ZColor, SDPG_Foreground, RenderBoxScale);
		}

		//
		{
			const FLinearColor& YColor = ((CoordinateFocusAxis & EAxisType::EAT_YZ) == EAxisType::EAT_YZ ? FocusColor : AxisColorY);
			const FLinearColor& ZColor = ((CoordinateFocusAxis & EAxisType::EAT_YZ) == EAxisType::EAT_YZ ? FocusColor : AxisColorZ);
			InPDI->DrawLine(InMatrix.TransformPosition(FVector(0, -24, 0) * FVector(RenderBoxScale)), InMatrix.TransformPosition(FVector(0, -12, 12) * FVector(RenderBoxScale)), YColor, SDPG_Foreground, RenderBoxScale);
			InPDI->DrawLine(InMatrix.TransformPosition(FVector(0, -12, 12) * FVector(RenderBoxScale)), InMatrix.TransformPosition(FVector(0, 0, 24) * FVector(RenderBoxScale)), ZColor, SDPG_Foreground, RenderBoxScale);
		}

		/*
		TArray<uint32> MeshIndices;
		TArray<FDynamicMeshVertex> MeshVerts;
		BuildTriangleVerts(MeshVerts, MeshIndices);

		{
			UMaterialInstanceDynamic* PMaterial = ((CoordinateFocusAxis & EAxisType::EAT_XY) == EAxisType::EAT_XY ? FocusPlaneMaterial : PlaneMaterial);

			FDynamicMeshBuilder MeshBuilder(InView->GetFeatureLevel());
			MeshBuilder.AddVertices(MeshVerts);
			MeshBuilder.AddTriangles(MeshIndices);
			MeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(0.f, 0.f, 90.f)) * InMatrix, PMaterial->GetRenderProxy(), DepthPriorityGroup, false, false, 0, InCollector);
		}

		{
			UMaterialInstanceDynamic* PMaterial = ((CoordinateFocusAxis & EAxisType::EAT_XZ) == EAxisType::EAT_XZ ? FocusPlaneMaterial : PlaneMaterial);

			FDynamicMeshBuilder MeshBuilder(InView->GetFeatureLevel());
			MeshBuilder.AddVertices(MeshVerts);
			MeshBuilder.AddTriangles(MeshIndices);
			MeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(0.f, 0.f, 0.f)) * InMatrix, PMaterial->GetRenderProxy(), DepthPriorityGroup, false, false, 0, InCollector);
		}

		{
			UMaterialInstanceDynamic* PMaterial = ((CoordinateFocusAxis & EAxisType::EAT_YZ) == EAxisType::EAT_YZ ? FocusPlaneMaterial : PlaneMaterial);

			FDynamicMeshBuilder MeshBuilder(InView->GetFeatureLevel());
			MeshBuilder.AddVertices(MeshVerts);
			MeshBuilder.AddTriangles(MeshIndices);
			MeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(0.f, 90.f, 0.f)) * InMatrix, PMaterial->GetRenderProxy(), DepthPriorityGroup, false, false, 0, InCollector);
		}
		*/
	}

	{
		TArray<uint32> MeshIndices;
		TArray<FDynamicMeshVertex> MeshVerts;
		BuildCubeVerts(4.f, FVector::ZeroVector, MeshVerts, MeshIndices);

		UMaterialInstanceDynamic* XYZMaterial = (CoordinateFocusAxis & EAxisType::EAT_Screen ? FocusAxisMaterial : AxisOriginMaterial);

		FDynamicMeshBuilder MeshBuilder(InView->GetFeatureLevel());
		MeshBuilder.AddVertices(MeshVerts);
		MeshBuilder.AddTriangles(MeshIndices);
		MeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * InMatrix, XYZMaterial->GetRenderProxy(), DepthPriorityGroup, false, false, 0, InCollector);
	}
}

void UScaleComponent::BuildScaleCollision(const FMatrix& InMatrix, FTriMeshCollisionData& CollisionData)
{
	{
		TArray<uint32> MeshIndices;
		TArray<FDynamicMeshVertex> MeshVerts;
		BuildScaleAxisVerts(MeshVerts, MeshIndices);

		// X
		{
			int32 CollisionNum = CollisionData.Vertices.Num();

			for (const FDynamicMeshVertex& Vertex : MeshVerts)
			{
				CollisionData.Vertices.Add((FTranslationMatrix(Vertex.Position) * FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(-90.f, 0.f, 0.f)) * InMatrix).GetOrigin());
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

		// Y
		{
			int32 CollisionNum = CollisionData.Vertices.Num();

			for (const FDynamicMeshVertex& Vertex : MeshVerts)
			{
				CollisionData.Vertices.Add((FTranslationMatrix(Vertex.Position) * FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(0.f, 0.f, -90.f)) * InMatrix).GetOrigin());
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

		// Z
		{
			int32 CollisionNum = CollisionData.Vertices.Num();

			for (const FDynamicMeshVertex& Vertex : MeshVerts)
			{
				CollisionData.Vertices.Add((FTranslationMatrix(Vertex.Position) * FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(0.f, 0.f, 0.f)) * InMatrix).GetOrigin());
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

	{
		TArray<uint32> MeshIndices;
		TArray<FDynamicMeshVertex> MeshVerts;
		BuildTriangleVerts(MeshVerts, MeshIndices);

		{
			int32 CollisionNum = CollisionData.Vertices.Num();

			for (const FDynamicMeshVertex& Vertex : MeshVerts)
			{
				CollisionData.Vertices.Add((FTranslationMatrix(Vertex.Position) * FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(0.f, 0.f, -90.f)) * InMatrix).GetOrigin());
			}

			for (int32 i = 0; i < MeshIndices.Num(); i += 3)
			{
				FTriIndices TriIndices;

				TriIndices.v0 = CollisionNum + MeshIndices[i + 0];
				TriIndices.v1 = CollisionNum + MeshIndices[i + 1];
				TriIndices.v2 = CollisionNum + MeshIndices[i + 2];

				CollisionData.Indices.Add(TriIndices);

				// 面数
				RnederDualAxisVertexNum += 1;
			}
		}

		{
			int32 CollisionNum = CollisionData.Vertices.Num();

			for (const FDynamicMeshVertex& Vertex : MeshVerts)
			{
				CollisionData.Vertices.Add((FTranslationMatrix(Vertex.Position) * FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(0.f, 0.f, 0.f)) * InMatrix).GetOrigin());
			}

			for (int32 i = 0; i < MeshIndices.Num(); i += 3)
			{
				FTriIndices TriIndices;

				TriIndices.v0 = CollisionNum + MeshIndices[i + 0];
				TriIndices.v1 = CollisionNum + MeshIndices[i + 1];
				TriIndices.v2 = CollisionNum + MeshIndices[i + 2];

				CollisionData.Indices.Add(TriIndices);

				// 面数
				RnederDualAxisVertexNum += 1;
			}
		}

		{
			int32 CollisionNum = CollisionData.Vertices.Num();

			for (const FDynamicMeshVertex& Vertex : MeshVerts)
			{
				CollisionData.Vertices.Add((FTranslationMatrix(Vertex.Position) * FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(0.f, -90.f, 0.f)) * InMatrix).GetOrigin());
			}

			for (int32 i = 0; i < MeshIndices.Num(); i += 3)
			{
				FTriIndices TriIndices;

				TriIndices.v0 = CollisionNum + MeshIndices[i + 0];
				TriIndices.v1 = CollisionNum + MeshIndices[i + 1];
				TriIndices.v2 = CollisionNum + MeshIndices[i + 2];

				CollisionData.Indices.Add(TriIndices);

				// 面数
				RnederDualAxisVertexNum += 1;
			}
		}
	}

	{
		TArray<uint32> MeshIndices;
		TArray<FDynamicMeshVertex> MeshVerts;
		BuildCubeVerts(4.f, FVector::ZeroVector, MeshVerts, MeshIndices);

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
			RenderSceneVertexNum += 1;
		}
	}
}

TArray<FVector> UScaleComponent::GetCoorAxisDirs()
{
	TArray<FVector> CoorAxisDirs;
	CoorAxisDirs.SetNum(3);

	CoorAxisDirs[0] = FRotationMatrix(GetComponentRotation()).GetScaledAxis(EAxis::X);
	CoorAxisDirs[1] = FRotationMatrix(GetComponentRotation()).GetScaledAxis(EAxis::Y);
	CoorAxisDirs[2] = FRotationMatrix(GetComponentRotation()).GetScaledAxis(EAxis::Z);

	return CoorAxisDirs;
}

void UScaleComponent::CacluMouseDragOffset()
{
	if (!bIsDragging())
		return;

	Super::CacluMouseDragOffset();
	InMovementDiff = FVector(CurrentPosition - LastMousePos, 0);
	LastMousePos = CurrentPosition;
	if (OnScaleOffectDelegate.IsBound())
	{
		OnScaleOffectDelegate.Execute((OutMovementScale));
	}
}
