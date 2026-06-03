// Fill out your copyright notice in the Description page of Project Settings.

#include "Tools/Input/CoordinateAxis.h"
#include "Tools/Input/InputHandler.h"
#include "Public/CoordinateUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Components/InputComponent.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "GameFramework/PlayerController.h"
#include "Components/CoordinateAxis/ScaleComponent.h"
#include "Components/CoordinateAxis/RotateComponent.h"
#include "Components/CoordinateAxis/TranslationComponent.h"
#include "Widget/SWidget/CoordAxis/SCoordAxisWidget.h"
#include "Public/GlobalObjects.h"
#include "Components/PanelWidget.h"
#include "Components/CanvasPanel.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "MyPlayerController.h"
#include "EditableTool.h"

#define COORAXIS_PROFILENAME (TEXT("BlockPivot"))
#define COORAXIS_OBJECTTYPE (ECC_WorldStatic)

bool ACoordinateAxis::bLocalCoordinateAxis = false;
TWeakObjectPtr<ACoordinateAxis> ACoordinateAxis::CoordinateAxis = nullptr;

// Sets default values
ACoordinateAxis::ACoordinateAxis()
{
    if(HasAnyFlags(RF_ClassDefaultObject))
        return;
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	/* Location */
	TranslateComponent = CreateDefaultSubobject<UTranslationComponent>(TEXT("TranslateComponent"));
	TranslateComponent->SetupAttachment(RootComponent);
	TranslateComponent->SetMobility(EComponentMobility::Movable);
	TranslateComponent->SetCollisionObjectType(COORAXIS_OBJECTTYPE);
	TranslateComponent->SetCollisionProfileName(COORAXIS_PROFILENAME);
	TranslateComponent->SetGenerateOverlapEvents(false);
	TranslateComponent->CastShadow = false;

	/* Rotation */
	RotateComponent = CreateDefaultSubobject<URotateComponent>(TEXT("RotateComponent"));
	RotateComponent->SetupAttachment(RootComponent);
	RotateComponent->SetMobility(EComponentMobility::Movable);
	RotateComponent->SetCollisionObjectType(COORAXIS_OBJECTTYPE);
	RotateComponent->SetCollisionProfileName(COORAXIS_PROFILENAME);
	RotateComponent->SetGenerateOverlapEvents(false);
	RotateComponent->CastShadow = false;

	/* Scale */
	ScaleComponent = CreateDefaultSubobject<UScaleComponent>(TEXT("ScaleComponent"));
	ScaleComponent->SetupAttachment(RootComponent);
	ScaleComponent->SetMobility(EComponentMobility::Movable);
	ScaleComponent->SetCollisionObjectType(COORAXIS_OBJECTTYPE);
	ScaleComponent->SetCollisionProfileName(COORAXIS_PROFILENAME);
	ScaleComponent->SetGenerateOverlapEvents(false);
	ScaleComponent->CastShadow = false;
}

// Called every frame
void ACoordinateAxis::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	/*if (EditActor && IsAllowScaleAutoUpdateRotation && ScaleComponent)
	{
		if (IEditable* EditAct = Cast<IEditable>(EditActor))
		{
			FRotator EditRotator;
			FVector EditScale;
			EditAct->EditRotationAndScale(EditRotator, EditScale);
			ScaleComponent->SetRelativeRotation(EditRotator);
		}
		else
			ScaleComponent->SetRelativeRotation(EditActor->GetActorRotation());
	}*/

	if(CoordAxisWidget && CoordAxisPanel)
	{
		UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(CoordAxisWidget);
		if (CanvasSlot)
		{
			FVector2D ScreenLocation;
			GetWorld()->GetFirstPlayerController()->ProjectWorldLocationToScreen(GetTopLocation(), ScreenLocation);
			
			FGeometry Geometry = CoordAxisPanel->GetCachedGeometry();
			FVector2D Position = Geometry.AbsoluteToLocal(ScreenLocation + Geometry.GetAbsolutePosition());
			CanvasSlot->SetPosition(Position);

			if (((Position.X <= 0 && Position.Y <= 0) || (Position.X > Geometry.Size.X && Position.Y > Geometry.Size.Y)))
			{
				if (CoordAxisWidget->GetWidget()->GetVisibility() == EVisibility::SelfHitTestInvisible)
					CoordAxisWidget->GetWidget()->SetVisibility(EVisibility::Collapsed);
			}
			else if(bSetVisible)
			{
				if (CoordAxisWidget->GetWidget()->GetVisibility() == EVisibility::Collapsed)
					CoordAxisWidget->GetWidget()->SetVisibility(EVisibility::SelfHitTestInvisible);
			}
		}
	}
}

