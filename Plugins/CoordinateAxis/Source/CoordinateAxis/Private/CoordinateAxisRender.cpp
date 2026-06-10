// CoordinateAxisRender.cpp
// 管理所有坐标轴的绘制逻辑 — Translation / Rotation / Scale
// 仅 .cpp 文件，无对应 .h。函数声明在 CoordinateAxisComponent.h 的 CoordinateAxisRenderFuncs 命名空间中。

#include "CoordinateAxisComponent.h"
#include "DynamicMeshBuilder.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "SceneView.h"

// ============================================================
// 内部辅助
// ============================================================

namespace
{
	static FDynamicMeshVertex AddVertex(
		const FVector& InPosition,
		const FVector2D& InTextureCoordinate,
		const FVector& InTangentX,
		const FVector& InTangentY,
		const FVector& InTangentZ,
		const FColor& InColor)
	{
		FDynamicMeshVertex Vertex;
		Vertex.Position = InPosition;
		Vertex.TextureCoordinate[0] = InTextureCoordinate;
		Vertex.TangentX = InTangentX;
		Vertex.TangentZ = InTangentZ;
		Vertex.TangentZ.Vector.W = GetBasisDeterminantSignByte(InTangentX, InTangentY, InTangentZ);
		Vertex.Color = InColor;
		return Vertex;
	}

	static const float INNER_AXIS_CIRCLE_RADIUS = 48.0f;
	static const float OUTER_AXIS_CIRCLE_RADIUS = 56.0f;
	static const float ROTATION_TEXT_RADIUS = 75.0f;
}

// ============================================================
// 基础几何体构建
// ============================================================

void CoordinateAxisRenderFuncs::BuildTranslationAxisVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices, float CylinderRadius)
{
	const float AxisLength = 35.0f;
	const float HalfHeight = AxisLength * 0.5f;
	const FVector Offset(0, 0, HalfHeight);
	const float ConeHeadOffset = 12.0f;

	CoordinateAxisRenderFuncs::BuildCylinderVerts(Offset, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), CylinderRadius, HalfHeight, 16, OutVerts, OutIndices);
	CoordinateAxisRenderFuncs::BuildConeVerts(FMath::DegreesToRadians(PI * 5), FMath::DegreesToRadians(PI * 5), 13.f, FVector(0.f, 0.f, AxisLength + ConeHeadOffset), 32, OutVerts, OutIndices);
}

void CoordinateAxisRenderFuncs::BuildCylinderVerts(const FVector& Base, const FVector& XAxis, const FVector& YAxis, const FVector& ZAxis, float Radius, float HalfHeight, uint32 Sides, TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices)
{
	const float AngleDelta = 2.0f * PI / Sides;
	FVector LastVertex = Base + XAxis * Radius;

	FVector2D TC = FVector2D(0.0f, 0.0f);
	float TCStep = 1.0f / Sides;

	FVector TopOffset = HalfHeight * ZAxis;
	int32 BaseVertIndex = OutVerts.Num();

	for (uint32 SideIndex = 0; SideIndex < Sides; SideIndex++)
	{
		const FVector Vertex = Base + (XAxis * FMath::Cos(AngleDelta * (SideIndex + 1)) + YAxis * FMath::Sin(AngleDelta * (SideIndex + 1))) * Radius;
		FVector Normal = Vertex - Base;
		Normal.Normalize();

		FDynamicMeshVertex MeshVertex;
		MeshVertex.Position = Vertex - TopOffset;
		MeshVertex.TextureCoordinate[0] = TC;
		MeshVertex.SetTangents(-ZAxis, (-ZAxis) ^ Normal, Normal);
		OutVerts.Add(MeshVertex);

		LastVertex = Vertex;
		TC.X += TCStep;
	}

	LastVertex = Base + XAxis * Radius;
	TC = FVector2D(0.0f, 1.0f);

	for (uint32 SideIndex = 0; SideIndex < Sides; SideIndex++)
	{
		const FVector Vertex = Base + (XAxis * FMath::Cos(AngleDelta * (SideIndex + 1)) + YAxis * FMath::Sin(AngleDelta * (SideIndex + 1))) * Radius;
		FVector Normal = Vertex - Base;
		Normal.Normalize();

		FDynamicMeshVertex MeshVertex;
		MeshVertex.Position = Vertex + TopOffset;
		MeshVertex.TextureCoordinate[0] = TC;
		MeshVertex.SetTangents(-ZAxis, (-ZAxis) ^ Normal, Normal);
		OutVerts.Add(MeshVertex);

		LastVertex = Vertex;
		TC.X += TCStep;
	}

	for (uint32 SideIndex = 1; SideIndex < Sides; SideIndex++)
	{
		int32 V0 = BaseVertIndex;
		int32 V1 = BaseVertIndex + SideIndex;
		int32 V2 = BaseVertIndex + ((SideIndex + 1) % Sides);

		OutIndices.Add(V0); OutIndices.Add(V1); OutIndices.Add(V2);
		OutIndices.Add(Sides + V2); OutIndices.Add(Sides + V1); OutIndices.Add(Sides + V0);
	}

	for (uint32 SideIndex = 0; SideIndex < Sides; SideIndex++)
	{
		int32 V0 = BaseVertIndex + SideIndex;
		int32 V1 = BaseVertIndex + ((SideIndex + 1) % Sides);
		int32 V2 = V0 + Sides;
		int32 V3 = V1 + Sides;

		OutIndices.Add(V0); OutIndices.Add(V2); OutIndices.Add(V1);
		OutIndices.Add(V2); OutIndices.Add(V3); OutIndices.Add(V1);
	}
}

void CoordinateAxisRenderFuncs::BuildConeVerts(float Angle1, float Angle2, float Scale, FVector Offset, uint32 NumSides, TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices)
{
	TArray<FVector> ConeVerts;
	ConeVerts.AddUninitialized(NumSides);

	for (uint32 i = 0; i < NumSides; i++)
	{
		float Fraction = (float)i / (float)(NumSides);
		float Azi = 2.f * PI * Fraction;
		ConeVerts[i] = (FTranslationMatrix(CalcConeVert(Angle1, Angle2, Azi)) * FRotationMatrix(FRotator(-90.f, 0.f, 0.f))).GetOrigin() * Scale + Offset;
	}

	for (uint32 i = 0; i < NumSides; i++)
	{
		FVector TriTangentZ = ConeVerts[(i + 1) % NumSides] ^ ConeVerts[i];
		FVector TriTangentY = ConeVerts[i];
		FVector TriTangentX = TriTangentZ ^ TriTangentY;

		FDynamicMeshVertex V0, V1, V2;
		V0.Position = Offset;
		V0.TextureCoordinate[0].X = 0.0f;
		V0.TextureCoordinate[0].Y = (float)i / NumSides;
		V0.SetTangents(TriTangentX, TriTangentY, FVector(-1, 0, 0));
		int32 I0 = OutVerts.Add(V0);

		V1.Position = ConeVerts[i];
		V1.TextureCoordinate[0].X = 1.0f;
		V1.TextureCoordinate[0].Y = (float)i / NumSides;
		FVector TriTangentZPrev = ConeVerts[i] ^ ConeVerts[i == 0 ? NumSides - 1 : i - 1];
		V1.SetTangents(TriTangentX, TriTangentY, (TriTangentZPrev + TriTangentZ).GetSafeNormal());
		int32 I1 = OutVerts.Add(V1);

		V2.Position = ConeVerts[(i + 1) % NumSides];
		V2.TextureCoordinate[0].X = 1.0f;
		V2.TextureCoordinate[0].Y = (float)((i + 1) % NumSides) / NumSides;
		FVector TriTangentZNext = ConeVerts[(i + 2) % NumSides] ^ ConeVerts[(i + 1) % NumSides];
		V2.SetTangents(TriTangentX, TriTangentY, (TriTangentZNext + TriTangentZ).GetSafeNormal());
		int32 I2 = OutVerts.Add(V2);

		if (Scale >= 0.f)
		{
			OutIndices.Add(I0); OutIndices.Add(I1); OutIndices.Add(I2);
		}
		else
		{
			OutIndices.Add(I0); OutIndices.Add(I2); OutIndices.Add(I1);
		}
	}
}

FVector CoordinateAxisRenderFuncs::CalcConeVert(float Angle1, float Angle2, float AzimuthAngle)
{
	float ang1 = FMath::Clamp<float>(Angle1, 0.01f, (float)PI - 0.01f);
	float ang2 = FMath::Clamp<float>(Angle2, 0.01f, (float)PI - 0.01f);

	float sinX_2 = FMath::Sin(0.5f * ang1);
	float sinY_2 = FMath::Sin(0.5f * ang2);

	float sinSqX_2 = sinX_2 * sinX_2;
	float sinSqY_2 = sinY_2 * sinY_2;

	float phi = FMath::Atan2(FMath::Sin(AzimuthAngle) * sinY_2, FMath::Cos(AzimuthAngle) * sinX_2);
	float sinPhi = FMath::Sin(phi);
	float cosPhi = FMath::Cos(phi);
	float sinSqPhi = sinPhi * sinPhi;
	float cosSqPhi = cosPhi * cosPhi;

	float rSq = sinSqX_2 * sinSqY_2 / (sinSqX_2 * sinSqPhi + sinSqY_2 * cosSqPhi);
	float r = FMath::Sqrt(rSq);
	float Sqr = FMath::Sqrt(1 - rSq);
	float alpha = r * cosPhi;
	float beta = r * sinPhi;

	FVector ConeVert;
	ConeVert.X = (1 - 2 * rSq);
	ConeVert.Y = 2 * Sqr * alpha;
	ConeVert.Z = 2 * Sqr * beta;

	return ConeVert;
}

