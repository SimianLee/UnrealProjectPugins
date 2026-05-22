// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"

/**
 * lunasvg 渲染辅助类
 * 将 SVG 文件 / 字符串渲染为 UTexture2D，供 UE4 使用
 */
class FSVGRenderHelper
{
public:
    /** 从磁盘文件加载 SVG 并渲染为指定尺寸的 Texture2D */
    static UTexture2D* LoadSVGFromFile(
        const FString& FilePath,
        int32 Width = -1,   // -1 = 使用 SVG 本征尺寸
        int32 Height = -1,
        FLinearColor BackgroundColor = FLinearColor::Transparent
    );

    /** 从内存中的 SVG 字符串渲染为 Texture2D */
    static UTexture2D* LoadSVGFromString(
        const FString& SVGContent,
        int32 Width = -1,
        int32 Height = -1,
        FLinearColor BackgroundColor = FLinearColor::Transparent
    );

    /** 从原始 SVG 字节数据渲染 */
    static UTexture2D* LoadSVGFromData(
        const TArray<uint8>& Data,
        int32 Width = -1,
        int32 Height = -1,
        FLinearColor BackgroundColor = FLinearColor::Transparent
    );

private:
    /** 将 lunasvg::Bitmap（RGBA plain）转换为 UTexture2D */
    static UTexture2D* BitmapToTexture2D(
        const uint8* RGBAData,
        int32 Width,
        int32 Height
    );

    /** 将 FLinearColor 转换为 lunasvg 背景色格式（0xRRGGBBAA）*/
    static uint32 LinearColorToLunaBGColor(FLinearColor Color);
};