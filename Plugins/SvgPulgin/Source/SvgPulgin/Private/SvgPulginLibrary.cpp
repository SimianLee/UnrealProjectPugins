// Fill out your copyright notice in the Description page of Project Settings.


#include "SvgPulginLibrary.h"
#include "SVGRenderHelper.h"
#include "Misc/Paths.h"

UTexture2D* USvgPulginLibrary::K2_LoadSVGFromFile(
	const FString& FilePath, int32 Width, int32 Height)
{
	return FSVGRenderHelper::LoadSVGFromFile(FilePath, Width, Height);
}

UTexture2D* USvgPulginLibrary::K2_LoadSVGFromString(
	const FString& SVGContent, int32 Width, int32 Height)
{
	return FSVGRenderHelper::LoadSVGFromString(SVGContent, Width, Height);
}

FString USvgPulginLibrary::K2_GetResourcesPath()
{
	FString PluginResourcesFolder = FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("SvgPulgin"), TEXT("Resources"));
	PluginResourcesFolder = FPaths::ConvertRelativePathToFull(PluginResourcesFolder);
	FPaths::MakePlatformFilename(PluginResourcesFolder);
	return PluginResourcesFolder;
}