void CoordinateAxisRenderFuncs::BuildSphereVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices)
{
	int32 NumSides = 10;
	int32 NumRings = 5;
	int32 NumVerts = (NumSides + 1) * (NumRings + 1);
	FDynamicMeshVertex* Verts = (FDynamicMeshVertex*)FMemory::Malloc(NumVerts * sizeof(FDynamicMeshVertex));
	FDynamicMeshVertex* ArcVerts = (FDynamicMeshVertex*)FMemory::Malloc((NumRings + 1) * sizeof(FDynamicMeshVertex));

	for (int32 i = 0; i < NumRings + 1; i++)
	{
		FDynamicMeshVertex* ArcVert = &ArcVerts[i];
		float angle = ((float)i / NumRings) * PI;
		ArcVert->Position.X = 0.0f;
		ArcVert->Position.Y = FMath::Sin(angle);
		ArcVert->Position.Z = FMath::Cos(angle);
		ArcVert->SetTangents(FVector(1, 0, 0), FVector(0.0f, -ArcVert->Position.Z, ArcVert->Position.Y), ArcVert->Position);
		ArcVert->TextureCoordinate[0].X = 0.0f;
		ArcVert->TextureCoordinate[0].Y = ((float)i / NumRings);
	}

	for (int32 s = 0; s < NumSides + 1; s++)
	{
		FRotator ArcRotator(0, 360.f * (float)s / NumSides, 0);
		FRotationMatrix ArcRot(ArcRotator);
		float XTexCoord = ((float)s / NumSides);

		for (int32 v = 0; v < NumRings + 1; v++)
		{
			int32 VIx = (NumRings + 1) * s + v;
			Verts[VIx].Position = ArcRot.TransformPosition(ArcVerts[v].Position);
			Verts[VIx].SetTangents(
				ArcRot.TransformVector(ArcVerts[v].TangentX.ToFVector()),
				ArcRot.TransformVector(ArcVerts[v].GetTangentY()),
				ArcRot.TransformVector(ArcVerts[v].TangentZ.ToFVector()));
			Verts[VIx].TextureCoordinate[0].X = XTexCoord;
			Verts[VIx].TextureCoordinate[0].Y = ArcVerts[v].TextureCoordinate[0].Y;
		}
	}

	for (int32 VertIdx = 0; VertIdx < NumVerts; VertIdx++)
	{
		OutVerts.Add(Verts[VertIdx]);
	}

	for (int32 s = 0; s < NumSides; s++)
	{
		int32 a0start = (s + 0) * (NumRings + 1);
		int32 a1start = (s + 1) * (NumRings + 1);

		for (int32 r = 0; r < NumRings; r++)
		{
			OutIndices.Add(a0start + r + 0);
			OutIndices.Add(a1start + r + 0);
			OutIndices.Add(a0start + r + 1);

			OutIndices.Add(a1start + r + 0);
			OutIndices.Add(a1start + r + 1);
			OutIndices.Add(a0start + r + 1);
		}
	}

	FMemory::Free(Verts);
	FMemory::Free(ArcVerts);
}

void CoordinateAxisRenderFuncs::BuildCornerVerts(float RenderBoxScale, TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices)
{
	float TH = RenderBoxScale * 1.2f;
	float TX = RenderBoxScale * 12.f / 2;
	float TY = RenderBoxScale * 1.2f / 2;
	float TZ = RenderBoxScale * 12.f / 2;

	// Top
	{
		int32 VI[4];
		VI[0] = OutVerts.Add(AddVertex(FVector(-TX, -TY, +TZ), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FColor::White));
		VI[1] = OutVerts.Add(AddVertex(FVector(-TX, +TY, +TZ), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FColor::White));
		VI[2] = OutVerts.Add(AddVertex(FVector(+TX, +TY, +TZ), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FColor::White));
		VI[3] = OutVerts.Add(AddVertex(FVector(+TX, -TY, +TZ), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FColor::White));
		OutIndices.Add(VI[0]); OutIndices.Add(VI[1]); OutIndices.Add(VI[2]);
		OutIndices.Add(VI[0]); OutIndices.Add(VI[2]); OutIndices.Add(VI[3]);
	}

	// Left
	{
		int32 VI[4];
		VI[0] = OutVerts.Add(AddVertex(FVector(-TX, -TY, TZ - TH), FVector2D::ZeroVector, FVector(0, 0, 1), FVector(0, 1, 0), FVector(-1, 0, 0), FColor::White));
		VI[1] = OutVerts.Add(AddVertex(FVector(-TX, -TY, TZ), FVector2D::ZeroVector, FVector(0, 0, 1), FVector(0, 1, 0), FVector(-1, 0, 0), FColor::White));
		VI[2] = OutVerts.Add(AddVertex(FVector(-TX, +TY, TZ), FVector2D::ZeroVector, FVector(0, 0, 1), FVector(0, 1, 0), FVector(-1, 0, 0), FColor::White));
		VI[3] = OutVerts.Add(AddVertex(FVector(-TX, +TY, TZ - TH), FVector2D::ZeroVector, FVector(0, 0, 1), FVector(0, 1, 0), FVector(-1, 0, 0), FColor::White));
		OutIndices.Add(VI[0]); OutIndices.Add(VI[1]); OutIndices.Add(VI[2]);
		OutIndices.Add(VI[0]); OutIndices.Add(VI[2]); OutIndices.Add(VI[3]);
	}

	// Front
	{
		int32 VI[5];
		VI[0] = OutVerts.Add(AddVertex(FVector(-TX, +TY, TZ - TH), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 0, -1), FVector(0, 1, 0), FColor::White));
		VI[1] = OutVerts.Add(AddVertex(FVector(-TX, +TY, +TZ), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 0, -1), FVector(0, 1, 0), FColor::White));
		VI[2] = OutVerts.Add(AddVertex(FVector(+TX - TH, +TY, +TX), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 0, -1), FVector(0, 1, 0), FColor::White));
		VI[3] = OutVerts.Add(AddVertex(FVector(+TX, +TY, +TZ), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 0, -1), FVector(0, 1, 0), FColor::White));
		VI[4] = OutVerts.Add(AddVertex(FVector(+TX - TH, +TY, TZ - TH), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 0, -1), FVector(0, 1, 0), FColor::White));
		OutIndices.Add(VI[0]); OutIndices.Add(VI[1]); OutIndices.Add(VI[2]);
		OutIndices.Add(VI[0]); OutIndices.Add(VI[2]); OutIndices.Add(VI[4]);
		OutIndices.Add(VI[4]); OutIndices.Add(VI[2]); OutIndices.Add(VI[3]);
	}

	// Back
	{
		int32 VI[5];
		VI[0] = OutVerts.Add(AddVertex(FVector(-TX, -TY, TZ - TH), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 0, 1), FVector(0, -1, 0), FColor::White));
		VI[1] = OutVerts.Add(AddVertex(FVector(-TX, -TY, +TZ), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 0, 1), FVector(0, -1, 0), FColor::White));
		VI[2] = OutVerts.Add(AddVertex(FVector(+TX - TH, -TY, +TX), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 0, 1), FVector(0, -1, 0), FColor::White));
		VI[3] = OutVerts.Add(AddVertex(FVector(+TX, -TY, +TZ), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 0, 1), FVector(0, -1, 0), FColor::White));
		VI[4] = OutVerts.Add(AddVertex(FVector(+TX - TH, -TY, TZ - TH), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 0, 1), FVector(0, -1, 0), FColor::White));
		OutIndices.Add(VI[0]); OutIndices.Add(VI[1]); OutIndices.Add(VI[2]);
		OutIndices.Add(VI[0]); OutIndices.Add(VI[2]); OutIndices.Add(VI[4]);
		OutIndices.Add(VI[4]); OutIndices.Add(VI[2]); OutIndices.Add(VI[3]);
	}

	// Bottom
	{
		int32 VI[4];
		VI[0] = OutVerts.Add(AddVertex(FVector(-TX, -TY, TZ - TH), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 0, -1), FVector(0, 0, 1), FColor::White));
		VI[1] = OutVerts.Add(AddVertex(FVector(-TX, +TY, TZ - TH), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 0, -1), FVector(0, 0, 1), FColor::White));
		VI[2] = OutVerts.Add(AddVertex(FVector(+TX - TH, +TY, TZ - TH), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 0, -1), FVector(0, 0, 1), FColor::White));
		VI[3] = OutVerts.Add(AddVertex(FVector(+TX - TH, -TY, TZ - TH), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 0, -1), FVector(0, 0, 1), FColor::White));
		OutIndices.Add(VI[0]); OutIndices.Add(VI[1]); OutIndices.Add(VI[2]);
		OutIndices.Add(VI[0]); OutIndices.Add(VI[2]); OutIndices.Add(VI[3]);
	}
}

void CoordinateAxisRenderFuncs::BuildSquareVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices)
{
	FVector Positions[4] = {
		FVector(0, 0, 0), FVector(0, 0, 1), FVector(1, 0, 1), FVector(1, 0, 0)
	};

	int32 VI[4];
	for (int32 i = 0; i < 4; i++)
	{
		VI[i] = OutVerts.Add(AddVertex((Positions[i] * 12.f), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FColor::White));
	}

	OutIndices.Add(VI[0]); OutIndices.Add(VI[1]); OutIndices.Add(VI[2]);
	OutIndices.Add(VI[0]); OutIndices.Add(VI[2]); OutIndices.Add(VI[3]);
}