void ACoordinateAxis::Destroyed()
{
	Super::Destroyed();

	if (CoordAxisPanel)
	{
		CoordAxisPanel->RemoveChild(CoordAxisWidget);
	}
	CoordAxisPanel = nullptr;
	CoordAxisWidget = nullptr;

	if (ACoordinateAxis::CoordinateAxis.Get() == this)
		ACoordinateAxis::CoordinateAxis = nullptr;

	if (RotateComponent)
		RotateComponent->RemovePopupString();
}

// Called when the game starts or when spawned
void ACoordinateAxis::BeginPlay()
{
	Super::BeginPlay();

	{	
		// 根据分辨率缩放
		FVector2D ViewportSize = FVector2D(GetWorld()->GetGameViewport()->Viewport->GetSizeXY());
		float Size = ViewportSize.X / 1920.f;
		SetActorScale3D(FVector(FMath::Max(Size, 1.f)));
	}

	ACoordinateAxis::CoordinateAxis = this;

	InitComponentMaterial();

	PopupCoordAxisWidget();

	SetCoordinateAxisMode(ECoordinateMode::ECM_TransformRotationZ);

	EnterEdit();
}

ACoordinateAxis* ACoordinateAxis::SpawnCoordinateAxis(UObject* Object)
{
	if (CoordinateAxis.Get())
	{
		CoordinateAxis->Destroy();
	}

	check(Object);
	CoordinateAxis = MakeWeakObjectPtr(Object->GetWorld()->SpawnActor<ACoordinateAxis>(ACoordinateAxis::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator));
	if (AActor* EditAct = Cast<AActor>(Object))
	{
		CoordinateAxis->SetEditActor(EditAct);
	}
	return CoordinateAxis.Get();
}

void ACoordinateAxis::SetRenderTranslation(bool IsRender, const EAxisType& InType/* = EAxisType::EAT_All*/)
{
	TranslateComponent->SetRenderComponent(IsRender);
	TranslateComponent->SetRenderCoordinateAxis(InType);
}


void ACoordinateAxis::SetRenderRotation(bool IsRender, const EAxisType& InType/* = EAxisType::EAT_All*/)
{
	RotateComponent->SetRenderComponent(IsRender);
	RotateComponent->SetRenderCoordinateAxis(InType);
}

void ACoordinateAxis::SetRenderScale(bool IsRender, const EAxisType& InType/* = EAxisType::EAT_All*/)
{
	if (ScaleComponent)
	{
		ScaleComponent->SetRenderComponent(IsRender);
		ScaleComponent->SetRenderCoordinateAxis(InType);
	}
}

void ACoordinateAxis::SetScaleRotation(const FRotator& InRotator)
{
	/*if (ScaleComponent)
		ScaleComponent->SetRelativeRotation(InRotator);*/
	SetLocalCoordinateAxis(ACoordinateAxis::bLocalCoordinateAxis);
}

void ACoordinateAxis::AutoUpdateScaleRotation(bool InAllow /*= true*/)
{
	IsAllowScaleAutoUpdateRotation = InAllow;
}

void ACoordinateAxis::SetEditActor(AActor* InActor)
{
	EditActor = InActor;
}

void ACoordinateAxis::ChangeCoordinate()
{
	switch (CoordinateMode)
	{
	case ECM_TransformRotationZ:
		CoordinateMode = ECM_Transform;
		break;
	case ECM_Transform:
		CoordinateMode = ECM_Rotation;
		break;
	case ECM_Rotation:
		CoordinateMode = ECM_Scale3D;
		break;
	case ECM_Scale3D:
		CoordinateMode = ECM_TransformRotationZ;
		break;
	}
	SetCoordinateAxisMode(CoordinateMode);
}

