// CoordinateAxisMovement.cpp
// 管理所有坐标轴的拖拽计算逻辑 — Translation / Rotation / Scale
// 仅 .cpp 文件，无对应 .h。函数声明在 CoordinateAxisComponent.h 的 CoordinateAxisMovementFuncs 命名空间中。

#include "CoordinateAxisComponent.h"
#include "SceneView.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"

#define CAMERA_LOCK_DAMPING_FACTOR .1f
#define MAX_CAMERA_MOVEMENT_SPEED 512.0f

namespace
{
	// ============================================================
	// 内部辅助
	// ============================================================

	static void GetAxisPlaneNormalAndMask(
		const FMatrix& InCoordSystem,
		const FVector& InAxis,
		const FVector& InDirToPixel,
		FVector& OutPlaneNormal,
		FVector& NormalToRemove)
	{
		FVector XAxis = InCoordSystem.TransformVector(FVector(1, 0, 0));
		FVector YAxis = InCoordSystem.TransformVector(FVector(0, 1, 0));
		FVector ZAxis = InCoordSystem.TransformVector(FVector(0, 0, 1));

		float XDot = FMath::Abs(InDirToPixel | XAxis);
		float YDot = FMath::Abs(InDirToPixel | YAxis);
		float ZDot = FMath::Abs(InDirToPixel | ZAxis);

		if ((InAxis | XAxis) > .1f)
		{
			OutPlaneNormal = (YDot > ZDot) ? YAxis : ZAxis;
			NormalToRemove = (YDot > ZDot) ? ZAxis : YAxis;
		}
		else if ((InAxis | YAxis) > .1f)
		{
			OutPlaneNormal = (XDot > ZDot) ? XAxis : ZAxis;
			NormalToRemove = (XDot > ZDot) ? ZAxis : XAxis;
		}
		else
		{
			OutPlaneNormal = (XDot > YDot) ? XAxis : YAxis;
			NormalToRemove = (XDot > YDot) ? YAxis : XAxis;
		}
	}

	static void GetPlaneNormalAndMask(
		const FVector& InAxis,
		FVector& OutPlaneNormal,
		FVector& NormalToRemove)
	{
		OutPlaneNormal = InAxis;
		NormalToRemove = InAxis;
	}

	static FVector GetTranslationInitialOffset(
		bool& bTranslationInitialOffsetCached,
		FVector& InitialTranslationOffset,
		FVector& InitialTranslationPosition,
		const FVector& InNewPosition,
		const FVector& InCurrentPosition)
	{
		if (!bTranslationInitialOffsetCached)
		{
			bTranslationInitialOffsetCached = true;
			InitialTranslationOffset = InNewPosition - InCurrentPosition;
			InitialTranslationPosition = InCurrentPosition;
		}
		return InitialTranslationOffset;
	}

	static FVector GetTranslationDelta(
		UCoordinateAxisComponent* InComp,
		const FAbsoluteMovementParams& InParams)
	{
		FPlane MovementPlane(InParams.Position, InParams.PlaneNormal);
		FVector ProposedEndofEyeVector =
			InParams.EyePos + (InParams.PixelDir * (InParams.Position - InParams.EyePos).Size());

		FVector RequestedPosition = InParams.Position;

		float DotProductWithPlaneNormal = InParams.PixelDir | InParams.PlaneNormal;
		if (FMath::Abs(DotProductWithPlaneNormal) > DELTA)
		{
			RequestedPosition = FMath::LinePlaneIntersection(InParams.EyePos, ProposedEndofEyeVector, MovementPlane);
		}

		FVector DeltaPosition = RequestedPosition - InParams.Position;
		FVector InitialOffset = GetTranslationInitialOffset(
			InComp->bTranslationInitialOffsetCached,
			InComp->InitialTranslationOffset,
			InComp->InitialTranslationPosition,
			RequestedPosition, InParams.Position);

		DeltaPosition -= InitialOffset;

		float MovementAlongMutedAxis = DeltaPosition | InParams.NormalToRemove;
		FVector OutDrag = DeltaPosition - (InParams.NormalToRemove * MovementAlongMutedAxis);

		if (InParams.bMovementLockedToCamera)
		{
			OutDrag *= CAMERA_LOCK_DAMPING_FACTOR;
			OutDrag.X = FMath::Clamp(OutDrag.X, -MAX_CAMERA_MOVEMENT_SPEED, MAX_CAMERA_MOVEMENT_SPEED);
			OutDrag.Y = FMath::Clamp(OutDrag.Y, -MAX_CAMERA_MOVEMENT_SPEED, MAX_CAMERA_MOVEMENT_SPEED);
			OutDrag.Z = FMath::Clamp(OutDrag.Z, -MAX_CAMERA_MOVEMENT_SPEED, MAX_CAMERA_MOVEMENT_SPEED);
		}

		FVector EyeToNewPosition = (InParams.Position + OutDrag) - InParams.EyePos;
		float BehindTheCameraDotProduct = EyeToNewPosition | InParams.CameraDir;

		if (BehindTheCameraDotProduct <= 0)
		{
			OutDrag = OutDrag.ZeroVector;
		}
		return OutDrag;
	}