void CoordinateAxisRenderFuncs::BuildScaleAxisVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices)
{
	const float AxisLength = 35.0f;
	const float HalfHeight = AxisLength * 0.5f;
	const FVector Offset(0, 0, HalfHeight);
	const float ConeHeadOffset = 1.0f;

	CoordinateAxisRenderFuncs::BuildCylinderVerts(Offset, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), 1.2f, HalfHeight, 16, OutVerts, OutIndices);
	CoordinateAxisRenderFuncs::BuildCubeVerts(4.f, FVector(0.f, 0.f, AxisLength + ConeHeadOffset), OutVerts, OutIndices);
}

void CoordinateAxisRenderFuncs::BuildCubeVerts(float Scale, FVector Offset, TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices)
{
	FVector Positions[4] = {
		FVector(-1, -1, +1), FVector(-1, +1, +1), FVector(+1, +1, +1), FVector(+1, -1, +1)
	};
	FVector2D UVs[4] = {
		FVector2D(0,0), FVector2D(0,1), FVector2D(1,1), FVector2D(1,0),
	};

	FRotator FaceRotations[6];
	FaceRotations[0] = FRotator(0, 0, 0);
	FaceRotations[1] = FRotator(90.f, 0, 0);
	FaceRotations[2] = FRotator(-90.f, 0, 0);
	FaceRotations[3] = FRotator(0, 0, 90.f);
	FaceRotations[4] = FRotator(0, 0, -90.f);
	FaceRotations[5] = FRotator(180.f, 0, 0);

	for (int32 f = 0; f < 6; f++)
	{
		FMatrix FaceTransform = FRotationMatrix(FaceRotations[f]);
		int32 VI[4];
		for (int32 i = 0; i < 4; i++)
		{
			VI[i] = OutVerts.Add(AddVertex(
				FaceTransform.TransformPosition(Positions[i] * Scale) + Offset,
				UVs[i],
				FaceTransform.TransformVector(FVector(1, 0, 0)),
				FaceTransform.TransformVector(FVector(0, 1, 0)),
				FaceTransform.TransformVector(FVector(0, 0, 1)),
				FColor::White));
		}
		OutIndices.Add(VI[0]); OutIndices.Add(VI[1]); OutIndices.Add(VI[2]);
		OutIndices.Add(VI[0]); OutIndices.Add(VI[2]); OutIndices.Add(VI[3]);
	}
}

void CoordinateAxisRenderFuncs::BuildTriangleVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices)
{
	FVector Positions[3] = { FVector(0, 0, 0), FVector(0, 0, 2), FVector(2, 0, 0) };
	int32 VI[3];
	for (int32 i = 0; i < 3; i++)
	{
		VI[i] = OutVerts.Add(AddVertex((Positions[i] * 12.f), FVector2D::ZeroVector, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FColor::White));
	}
	OutIndices.Add(VI[0]); OutIndices.Add(VI[1]); OutIndices.Add(VI[2]);
}

void CoordinateAxisRenderFuncs::BuildThickArcVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices, const FVector& Axis0, const FVector& Axis1, const float StartAngle, const float EndAngle, float OuterRadius, float InnerRadius, const FColor& Color)
{
	const int32 CircleSides = 24;
	const int32 NumPoints = FMath::TruncToInt(CircleSides * (EndAngle - StartAngle) / (PI / 2)) + 1;

	FColor TriangleColor = Color;
	FColor RingColor = Color;
	RingColor.A = MAX_uint8;

	FVector ZAxis = Axis0 ^ Axis1;

	for (int32 RadiusIndex = 0; RadiusIndex < 2; ++RadiusIndex)
	{
		float Radius = (RadiusIndex == 0) ? OuterRadius : InnerRadius;
		float TCRadius = Radius / (float)OuterRadius;
		for (int32 VertexIndex = 0; VertexIndex <= NumPoints; VertexIndex++)
		{
			float Percent = VertexIndex / (float)NumPoints;
			float Angle = FMath::Lerp(StartAngle, EndAngle, Percent);
			float AngleDeg = FRotator::ClampAxis(Angle * 180.f / PI);

			FVector VertexDir = Axis0.RotateAngleAxis(AngleDeg, ZAxis);
			VertexDir.Normalize();

			float TCAngle = Percent * (PI / 2);
			FVector2D TC(TCRadius * FMath::Cos(Angle), TCRadius * FMath::Sin(Angle));

			const FVector VertexPosition = VertexDir * Radius;
			FVector Normal = VertexPosition;
			Normal.Normalize();

			FDynamicMeshVertex MeshVertex;
			MeshVertex.Position = VertexPosition;
			MeshVertex.Color = TriangleColor;
			MeshVertex.TextureCoordinate[0] = TC;
			MeshVertex.SetTangents(-ZAxis, (-ZAxis) ^ Normal, Normal);
			OutVerts.Add(MeshVertex);
		}
	}

	int32 InnerVertexStartIndex = NumPoints + 1;
	for (int32 VertexIndex = 0; VertexIndex < NumPoints; VertexIndex++)
	{
		OutIndices.Add(VertexIndex);
		OutIndices.Add(VertexIndex + 1);
		OutIndices.Add(InnerVertexStartIndex + VertexIndex);

		OutIndices.Add(VertexIndex + 1);
		OutIndices.Add(InnerVertexStartIndex + VertexIndex + 1);
		OutIndices.Add(InnerVertexStartIndex + VertexIndex);
	}
}

void CoordinateAxisRenderFuncs::BuildStartStopMarkerVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices, const FVector& Axis0, const FVector& Axis1, const float Angle, const FColor& Color)
{
	const float ArrowHeightPercent = 0.8f;
	const float InnerDistance = 48.0f;
	const float OuterDistance = 56.0f;
	const float RingHeight = OuterDistance - InnerDistance;
	const float ArrowHeight = RingHeight * ArrowHeightPercent;
	const float ThirtyDegrees = PI / 6.0f;
	const float HalfArrowidth = ArrowHeight * FMath::Tan(ThirtyDegrees);

	FVector ZAxis = Axis0 ^ Axis1;
	FVector RotatedAxis0 = Axis0.RotateAngleAxis(Angle, ZAxis);
	FVector RotatedAxis1 = Axis1.RotateAngleAxis(Angle, ZAxis);

	FVector Vertices[3];
	Vertices[0] = OuterDistance * RotatedAxis0;
	Vertices[1] = Vertices[0] + (ArrowHeight)*RotatedAxis0 - HalfArrowidth * RotatedAxis1;
	Vertices[2] = Vertices[1] + (2 * HalfArrowidth) * RotatedAxis1;

	for (int32 i = 0; i < 3; i++)
	{
		FDynamicMeshVertex MeshVertex;
		MeshVertex.Position = Vertices[i];
		MeshVertex.Color = Color;
		MeshVertex.TextureCoordinate[0] = FVector2D(0.0f, 0.0f);
		MeshVertex.SetTangents(RotatedAxis0, RotatedAxis1, (RotatedAxis0) ^ RotatedAxis1);
		OutVerts.Add(MeshVertex);
	}

	OutIndices.Add(0); OutIndices.Add(1); OutIndices.Add(2);
}

void CoordinateAxisRenderFuncs::BuildSnapMarkerVerts(TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices, const FVector& Axis0, const FVector& Axis1, const float WidthPercent, const float PercentSize, const FColor& Color)
{
	const float InnerDistance = 48.f;
	const float OuterDistance = 56.f;
	const float MaxMarkerHeight = OuterDistance - InnerDistance;
	const float MarkerWidth = MaxMarkerHeight * WidthPercent;
	const float MarkerHeight = MaxMarkerHeight * PercentSize;

	FVector Vertices[4];
	Vertices[0] = (OuterDistance)*Axis0 - (MarkerWidth * 0.5f) * Axis1;
	Vertices[1] = Vertices[0] + (MarkerWidth)*Axis1;
	Vertices[2] = (OuterDistance - MarkerHeight) * Axis0 - (MarkerWidth * 0.5f) * Axis1;
	Vertices[3] = Vertices[2] + (MarkerWidth)*Axis1;

	if (WidthPercent > 0.0f)
	{
		for (int32 i = 0; i < 4; i++)
		{
			FDynamicMeshVertex MeshVertex;
			MeshVertex.Position = Vertices[i];
			MeshVertex.Color = Color;
			MeshVertex.TextureCoordinate[0] = FVector2D(0.0f, 0.0f);
			MeshVertex.SetTangents(Axis0, Axis1, (Axis0) ^ Axis1);
			OutVerts.Add(MeshVertex);
		}

		OutIndices.Add(0); OutIndices.Add(1); OutIndices.Add(2);
		OutIndices.Add(1); OutIndices.Add(3); OutIndices.Add(2);
	}
}

// ============================================================
// 碰撞数据辅助
// ============================================================

namespace
{
	static void AddVertsToCollision(
		const TArray<FDynamicMeshVertex>& MeshVerts,
		const TArray<uint32>& MeshIndices,
		const FMatrix& Transform,
		FTriMeshCollisionData& CollisionData,
		int32& InOutFaceCount)
	{
		int32 CollisionNum = CollisionData.Vertices.Num();

		for (const FDynamicMeshVertex& Vertex : MeshVerts)
		{
			CollisionData.Vertices.Add(Transform.TransformPosition(Vertex.Position));
		}

		for (int32 i = 0; i < MeshIndices.Num(); i += 3)
		{
			FTriIndices TriIndices;
			TriIndices.v0 = CollisionNum + MeshIndices[i + 0];
			TriIndices.v1 = CollisionNum + MeshIndices[i + 1];
			TriIndices.v2 = CollisionNum + MeshIndices[i + 2];
			CollisionData.Indices.Add(TriIndices);
			InOutFaceCount += 1;
		}
	}