void ACoordinateAxis::SetCoordinateAxisMode(ECoordinateMode InMode)
{
// 	if (CoordinateMode == InMode)
// 	{
// 		if (CoordAxisWidget)
// 			CoordAxisWidget->GetWidget()->RetractedCoordAxisType();
// 		return;
// 	}

	CoordinateMode = InMode;

	SetRenderScale(false);
	SetRenderRotation(false);
	SetRenderTranslation(false);
	switch (CoordinateMode)
	{
	case ECM_None:
		break;
	case ECM_TransformRotationZ:
		SetRenderTranslation(true, EAxisType::EAT_All);
		SetRenderRotation(true, EAxisType::EAT_Z);
		TranslateComponent->SetCoorAixsMode(CAM_Mix);
		RotateComponent->SetCoorAixsMode(CAM_Mix);
		if (CoordAxisWidget)
			CoordAxisWidget->GetWidget()->SetLocaAndRotaWidget();
		break;
	case ECM_Transform:
		SetRenderTranslation(true, EAxisType::EAT_All);
		TranslateComponent->SetCoorAixsMode(CAM_Normal);
		if (CoordAxisWidget)
			CoordAxisWidget->GetWidget()->SetTranslationWidget();
		break;
	case ECM_Rotation:
		SetRenderRotation(true, EAxisType::EAT_All);
		RotateComponent->SetCoorAixsMode(CAM_Normal);
		if (CoordAxisWidget)
			CoordAxisWidget->GetWidget()->SetRotateWidget();
		break;
	case ECM_Scale3D:
		SetRenderScale(true, EAxisType::EAT_All);
		ScaleComponent->SetCoorAixsMode(CAM_Normal);
		if (CoordAxisWidget)
			CoordAxisWidget->GetWidget()->SetScaleWidget();
		break;
	default:
		break;
	}
}

ECoordinateMode ACoordinateAxis::GetCoordinateAxisMode()
{
	return CoordinateMode;
}

bool ACoordinateAxis::TryDrag()
{
	FHitResult HitResult;
	InputHandler_GetHitResultUnderCursorByChanel(HitResult, ECC_GameTraceChannel2, true, false, false, 0.f, IsInputTouch);
	if (!HitResult.bBlockingHit)
		return false;

	bool IsTryDrag = GetFocusCoorAxis() != EAT_None;
	return IsTryDrag;
}

FVector ACoordinateAxis::GetCachedTotalScale()
{
	return CachedTotalScale;
}

FRotator ACoordinateAxis::GetCachedTotalRotation()
{
	return CachedTotalRotation;
}

FVector ACoordinateAxis::GetCachedTotalOffset()
{
	return CachedTotalOffset;
}

EAxisType ACoordinateAxis::GetFocusCoorAxis()
{
	EAxisType FoucsType = EAxisType::EAT_None;
	switch (CoordinateMode)
	{
	case ECM_TransformRotationZ:
		FoucsType = RotateComponent->GetFocusAxis();
		FoucsType = (FoucsType == EAxisType::EAT_None) ? TranslateComponent->GetFocusAxis() : FoucsType;
		break;
	case ECM_Transform:
		FoucsType = TranslateComponent->GetFocusAxis();
		break;
	case ECM_Rotation:
		FoucsType = RotateComponent->GetFocusAxis();
		break;
	case ECM_Scale3D:
		FoucsType = ScaleComponent->GetFocusAxis();
		break;
	}
	return FoucsType;
}

bool ACoordinateAxis::GetIsDragging()
{
	bool IsDragging = false;
	switch (CoordinateMode)
	{
	case ECM_TransformRotationZ:
		IsDragging = RotateComponent->bIsDragging();
		IsDragging = IsDragging ? IsDragging : TranslateComponent->bIsDragging();
		break;
	case ECM_Transform:
		IsDragging = TranslateComponent->bIsDragging();
		break;
	case ECM_Rotation:
		IsDragging = RotateComponent->bIsDragging();
		break;
	case ECM_Scale3D:
		IsDragging = ScaleComponent->bIsDragging();
		break;
	}
	return IsDragging;
}

FVector ACoordinateAxis::GetTopLocation()
{
	FVector TopVector = FVector::ZeroVector;
	switch (CoordinateMode)
	{
	case ECM_Scale3D:
		TopVector = ScaleComponent->GetTopLocation();
		break;
	case ECM_Rotation:
		TopVector = RotateComponent->GetTopLocation();
		break;
	case ECM_Transform:
		TopVector = TranslateComponent->GetTopLocation();
		break;
	case ECM_TransformRotationZ:
		TopVector = TranslateComponent->GetTopLocation();
		break;
	}
	return TopVector;
}