	static void ConvertMouseToAxis_Translate_Internal(
		UCoordinateAxisComponent* InComp,
		const FMatrix& InputCoordSystem,
		FAbsoluteMovementParams& InOutParams,
		FVector& OutDrag)
	{
		switch (InComp->GetFocusAxis())
		{
		case EAxisType::EAT_X:
			GetAxisPlaneNormalAndMask(InputCoordSystem, InOutParams.XAxis, InOutParams.CameraDir, InOutParams.PlaneNormal, InOutParams.NormalToRemove);
			break;
		case EAxisType::EAT_Y:
			GetAxisPlaneNormalAndMask(InputCoordSystem, InOutParams.YAxis, InOutParams.CameraDir, InOutParams.PlaneNormal, InOutParams.NormalToRemove);
			break;
		case EAxisType::EAT_Z:
			GetAxisPlaneNormalAndMask(InputCoordSystem, InOutParams.ZAxis, InOutParams.CameraDir, InOutParams.PlaneNormal, InOutParams.NormalToRemove);
			break;
		case EAxisType::EAT_XY:
			GetPlaneNormalAndMask(InOutParams.ZAxis, InOutParams.PlaneNormal, InOutParams.NormalToRemove);
			break;
		case EAxisType::EAT_XZ:
			GetPlaneNormalAndMask(InOutParams.YAxis, InOutParams.PlaneNormal, InOutParams.NormalToRemove);
			break;
		case EAxisType::EAT_YZ:
			GetPlaneNormalAndMask(InOutParams.XAxis, InOutParams.PlaneNormal, InOutParams.NormalToRemove);
			break;
		case EAxisType::EAT_Screen:
			GetPlaneNormalAndMask(InOutParams.ZAxis, InOutParams.PlaneNormal, InOutParams.NormalToRemove);
			InOutParams.bMovementLockedToCamera = false;
			break;
		}

		OutDrag = GetTranslationDelta(InComp, InOutParams);
	}

	static void ConvertMouseToAxis_Translate(
		UCoordinateAxisComponent* InComp,
		const FSceneView* InView,
		const FVector2D& InMousePosition,
		FVector& OutDrag)
	{
		FVector4 ScreenPos = InView->PixelToScreen(InMousePosition.X, InMousePosition.Y, 0);
		const float ScreenX = ScreenPos.X;
		const float ScreenY = ScreenPos.Y;

		const FMatrix InvViewMatrix = InView->ViewMatrices.GetInvViewMatrix();
		const FMatrix InvProjMatrix = InView->ViewMatrices.GetInvProjectionMatrix();

		FAbsoluteMovementParams Params;
		Params.EyePos = InView->ViewMatrices.GetViewOrigin();
		Params.PixelDir = InvViewMatrix.TransformVector(FVector(InvProjMatrix.TransformFVector4(FVector4(ScreenX * GNearClippingPlane, ScreenY * GNearClippingPlane, 0.0f, GNearClippingPlane)))).GetSafeNormal();
		Params.CameraDir = InView->GetViewDirection();
		Params.Position = InComp->GetComponentLocation();
		if (Params.EyePos == Params.Position)
			Params.Position += FVector::OneVector;
		Params.bMovementLockedToCamera = false;
		Params.bPositionSnapping = true;

		FMatrix InputCoordSystem = FQuatRotationMatrix(InComp->GetComponentQuat());
		if (!InputCoordSystem.Equals(FMatrix::Identity))
		{
			InputCoordSystem.RemoveScaling();
		}

		Params.XAxis = InputCoordSystem.TransformVector(FVector(1, 0, 0));
		Params.YAxis = InputCoordSystem.TransformVector(FVector(0, 1, 0));
		Params.ZAxis = InputCoordSystem.TransformVector(FVector(0, 0, 1));

		ConvertMouseToAxis_Translate_Internal(InComp, InputCoordSystem, Params, OutDrag);
	}