	static void CacheRotationHUDText(UCoordinateAxisComponent* InComp, const FSceneView* InView, const FVector& Axis0, const FVector& Axis1, const float AngleOfChange, const float InScale)
	{
		// 此功能由 Component 的 TickWidgetComponent 处理
	}
}

// ============================================================
// Translation 渲染 & 碰撞
// ============================================================

void CoordinateAxisRenderFuncs::RenderTranslation(UCoordinateAxisComponent* InComp, const FSceneView* InView, FPrimitiveDrawInterface* InPDI, FMeshElementCollector& InCollector, const FMatrix& InMatrix)
{
	float RenderBoxScale = InComp->RenderBoxScale;

	// 主轴（圆柱 + 锥形箭头）
	{
		TArray<uint32> MeshIndices;
		TArray<FDynamicMeshVertex> MeshVerts;
		CoordinateAxisRenderFuncs::BuildTranslationAxisVerts(MeshVerts, MeshIndices);

		// X
		{
			UMaterialInstanceDynamic* XMaterial = (InComp->GetFocusAxis() & EAxisType::EAT_X ? InComp->FocusAxisMaterial : InComp->AxisMaterialX);
			FDynamicMeshBuilder MeshBuilder(InView->GetFeatureLevel());
			MeshBuilder.AddVertices(MeshVerts);
			MeshBuilder.AddTriangles(MeshIndices);
			MeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(-90.f, 0.f, 0.f)) * InMatrix, XMaterial->GetRenderProxy(), SDPG_Foreground, false, false, 0, InCollector);
		}

		// Y
		{
			UMaterialInstanceDynamic* YMaterial = (InComp->GetFocusAxis() & EAxisType::EAT_Y ? InComp->FocusAxisMaterial : InComp->AxisMaterialY);
			FDynamicMeshBuilder MeshBuilder(InView->GetFeatureLevel());
			MeshBuilder.AddVertices(MeshVerts);
			MeshBuilder.AddTriangles(MeshIndices);
			MeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(0.f, 0.f, -90.f)) * InMatrix, YMaterial->GetRenderProxy(), SDPG_Foreground, false, false, 0, InCollector);
		}

		// Z
		{
			UMaterialInstanceDynamic* ZMaterial = (InComp->GetFocusAxis() & EAxisType::EAT_Z ? InComp->FocusAxisMaterial : InComp->AxisMaterialZ);
			FDynamicMeshBuilder MeshBuilder(InView->GetFeatureLevel());
			MeshBuilder.AddVertices(MeshVerts);
			MeshBuilder.AddTriangles(MeshIndices);
			MeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(0.f, 0.f, 0.f)) * InMatrix, ZMaterial->GetRenderProxy(), SDPG_Foreground, false, false, 0, InCollector);
		}
	}

	// 拐角 和 平面
	{
		TArray<uint32> CornerMeshIndices;
		TArray<FDynamicMeshVertex> CornerMeshVerts;
		CoordinateAxisRenderFuncs::BuildCornerVerts(RenderBoxScale, CornerMeshVerts, CornerMeshIndices);

		TArray<uint32> SquareMeshIndices;
		TArray<FDynamicMeshVertex> SquareMeshVerts;
		CoordinateAxisRenderFuncs::BuildSquareVerts(SquareMeshVerts, SquareMeshIndices);

		// XY
		{
			UMaterialInstanceDynamic* XMaterial = ((InComp->GetFocusAxis() & EAxisType::EAT_XY) == EAxisType::EAT_XY ? InComp->FocusAxisMaterial : InComp->AxisMaterialX);
			UMaterialInstanceDynamic* YMaterial = ((InComp->GetFocusAxis() & EAxisType::EAT_XY) == EAxisType::EAT_XY ? InComp->FocusAxisMaterial : InComp->AxisMaterialY);
			UMaterialInstanceDynamic* PMaterial = ((InComp->GetFocusAxis() & EAxisType::EAT_XY) == EAxisType::EAT_XY ? InComp->FocusPlaneMaterial : InComp->PlaneMaterial);

			FDynamicMeshBuilder MB1(InView->GetFeatureLevel());
			MB1.AddVertices(CornerMeshVerts); MB1.AddTriangles(CornerMeshIndices);
			MB1.GetMesh(FScaleMatrix(FVector(-1, 1, 1)) * FRotationMatrix(FRotator(-90, 0, 0)) * FTranslationMatrix(FVector(RenderBoxScale * FVector(7, 0, 7))) * FRotationMatrix(FRotator(0, 0, -90)) * InMatrix, XMaterial->GetRenderProxy(), SDPG_Foreground, false, false, 0, InCollector);

			FDynamicMeshBuilder MB2(InView->GetFeatureLevel());
			MB2.AddVertices(CornerMeshVerts); MB2.AddTriangles(CornerMeshIndices);
			MB2.GetMesh(FTranslationMatrix(FVector(RenderBoxScale * FVector(7, 0, 7))) * FRotationMatrix(FRotator(0, 0, -90)) * InMatrix, YMaterial->GetRenderProxy(), SDPG_Foreground, false, false, 0, InCollector);

			FDynamicMeshBuilder MB3(InView->GetFeatureLevel());
			MB3.AddVertices(SquareMeshVerts); MB3.AddTriangles(SquareMeshIndices);
			MB3.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(0, 0, -90)) * InMatrix, PMaterial->GetRenderProxy(), SDPG_Foreground, false, false, 0, InCollector);
		}

		// XZ
		{
			UMaterialInstanceDynamic* XMaterial = ((InComp->GetFocusAxis() & EAxisType::EAT_XZ) == EAxisType::EAT_XZ ? InComp->FocusAxisMaterial : InComp->AxisMaterialX);
			UMaterialInstanceDynamic* ZMaterial = ((InComp->GetFocusAxis() & EAxisType::EAT_XZ) == EAxisType::EAT_XZ ? InComp->FocusAxisMaterial : InComp->AxisMaterialZ);
			UMaterialInstanceDynamic* PMaterial = ((InComp->GetFocusAxis() & EAxisType::EAT_XZ) == EAxisType::EAT_XZ ? InComp->FocusPlaneMaterial : InComp->PlaneMaterial);

			FDynamicMeshBuilder MB1(InView->GetFeatureLevel());
			MB1.AddVertices(CornerMeshVerts); MB1.AddTriangles(CornerMeshIndices);
			MB1.GetMesh(FScaleMatrix(FVector(-1, 1, 1)) * FRotationMatrix(FRotator(-90, 0, 0)) * FTranslationMatrix(FVector(RenderBoxScale * FVector(7, 0, 7))) * FRotationMatrix(FRotator(0, 0, 0)) * InMatrix, XMaterial->GetRenderProxy(), SDPG_Foreground, false, false, 0, InCollector);

			FDynamicMeshBuilder MB2(InView->GetFeatureLevel());
			MB2.AddVertices(CornerMeshVerts); MB2.AddTriangles(CornerMeshIndices);
			MB2.GetMesh(FTranslationMatrix(FVector(RenderBoxScale * FVector(7, 0, 7))) * FRotationMatrix(FRotator(0, 0, 0)) * InMatrix, ZMaterial->GetRenderProxy(), SDPG_Foreground, false, false, 0, InCollector);

			FDynamicMeshBuilder MB3(InView->GetFeatureLevel());
			MB3.AddVertices(SquareMeshVerts); MB3.AddTriangles(SquareMeshIndices);
			MB3.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(0, 0, 0)) * InMatrix, PMaterial->GetRenderProxy(), SDPG_Foreground, false, false, 0, InCollector);
		}

		// YZ
		{
			UMaterialInstanceDynamic* YMaterial = ((InComp->GetFocusAxis() & EAxisType::EAT_YZ) == EAxisType::EAT_YZ ? InComp->FocusAxisMaterial : InComp->AxisMaterialY);
			UMaterialInstanceDynamic* ZMaterial = ((InComp->GetFocusAxis() & EAxisType::EAT_YZ) == EAxisType::EAT_YZ ? InComp->FocusAxisMaterial : InComp->AxisMaterialZ);
			UMaterialInstanceDynamic* PMaterial = ((InComp->GetFocusAxis() & EAxisType::EAT_YZ) == EAxisType::EAT_YZ ? InComp->FocusPlaneMaterial : InComp->PlaneMaterial);

			FDynamicMeshBuilder MB1(InView->GetFeatureLevel());
			MB1.AddVertices(CornerMeshVerts); MB1.AddTriangles(CornerMeshIndices);
			MB1.GetMesh(FScaleMatrix(FVector(-1, 1, 1)) * FRotationMatrix(FRotator(-90, 0, 0)) * FTranslationMatrix(FVector(RenderBoxScale * FVector(7, 0, 7))) * FRotationMatrix(FRotator(0, -90, 0)) * InMatrix, YMaterial->GetRenderProxy(), SDPG_Foreground, false, false, 0, InCollector);

			FDynamicMeshBuilder MB2(InView->GetFeatureLevel());
			MB2.AddVertices(CornerMeshVerts); MB2.AddTriangles(CornerMeshIndices);
			MB2.GetMesh(FTranslationMatrix(FVector(RenderBoxScale * FVector(7, 0, 7))) * FRotationMatrix(FRotator(0, -90, 0)) * InMatrix, ZMaterial->GetRenderProxy(), SDPG_Foreground, false, false, 0, InCollector);

			FDynamicMeshBuilder MB3(InView->GetFeatureLevel());
			MB3.AddVertices(SquareMeshVerts); MB3.AddTriangles(SquareMeshIndices);
			MB3.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(0, -90, 0)) * InMatrix, PMaterial->GetRenderProxy(), SDPG_Foreground, false, false, 0, InCollector);
		}
	}

	// 原点球体
	{
		TArray<uint32> MeshIndices;
		TArray<FDynamicMeshVertex> MeshVerts;
		CoordinateAxisRenderFuncs::BuildSphereVerts(MeshVerts, MeshIndices);

		UMaterialInstanceDynamic* XYZMaterial = (InComp->GetFocusAxis() & EAxisType::EAT_Screen ? InComp->FocusAxisMaterial : InComp->AxisOriginMaterial);

		FDynamicMeshBuilder MeshBuilder(InView->GetFeatureLevel());
		MeshBuilder.AddVertices(MeshVerts);
		MeshBuilder.AddTriangles(MeshIndices);
		MeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale * 4.f)) * InMatrix, XYZMaterial->GetRenderProxy(), SDPG_Foreground, false, false, 0, InCollector);
	}
}