void ACoordinateAxis::PopupCoordAxisWidget()
{
	if (UCoordinateUtils::IsEarthLevel())
		return;

	if (!CoordAxisPanel)
	{
		CoordAxisPanel = UGlobalObjects::GetGlobalObject()->CoordAxisPanel;
	}

	if (!CoordAxisWidget)
	{
		CoordAxisWidget = NewObject<class UCoordAxisWidget>();
		CoordAxisWidget->GetWidget()->CoordAxis = this;

		UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(CoordAxisPanel->AddChild(CoordAxisWidget));
		Slot->SetSize(FVector2D(200.f, 200.f));
	}
}

void ACoordinateAxis::UpdateCoorAxisEnable()
{
	if (CoordAxisWidget)
	{
		bool Location = true;
		bool Rotation = true;
		bool Scale = true;
		if (OnCoordAxisDelegateState.IsBound())
		{
			OnCoordAxisDelegateState.Execute(Location, Rotation, Scale);
		}

		CoordAxisWidget->GetWidget()->SetWidgetEnable(true, (OnLocationOffset.IsBound() && Location), (OnRotationOffset.IsBound() && Rotation), (OnScaleDelegate.IsBound() && Scale));
	}
}

void ACoordinateAxis::SetWidgetVisible(bool bVisible)
{
	if (CoordAxisWidget)
	{
		CoordAxisWidget->GetWidget()->SetVisibility(bVisible ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed);
		bSetVisible = bVisible;
	}
}

void ACoordinateAxis::SetParamWidgetVisible(bool bVisible)
{
	if (CoordAxisWidget)
	{
		CoordAxisWidget->GetWidget()->SetParamWidgetVisible(bVisible);
	}
}

void ACoordinateAxis::SetParamWidgetRotator(const FRotator& Rotator)
{
	if (CoordAxisWidget)
	{
		CoordAxisWidget->GetWidget()->SetValueRotation(Rotator);
		UpdateParamWidget();

		SetScaleRotation(Rotator);
	}
}

void ACoordinateAxis::SetParamWidgetScale(const FVector& Scale)
{
	if (CoordAxisWidget)
	{
		CoordAxisWidget->GetWidget()->SetValueScale(Scale);
		UpdateParamWidget();
	}
}

void ACoordinateAxis::SetParamWidgetLocation(const Coordinate3& LocalLocation, bool bAbsLocation/* = true*/)
{
	if (CoordAxisWidget)
	{
		Coordinate3 NewLocation = LocalLocation;

		if (bAbsLocation)
			UCoordinateUtils::TransformWorldLocalToUserCoordinate(NewLocation.X, NewLocation.Y, NewLocation.Z);

		CoordAxisWidget->GetWidget()->SetValueLocation(NewLocation, bAbsLocation);
		UpdateParamWidget();
	}
}

void ACoordinateAxis::UpdateParamWidget()
{
	if (CoordAxisWidget)
	{
		CoordAxisWidget->GetWidget()->UpdateWidgetValueParam();
	}
}

void ACoordinateAxis::SetLocalCoordinateAxis(const bool& bVal, const FRotator& Rotator /*= FRotator::ZeroRotator*/)
{
	ACoordinateAxis::bLocalCoordinateAxis = bVal;

	FRotator EditRotator = FRotator::ZeroRotator;
	if (bVal)
	{
		if (IEditable* EditAct = Cast<IEditable>(EditActor))
		{
			FVector EditScale;
			EditAct->EditRotationAndScale(EditRotator, EditScale);
		}
		else if (EditActor.IsValid())
			EditRotator = EditActor->GetActorRotation();
	}

	if (RotateComponent)
		RotateComponent->SetRelativeRotation(EditRotator);
	if (TranslateComponent)
		TranslateComponent->SetRelativeRotation(EditRotator);
	if (ScaleComponent)
		ScaleComponent->SetRelativeRotation(EditRotator);
}

