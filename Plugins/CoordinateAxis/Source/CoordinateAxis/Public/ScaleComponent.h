// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/CoordinateAxis/CoordinateAxisComponent.h"
#include "ScaleComponent.generated.h"

/**
 * 
 */
UCLASS()
class AIRCITYPLUGIN_API UScaleComponent : public UCoordinateAxisComponent
{
	GENERATED_BODY()

public:

	DECLARE_DELEGATE_OneParam(FScaleOffectDelegate, const FVector&);

protected:/* Render */

	UScaleComponent();

	void Render(const FSceneView* InView, FPrimitiveDrawInterface* InPDI, FMeshElementCollector& InCollector) override;

	void BuildCollision(FTriMeshCollisionData& InCollisionData) override;

	void RenderScale(const FSceneView* InView, FPrimitiveDrawInterface* InPDI, FMeshElementCollector& InCollector, const FMatrix& InMatrix);

	void BuildScaleCollision(const FMatrix& InMatrix, FTriMeshCollisionData& InCollisionData);

protected:

	TArray<FVector> GetCoorAxisDirs() override;

	void CacluMouseDragOffset() override;

	UPROPERTY()
	float DragScaleCoorAxisRate = 0.1;

public:/* Move */

	FScaleOffectDelegate OnScaleOffectDelegate = FScaleOffectDelegate();
};
