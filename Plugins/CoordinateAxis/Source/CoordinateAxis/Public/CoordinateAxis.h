// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Tools/Input/InputHandler.h"
#include "GameFramework/Actor.h"
#include "Delegates/DelegateCombinations.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/CoordinateAxis/CoordinateAxisComponent.h"
#include "Common/BlueprintTypes.h"
#include "Public/Helper/PopupHelper.h"
#include "CoordinateAxis.generated.h"

UENUM(BlueprintType)
enum ECoordinateMode
{
	ECM_None = 0,
	ECM_Scale3D,
	ECM_Rotation,
	ECM_Transform,
	ECM_TransformRotationZ,
};

UCLASS()
class AIRCITYPLUGIN_API ACoordinateAxis : public AInputHandler
{
	GENERATED_BODY()

protected:

	DECLARE_DELEGATE_OneParam(FScaleDelegate, FVector);

	DECLARE_DELEGATE_RetVal_OneParam(FRotator, FRotateDelegate, FRotator);

	DECLARE_DELEGATE_RetVal_OneParam(Coordinate3, FLocationDelegate, FVector);

	DECLARE_DELEGATE_ThreeParams(FCoordAxisDelegateState, bool &, bool &, bool &);

public:	

	// Sets default values for this actor's properties
	ACoordinateAxis();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void Destroyed() override;

protected:

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:/*  */

	UFUNCTION()
	static ACoordinateAxis* SpawnCoordinateAxis(UObject* Object);

	UFUNCTION(BlueprintCallable)
	void SetRenderTranslation(bool IsRender, const EAxisType& InType = EAxisType::EAT_All);

	UFUNCTION(BlueprintCallable)
	void SetRenderRotation(bool IsRender, const EAxisType& InType = EAxisType::EAT_All);

	UFUNCTION(BlueprintCallable)
	void SetRenderScale(bool IsRender, const EAxisType& InType = EAxisType::EAT_All);

	UFUNCTION(BlueprintCallable)
	void SetScaleRotation(const FRotator& InRotator);

	UFUNCTION(BlueprintCallable)
	void AutoUpdateScaleRotation(bool InAllow = true);

	UFUNCTION(BlueprintCallable)
	void SetEditActor(AActor* InActor);

	// 切换模式
	UFUNCTION(BlueprintCallable)
	void ChangeCoordinate();

	UFUNCTION(BlueprintCallable)
	void SetCoordinateAxisMode(ECoordinateMode InMode);

	UFUNCTION(BlueprintCallable)
	ECoordinateMode GetCoordinateAxisMode();

	UFUNCTION(BlueprintCallable)
	bool TryDrag();

	UFUNCTION(BlueprintPure)
	FVector GetCachedTotalScale();

	UFUNCTION(BlueprintPure)
	FRotator GetCachedTotalRotation();

	UFUNCTION(BlueprintPure)
	FVector GetCachedTotalOffset();

	EAxisType GetFocusCoorAxis();

	bool GetIsDragging();

	// Widget
	FVector GetTopLocation();

	void PopupCoordAxisWidget();

	void UpdateCoorAxisEnable();

	void SetWidgetVisible(bool bVisible);

	void SetParamWidgetVisible(bool bVisible);

	void SetParamWidgetRotator(const FRotator& Rotator);

	void SetParamWidgetScale(const FVector& Scale);

	void SetParamWidgetLocation(const Coordinate3& LocalLocation, bool bAbsLocation = true);

	void UpdateParamWidget();

	void SetLocalCoordinateAxis(const bool& bVal, const FRotator& Rotator = FRotator::ZeroRotator);

protected:

	ECoordinateMode CoordinateMode = ECoordinateMode::ECM_None;

	UPROPERTY()
	bool IsAllowScaleAutoUpdateRotation = true;

	UPROPERTY()
	TWeakObjectPtr<AActor> EditActor = nullptr;

	FVector CachedTotalScale = FVector::ZeroVector;

	FRotator CachedTotalRotation = FRotator::ZeroRotator;

	FVector CachedTotalOffset = FVector::ZeroVector;

public:/* Start Input */

