// Fill out your copyright notice in the Description page of Project Settings.


#include "CoordinateAxisActor.h"
#include "CoordinateAxisComponent.h"

// Sets default values
ACoordinateAxisActor::ACoordinateAxisActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CoordinateAxisComponent = 
		CreateDefaultSubobject<UCoordinateAxisComponent>(TEXT("CoordinateAxisComponent"));
}

// Called when the game starts or when spawned
void ACoordinateAxisActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ACoordinateAxisActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

