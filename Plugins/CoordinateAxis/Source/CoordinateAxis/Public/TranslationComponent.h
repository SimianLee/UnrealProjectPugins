// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/CoordinateAxis/CoordinateAxisComponent.h"
#include "TranslationComponent.generated.h"

/**
 * 
 */

USTRUCT()
struct FTranslationRenderData
{

	GENERATED_BODY()

public:

	// 轴
	TArray<uint32> AxisMeshIndices;
	TArray<FDynamicMeshVertex> AxisMeshVerts;

	// 中心
	TArray<uint32> OriginMeshIndices;
	TArray<FDynamicMeshVertex> OriginMeshVerts;

	// 拐角边
	TArray<uint32> CornerMeshIndices;
	TArray<FDynamicMeshVertex> CornerMeshVerts;

	// 拐角面
	TArray<uint32> CornerPlaneMeshIndices;
	TArray<FDynamicMeshVertex> CornerPlaneMeshVerts;
};

UCLASS()
class AIRCITYPLUGIN_API UTranslationComponent : public UCoordinateAxisComponent
{
	GENERATED_BODY()

public:

	DECLARE_DELEGATE_OneParam(FTranslationOffsetDelegate, const FVector&);

protected: /* Render */

	UTranslationComponent();

	void Render(const FSceneView* InView, FPrimitiveDrawInterface* InPDI, FMeshElementCollector& InCollector) override;

	void BuildCollision(FTriMeshCollisionData& InCollisionData) override;

	void RenderTranslation(const FSceneView* InView, FPrimitiveDrawInterface* InPDI, FMeshElementCollector& InCollector, const FMatrix& InMatrix);

	void BuildTranslationCollision(const FMatrix& InMatrix, FTriMeshCollisionData& InCollisionData);

protected: /* Move */

	void CacluMouseDragOffset() override;

public:

	FTranslationOffsetDelegate OnTranslationOffsetDelegate = FTranslationOffsetDelegate();

	FTranslationOffsetDelegate OnTranslationFinalDelegate = FTranslationOffsetDelegate();
};