void CoordinateAxisRenderFuncs::BuildTranslationCollision(UCoordinateAxisComponent* InComp, const FMatrix& InMatrix, FTriMeshCollisionData& CollisionData)
{
	float RenderBoxScale = InComp->RenderBoxScale;

	// 主轴碰撞
	{
		TArray<uint32> MeshIndices;
		TArray<FDynamicMeshVertex> MeshVerts;
		CoordinateAxisRenderFuncs::BuildTranslationAxisVerts(MeshVerts, MeshIndices, 2);

		// X
		{
			FMatrix XForm = FTranslationMatrix(FVector::ZeroVector) * FScaleMatrix(FVector(RenderBoxScale * 1.2f)) * FRotationMatrix(FRotator(-90.f, 0.f, 0.f)) * InMatrix;
			AddVertsToCollision(MeshVerts, MeshIndices, XForm, CollisionData, InComp->RenderAxisVertexNum);
		}
		// Y
		{
			FMatrix YForm = FTranslationMatrix(FVector::ZeroVector) * FScaleMatrix(FVector(RenderBoxScale * 1.2f)) * FRotationMatrix(FRotator(0.f, 0.f, -90.f)) * InMatrix;
			AddVertsToCollision(MeshVerts, MeshIndices, YForm, CollisionData, InComp->RenderAxisVertexNum);
		}
		// Z
		{
			FMatrix ZForm = FTranslationMatrix(FVector::ZeroVector) * FScaleMatrix(FVector(RenderBoxScale * 1.2f)) * FRotationMatrix(FRotator(0.f, 0.f, 0.f)) * InMatrix;
			AddVertsToCollision(MeshVerts, MeshIndices, ZForm, CollisionData, InComp->RenderAxisVertexNum);
		}
	}

	// 拐角碰撞
	{
		TArray<uint32> CornerMeshIndices;
		TArray<FDynamicMeshVertex> CornerMeshVerts;
		CoordinateAxisRenderFuncs::BuildCornerVerts(RenderBoxScale, CornerMeshVerts, CornerMeshIndices);

		TArray<uint32> SquareMeshIndices;
		TArray<FDynamicMeshVertex> SquareMeshVerts;
		CoordinateAxisRenderFuncs::BuildSquareVerts(SquareMeshVerts, SquareMeshIndices);

		// XY
		{
			FMatrix Form1 = FTranslationMatrix(FVector::ZeroVector) * FScaleMatrix(FVector(-1, 1, 1)) * FRotationMatrix(FRotator(-90, 0, 0)) * FTranslationMatrix(FVector(RenderBoxScale * FVector(7, 0, 7))) * FRotationMatrix(FRotator(0, 0, -90)) * InMatrix;
			AddVertsToCollision(CornerMeshVerts, CornerMeshIndices, Form1, CollisionData, InComp->RenderDualAxisVertexNum);

			FMatrix Form2 = FTranslationMatrix(FVector::ZeroVector) * FTranslationMatrix(FVector(RenderBoxScale * FVector(7, 0, 7))) * FRotationMatrix(FRotator(0, 0, -90)) * InMatrix;
			AddVertsToCollision(CornerMeshVerts, CornerMeshIndices, Form2, CollisionData, InComp->RenderDualAxisVertexNum);

			FMatrix Form3 = FTranslationMatrix(FVector::ZeroVector) * FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(0, 0, -90)) * InMatrix;
			AddVertsToCollision(SquareMeshVerts, SquareMeshIndices, Form3, CollisionData, InComp->RenderDualAxisVertexNum);
		}
		// XZ
		{
			FMatrix Form1 = FTranslationMatrix(FVector::ZeroVector) * FScaleMatrix(FVector(-1, 1, 1)) * FRotationMatrix(FRotator(-90, 0, 0)) * FTranslationMatrix(FVector(RenderBoxScale * FVector(7, 0, 7))) * FRotationMatrix(FRotator(0, 0, 0)) * InMatrix;
			AddVertsToCollision(CornerMeshVerts, CornerMeshIndices, Form1, CollisionData, InComp->RenderDualAxisVertexNum);

			FMatrix Form2 = FTranslationMatrix(FVector::ZeroVector) * FTranslationMatrix(FVector(RenderBoxScale * FVector(7, 0, 7))) * FRotationMatrix(FRotator(0, 0, 0)) * InMatrix;
			AddVertsToCollision(CornerMeshVerts, CornerMeshIndices, Form2, CollisionData, InComp->RenderDualAxisVertexNum);

			FMatrix Form3 = FTranslationMatrix(FVector::ZeroVector) * FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(0, 0, 0)) * InMatrix;
			AddVertsToCollision(SquareMeshVerts, SquareMeshIndices, Form3, CollisionData, InComp->RenderDualAxisVertexNum);
		}
		// YZ
		{
			FMatrix Form1 = FTranslationMatrix(FVector::ZeroVector) * FScaleMatrix(FVector(-1, 1, 1)) * FRotationMatrix(FRotator(-90, 0, 0)) * FTranslationMatrix(FVector(RenderBoxScale * FVector(7, 0, 7))) * FRotationMatrix(FRotator(0, -90, 0)) * InMatrix;
			AddVertsToCollision(CornerMeshVerts, CornerMeshIndices, Form1, CollisionData, InComp->RenderDualAxisVertexNum);

			FMatrix Form2 = FTranslationMatrix(FVector::ZeroVector) * FTranslationMatrix(FVector(RenderBoxScale * FVector(7, 0, 7))) * FRotationMatrix(FRotator(0, -90, 0)) * InMatrix;
			AddVertsToCollision(CornerMeshVerts, CornerMeshIndices, Form2, CollisionData, InComp->RenderDualAxisVertexNum);

			FMatrix Form3 = FTranslationMatrix(FVector::ZeroVector) * FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(0, -90, 0)) * InMatrix;
			AddVertsToCollision(SquareMeshVerts, SquareMeshIndices, Form3, CollisionData, InComp->RenderDualAxisVertexNum);
		}
	}

	// 原点球体碰撞
	{
		TArray<uint32> MeshIndices;
		TArray<FDynamicMeshVertex> MeshVerts;
		CoordinateAxisRenderFuncs::BuildSphereVerts(MeshVerts, MeshIndices);

		FMatrix SphereForm = FTranslationMatrix(FVector::ZeroVector) * FScaleMatrix(FVector(RenderBoxScale * 4.f)) * InMatrix;
		AddVertsToCollision(MeshVerts, MeshIndices, SphereForm, CollisionData, InComp->RenderSceneVertexNum);
	}
}

// ============================================================
// Rotation 渲染 & 碰撞
// ============================================================

