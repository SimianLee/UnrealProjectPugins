// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/CoordinateAxis/TranslationComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Interfaces/Interface_CollisionDataProvider.h"

UTranslationComponent::UTranslationComponent()
{
    if(HasAnyFlags(RF_ClassDefaultObject))
        return;
    CoorAxisType = ECoorAxisRenderType::CART_Translation;
}

void UTranslationComponent::Render(const FSceneView* InView, FPrimitiveDrawInterface* InPDI, FMeshElementCollector& InCollector)
{
	Super::Render(InView, InPDI, InCollector);

	RenderTranslation(InView, InPDI, InCollector, (FScaleMatrix(GetComponentScale()) * FRotationMatrix(GetComponentRotation()) * FTranslationMatrix(GetComponentLocation())));

	//
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

void UTranslationComponent::BuildCollision(FTriMeshCollisionData& InCollisionData)
{
	Super::BuildCollision(InCollisionData);

	BuildTranslationCollision((FScaleMatrix(FVector(1)) * FRotationMatrix(FRotator(0)) * FTranslationMatrix(FVector(0))), InCollisionData);
}

void UTranslationComponent::RenderTranslation(const FSceneView* InView, FPrimitiveDrawInterface* InPDI, FMeshElementCollector& InCollector, const FMatrix& InMatrix)
{
	{
		TArray<uint32> MeshIndices;
		TArray<FDynamicMeshVertex> MeshVerts;
		BuildTranslationAxisVerts(MeshVerts, MeshIndices);

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
		TArray<uint32> CornerMeshIndices;
		TArray<FDynamicMeshVertex> CornerMeshVerts;
		BuildCornerVerts(CornerMeshVerts, CornerMeshIndices);

		TArray<uint32> SquareMeshIndices;
		TArray<FDynamicMeshVertex> SquareMeshVerts;
		BuildSquareVerts(SquareMeshVerts, SquareMeshIndices);

		//
		{
			UMaterialInstanceDynamic* XMaterial = ((CoordinateFocusAxis & EAxisType::EAT_XY) == EAxisType::EAT_XY ? FocusAxisMaterial : AxisMaterialX);
			UMaterialInstanceDynamic* YMaterial = ((CoordinateFocusAxis & EAxisType::EAT_XY) == EAxisType::EAT_XY ? FocusAxisMaterial : AxisMaterialY);
			UMaterialInstanceDynamic* PMaterial = ((CoordinateFocusAxis & EAxisType::EAT_XY) == EAxisType::EAT_XY ? FocusPlaneMaterial : PlaneMaterial);

			FDynamicMeshBuilder MeshBuilderOne(InView->GetFeatureLevel());
			MeshBuilderOne.AddVertices(CornerMeshVerts);
			MeshBuilderOne.AddTriangles(CornerMeshIndices);
			MeshBuilderOne.GetMesh
			(
				FScaleMatrix(FVector(-1, 1, 1)) * FRotationMatrix(FRotator(-90, 0, 0)) * FTranslationMatrix(FVector(RenderBoxScale * FVector(7, 0, 7))) * FRotationMatrix(FRotator(0, 0, -90)) * InMatrix,
				XMaterial->GetRenderProxy(),
				DepthPriorityGroup,
				false,
				false,
				0,
				InCollector
			);

			FDynamicMeshBuilder MeshBuilderTwo(InView->GetFeatureLevel());
			MeshBuilderTwo.AddVertices(CornerMeshVerts);
			MeshBuilderTwo.AddTriangles(CornerMeshIndices);
			MeshBuilderTwo.GetMesh
			(
				FTranslationMatrix(FVector(RenderBoxScale * FVector(7, 0, 7))) * FRotationMatrix(FRotator(0, 0, -90)) * InMatrix,
				YMaterial->GetRenderProxy(),
				DepthPriorityGroup,
				false,
				false,
				0,
				InCollector
			);

			FDynamicMeshBuilder MeshBuilderThree(InView->GetFeatureLevel());
			MeshBuilderThree.AddVertices(SquareMeshVerts);
			MeshBuilderThree.AddTriangles(SquareMeshIndices);
			MeshBuilderThree.GetMesh
			(
				FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(0, 0, -90)) * InMatrix,
				PMaterial->GetRenderProxy(),
				DepthPriorityGroup,
				false,
				false,
				0,
				InCollector
			);
		}

		//
		{
			UMaterialInstanceDynamic* XMaterial = ((CoordinateFocusAxis & EAxisType::EAT_XZ) == EAxisType::EAT_XZ ? FocusAxisMaterial : AxisMaterialX);
			UMaterialInstanceDynamic* ZMaterial = ((CoordinateFocusAxis & EAxisType::EAT_XZ) == EAxisType::EAT_XZ ? FocusAxisMaterial : AxisMaterialZ);
			UMaterialInstanceDynamic* PMaterial = ((CoordinateFocusAxis & EAxisType::EAT_XZ) == EAxisType::EAT_XZ ? FocusPlaneMaterial : PlaneMaterial);

			FDynamicMeshBuilder MeshBuilderOne(InView->GetFeatureLevel());
			MeshBuilderOne.AddVertices(CornerMeshVerts);
			MeshBuilderOne.AddTriangles(CornerMeshIndices);
			MeshBuilderOne.GetMesh
			(
				FScaleMatrix(FVector(-1, 1, 1)) * FRotationMatrix(FRotator(-90, 0, 0)) * FTranslationMatrix(FVector(RenderBoxScale * FVector(7, 0, 7))) * FRotationMatrix(FRotator(0, 0, 0)) * InMatrix,
				XMaterial->GetRenderProxy(),
				DepthPriorityGroup,
				false,
				false,
				0,
				InCollector
			);

			FDynamicMeshBuilder MeshBuilderTwo(InView->GetFeatureLevel());
			MeshBuilderTwo.AddVertices(CornerMeshVerts);
			MeshBuilderTwo.AddTriangles(CornerMeshIndices);
			MeshBuilderTwo.GetMesh
			(
				FTranslationMatrix(FVector(RenderBoxScale * FVector(7, 0, 7))) * FRotationMatrix(FRotator(0, 0, 0)) * InMatrix,
				ZMaterial->GetRenderProxy(),
				DepthPriorityGroup,
				false,
				false,
				0,
				InCollector
			);

			FDynamicMeshBuilder MeshBuilderThree(InView->GetFeatureLevel());
			MeshBuilderThree.AddVertices(SquareMeshVerts);
			MeshBuilderThree.AddTriangles(SquareMeshIndices);
			MeshBuilderThree.GetMesh
			(
				FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(0, 0, 0)) * InMatrix,
				PMaterial->GetRenderProxy(),
				DepthPriorityGroup,
				false,
				false,
				0,
				InCollector
			);
		}

		//
		{
			UMaterialInstanceDynamic* YMaterial = ((CoordinateFocusAxis & EAxisType::EAT_YZ) == EAxisType::EAT_YZ ? FocusAxisMaterial : AxisMaterialY);
			UMaterialInstanceDynamic* ZMaterial = ((CoordinateFocusAxis & EAxisType::EAT_YZ) == EAxisType::EAT_YZ ? FocusAxisMaterial : AxisMaterialZ);
			UMaterialInstanceDynamic* PMaterial = ((CoordinateFocusAxis & EAxisType::EAT_YZ) == EAxisType::EAT_YZ ? FocusPlaneMaterial : PlaneMaterial);

			FDynamicMeshBuilder MeshBuilderOne(InView->GetFeatureLevel());
			MeshBuilderOne.AddVertices(CornerMeshVerts);
			MeshBuilderOne.AddTriangles(CornerMeshIndices);
			MeshBuilderOne.GetMesh
			(
				FScaleMatrix(FVector(-1, 1, 1)) * FRotationMatrix(FRotator(-90, 0, 0)) * FTranslationMatrix(FVector(RenderBoxScale * FVector(7, 0, 7))) * FRotationMatrix(FRotator(0, -90, 0)) * InMatrix,
				YMaterial->GetRenderProxy(),
				DepthPriorityGroup,
				false,
				false,
				0,
				InCollector
			);

			FDynamicMeshBuilder MeshBuilderTwo(InView->GetFeatureLevel());
			MeshBuilderTwo.AddVertices(CornerMeshVerts);
			MeshBuilderTwo.AddTriangles(CornerMeshIndices);
			MeshBuilderTwo.GetMesh
			(
				FTranslationMatrix(FVector(RenderBoxScale * FVector(7, 0, 7))) * FRotationMatrix(FRotator(0, -90, 0)) * InMatrix,
				ZMaterial->GetRenderProxy(),
				DepthPriorityGroup,
				false,
				false,
				0,
				InCollector
			);

			FDynamicMeshBuilder MeshBuilderThree(InView->GetFeatureLevel());
			MeshBuilderThree.AddVertices(SquareMeshVerts);
			MeshBuilderThree.AddTriangles(SquareMeshIndices);
			MeshBuilderThree.GetMesh
			(
				FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(0, -90, 00)) * InMatrix,
				PMaterial->GetRenderProxy(),
				DepthPriorityGroup,
				false,
				false,
				0,
				InCollector
			);
		}
	}

	{
		TArray<uint32> MeshIndices;
		TArray<FDynamicMeshVertex> MeshVerts;
		BuildSphereVerts(MeshVerts, MeshIndices);

		// Origin
		{
			UMaterialInstanceDynamic* XYZMaterial = (CoordinateFocusAxis & EAxisType::EAT_Screen ? FocusAxisMaterial : AxisOriginMaterial);

			FDynamicMeshBuilder MeshBuilder(InView->GetFeatureLevel());
			MeshBuilder.AddVertices(MeshVerts);
			MeshBuilder.AddTriangles(MeshIndices);
			MeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale * 4.f)) * InMatrix, XYZMaterial->GetRenderProxy(), DepthPriorityGroup, false, false, 0, InCollector);
		}
	}
}

