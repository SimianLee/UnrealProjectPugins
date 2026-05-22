// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SvgPulginLibrary.generated.h"

/**
 * 
 */
UCLASS()
class SVGPULGIN_API USvgPulginLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	/** 从文件路径加载 SVG，返回 Texture2D（可用于 UI Image / Material） */
	UFUNCTION(BlueprintCallable, Category = "SVG Loader",
		meta = (DisplayName = "Load SVG From File"))
	static UTexture2D* K2_LoadSVGFromFile(
		const FString& FilePath,
		int32 Width = 256,
		int32 Height = 256
	);

	/** 从 SVG 字符串内容加载，返回 Texture2D */
	UFUNCTION(BlueprintCallable, Category = "SVG Loader",
		meta = (DisplayName = "Load SVG From String"))
	static UTexture2D* K2_LoadSVGFromString(
		const FString& SVGContent,
		int32 Width = 256,
		int32 Height = 256
	);

	UFUNCTION(BlueprintPure, Category = "SVG Loader",
		meta = (DisplayName = "Get Resources Path"))
	static FString K2_GetResourcesPath();
	
};