namespace
{
	static void RenderRotationArc(UCoordinateAxisComponent* InComp, EAxisType InAxis, const FSceneView* InView, FPrimitiveDrawInterface* InPDI, FMeshElementCollector& InCollector, const FVector& Axis0, const FVector& Axis1, const FVector& InDirectionToWidget, const FColor& InColor, const FMatrix& InMatrix, FVector2D& OutAxisDir)
	{
		float RenderBoxScale = InComp->RenderBoxScale;
		ECoordinateAxisMode CoorAxisMode = InComp->GetCoorAxisMode();
		EAxisType CoordinateFocusAxis = InComp->GetFocusAxis();
		EAxisType CoordinateRenderAxis = InComp->GetRenderAxis();
		bool bIsDragging = InComp->IsDragging();

		const uint8 LargeOuterAlpha = 0x7f;
		const uint8 SmallOuterAlpha = 0x0f;

		bool bIsPerspective = (InView->ViewMatrices.GetProjectionMatrix().M[3][3] < 1.0f);
		bool bIsOrtho = !bIsPerspective;

		FColor ArcColor = InColor;
		ArcColor.A = LargeOuterAlpha;

		if (bIsDragging)
		{
			if (CoordinateFocusAxis & InAxis)
			{
				float DeltaRotation = InComp->TotalDeltaRotation;
				float AdjustedDeltaRotation = -DeltaRotation;
				float AbsRotation = FRotator::ClampAxis(FMath::Abs(DeltaRotation));
				float AngleOfChangeRadians(AbsRotation * PI / 180.f);

				float StartAngle = AdjustedDeltaRotation < 0.0f ? -AngleOfChangeRadians : 0.0f;
				float FilledAngle = AngleOfChangeRadians;

				FVector ZAxis = Axis0 ^ Axis1;

				ArcColor.A = LargeOuterAlpha;
				{
					// 外圈填充弧
					TArray<uint32> MeshIndices;
					TArray<FDynamicMeshVertex> MeshVerts;
					CoordinateAxisRenderFuncs::BuildThickArcVerts(MeshVerts, MeshIndices, Axis0, Axis1, StartAngle, StartAngle + FilledAngle, 48.f, 56.f, ArcColor);

					FDynamicMeshBuilder MeshBuilder(InView->GetFeatureLevel());
					MeshBuilder.AddVertices(MeshVerts);
					MeshBuilder.AddTriangles(MeshIndices);
					MeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * InMatrix, InComp->ArcPlaneMaterial->GetRenderProxy(), SDPG_Foreground, false, false, 0, InCollector);

					for (int32 Index = 0; Index < MeshVerts.Num() - 1; Index++)
					{
						if (Index != ((MeshVerts.Num() * 0.5) - 1))
							InPDI->DrawLine((FTranslationMatrix(MeshVerts[Index].Position) * (FScaleMatrix(FVector(RenderBoxScale)) * InMatrix)).GetOrigin(), (FTranslationMatrix(MeshVerts[Index + 1].Position) * (FScaleMatrix(FVector(RenderBoxScale)) * InMatrix)).GetOrigin(), ArcColor, SDPG_Foreground);
					}

					// 内圈填充弧
					TArray<uint32> InnerMeshIndices;
					TArray<FDynamicMeshVertex> InnerMeshVerts;
					CoordinateAxisRenderFuncs::BuildThickArcVerts(InnerMeshVerts, InnerMeshIndices, Axis0, Axis1, StartAngle, StartAngle + FilledAngle, (CoorAxisMode == CAM_Mix) ? 20.f : 0.f, 48.f, ArcColor);

					FDynamicMeshBuilder InnerMeshBuilder(InView->GetFeatureLevel());
					InnerMeshBuilder.AddVertices(InnerMeshVerts);
					InnerMeshBuilder.AddTriangles(InnerMeshIndices);
					InnerMeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * InMatrix, InComp->ArcPlaneMaterial->GetRenderProxy(), SDPG_Foreground, false, false, 0, InCollector);

					for (int32 Index = 0; Index < InnerMeshVerts.Num() - 1; Index++)
					{
						if (Index != ((InnerMeshVerts.Num() * 0.5) - 1))
							InPDI->DrawLine((FTranslationMatrix(InnerMeshVerts[Index].Position) * (FScaleMatrix(FVector(RenderBoxScale)) * InMatrix)).GetOrigin(), (FTranslationMatrix(InnerMeshVerts[Index + 1].Position) * (FScaleMatrix(FVector(RenderBoxScale)) * InMatrix)).GetOrigin(), ArcColor, SDPG_Foreground);
					}
				}

				ArcColor.A = SmallOuterAlpha;
				{
					// 外圈剩余弧
					TArray<uint32> MeshIndices;
					TArray<FDynamicMeshVertex> MeshVerts;
					CoordinateAxisRenderFuncs::BuildThickArcVerts(MeshVerts, MeshIndices, Axis0, Axis1, StartAngle + FilledAngle, StartAngle + 2 * PI, 48.f, 56.f, ArcColor);

					FDynamicMeshBuilder MeshBuilder(InView->GetFeatureLevel());
					MeshBuilder.AddVertices(MeshVerts);
					MeshBuilder.AddTriangles(MeshIndices);
					MeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * InMatrix, InComp->ArcPlaneMaterial->GetRenderProxy(), SDPG_Foreground, false, false, 0, InCollector);

					// 内圈剩余弧
					TArray<uint32> InnerMeshIndices;
					TArray<FDynamicMeshVertex> InnerMeshVerts;
					CoordinateAxisRenderFuncs::BuildThickArcVerts(InnerMeshVerts, InnerMeshIndices, Axis0, Axis1, StartAngle + FilledAngle, StartAngle + 2 * PI, (CoorAxisMode == CAM_Mix) ? 20.f : 0.f, 48.f, ArcColor);

					FDynamicMeshBuilder InnerMeshBuilder(InView->GetFeatureLevel());
					InnerMeshBuilder.AddVertices(InnerMeshVerts);
					InnerMeshBuilder.AddTriangles(InnerMeshIndices);
					InnerMeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * InMatrix, InComp->ArcPlaneMaterial->GetRenderProxy(), SDPG_Foreground, false, false, 0, InCollector);
				}

				// 箭头
				{
					TArray<uint32> MeshIndices;
					TArray<FDynamicMeshVertex> MeshVerts;
					CoordinateAxisRenderFuncs::BuildStartStopMarkerVerts(MeshVerts, MeshIndices, Axis0, Axis1, AdjustedDeltaRotation, ArcColor);

					FDynamicMeshBuilder MeshBuilder(InView->GetFeatureLevel());
					MeshBuilder.AddVertices(MeshVerts);
					MeshBuilder.AddTriangles(MeshIndices);
					MeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * InMatrix, InComp->ArcPlaneMaterial->GetRenderProxy(), SDPG_Foreground, false, false, 0, InCollector);
				}

				// 刻度标记
				float DeltaAngle = 5.f;
				float TickMarker = 22.5f;
				// FVector ZAxis = Axis0 ^ Axis1;
				for (float Angle = 0; Angle < 360.f; Angle += DeltaAngle)
				{
					FVector GridAxis0 = Axis0.RotateAngleAxis(Angle, ZAxis);
					FVector GridAxis1 = Axis1.RotateAngleAxis(Angle, ZAxis);
					float PercentSize = (FMath::Fmod(Angle, TickMarker) == 0) ? .75f : .25f;
					if (FMath::Fmod(Angle, 90.f) != 0)
					{
						TArray<uint32> MkIndices;
						TArray<FDynamicMeshVertex> MkVerts;
						CoordinateAxisRenderFuncs::BuildSnapMarkerVerts(MkVerts, MkIndices, GridAxis0, GridAxis1, 0.1f, PercentSize, ArcColor);

						FDynamicMeshBuilder MkBuilder(InView->GetFeatureLevel());
						MkBuilder.AddVertices(MkVerts);
						MkBuilder.AddTriangles(MkIndices);
						MkBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * InMatrix, InComp->ArcPlaneMaterial->GetRenderProxy(), SDPG_Foreground, false, false, 0, InCollector);
					}
				}
			}
		}
		else
		{
			bool bMirrorAxis0 = true, bMirrorAxis1 = true;
			if (CoorAxisMode == CAM_Normal)
			{
				bMirrorAxis0 = ((Axis0 | InDirectionToWidget) <= 0.0f);
				bMirrorAxis1 = ((Axis1 | InDirectionToWidget) <= 0.0f);
			}
			if (CoorAxisMode == CAM_Mix)
			{
				bMirrorAxis0 = true;
				bMirrorAxis1 = false;
			}
			FVector RenderAxis0 = bMirrorAxis0 ? Axis0 : -Axis0;
			FVector RenderAxis1 = bMirrorAxis1 ? Axis1 : -Axis1;
			float Direction = (bMirrorAxis0 ^ bMirrorAxis1) ? -1.0f : 1.0f;

			// 外圈
			{
				FColor OuterColor = (CoordinateFocusAxis & InAxis) ? InComp->FocusColor.ToRGBE() : ArcColor;
				OuterColor.A = InColor.A;

				TArray<uint32> MeshIndices;
				TArray<FDynamicMeshVertex> MeshVerts;
				CoordinateAxisRenderFuncs::BuildThickArcVerts(MeshVerts, MeshIndices, RenderAxis0, RenderAxis1, 0.f, PI / 2, 48.f, 56.f, OuterColor);

				FDynamicMeshBuilder MeshBuilder(InView->GetFeatureLevel());
				MeshBuilder.AddVertices(MeshVerts);
				MeshBuilder.AddTriangles(MeshIndices);
				MeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * InMatrix, InComp->ArcPlaneMaterial->GetRenderProxy(), SDPG_Foreground, false, false, 0, InCollector);

				for (int32 Index = 0; Index < MeshVerts.Num() - 1; Index++)
				{
					if (Index != ((MeshVerts.Num() * 0.5) - 1))
						InPDI->DrawLine((FTranslationMatrix(MeshVerts[Index].Position) * (FScaleMatrix(FVector(RenderBoxScale)) * InMatrix)).GetOrigin(), (FTranslationMatrix(MeshVerts[Index + 1].Position) * (FScaleMatrix(FVector(RenderBoxScale)) * InMatrix)).GetOrigin(), ArcColor, SDPG_Foreground);
				}
			}

			// 内圈
			{
				FColor InnerColor = (CoordinateFocusAxis & InAxis) ? InComp->FocusColor.ToRGBE() : ArcColor;
				InnerColor.A = 0x0f;

				TArray<uint32> MeshIndices;
				TArray<FDynamicMeshVertex> MeshVerts;
				CoordinateAxisRenderFuncs::BuildThickArcVerts(MeshVerts, MeshIndices, RenderAxis0, RenderAxis1, 0.f, PI / 2, (CoorAxisMode == CAM_Mix) ? 20.f : 0.f, 48.f, InnerColor);

				FDynamicMeshBuilder MeshBuilder(InView->GetFeatureLevel());
				MeshBuilder.AddVertices(MeshVerts);
				MeshBuilder.AddTriangles(MeshIndices);
				MeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * InMatrix, InComp->ArcPlaneMaterial->GetRenderProxy(), SDPG_Foreground, false, false, 0, InCollector);

				for (int32 Index = 0; Index < MeshVerts.Num() - 1; Index++)
				{
					if (Index != ((MeshVerts.Num() * 0.5) - 1))
						InPDI->DrawLine((FTranslationMatrix(MeshVerts[Index].Position) * (FScaleMatrix(FVector(RenderBoxScale)) * InMatrix)).GetOrigin(), (FTranslationMatrix(MeshVerts[Index + 1].Position) * (FScaleMatrix(FVector(RenderBoxScale)) * InMatrix)).GetOrigin(), ArcColor, SDPG_Foreground);
				}
			}

			// 计算屏幕方向
			FVector2D Axis0ScreenLocation;
			if (!InView->ScreenToPixel(InView->WorldToScreen(InComp->GetComponentLocation() + RenderAxis0 * 64.0f), Axis0ScreenLocation))
			{
				Axis0ScreenLocation.X = Axis0ScreenLocation.Y = 0;
			}

			FVector2D Axis1ScreenLocation;
			if (!InView->ScreenToPixel(InView->WorldToScreen(InComp->GetComponentLocation() + RenderAxis1 * 64.0f), Axis1ScreenLocation))
			{
				Axis1ScreenLocation.X = Axis1ScreenLocation.Y = 0;
			}

			OutAxisDir = ((Axis1ScreenLocation - Axis0ScreenLocation) * Direction).GetSafeNormal();
		}
	}

	static void BuildRotationArcCollision(UCoordinateAxisComponent* InComp, EAxisType InAxis, const FVector& Axis0, const FVector& Axis1, const FVector& InDirectionToWidget, FTriMeshCollisionData& CollisionData, const FMatrix& InMatrix)
	{
		float RenderBoxScale = InComp->RenderBoxScale;
		ECoordinateAxisMode CoorAxisMode = InComp->GetCoorAxisMode();
		EAxisType CoordinateFocusAxis = InComp->GetFocusAxis();
		bool bIsDragging = InComp->IsDragging();
		int32& RenderAxisVertexNum = InComp->RenderAxisVertexNum;

		if (bIsDragging)
		{
			float DeltaRotation = InComp->TotalDeltaRotation;
			float AdjustedDeltaRotation = -DeltaRotation;
			float AbsRotation = FRotator::ClampAxis(FMath::Abs(DeltaRotation));
			float AngleOfChangeRadians(AbsRotation * PI / 180.f);

			float StartAngle = AdjustedDeltaRotation < 0.0f ? -AngleOfChangeRadians : 0.0f;
			float FilledAngle = AngleOfChangeRadians;

			{
				TArray<uint32> MeshIndices;
				TArray<FDynamicMeshVertex> MeshVerts;
				CoordinateAxisRenderFuncs::BuildThickArcVerts(MeshVerts, MeshIndices, Axis0, Axis1, StartAngle, StartAngle + FilledAngle, 48.f, 56.f, FColor::Blue);

				FMatrix Form = FTranslationMatrix(FVector::ZeroVector) * FScaleMatrix(FVector(RenderBoxScale)) * InMatrix;
				AddVertsToCollision(MeshVerts, MeshIndices, Form, CollisionData, RenderAxisVertexNum);
			}

			{
				TArray<uint32> MeshIndices;
				TArray<FDynamicMeshVertex> MeshVerts;
				CoordinateAxisRenderFuncs::BuildThickArcVerts(MeshVerts, MeshIndices, Axis0, Axis1, StartAngle, StartAngle + FilledAngle, (CoorAxisMode == CAM_Mix) ? 20.f : 0.f, 48.f, FColor::Blue);

				FMatrix Form = FTranslationMatrix(FVector::ZeroVector) * FScaleMatrix(FVector(RenderBoxScale)) * InMatrix;
				AddVertsToCollision(MeshVerts, MeshIndices, Form, CollisionData, RenderAxisVertexNum);
			}

			{
				TArray<uint32> MeshIndices;
				TArray<FDynamicMeshVertex> MeshVerts;
				CoordinateAxisRenderFuncs::BuildThickArcVerts(MeshVerts, MeshIndices, Axis0, Axis1, StartAngle + FilledAngle, StartAngle + 2 * PI, 48.f, 56.f, FColor::Blue);

				FMatrix Form = FTranslationMatrix(FVector::ZeroVector) * FScaleMatrix(FVector(RenderBoxScale)) * InMatrix;
				AddVertsToCollision(MeshVerts, MeshIndices, Form, CollisionData, RenderAxisVertexNum);
			}

			{
				TArray<uint32> MeshIndices;
				TArray<FDynamicMeshVertex> MeshVerts;
				CoordinateAxisRenderFuncs::BuildThickArcVerts(MeshVerts, MeshIndices, Axis0, Axis1, StartAngle + FilledAngle, StartAngle + 2 * PI, (CoorAxisMode == CAM_Mix) ? 20.f : 0.f, 48.f, FColor::Blue);

				FMatrix Form = FTranslationMatrix(FVector::ZeroVector) * FScaleMatrix(FVector(RenderBoxScale)) * InMatrix;
				AddVertsToCollision(MeshVerts, MeshIndices, Form, CollisionData, RenderAxisVertexNum);
			}
		}
		else
		{
			bool bMirrorAxis0 = true, bMirrorAxis1 = true;
			if (CoorAxisMode == CAM_Normal)
			{
				bMirrorAxis0 = ((Axis0 | InDirectionToWidget) <= 0.0f);
				bMirrorAxis1 = ((Axis1 | InDirectionToWidget) <= 0.0f);
			}
			if (CoorAxisMode == CAM_Mix)
			{
				bMirrorAxis0 = true;
				bMirrorAxis1 = false;
			}
			FVector RenderAxis0 = bMirrorAxis0 ? Axis0 : -Axis0;
			FVector RenderAxis1 = bMirrorAxis1 ? Axis1 : -Axis1;

			{
				TArray<uint32> MeshIndices;
				TArray<FDynamicMeshVertex> MeshVerts;
				CoordinateAxisRenderFuncs::BuildThickArcVerts(MeshVerts, MeshIndices, RenderAxis0, RenderAxis1, 0.f, PI / 2, 48.f, 56.f, FColor::Blue);

				FMatrix Form = FTranslationMatrix(FVector::ZeroVector) * FScaleMatrix(FVector(RenderBoxScale)) * InMatrix;
				AddVertsToCollision(MeshVerts, MeshIndices, Form, CollisionData, RenderAxisVertexNum);
			}

			{
				TArray<uint32> MeshIndices;
				TArray<FDynamicMeshVertex> MeshVerts;
				CoordinateAxisRenderFuncs::BuildThickArcVerts(MeshVerts, MeshIndices, RenderAxis0, RenderAxis1, 0.f, PI / 2, (CoorAxisMode == CAM_Mix) ? 20.f : 0.f, 48.f, FColor::Blue);

				FMatrix Form = FTranslationMatrix(FVector::ZeroVector) * FScaleMatrix(FVector(RenderBoxScale)) * InMatrix;
				AddVertsToCollision(MeshVerts, MeshIndices, Form, CollisionData, RenderAxisVertexNum);
			}
		}
	}
}

