// Fill out your copyright notice in the Description page of Project Settings.


#include "SVGRenderHelper.h"
#include "TextureResource.h"
#include "Misc/FileHelper.h"
#include "RenderUtils.h"
#include "Misc/Paths.h"
#include "lunasvg.h"

using namespace lunasvg;

// ─────────────────────────────────────────────────────────
// 辅助：FLinearColor → 0xRRGGBBAA（lunasvg backgroundColor 格式）
// ─────────────────────────────────────────────────────────
uint32 FSVGRenderHelper::LinearColorToLunaBGColor(FLinearColor Color)
{
	auto ToU8 = [](float v) { return (uint8)FMath::Clamp(FMath::RoundToInt(v * 255.f), 0, 255); };
	uint8 R = ToU8(Color.R), G = ToU8(Color.G),
		B = ToU8(Color.B), A = ToU8(Color.A);
	return ((uint32)R << 24) | ((uint32)G << 16) | ((uint32)B << 8) | (uint32)A;
}

// ─────────────────────────────────────────────────────────
// 核心：将 RGBA 字节数组 → UTexture2D
// ─────────────────────────────────────────────────────────
UTexture2D* FSVGRenderHelper::BitmapToTexture2D(
	const uint8* RGBAData, int32 Width, int32 Height)
{
	if (!RGBAData || Width <= 0 || Height <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("SVGRenderHelper: Invalid bitmap data"));
		return nullptr;
	}

	// 1. 创建 Transient Texture2D（内存中，不持久化到资产）
	UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PF_R8G8B8A8);
	if (!Texture)
	{
		UE_LOG(LogTemp, Error, TEXT("SVGRenderHelper: Failed to create UTexture2D"));
		return nullptr;
	}

	// 2. 锁定 Mip 0 并写入像素数据
	FTexture2DMipMap& Mip = Texture->PlatformData->Mips[0];
	void* TextureData = Mip.BulkData.Lock(LOCK_READ_WRITE);

	FMemory::Memcpy(TextureData, RGBAData, Width * Height * 4);

	Mip.BulkData.Unlock();

	// 3. 纹理属性：无 sRGB、无 Mipmap（SVG 通常用于 UI，保持像素精度）
	Texture->SRGB = false;   // 如需 sRGB 可改为 true
	Texture->CompressionSettings = TC_VectorDisplacementmap; // 不压缩，保持 RGBA 精度
	Texture->MipGenSettings = TMGS_NoMipmaps;
	Texture->Filter = TF_Bilinear;
	Texture->NeverStream = true;

	// 4. 提交到 GPU
	Texture->UpdateResource();

	return Texture;
}

// ─────────────────────────────────────────────────────────
// 从文件加载 SVG
// ─────────────────────────────────────────────────────────
UTexture2D* FSVGRenderHelper::LoadSVGFromFile(
	const FString& FilePath, int32 Width, int32 Height,
	FLinearColor BackgroundColor)
{
	// 将 FString 转换为 std::string（UTF-8 路径）
	std::string NativePath(TCHAR_TO_UTF8(*FilePath));

	auto Doc = Document::loadFromFile(NativePath);
	if (!Doc)
	{
		UE_LOG(LogTemp, Warning, TEXT("SVGRenderHelper: Cannot load SVG file: %s"), *FilePath);
		return nullptr;
	}

	uint32 BgColor = LinearColorToLunaBGColor(BackgroundColor);

	// renderToBitmap: 内部格式为 ARGB32 Premultiplied
	Bitmap Bmp = Doc->renderToBitmap(Width, Height, BgColor);
	if (Bmp.isNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("SVGRenderHelper: renderToBitmap failed"));
		return nullptr;
	}

	// convertToRGBA: ARGB32_Premul → RGBA plain（UE4 需要 RGBA）
	Bmp.convertToRGBA();

	return BitmapToTexture2D(Bmp.data(), Bmp.width(), Bmp.height());
}

// ─────────────────────────────────────────────────────────
// 从 SVG 字符串加载
// ─────────────────────────────────────────────────────────
UTexture2D* FSVGRenderHelper::LoadSVGFromString(
	const FString& SVGContent, int32 Width, int32 Height,
	FLinearColor BackgroundColor)
{
	std::string SVGStr(TCHAR_TO_UTF8(*SVGContent));

	auto Doc = Document::loadFromData(SVGStr);
	if (!Doc) return nullptr;

	uint32 BgColor = LinearColorToLunaBGColor(BackgroundColor);
	Bitmap Bmp = Doc->renderToBitmap(Width, Height, BgColor);
	if (Bmp.isNull()) return nullptr;

	Bmp.convertToRGBA();
	return BitmapToTexture2D(Bmp.data(), Bmp.width(), Bmp.height());
}

// ─────────────────────────────────────────────────────────
// 从字节数组加载
// ─────────────────────────────────────────────────────────
UTexture2D* FSVGRenderHelper::LoadSVGFromData(
	const TArray<uint8>& Data, int32 Width, int32 Height,
	FLinearColor BackgroundColor)
{
	if (Data.Num() == 0) return nullptr;

	auto Doc = Document::loadFromData(
		reinterpret_cast<const char*>(Data.GetData()), Data.Num());
	if (!Doc) return nullptr;

	uint32 BgColor = LinearColorToLunaBGColor(BackgroundColor);
	Bitmap Bmp = Doc->renderToBitmap(Width, Height, BgColor);
	if (Bmp.isNull()) return nullptr;

	Bmp.convertToRGBA();
	return BitmapToTexture2D(Bmp.data(), Bmp.width(), Bmp.height());
}