void ACoordinateAxis::EnterEdit()
{
	RefreshInputHandler(true);

	NotifyTranslationDrag();

	CachedTotalScale = FVector::ZeroVector;
	CachedTotalRotation = FRotator::ZeroRotator;
	CachedTotalOffset = FVector::ZeroVector;
}

void ACoordinateAxis::ExitEdit()
{
	RefreshInputHandler(true);
}

void ACoordinateAxis::RefreshInputHandler(bool bEnableInput)
{
	if (bEnableInput)
		EnableInput(Controller);
	else
		DisableInput(Controller);

	ClearKeyInput();
	ClearAxisInput();

	if (bEnableInput)
		BindKeyEvent();
}

void ACoordinateAxis::BindKeyEvent()
{
	{
		FInputActionUnifiedDelegate LeftDownDelegate;
		LeftDownDelegate.BindDelegate(this, &ACoordinateAxis::OnLeftMouseButtonDown);
		RegisterAdvancedInputKey(FInputChord(EKeys::LeftMouseButton, false, false, false, false), IE_Pressed, LeftDownDelegate, false);

		// InputComponent->BindKey(EKeys::LeftMouseButton, EInputEvent::IE_Pressed, this, &ACoordinateAxis::OnLeftMouseButtonDown);
	}

	{
		FInputActionUnifiedDelegate LeftUpDelegate;
		LeftUpDelegate.BindDelegate(this, &ACoordinateAxis::OnLeftMouseButtonUp);
		RegisterAdvancedInputKey(FInputChord(EKeys::LeftMouseButton, false, false, false, false), IE_Released, LeftUpDelegate, false);

		// InputComponent->BindKey(EKeys::LeftMouseButton, EInputEvent::IE_Released, this, &ACoordinateAxis::OnLeftMouseButtonUp);
	}

	{
		FInputActionUnifiedDelegate LeftDownDelegate;
		LeftDownDelegate.BindDelegate(this, &ACoordinateAxis::OnLeftControlButtonDown);
		RegisterAdvancedInputKey(FInputChord(EKeys::LeftControl, false, false, false, false), IE_Pressed, LeftDownDelegate, false);
	}

	{
		FInputActionUnifiedDelegate LeftDownDelegate;
		LeftDownDelegate.BindDelegate(this, &ACoordinateAxis::OnLeftControlButtonUp);
		RegisterAdvancedInputKey(FInputChord(EKeys::LeftControl, false, false, false, false), IE_Released, LeftDownDelegate, false);
	}

	{
		FInputActionUnifiedDelegate LeftDownDelegate;
		LeftDownDelegate.BindDelegate(this, &ACoordinateAxis::OnLeftControlButtonDown);
		RegisterAdvancedInputKey(FInputChord(EKeys::RightControl, false, false, false, false), IE_Pressed, LeftDownDelegate, false);
	}

	{
		FInputActionUnifiedDelegate LeftDownDelegate;
		LeftDownDelegate.BindDelegate(this, &ACoordinateAxis::OnLeftControlButtonUp);
		RegisterAdvancedInputKey(FInputChord(EKeys::RightControl, false, false, false, false), IE_Released, LeftDownDelegate, false);
	}

	{
		FInputActionUnifiedDelegate SpaceDownDelegate;
		SpaceDownDelegate.BindDelegate(this, &ACoordinateAxis::OnSpaceButtonDown);
		RegisterAdvancedInputKey(FInputChord(EKeys::SpaceBar, false, false, false, false), IE_Pressed, SpaceDownDelegate, false);
	}

	// Input Touch
	{
		FInputActionUnifiedDelegate InputTouchDownPressed;
		InputTouchDownPressed.BindDelegate(this, &ACoordinateAxis::OnInputTouchDown);
		RegisterAdvancedInputKey(FInputChord(EKeys::TouchKeys[0], false, false, false, false), IE_Pressed, InputTouchDownPressed, false);
	}

	{
		FInputActionUnifiedDelegate InputTouchUpPressed;
		InputTouchUpPressed.BindDelegate(this, &ACoordinateAxis::OnInputTouchUp);
		RegisterAdvancedInputKey(FInputChord(EKeys::TouchKeys[0], false, false, false, false), IE_Released, InputTouchUpPressed, false);
	}
}