	static void ConvertMouseToAxis_Rotate(
		UCoordinateAxisComponent* InComp,
		FVector2D DragDir,
		FVector& InOutDelta,
		FRotator& OutRotation)
	{
		FVector2D XAxisDir = InComp->ScreenXAxisDir;
		FVector2D YAxisDir = InComp->ScreenYAxisDir;
		FVector2D ZAxisDir = InComp->ScreenZAxisDir;
		FMatrix CustomCoordSystem = InComp->CustomCoordSystem;

		if (InComp->GetFocusAxis() == EAxisType::EAT_X)
		{
			FRotator Rotation = FRotator(0, 0, FVector2D::DotProduct(XAxisDir, DragDir));

			FVector2D EffectiveDelta = XAxisDir * Rotation.Roll;
			InComp->CurrentDeltaRotation = Rotation.Roll;
			InOutDelta = FVector(EffectiveDelta.X, -EffectiveDelta.Y, 0.0f);
			OutRotation = (CustomCoordSystem.Inverse() * FRotationMatrix(Rotation) * CustomCoordSystem).Rotator();
		}
		else if (InComp->GetFocusAxis() == EAxisType::EAT_Y)
		{
			FRotator Rotation = FRotator(FVector2D::DotProduct(YAxisDir, DragDir), 0, 0);

			FVector2D EffectiveDelta = YAxisDir * Rotation.Pitch;
			InComp->CurrentDeltaRotation = Rotation.Pitch;
			InOutDelta = FVector(EffectiveDelta.X, -EffectiveDelta.Y, 0.0f);
			OutRotation = (CustomCoordSystem.Inverse() * FRotationMatrix(Rotation) * CustomCoordSystem).Rotator();
		}
		else if (InComp->GetFocusAxis() == EAxisType::EAT_Z)
		{
			FRotator Rotation = FRotator(0, FVector2D::DotProduct(ZAxisDir, DragDir), 0);

			FVector2D EffectiveDelta = ZAxisDir * Rotation.Yaw;
			InComp->CurrentDeltaRotation = Rotation.Yaw;
			InOutDelta = FVector(EffectiveDelta.X, -EffectiveDelta.Y, 0.0f);
			OutRotation = (CustomCoordSystem.Inverse() * FRotationMatrix(Rotation) * CustomCoordSystem).Rotator();
		}
	}

	static void ConvertMouseToAxis_Scale(
		UCoordinateAxisComponent* InComp,
		FVector2D DragDir,
		FVector& InOutDelta,
		FVector& OutScale)
	{
		FVector2D AxisDir = FVector2D::ZeroVector;

		if (InComp->GetFocusAxis() & EAxisType::EAT_X)
			AxisDir += InComp->ScreenXAxisDir;
		if (InComp->GetFocusAxis() & EAxisType::EAT_Y)
			AxisDir += InComp->ScreenYAxisDir;
		if (InComp->GetFocusAxis() & EAxisType::EAT_Z)
			AxisDir += InComp->ScreenZAxisDir;

		AxisDir.Normalize();
		const float ScaleDelta = FVector2D::DotProduct(AxisDir, DragDir);

		OutScale = FVector(
			(InComp->GetFocusAxis() & EAxisType::EAT_X) ? ScaleDelta : 0.0f,
			(InComp->GetFocusAxis() & EAxisType::EAT_Y) ? ScaleDelta : 0.0f,
			(InComp->GetFocusAxis() & EAxisType::EAT_Z) ? ScaleDelta : 0.0f);

		const float ScaleMax = OutScale.GetMax();
		const float ScaleMin = OutScale.GetMin();
		const float ScaleApplied = (ScaleMax > -ScaleMin) ? ScaleMax : ScaleMin;
		const FVector2D EffectiveDelta = AxisDir * ScaleApplied;
		InOutDelta = FVector(EffectiveDelta.X, -EffectiveDelta.Y, 0.0f);
	}
}

// ============================================================
// 公开接口
// ============================================================

void CoordinateAxisMovementFuncs::ConvertMouseMovementToAxisMovement(
	UCoordinateAxisComponent* InComp,
	const FSceneView* InView,
	const FVector2D& InMousePosition,
	FVector& InOutDelta,
	FVector& OutDrag,
	FRotator& OutRotation,
	FVector& OutScale)
{
	OutDrag = FVector::ZeroVector;
	OutRotation = FRotator::ZeroRotator;
	OutScale = FVector::ZeroVector;
	InComp->CustomCoordSystem = FQuatRotationMatrix(InComp->GetComponentQuat());

	switch (InComp->GetAxisType())
	{
	case CART_Translation:
		ConvertMouseToAxis_Translate(InComp, InView, InMousePosition, OutDrag);
		break;
	case CART_Rotate:
		ConvertMouseToAxis_Rotate(InComp, FVector2D(InOutDelta.X, -InOutDelta.Y), InOutDelta, OutRotation);
		break;
	case CART_Scale:
		ConvertMouseToAxis_Scale(InComp, FVector2D(InOutDelta.X, -InOutDelta.Y), InOutDelta, OutScale);
		break;
	default:
		break;
	}
}

