// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class SvgPulgin : ModuleRules
{
	public SvgPulgin(ReadOnlyTargetRules Target) : base(Target)
	{
        CppStandard = CppStandardVersion.Cpp17;
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        string LunasvgRoot = Path.Combine(ModuleDirectory, "Private", "lunasvg");
        string PlutovgSource = Path.Combine(LunasvgRoot, "plutovg", "source");
        string PlutovgInclude = Path.Combine(LunasvgRoot, "plutovg", "include");

        // 头文件搜索路径
        PublicIncludePaths.Add(Path.Combine(LunasvgRoot, "include"));
        PublicIncludePaths.Add(PlutovgInclude);

        // C++ 源文件
        PrivateIncludePaths.Add(Path.Combine(LunasvgRoot, "source"));

        // ---- 收集所有 .c 和 .cpp 源文件 ----
        string[] LunasvgCpp = Directory.GetFiles(Path.Combine(LunasvgRoot, "source"), "*.cpp", SearchOption.TopDirectoryOnly);
        string[] PlutovgC = Directory.GetFiles(PlutovgSource, "*.c", SearchOption.TopDirectoryOnly);

        // 将所有文件加入编译列表（UE4 UBT 会自动处理 C++ 编译）
        foreach (var f in LunasvgCpp) { PublicAdditionalShadowFiles.Add(f); }
        foreach (var f in PlutovgC) { PublicAdditionalShadowFiles.Add(f); }

        // 声明 ExtraCompilationFields 让 UBT 编译这些 C 文件为 C++
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicDefinitions.Add("PLUTOVG_CPP=1");
        }

        PublicDependencyModuleNames.AddRange(new string[] 
        {
            "Core", 
            "CoreUObject", 
            "Engine",
            "RHI", 
            "RenderCore", 
            "ImageWrapper"
        });
    }
}