void ACoordinateAxis::OnLeftMouseButtonDown()
{
	IsPrepareStartDrag = true;

	if (GetFocusCoorAxis() == EAT_None)
		MouseDownNotSelectDelegate.ExecuteIfBound();
	else
		OnMouseDownSelectAxisDelegate.ExecuteIfBound();

	RegisterMouseAxisInput();
	StartCoordinateAixsDragging();

	// AMyPlayerController::SetCurrentMouseCursor(EMouseCursor::Type::Default);
}

void ACoordinateAxis::OnLeftMouseButtonUp()
{
	IsPrepareStartDrag = false;

	NotifyStopDrag();
	ClearAxisInput();
	StopCoordinateAixsDragging();

	// AMyPlayerController::RecoverPreviousMouseCursor();
}

void ACoordinateAxis::OnLeftControlButtonDown()
{
	IsCopySelectActor = true;
}

void ACoordinateAxis::OnLeftControlButtonUp()
{
	IsCopySelectActor = false;
}

void ACoordinateAxis::OnInputTouchDown()
{
	if (!IsInputTouch)
	{
		if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(UGlobalObjects::GetGlobalObject()->GetCurrentWorld(), 0))
		{
			FHitResult HitResult;
			FVector WorldLocation, WorldDirection;

			bool bIsPressed = false;
			FVector2D TouchPosition = FVector2D::ZeroVector;
			PlayerController->GetInputTouchState(ETouchIndex::Type::Touch1, TouchPosition.X, TouchPosition.Y, bIsPressed);
			UGameplayStatics::DeprojectScreenToWorld(PlayerController, TouchPosition, WorldLocation, WorldDirection);

			FVector StartLocation = WorldLocation;
			FVector EndLocation = StartLocation + WorldDirection * 100000000.f;
			FCollisionQueryParams CollisionQueryParams;
			CollisionQueryParams.bTraceComplex = true;
			CollisionQueryParams.bReturnFaceIndex = true;
			GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_GameTraceChannel2, CollisionQueryParams);

			if (HitResult.bBlockingHit && Cast<UCoordinateAxisComponent>(HitResult.GetComponent()))
			{
				if (UCoordinateAxisComponent* CoorAxisComp = Cast<UCoordinateAxisComponent>(HitResult.GetComponent()))
				{
					CoorAxisComp->SetTouchDragging(true);
					CoorAxisComp->UpdateFocusAxis();

					IsInputTouch = true;
					IsPrepareStartDrag = true;

					OnMouseDownSelectAxisDelegate.ExecuteIfBound();

					RegisterMouseAxisInput();
					StartCoordinateAixsDragging();
				}
			}
			else
			{
				MouseDownNotSelectDelegate.ExecuteIfBound();
			}
		}
	}
}

void ACoordinateAxis::OnInputTouchUp()
{
	if (IsInputTouch)
	{
		IsInputTouch = false;
		IsPrepareStartDrag = false;

		NotifyStopDrag();
		ClearAxisInput();
		StopCoordinateAixsDragging();
	}
}

void ACoordinateAxis::OnSpaceButtonDown()
{

}

void ACoordinateAxis::OnMouseMove(float Val)
{

}

void ACoordinateAxis::RegisterEngineAxisKeys()
{
	RegisterEngineDefinedAxisMapping(FInputAxisKeyMapping("InputHandler_MoveForward", EKeys::W, 1.f));
	RegisterEngineDefinedAxisMapping(FInputAxisKeyMapping("InputHandler_MoveForward", EKeys::S, -1.f));
	RegisterEngineDefinedAxisMapping(FInputAxisKeyMapping("InputHandler_MoveRight", EKeys::A, -1.f));
	RegisterEngineDefinedAxisMapping(FInputAxisKeyMapping("InputHandler_MoveRight", EKeys::D, 1.f));
	RegisterEngineDefinedAxisMapping(FInputAxisKeyMapping("InputHandler_MouseX", EKeys::MouseX, 1.f));
	RegisterEngineDefinedAxisMapping(FInputAxisKeyMapping("InputHandler_MouseY", EKeys::MouseY, -1.f));
}

void ACoordinateAxis::RegisterMouseAxisInput()
{
	if (!InputComponent)
		return;

	InputComponent->BindAxis("InputHandler_MouseX", this, &ACoordinateAxis::OnMouseMove);
	InputComponent->BindAxis("InputHandler_MouseY", this, &ACoordinateAxis::OnMouseMove);
}