void CoordinateAxisMovementFuncs::PrepareStartDragMovement(
	UCoordinateAxisComponent* InComp,
	const FVector2D& InCurrentPos)
{
	InComp->DragMovementInfos.Empty();

	if (InComp->GetFocusAxis() & EAxisType::EAT_Screen)
	{
		InComp->DragMovementInfos.Add(GetStartDragMovement(InComp, 0, InCurrentPos));
		InComp->DragMovementInfos.Add(GetStartDragMovement(InComp, 1, InCurrentPos));
		InComp->DragMovementInfos.Add(GetStartDragMovement(InComp, 2, InCurrentPos));
	}
	else
	{
		if (InComp->GetFocusAxis() & EAxisType::EAT_X)
			InComp->DragMovementInfos.Add(GetStartDragMovement(InComp, 0, InCurrentPos));
		if (InComp->GetFocusAxis() & EAxisType::EAT_Y)
			InComp->DragMovementInfos.Add(GetStartDragMovement(InComp, 1, InCurrentPos));
		if (InComp->GetFocusAxis() & EAxisType::EAT_Z)
			InComp->DragMovementInfos.Add(GetStartDragMovement(InComp, 2, InCurrentPos));
	}
}

void CoordinateAxisMovementFuncs::CalcTranslationDragDelta(
	UCoordinateAxisComponent* InComp,
	const FVector2D& InCurrentPos,
	FVector& OutMoveDelta)
{
	OutMoveDelta = FVector::ZeroVector;

	for (const FDragMovementInfo& Info : InComp->DragMovementInfos)
	{
		OutMoveDelta += GetDragLocation(InComp, Info, InCurrentPos);
	}
}

void CoordinateAxisMovementFuncs::ResetMovementState(UCoordinateAxisComponent* InComp)
{
	InComp->DragMovementInfos.Empty();
	InComp->bTranslationInitialOffsetCached = false;
	InComp->InitialTranslationPosition = FVector::ZeroVector;
	InComp->InitialTranslationOffset = FVector::ZeroVector;
}

FDragMovementInfo CoordinateAxisMovementFuncs::GetStartDragMovement(
	UCoordinateAxisComponent* InComp,
	int32 InCoordinateIndex,
	const FVector2D& InCurrentPos)
{
	FDragMovementInfo OutInfo;
	OutInfo.CoordinateIndex = InCoordinateIndex;

	APlayerController* Controller = UGameplayStatics::GetPlayerController(InComp, 0);
	if (!Controller) return OutInfo;

	FVector FaceNormal;
	FaceNormal = -UGameplayStatics::GetPlayerCameraManager(Controller, 0)->GetCameraRotation().Vector().GetUnsafeNormal();

	int32 MaxIndex = 0;
	int32 CoorAxisIndex = 0;
	float MaxCosineValue = 0.0f;
	TArray<FVector> CandidatePlaneNormal;
	TArray<FVector> CoorAxisDirs = InComp->GetCoorAxisDirs();

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

	FVector StartDragLocation = InComp->GetComponentLocation();
	OutInfo.MovementPlane = FPlane(StartDragLocation, CandidatePlaneNormal[MaxIndex]);
	OutInfo.MovementDirVector = CoorAxisDirs[InCoordinateIndex];

	const float HitDistance = 100000000.f;
	FVector WorldOrigin;
	FVector WorldDirection;
	if (UGameplayStatics::DeprojectScreenToWorld(Controller, InCurrentPos, WorldOrigin, WorldDirection))
	{
		OutInfo.DragStartPlanePosition = FMath::LinePlaneIntersection(
			WorldOrigin - WorldDirection * HitDistance,
			WorldOrigin + WorldDirection * HitDistance,
			OutInfo.MovementPlane);
	}
	return OutInfo;
}

FVector CoordinateAxisMovementFuncs::GetDragLocation(
	UCoordinateAxisComponent* InComp,
	const FDragMovementInfo& Info,
	const FVector2D& InCurrentPos)
{
	APlayerController* Controller = UGameplayStatics::GetPlayerController(InComp, 0);
	if (!Controller) return FVector(0);

	FVector WorldOrigin;
	FVector WorldDirection;
	FVector HitPlanePosition;
	const float HitResultTraceDistance = 100000000.f;

	if (UGameplayStatics::DeprojectScreenToWorld(Controller, InCurrentPos, WorldOrigin, WorldDirection))
	{
		HitPlanePosition = FMath::LinePlaneIntersection(
			WorldOrigin - WorldDirection * HitResultTraceDistance,
			WorldOrigin + WorldDirection * HitResultTraceDistance,
			Info.MovementPlane);

		FVector PlaneDeltaPosition = (HitPlanePosition - Info.DragStartPlanePosition);
		FVector MoveDeltaFromStartDragLocation = Info.MovementDirVector * (PlaneDeltaPosition | Info.MovementDirVector);
		return MoveDeltaFromStartDragLocation;
	}
	return FVector(0);
}