void CoordinateAxisRenderFuncs::RenderRotate(UCoordinateAxisComponent* InComp, const FSceneView* InView, FPrimitiveDrawInterface* InPDI, FMeshElementCollector& InCollector, const FMatrix& InMatrix)
{
	FVector XAxis = FVector(1, 0, 0);
	FVector YAxis = FVector(0, 1, 0);
	FVector ZAxis = FVector(0, 0, 1);

	FVector DirectionToWidget = InView->IsPerspectiveProjection() ? (InComp->GetComponentLocation() - InView->ViewMatrices.GetViewOrigin()) : -InView->GetViewDirection();
	DirectionToWidget.Normalize();

	EAxisType CoordinateRenderAxis = InComp->GetRenderAxis();

	if (CoordinateRenderAxis & EAxisType::EAT_X)
	{
		RenderRotationArc(InComp, EAxisType::EAT_X, InView, InPDI, InCollector, ZAxis, YAxis, DirectionToWidget, InComp->AxisColorX.ToFColor(true), InMatrix, InComp->ScreenXAxisDir);
	}

	if (CoordinateRenderAxis & EAxisType::EAT_Y)
	{
		RenderRotationArc(InComp, EAxisType::EAT_Y, InView, InPDI, InCollector, XAxis, ZAxis, DirectionToWidget, InComp->AxisColorY.ToFColor(true), InMatrix, InComp->ScreenYAxisDir);
	}

	if (CoordinateRenderAxis & EAxisType::EAT_Z)
	{
		RenderRotationArc(InComp, EAxisType::EAT_Z, InView, InPDI, InCollector, XAxis, YAxis, DirectionToWidget, InComp->AxisColorZ.ToFColor(true), InMatrix, InComp->ScreenZAxisDir);
	}
}

void CoordinateAxisRenderFuncs::BuildRotateCollision(UCoordinateAxisComponent* InComp, const FMatrix& InMatrix, FTriMeshCollisionData& CollisionData)
{
	FVector XAxis = FVector(1, 0, 0);
	FVector YAxis = FVector(0, 1, 0);
	FVector ZAxis = FVector(0, 0, 1);

	EAxisType CoordinateRenderAxis = InComp->GetRenderAxis();

	if (CoordinateRenderAxis & EAxisType::EAT_X)
	{
		BuildRotationArcCollision(InComp, EAxisType::EAT_X, ZAxis, YAxis, InComp->GetDirectionToWidget(), CollisionData, InMatrix);
	}

	if (CoordinateRenderAxis & EAxisType::EAT_Y)
	{
		BuildRotationArcCollision(InComp, EAxisType::EAT_Y, XAxis, ZAxis, InComp->GetDirectionToWidget(), CollisionData, InMatrix);
	}

	if (CoordinateRenderAxis & EAxisType::EAT_Z)
	{
		BuildRotationArcCollision(InComp, EAxisType::EAT_Z, XAxis, YAxis, InComp->GetDirectionToWidget(), CollisionData, InMatrix);
	}
}