void UTranslationComponent::BuildTranslationCollision(const FMatrix& InMatrix, FTriMeshCollisionData& CollisionData)
{
	{
		TArray<uint32> MeshIndices;
		TArray<FDynamicMeshVertex> MeshVerts;
		BuildTranslationAxisVerts(MeshVerts, MeshIndices, 2);

		// X
		{
			int32 CollisionNum = CollisionData.Vertices.Num();

			for (const FDynamicMeshVertex& Vertex : MeshVerts)
			{
				CollisionData.Vertices.Add((FTranslationMatrix(Vertex.Position) * FScaleMatrix(FVector(RenderBoxScale * 1.2f)) * FRotationMatrix(FRotator(-90.f, 0.f, 0.f)) * InMatrix).GetOrigin());
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
				CollisionData.Vertices.Add((FTranslationMatrix(Vertex.Position) * FScaleMatrix(FVector(RenderBoxScale * 1.2f)) * FRotationMatrix(FRotator(0.f, 0.f, -90.f)) * InMatrix).GetOrigin());
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
				CollisionData.Vertices.Add((FTranslationMatrix(Vertex.Position) * FScaleMatrix(FVector(RenderBoxScale * 1.2f)) * FRotationMatrix(FRotator(0.f, 0.f, 0.f)) * InMatrix).GetOrigin());
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
		TArray<uint32> CornerMeshIndices;
		TArray<FDynamicMeshVertex> CornerMeshVerts;
		BuildCornerVerts(CornerMeshVerts, CornerMeshIndices);

		TArray<uint32> SquareMeshIndices;
		TArray<FDynamicMeshVertex> SquareMeshVerts;
		BuildSquareVerts(SquareMeshVerts, SquareMeshIndices);

		//
		{
			{
				int32 CollisionNum = CollisionData.Vertices.Num();

				for (const FDynamicMeshVertex& Vertex : CornerMeshVerts)
				{
					CollisionData.Vertices.Add((FTranslationMatrix(Vertex.Position) * FScaleMatrix(FVector(-1, 1, 1)) * FRotationMatrix(FRotator(-90, 0, 0)) * FTranslationMatrix(FVector(RenderBoxScale * FVector(7, 0, 7))) * FRotationMatrix(FRotator(0, 0, -90)) * InMatrix).GetOrigin());
				}

				for (int32 i = 0; i < CornerMeshIndices.Num(); i += 3)
				{
					FTriIndices TriIndices;

					TriIndices.v0 = CollisionNum + CornerMeshIndices[i + 0];
					TriIndices.v1 = CollisionNum + CornerMeshIndices[i + 1];
					TriIndices.v2 = CollisionNum + CornerMeshIndices[i + 2];

					CollisionData.Indices.Add(TriIndices);

					// 面数
					RnederDualAxisVertexNum += 1;
				}
			}

			{
				int32 CollisionNum = CollisionData.Vertices.Num();

				for (const FDynamicMeshVertex& Vertex : CornerMeshVerts)
				{
					CollisionData.Vertices.Add((FTranslationMatrix(Vertex.Position) * FTranslationMatrix(FVector(RenderBoxScale * FVector(7, 0, 7))) * FRotationMatrix(FRotator(0, 0, -90)) * InMatrix).GetOrigin());
				}

				for (int32 i = 0; i < CornerMeshIndices.Num(); i += 3)
				{
					FTriIndices TriIndices;

					TriIndices.v0 = CollisionNum + CornerMeshIndices[i + 0];
					TriIndices.v1 = CollisionNum + CornerMeshIndices[i + 1];
					TriIndices.v2 = CollisionNum + CornerMeshIndices[i + 2];

					CollisionData.Indices.Add(TriIndices);

					// 面数
					RnederDualAxisVertexNum += 1;
				}
			}

			{
				int32 CollisionNum = CollisionData.Vertices.Num();

				for (const FDynamicMeshVertex& Vertex : SquareMeshVerts)
				{
					CollisionData.Vertices.Add((FTranslationMatrix(Vertex.Position) * FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(0, 0, -90)) * InMatrix).GetOrigin());
				}

				for (int32 i = 0; i < SquareMeshIndices.Num(); i += 3)
				{
					FTriIndices TriIndices;

					TriIndices.v0 = CollisionNum + SquareMeshIndices[i + 0];
					TriIndices.v1 = CollisionNum + SquareMeshIndices[i + 1];
					TriIndices.v2 = CollisionNum + SquareMeshIndices[i + 2];

					CollisionData.Indices.Add(TriIndices);

					// 面数
					RnederDualAxisVertexNum += 1;
				}
			}
		}

		//
		{
			{
				int32 CollisionNum = CollisionData.Vertices.Num();

				for (const FDynamicMeshVertex& Vertex : CornerMeshVerts)
				{

					CollisionData.Vertices.Add((FTranslationMatrix(Vertex.Position) * FScaleMatrix(FVector(-1, 1, 1)) * FRotationMatrix(FRotator(-90, 0, 0)) * FTranslationMatrix(FVector(RenderBoxScale * FVector(7, 0, 7))) * FRotationMatrix(FRotator(0, 0, 0)) * InMatrix).GetOrigin());
				}

				for (int32 i = 0; i < CornerMeshIndices.Num(); i += 3)
				{
					FTriIndices TriIndices;

					TriIndices.v0 = CollisionNum + CornerMeshIndices[i + 0];
					TriIndices.v1 = CollisionNum + CornerMeshIndices[i + 1];
					TriIndices.v2 = CollisionNum + CornerMeshIndices[i + 2];

					CollisionData.Indices.Add(TriIndices);

					// 面数
					RnederDualAxisVertexNum += 1;
				}
			}

			{
				int32 CollisionNum = CollisionData.Vertices.Num();

				for (const FDynamicMeshVertex& Vertex : CornerMeshVerts)
				{
					CollisionData.Vertices.Add((FTranslationMatrix(Vertex.Position) * FTranslationMatrix(FVector(RenderBoxScale * FVector(7, 0, 7))) * FRotationMatrix(FRotator(0, 0, 0)) * InMatrix).GetOrigin());
				}

				for (int32 i = 0; i < CornerMeshIndices.Num(); i += 3)
				{
					FTriIndices TriIndices;

					TriIndices.v0 = CollisionNum + CornerMeshIndices[i + 0];
					TriIndices.v1 = CollisionNum + CornerMeshIndices[i + 1];
					TriIndices.v2 = CollisionNum + CornerMeshIndices[i + 2];

					CollisionData.Indices.Add(TriIndices);

					// 面数
					RnederDualAxisVertexNum += 1;
				}
			}

			{
				int32 CollisionNum = CollisionData.Vertices.Num();

				for (const FDynamicMeshVertex& Vertex : SquareMeshVerts)
				{
					CollisionData.Vertices.Add((FTranslationMatrix(Vertex.Position) * FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(0, 0, 0)) * InMatrix).GetOrigin());
				}

				for (int32 i = 0; i < SquareMeshIndices.Num(); i += 3)
				{
					FTriIndices TriIndices;

					TriIndices.v0 = CollisionNum + SquareMeshIndices[i + 0];
					TriIndices.v1 = CollisionNum + SquareMeshIndices[i + 1];
					TriIndices.v2 = CollisionNum + SquareMeshIndices[i + 2];

					CollisionData.Indices.Add(TriIndices);

					// 面数
					RnederDualAxisVertexNum += 1;
				}
			}
		}

		//
		{
			{
				int32 CollisionNum = CollisionData.Vertices.Num();

				for (const FDynamicMeshVertex& Vertex : CornerMeshVerts)
				{

					CollisionData.Vertices.Add((FTranslationMatrix(Vertex.Position) * FScaleMatrix(FVector(-1, 1, 1)) * FRotationMatrix(FRotator(-90, 0, 0)) * FTranslationMatrix(FVector(RenderBoxScale * FVector(7, 0, 7))) * FRotationMatrix(FRotator(0, -90, 0)) * InMatrix).GetOrigin());
				}

				for (int32 i = 0; i < CornerMeshIndices.Num(); i += 3)
				{
					FTriIndices TriIndices;

					TriIndices.v0 = CollisionNum + CornerMeshIndices[i + 0];
					TriIndices.v1 = CollisionNum + CornerMeshIndices[i + 1];
					TriIndices.v2 = CollisionNum + CornerMeshIndices[i + 2];

					CollisionData.Indices.Add(TriIndices);

					// 面数
					RnederDualAxisVertexNum += 1;
				}
			}

			{
				int32 CollisionNum = CollisionData.Vertices.Num();

				for (const FDynamicMeshVertex& Vertex : CornerMeshVerts)
				{
					CollisionData.Vertices.Add((FTranslationMatrix(Vertex.Position) * FTranslationMatrix(FVector(RenderBoxScale * FVector(7, 0, 7))) * FRotationMatrix(FRotator(0, -90, 0)) * InMatrix).GetOrigin());
				}

				for (int32 i = 0; i < CornerMeshIndices.Num(); i += 3)
				{
					FTriIndices TriIndices;

					TriIndices.v0 = CollisionNum + CornerMeshIndices[i + 0];
					TriIndices.v1 = CollisionNum + CornerMeshIndices[i + 1];
					TriIndices.v2 = CollisionNum + CornerMeshIndices[i + 2];

					CollisionData.Indices.Add(TriIndices);

					// 面数
					RnederDualAxisVertexNum += 1;
				}
			}

			{
				int32 CollisionNum = CollisionData.Vertices.Num();

				for (const FDynamicMeshVertex& Vertex : SquareMeshVerts)
				{
					CollisionData.Vertices.Add((FTranslationMatrix(Vertex.Position) * FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(0, -90, 0)) * InMatrix).GetOrigin());
				}

				for (int32 i = 0; i < SquareMeshIndices.Num(); i += 3)
				{
					FTriIndices TriIndices;

					TriIndices.v0 = CollisionNum + SquareMeshIndices[i + 0];
					TriIndices.v1 = CollisionNum + SquareMeshIndices[i + 1];
					TriIndices.v2 = CollisionNum + SquareMeshIndices[i + 2];

					CollisionData.Indices.Add(TriIndices);

					// 面数
					RnederDualAxisVertexNum += 1;
				}
			}
		}
	}

	{
		TArray<uint32> MeshIndices;
		TArray<FDynamicMeshVertex> MeshVerts;
		BuildSphereVerts(MeshVerts, MeshIndices);

		// Origin
		{
			int32 CollisionNum = CollisionData.Vertices.Num();

			for (const FDynamicMeshVertex& Vertex : MeshVerts)
			{
				CollisionData.Vertices.Add((FTranslationMatrix(Vertex.Position) * FScaleMatrix(FVector(RenderBoxScale * 4.f)) * InMatrix).GetOrigin());
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
}

void UTranslationComponent::CacluMouseDragOffset()
{
	if (!bIsDragging())
		return;

	Super::CacluMouseDragOffset();

	GetAttachParent()->AddWorldOffset(OutMovementDrag);
	if (OnTranslationOffsetDelegate.IsBound())
	{
		OnTranslationOffsetDelegate.Execute((OutMovementDrag));
	}
	if (OnTranslationFinalDelegate.IsBound())
	{
		OnTranslationFinalDelegate.Execute((GetComponentLocation()));
	}
}