	UFUNCTION(BlueprintCallable)
	void EnterEdit();

	UFUNCTION(BlueprintCallable)
	void ExitEdit();

	UFUNCTION(BlueprintCallable)
	void RefreshInputHandler(bool bEnableInput);

protected:

	void BindKeyEvent();
	
	void OnLeftMouseButtonDown();
	
	void OnLeftMouseButtonUp();

	void OnLeftControlButtonDown();

	void OnLeftControlButtonUp();

	void OnInputTouchDown();

	void OnInputTouchUp();
	
	void OnSpaceButtonDown();

	void OnMouseMove(float Val);

	void RegisterEngineAxisKeys();

	void RegisterMouseAxisInput();

	UFUNCTION()
	void StartCoordinateAixsDragging();

	UFUNCTION()
	void StopCoordinateAixsDragging();

private:

	bool IsPrepareStartDrag = false;

	bool IsStartDrag = false;

	bool IsInputTouch = false;

	bool IsCopySelectActor = false;
public:

	FSimpleDelegate OnMouseDownSelectAxisDelegate = FSimpleDelegate();

	FSimpleDelegate StartDragDelegate = FSimpleDelegate();

	FSimpleDelegate StopDragDelegate = FSimpleDelegate();

	FSimpleDelegate MouseDownNotSelectDelegate = FSimpleDelegate();

	FScaleDelegate OnScaleDelegate = FScaleDelegate();

	FRotateDelegate OnRotationOffset = FRotateDelegate();

	FLocationDelegate OnLocationOffset = FLocationDelegate();

	// 获取当前坐标绑定状态
	FCoordAxisDelegateState OnCoordAxisDelegateState;
	
public:

	void NotifyTranslationDrag();

	void NotifyStartDrag();

	void NotifyStopDrag();

	void NotifyScaleOffset(const FVector& Offset);

	void NotifyRotationOffset(const FRotator& Offset);

	void NotifyLocationOffset(const FVector& Offset);

	void CopySelectActor();

private:

	UPROPERTY()
	class UTranslationComponent* TranslateComponent;

	UPROPERTY()
	class URotateComponent* RotateComponent;

	UPROPERTY()
	class UScaleComponent* ScaleComponent;

	bool bSetVisible = true;

protected:

	void InitComponentMaterial();

	// Color
	FLinearColor AxisColorX, AxisColorY, AxisColorZ, PlaneClor, AxisOriginColor, FocusColor, FocusPlaneClor, LucencyColor;

	// Material Instance
	UPROPERTY()
	UMaterialInstanceDynamic* AxisMaterialX;

	UPROPERTY()
	UMaterialInstanceDynamic* AxisMaterialY;

	UPROPERTY()
	UMaterialInstanceDynamic* AxisMaterialZ;

	UPROPERTY()
	UMaterialInstanceDynamic* PlaneMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* AxisOriginMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* FocusAxisMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* FocusPlaneMaterial;

	UPROPERTY()
	UMaterialInterface* ArcPlaneMaterial;

protected:

	UPROPERTY()
	class UCanvasPanel* CoordAxisPanel = nullptr;

	UPROPERTY()
	class UCoordAxisWidget* CoordAxisWidget = nullptr;

public:

	UFUNCTION(BlueprintPure)
	static bool IsSelectCoordAxis();

	static bool bLocalCoordinateAxis;

	static TWeakObjectPtr<ACoordinateAxis> CoordinateAxis;
};

UCLASS()
class AIRCITYPLUGIN_API UCoordAxisWidget : public UWidget
{
	GENERATED_BODY()

public:

	TSharedPtr<class SCoordAxisWidget> GetWidget();

protected:

	//~ Begin UWidget Interface
	virtual void SynchronizeProperties() override;
	//~ End UWidget Interface

	//~ Begin UVisual Interface
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	//~ End UVisual Interface

	//~ Begin UWidget Interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget

#if WITH_EDITOR

	// UWidget interface
	virtual const FText GetPaletteCategory() override;
	// End UWidget interface

#endif

protected:

	TSharedPtr<class SCoordAxisWidget> CoordAxisWidget;

};