void ACoordinateAxis::StartCoordinateAixsDragging()
{
	if (CoordAxisWidget)
		CoordAxisWidget->GetWidget()->RetractedCoordAxisType();

	if(GetFocusCoorAxis() != EAT_None)
	{
		if (TranslateComponent->bIsRender())
			TranslateComponent->StartDragging();
		if (RotateComponent->bIsRender())
			RotateComponent->StartDragging();
		if (ScaleComponent->bIsRender())
			ScaleComponent->StartDragging();
	}
}

void ACoordinateAxis::StopCoordinateAixsDragging()
{
	if(TranslateComponent->bIsRender())
		TranslateComponent->StopDragging();
	if (RotateComponent->bIsRender())
		RotateComponent->StopDragging();
	if (ScaleComponent->bIsRender())
		ScaleComponent->StopDragging();
}

void ACoordinateAxis::NotifyTranslationDrag()
{
	if(ScaleComponent)
	{
		ScaleComponent->OnScaleOffectDelegate.BindLambda([this](const FVector& InOffect) 
		{
			if (IsPrepareStartDrag && (FMath::Abs(InOffect.X) > 0.f || FMath::Abs(InOffect.Y) > 0.f || FMath::Abs(InOffect.Z) > 0.f))
			{
				NotifyStartDrag();
				IsPrepareStartDrag = false;
			}

			NotifyScaleOffset(InOffect);
			CachedTotalScale += InOffect;
		});
	}
	if(RotateComponent)
	{
		RotateComponent->OnRotationOffsetDelegate.BindLambda([this](const FRotator& InOffect) 
		{
			if (IsPrepareStartDrag && (FMath::Abs(InOffect.Pitch) > 0.f || FMath::Abs(InOffect.Yaw) > 0.f || FMath::Abs(InOffect.Roll) > 0.f))
			{
				NotifyStartDrag();
				IsPrepareStartDrag = false;
			}

			NotifyRotationOffset(InOffect);
			CachedTotalRotation += InOffect;
		});
	}
	if(TranslateComponent)
	{
		TranslateComponent->OnTranslationOffsetDelegate.BindLambda([this](const FVector& InOffect) 
		{
			if (IsPrepareStartDrag && (FMath::Abs(InOffect.X) > 0.f || FMath::Abs(InOffect.Y) > 0.f || FMath::Abs(InOffect.Z) > 0.f))
			{
				NotifyStartDrag();
				IsPrepareStartDrag = false;
			}

			NotifyLocationOffset(InOffect);
			CachedTotalOffset += InOffect;
		});
	}
}

void ACoordinateAxis::NotifyStartDrag()
{
	if(GetFocusCoorAxis() != EAT_None && GetIsDragging())
	{
		IsStartDrag = true;
		StartDragDelegate.ExecuteIfBound();
		CopySelectActor();
	}
}

void ACoordinateAxis::NotifyStopDrag()
{
	if (IsStartDrag)
	{
		IsStartDrag = false;
		StopDragDelegate.ExecuteIfBound();
	}
}

void ACoordinateAxis::NotifyScaleOffset(const FVector& Offset)
{
	if (OnScaleDelegate.IsBound())
	{
		OnScaleDelegate.Execute(Offset);
	}

	if (CoordAxisWidget)
	{
		FVector Scale = CoordAxisWidget->GetWidget()->GetValueScale();
		CoordAxisWidget->GetWidget()->SetValueScale(Scale + Offset);
		UpdateParamWidget();
	}
}

void ACoordinateAxis::NotifyRotationOffset(const FRotator& Offset)
{
	FRotator Rotation = FRotator::ZeroRotator;
	if (OnRotationOffset.IsBound())
	{
		Rotation = OnRotationOffset.Execute(Offset);
	}

	if (CoordAxisWidget)
	{
		CoordAxisWidget->GetWidget()->SetValueRotation(Rotation);
		UpdateParamWidget();
	}

	SetScaleRotation(Rotation);
}

