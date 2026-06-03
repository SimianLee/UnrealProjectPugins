#include "Components/CoordinateAxis/CoordinateAxisComponent.h"

#define CAMERA_LOCK_DAMPING_FACTOR .1f
#define MAX_CAMERA_MOVEMENT_SPEED 512.0f

void UCoordinateAxisComponent::GetAxisPlaneNormalAndMask(
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

void UCoordinateAxisComponent::GetPlaneNormalAndMask(
	const FVector& InAxis,
	FVector& OutPlaneNormal,
	FVector& NormalToRemove)
{
	OutPlaneNormal = InAxis;
	NormalToRemove = InAxis;
}

FVector UCoordinateAxisComponent::GetTranslationInitialOffset(
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

FVector UCoordinateAxisComponent::GetTranslationDelta(
	const FAbsoluteMovementParams& InParams)
{
	FPlane MovementPlane(InParams.Position, InParams.PlaneNormal);
	FVector ProposedEndofEyeVector =
		InParams.EyePos + (InParams.PixelDir * (InParams.Position - InParams.EyePos).Size());

	//default to not moving
	FVector RequestedPosition = InParams.Position;

	float DotProductWithPlaneNormal = InParams.PixelDir | InParams.PlaneNormal;
	//check to make sure we're not co-planar
	if (FMath::Abs(DotProductWithPlaneNormal) > DELTA)
	{
		//Get closest point on plane
		RequestedPosition = FMath::LinePlaneIntersection(InParams.EyePos, ProposedEndofEyeVector, MovementPlane);
	}

	//drag is a delta position, so just update the different between the previous position and the new position
	FVector DeltaPosition = RequestedPosition - InParams.Position;

	//Retrieve the initial offset, passing in the current requested position and the current position
	FVector InitialOffset = GetTranslationInitialOffset(RequestedPosition, InParams.Position);

	//subtract off the initial offset (where the widget was clicked) to prevent popping
	DeltaPosition -= InitialOffset;

	//remove the component along the normal we want to mute
	float MovementAlongMutedAxis = DeltaPosition | InParams.NormalToRemove;
	FVector OutDrag = DeltaPosition - (InParams.NormalToRemove * MovementAlongMutedAxis);

	if (InParams.bMovementLockedToCamera)
	{
		//DAMPEN ABSOLUTE MOVEMENT when the camera is locked to the object
		OutDrag *= CAMERA_LOCK_DAMPING_FACTOR;
		OutDrag.X = FMath::Clamp(OutDrag.X, -MAX_CAMERA_MOVEMENT_SPEED, MAX_CAMERA_MOVEMENT_SPEED);
		OutDrag.Y = FMath::Clamp(OutDrag.Y, -MAX_CAMERA_MOVEMENT_SPEED, MAX_CAMERA_MOVEMENT_SPEED);
		OutDrag.Z = FMath::Clamp(OutDrag.Z, -MAX_CAMERA_MOVEMENT_SPEED, MAX_CAMERA_MOVEMENT_SPEED);
	}

	//get the distance from the original position to the new proposed position
	FVector DeltaFromStart = InParams.Position + OutDrag - InitialTranslationPosition;

	//Get the vector from the eye to the proposed new position (to make sure it's not behind the camera
	FVector EyeToNewPosition = (InParams.Position + OutDrag) - InParams.EyePos;
	float BehindTheCameraDotProduct = EyeToNewPosition | InParams.CameraDir;

	//Don't let the requested position go behind the camera
	if (BehindTheCameraDotProduct <= 0)
	{
		OutDrag = OutDrag.ZeroVector;
	}
	return OutDrag;
}

void UCoordinateAxisComponent::ConvertMouseToAxis_Translate(
	const FMatrix& InputCoordSystem,
	FAbsoluteMovementParams& InOutParams,
	FVector& OutDrag)
{
	switch (GetFocusAxis())
	{
	case EAxisType::EAT_X:
		GetAxisPlaneNormalAndMask(InputCoordSystem, InOutParams.XAxis, InOutParams.CameraDir, InOutParams.PlaneNormal,
			InOutParams.NormalToRemove);
		break;
	case EAxisType::EAT_Y:
		GetAxisPlaneNormalAndMask(InputCoordSystem, InOutParams.YAxis, InOutParams.CameraDir, InOutParams.PlaneNormal,
			InOutParams.NormalToRemove);
		break;
	case EAxisType::EAT_Z:
		GetAxisPlaneNormalAndMask(InputCoordSystem, InOutParams.ZAxis, InOutParams.CameraDir, InOutParams.PlaneNormal,
			InOutParams.NormalToRemove);
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
		// InOutParams.XAxis = InView->ViewMatrices.GetViewMatrix().GetColumn(0);
		// InOutParams.YAxis = InView->ViewMatrices.GetViewMatrix().GetColumn(1);
		// InOutParams.ZAxis = InView->ViewMatrices.GetViewMatrix().GetColumn(2);
		GetPlaneNormalAndMask(InOutParams.ZAxis, InOutParams.PlaneNormal, InOutParams.NormalToRemove);
		//do not damp the movement in this case, we also want to snap
		InOutParams.bMovementLockedToCamera = false;
		break;
	}

	OutDrag = GetTranslationDelta(InOutParams);
}

void UCoordinateAxisComponent::ConvertMouseToAxis_Translate(
	const FSceneView* InView,
	const FVector2D& InMousePosition,
	FVector& OutDrag)
{
	// Compute a world space ray from the screen space mouse coordinates
	FVector4 ScreenPos = InView->PixelToScreen(InMousePosition.X, InMousePosition.Y, 0);
	const float ScreenX = ScreenPos.X;
	const float ScreenY = ScreenPos.Y;

	const FMatrix InvViewMatrix = InView->ViewMatrices.GetInvViewMatrix();
	const FMatrix InvProjMatrix = InView->ViewMatrices.GetInvProjectionMatrix();

	FAbsoluteMovementParams Params;
	Params.EyePos = InView->ViewMatrices.GetViewOrigin();
	Params.PixelDir = InvViewMatrix.TransformVector(FVector(InvProjMatrix.TransformFVector4(FVector4(ScreenX * GNearClippingPlane, ScreenY * GNearClippingPlane, 0.0f, GNearClippingPlane)))).GetSafeNormal();
	Params.CameraDir = InView->GetViewDirection();
	Params.Position = GetComponentLocation();
	if (Params.EyePos == Params.Position)
		Params.Position += FVector::OneVector;
	//dampen by
	Params.bMovementLockedToCamera = false;
	Params.bPositionSnapping = true;

	FMatrix InputCoordSystem = FQuatRotationMatrix(GetComponentQuat());
	if (!InputCoordSystem.Equals(FMatrix::Identity))
	{
		InputCoordSystem.RemoveScaling();
	}

	Params.XAxis = InputCoordSystem.TransformVector(FVector(1, 0, 0));
	Params.YAxis = InputCoordSystem.TransformVector(FVector(0, 1, 0));
	Params.ZAxis = InputCoordSystem.TransformVector(FVector(0, 0, 1));

	ConvertMouseToAxis_Translate(InputCoordSystem, Params, OutDrag);
}

void UCoordinateAxisComponent::ConvertMouseToAxis_Rotate(
	FVector2D DragDir,
	FVector& InOutDelta,
	FRotator& OutRotation)
{
	if (GetFocusAxis() == EAxisType::EAT_X)
	{
		// Get rotation in widget local space
		FRotator Rotation = FRotator(0, 0, FVector2D::DotProduct(XAxisDir, DragDir));
		// FSnappingUtils::SnapRotatorToGrid(Rotation);

		// Use to calculate the new input delta
		FVector2D EffectiveDelta = XAxisDir * Rotation.Roll;

		// Record delta rotation (used by the widget to render the accumulated delta)
		CurrentDeltaRotation = Rotation.Roll;

		// Adjust the input delta according to how much rotation was actually applied
		InOutDelta = FVector(EffectiveDelta.X, -EffectiveDelta.Y, 0.0f);

		// Need to get the delta rotation in the current coordinate space of the widget
		OutRotation = (CustomCoordSystem.Inverse() * FRotationMatrix(Rotation) * CustomCoordSystem).Rotator();
	}
	else if (GetFocusAxis() == EAxisType::EAT_Y)
	{
		// Get rotation in widget local space
		FRotator Rotation = FRotator(FVector2D::DotProduct(YAxisDir, DragDir), 0, 0);
		// FSnappingUtils::SnapRotatorToGrid(Rotation);

		// Use to calculate the new input delta
		FVector2D EffectiveDelta = YAxisDir * Rotation.Pitch;

		// Record delta rotation (used by the widget to render the accumulated delta)
		CurrentDeltaRotation = Rotation.Pitch;

		// Adjust the input delta according to how much rotation was actually applied
		InOutDelta = FVector(EffectiveDelta.X, -EffectiveDelta.Y, 0.0f);

		// Need to get the delta rotation in the current coordinate space of the widget
		OutRotation = (CustomCoordSystem.Inverse() * FRotationMatrix(Rotation) * CustomCoordSystem).Rotator();
	}
	else if (GetFocusAxis() == EAxisType::EAT_Z)
	{
		// Get rotation in widget local space
		FRotator Rotation = FRotator(0, FVector2D::DotProduct(ZAxisDir, DragDir), 0);
		// FSnappingUtils::SnapRotatorToGrid(Rotation);

		// Use to calculate the new input delta
		FVector2D EffectiveDelta = ZAxisDir * Rotation.Yaw;

		// Record delta rotation (used by the widget to render the accumulated delta)
		CurrentDeltaRotation = Rotation.Yaw;

		// Adjust the input delta according to how much rotation was actually applied
		InOutDelta = FVector(EffectiveDelta.X, -EffectiveDelta.Y, 0.0f);

		// Need to get the delta rotation in the current coordinate space of the widget
		OutRotation = (CustomCoordSystem.Inverse() * FRotationMatrix(Rotation) * CustomCoordSystem).Rotator();
	}
}

void UCoordinateAxisComponent::ConvertMouseToAxis_Scale(
	FVector2D DragDir,
	FVector& InOutDelta,
	FVector& OutScale)
{
	FVector2D AxisDir = FVector2D::ZeroVector;

	if (GetFocusAxis() & EAxisType::EAT_X)
	{
		AxisDir += XAxisDir;
	}

	if (GetFocusAxis() & EAxisType::EAT_Y)
	{
		AxisDir += YAxisDir;
	}

	if (GetFocusAxis() & EAxisType::EAT_Z)
	{
		AxisDir += ZAxisDir;
	}

	AxisDir.Normalize();
	const float ScaleDelta = FVector2D::DotProduct(AxisDir, DragDir);

	OutScale = FVector(
		(GetFocusAxis() & EAxisType::EAT_X) ? ScaleDelta : 0.0f,
		(GetFocusAxis() & EAxisType::EAT_Y) ? ScaleDelta : 0.0f,
		(GetFocusAxis() & EAxisType::EAT_Z) ? ScaleDelta : 0.0f);

	// Convert to effective screen space delta, and replace input delta, adjusted for inverted screen space Y axis
	const float ScaleMax = OutScale.GetMax();
	const float ScaleMin = OutScale.GetMin();
	const float ScaleApplied = (ScaleMax > -ScaleMin) ? ScaleMax : ScaleMin;
	const FVector2D EffectiveDelta = AxisDir * ScaleApplied;
	InOutDelta = FVector(EffectiveDelta.X, -EffectiveDelta.Y, 0.0f);
}

void UCoordinateAxisComponent::ConvertMouseMovementToAxisMovement(
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
	CustomCoordSystem = FQuatRotationMatrix(GetComponentQuat());

	switch (GetCoorAxisType())
	{
	case CART_Translation:
		ConvertMouseToAxis_Translate(InView, InMousePosition, OutDrag);
		break;
	case CART_Rotate:
		ConvertMouseToAxis_Rotate(FVector2D(InOutDelta.X, -InOutDelta.Y), InOutDelta, OutRotation);
		break;
	case CART_Scale:
		ConvertMouseToAxis_Scale(FVector2D(InOutDelta.X, -InOutDelta.Y), InOutDelta, OutScale);
		break;
	}
}