// ============================================================
// Scale 渲染 & 碰撞
// ============================================================

void CoordinateAxisRenderFuncs::RenderScale(UCoordinateAxisComponent* InComp, const FSceneView* InView, FPrimitiveDrawInterface* InPDI, FMeshElementCollector& InCollector, const FMatrix& InMatrix)
{
	float RenderBoxScale = InComp->RenderBoxScale;
	EAxisType CoordinateFocusAxis = InComp->GetFocusAxis();

	// 三轴缩放箭头
	{
		TArray<uint32> MeshIndices;
		TArray<FDynamicMeshVertex> MeshVerts;
		CoordinateAxisRenderFuncs::BuildScaleAxisVerts(MeshVerts, MeshIndices);

		// X
		{
			UMaterialInstanceDynamic* XMaterial = (CoordinateFocusAxis & EAxisType::EAT_X ? InComp->FocusAxisMaterial : InComp->AxisMaterialX);
			FDynamicMeshBuilder MeshBuilder(InView->GetFeatureLevel());
			MeshBuilder.AddVertices(MeshVerts);
			MeshBuilder.AddTriangles(MeshIndices);
			MeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(-90.f, 0.f, 0.f)) * InMatrix, XMaterial->GetRenderProxy(), SDPG_Foreground, false, false, 0, InCollector);
		}

		// Y
		{
			UMaterialInstanceDynamic* YMaterial = (CoordinateFocusAxis & EAxisType::EAT_Y ? InComp->FocusAxisMaterial : InComp->AxisMaterialY);
			FDynamicMeshBuilder MeshBuilder(InView->GetFeatureLevel());
			MeshBuilder.AddVertices(MeshVerts);
			MeshBuilder.AddTriangles(MeshIndices);
			MeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(0.f, 0.f, -90.f)) * InMatrix, YMaterial->GetRenderProxy(), SDPG_Foreground, false, false, 0, InCollector);
		}

		// Z
		{
			UMaterialInstanceDynamic* ZMaterial = (CoordinateFocusAxis & EAxisType::EAT_Z ? InComp->FocusAxisMaterial : InComp->AxisMaterialZ);
			FDynamicMeshBuilder MeshBuilder(InView->GetFeatureLevel());
			MeshBuilder.AddVertices(MeshVerts);
			MeshBuilder.AddTriangles(MeshIndices);
			MeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(0.f, 0.f, 0.f)) * InMatrix, ZMaterial->GetRenderProxy(), SDPG_Foreground, false, false, 0, InCollector);
		}
	}

	// 拐角连接线
	{
		const FLinearColor& XColor = ((CoordinateFocusAxis & EAxisType::EAT_XY) == EAxisType::EAT_XY ? InComp->FocusColor : InComp->AxisColorX);
		const FLinearColor& YColor = ((CoordinateFocusAxis & EAxisType::EAT_XY) == EAxisType::EAT_XY ? InComp->FocusColor : InComp->AxisColorY);
		InPDI->DrawLine(InMatrix.TransformPosition(FVector(24, 0, 0) * FVector(RenderBoxScale)), InMatrix.TransformPosition(FVector(12, -12, 0) * FVector(RenderBoxScale)), XColor, SDPG_Foreground, RenderBoxScale);
		InPDI->DrawLine(InMatrix.TransformPosition(FVector(12, -12, 0) * FVector(RenderBoxScale)), InMatrix.TransformPosition(FVector(0, -24, 0) * FVector(RenderBoxScale)), YColor, SDPG_Foreground, RenderBoxScale);

		const FLinearColor& XZ_XColor = ((CoordinateFocusAxis & EAxisType::EAT_XZ) == EAxisType::EAT_XZ ? InComp->FocusColor : InComp->AxisColorX);
		const FLinearColor& ZColor = ((CoordinateFocusAxis & EAxisType::EAT_XZ) == EAxisType::EAT_XZ ? InComp->FocusColor : InComp->AxisColorZ);
		InPDI->DrawLine(InMatrix.TransformPosition(FVector(24, 0, 0) * FVector(RenderBoxScale)), InMatrix.TransformPosition(FVector(12, 0, 12) * FVector(RenderBoxScale)), XZ_XColor, SDPG_Foreground, RenderBoxScale);
		InPDI->DrawLine(InMatrix.TransformPosition(FVector(12, 0, 12) * FVector(RenderBoxScale)), InMatrix.TransformPosition(FVector(0, 0, 24) * FVector(RenderBoxScale)), ZColor, SDPG_Foreground, RenderBoxScale);

		const FLinearColor& YZ_YColor = ((CoordinateFocusAxis & EAxisType::EAT_YZ) == EAxisType::EAT_YZ ? InComp->FocusColor : InComp->AxisColorY);
		const FLinearColor& YZ_ZColor = ((CoordinateFocusAxis & EAxisType::EAT_YZ) == EAxisType::EAT_YZ ? InComp->FocusColor : InComp->AxisColorZ);
		InPDI->DrawLine(InMatrix.TransformPosition(FVector(0, -24, 0) * FVector(RenderBoxScale)), InMatrix.TransformPosition(FVector(0, -12, 12) * FVector(RenderBoxScale)), YZ_YColor, SDPG_Foreground, RenderBoxScale);
		InPDI->DrawLine(InMatrix.TransformPosition(FVector(0, -12, 12) * FVector(RenderBoxScale)), InMatrix.TransformPosition(FVector(0, 0, 24) * FVector(RenderBoxScale)), YZ_ZColor, SDPG_Foreground, RenderBoxScale);
	}

	// 原点立方体
	{
		TArray<uint32> MeshIndices;
		TArray<FDynamicMeshVertex> MeshVerts;
		CoordinateAxisRenderFuncs::BuildCubeVerts(4.f, FVector::ZeroVector, MeshVerts, MeshIndices);

		UMaterialInstanceDynamic* XYZMaterial = (CoordinateFocusAxis & EAxisType::EAT_Screen ? InComp->FocusAxisMaterial : InComp->AxisOriginMaterial);

		FDynamicMeshBuilder MeshBuilder(InView->GetFeatureLevel());
		MeshBuilder.AddVertices(MeshVerts);
		MeshBuilder.AddTriangles(MeshIndices);
		MeshBuilder.GetMesh(FScaleMatrix(FVector(RenderBoxScale)) * InMatrix, XYZMaterial->GetRenderProxy(), SDPG_Foreground, false, false, 0, InCollector);
	}
}

void CoordinateAxisRenderFuncs::BuildScaleCollision(UCoordinateAxisComponent* InComp, const FMatrix& InMatrix, FTriMeshCollisionData& CollisionData)
{
	float RenderBoxScale = InComp->RenderBoxScale;

	// 三轴碰撞
	{
		TArray<uint32> MeshIndices;
		TArray<FDynamicMeshVertex> MeshVerts;
		CoordinateAxisRenderFuncs::BuildScaleAxisVerts(MeshVerts, MeshIndices);

		FMatrix XForm = FTranslationMatrix(FVector::ZeroVector) * FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(-90.f, 0.f, 0.f)) * InMatrix;
		AddVertsToCollision(MeshVerts, MeshIndices, XForm, CollisionData, InComp->RenderAxisVertexNum);

		FMatrix YForm = FTranslationMatrix(FVector::ZeroVector) * FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(0.f, 0.f, -90.f)) * InMatrix;
		AddVertsToCollision(MeshVerts, MeshIndices, YForm, CollisionData, InComp->RenderAxisVertexNum);

		FMatrix ZForm = FTranslationMatrix(FVector::ZeroVector) * FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(0.f, 0.f, 0.f)) * InMatrix;
		AddVertsToCollision(MeshVerts, MeshIndices, ZForm, CollisionData, InComp->RenderAxisVertexNum);
	}

	// 拐角面碰撞
	{
		TArray<uint32> MeshIndices;
		TArray<FDynamicMeshVertex> MeshVerts;
		CoordinateAxisRenderFuncs::BuildTriangleVerts(MeshVerts, MeshIndices);

		FMatrix Form1 = FTranslationMatrix(FVector::ZeroVector) * FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(0.f, 0.f, -90.f)) * InMatrix;
		AddVertsToCollision(MeshVerts, MeshIndices, Form1, CollisionData, InComp->RenderDualAxisVertexNum);

		FMatrix Form2 = FTranslationMatrix(FVector::ZeroVector) * FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(0.f, 0.f, 0.f)) * InMatrix;
		AddVertsToCollision(MeshVerts, MeshIndices, Form2, CollisionData, InComp->RenderDualAxisVertexNum);

		FMatrix Form3 = FTranslationMatrix(FVector::ZeroVector) * FScaleMatrix(FVector(RenderBoxScale)) * FRotationMatrix(FRotator(0.f, -90.f, 0.f)) * InMatrix;
		AddVertsToCollision(MeshVerts, MeshIndices, Form3, CollisionData, InComp->RenderDualAxisVertexNum);
	}

	// 原点立方体碰撞
	{
		TArray<uint32> MeshIndices;
		TArray<FDynamicMeshVertex> MeshVerts;
		CoordinateAxisRenderFuncs::BuildCubeVerts(4.f, FVector::ZeroVector, MeshVerts, MeshIndices);

		FMatrix Form = FTranslationMatrix(FVector::ZeroVector) * FScaleMatrix(FVector(RenderBoxScale)) * InMatrix;
		AddVertsToCollision(MeshVerts, MeshIndices, Form, CollisionData, InComp->RenderSceneVertexNum);
	}
}