void ACoordinateAxis::NotifyLocationOffset(const FVector& Offset)
{
	Coordinate3 Location = Coordinate3(0);
	if (OnLocationOffset.IsBound())
	{
		Location = OnLocationOffset.Execute(Offset);
	}

	if (CoordAxisWidget)
	{
		if(CoordAxisWidget->GetWidget()->GetIsAbsLocation())
			UCoordinateUtils::TransformWorldLocalToUserCoordinate(Location.X, Location.Y, Location.Z);

		CoordAxisWidget->GetWidget()->SetValueLocation(Location, CoordAxisWidget->GetWidget()->GetIsAbsLocation() );
		UpdateParamWidget();
	}
}

void ACoordinateAxis::CopySelectActor()
{
	if (IsCopySelectActor)
	{
		IsCopySelectActor = false;
		if (AEditableTool* EditTool = AEditableTool::GetEditableTool())
		{
			AActor* NewEditActor = EditActor.IsValid() ? EditActor.Get() : EditTool->GetSelectEditableActor();
			if (NewEditActor)
			{
				EditTool->CopySelectActor(NewEditActor, UCoordinateUtils::GetWorldAbsoluteCoordinate(NewEditActor->GetActorLocation()), false);
			}
		}
	}
}

void ACoordinateAxis::InitComponentMaterial()
{
	AxisColorX = FLinearColor(0.60f, 0.02f, 0.0f, 1.f);
	AxisColorY = FLinearColor(0.15f, 0.40f, 0.0f, 1.f);
	AxisColorZ = FLinearColor(0.03f, 0.20f, 0.9f, 1.f);
	PlaneClor = FLinearColor(1.f, 1.f, 0.f, 0.3f);
	AxisOriginColor = FLinearColor(1.f, 1.f, 1.f, 1.f);
	FocusColor = FLinearColor(1.f, 1.f, 0.f, 1.f);
	FocusPlaneClor = FLinearColor(1.f, 1.f, 0.f, 0.8f);
	LucencyColor = FLinearColor(0.f, 0.f, 0.f, 0.f);

	// Init Material
	UMaterial* AxisMaterialBase = LoadObject<UMaterial>(nullptr, TEXT("Material'/AirCityPlugin/ArtResources/CoorAxisMaterial/CoorAxisMaterial.CoorAxisMaterial'"));
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

	ArcPlaneMaterial = LoadObject<UMaterial>(nullptr, TEXT("Material'/AirCityPlugin/ArtResources/CoorAxisMaterial/RotationColorMaterial.RotationColorMaterial'"));
	check(ArcPlaneMaterial);

	if (TranslateComponent)
		TranslateComponent->SetComponentMaterial(AxisOriginMaterial, AxisMaterialX, AxisMaterialY, AxisMaterialZ, FocusAxisMaterial, PlaneMaterial, FocusPlaneMaterial, ArcPlaneMaterial);
	if (RotateComponent)
		RotateComponent->SetComponentMaterial(AxisOriginMaterial, AxisMaterialX, AxisMaterialY, AxisMaterialZ, FocusAxisMaterial, PlaneMaterial, FocusPlaneMaterial, ArcPlaneMaterial);
	if (ScaleComponent)
		ScaleComponent->SetComponentMaterial(AxisOriginMaterial, AxisMaterialX, AxisMaterialY, AxisMaterialZ, FocusAxisMaterial, PlaneMaterial, FocusPlaneMaterial, ArcPlaneMaterial);
}

bool ACoordinateAxis::IsSelectCoordAxis()
{
	if (CoordinateAxis.IsValid())
		return CoordinateAxis.Get()->IsInputTouch;
	return false;
}

TSharedPtr<class SCoordAxisWidget> UCoordAxisWidget::GetWidget()
{
	if (!CoordAxisWidget.IsValid())
		CoordAxisWidget = SNew(SCoordAxisWidget);
	return CoordAxisWidget;
}

void UCoordAxisWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
}

void UCoordAxisWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	CoordAxisWidget.Reset();
}

TSharedRef<SWidget> UCoordAxisWidget::RebuildWidget()
{
	if (!CoordAxisWidget.IsValid())
		CoordAxisWidget = SNew(SCoordAxisWidget);
	return CoordAxisWidget.ToSharedRef();
}

#if WITH_EDITOR
const FText UCoordAxisWidget::GetPaletteCategory()
{
	return NSLOCTEXT("CoordAxisWidget", "Geometry", "Geometry");
}
#endif
