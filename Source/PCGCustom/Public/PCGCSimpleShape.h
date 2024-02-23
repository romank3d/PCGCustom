// Copyright Roman K. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PCGSettings.h"

#include "PCGCSimpleShape.generated.h"

UENUM()
enum class EPCGCSImpleShapePointLineMode : uint8
{
	//Shape Modes
	Point,
	Line,
	Rectangle,
	Circle
};

UENUM()
enum class EPCGCShapePointLineMode : uint8
{
	Direction,
	SetPosition
};

UENUM()
enum class EPCGCInterpolationMode : uint8
{
	Step,
	Subdivision
};

UENUM()
enum class EPCGCRectangleInterpolationMode : uint8
{
	Step,
	Subdivision,
	StepLW,
	SubdivisionLW,
};

USTRUCT(BlueprintType)
struct PCGCUSTOM_API FPCGCSinglePointSettings
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
		FVector PointOriginPosition = FVector(0.0f, 0.0f, 0.0f);
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
		FRotator PointOrientation = FRotator(0.0f, 0.0f, 0.0f);

};

USTRUCT(BlueprintType)
struct PCGCUSTOM_API FPCGCLineSettings
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_NotOverridable))
		EPCGCShapePointLineMode Mode = EPCGCShapePointLineMode::Direction;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_NotOverridable))
		bool bLineEndPointsOnly = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "Mode == EPCGCShapePointLineMode::Direction", EditConditionHides, ClampMin = "0.1", PCG_Overridable))
		double LineLenght = 400.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "Mode == EPCGCShapePointLineMode::Direction", EditConditionHides, PCG_Overridable))
		FRotator LineDirection = FRotator(0.0f, 0.0f, 0.0f);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "Mode == EPCGCShapePointLineMode::SetPosition", EditConditionHides, PCG_Overridable))
		FVector LineOriginPosition = FVector(0.0f, 0.0f, 0.0f);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "Mode == EPCGCShapePointLineMode::SetPosition", EditConditionHides, PCG_Overridable))
		FVector LineTargetPosition = FVector(400.0f, 0.0f, 0.0f);



	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_NotOverridable, EditCondition = "!bLineEndPointsOnly", EditConditionHides))
		EPCGCInterpolationMode LineInterpolation = EPCGCInterpolationMode::Step;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "!bLineEndPointsOnly && LineInterpolation == EPCGCInterpolationMode::Step", EditConditionHides, ClampMin = "0.1", PCG_Overridable))
		double LineStep = 100.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "!bLineEndPointsOnly && LineInterpolation == EPCGCInterpolationMode::Subdivision", EditConditionHides, ClampMin = "1", PCG_Overridable))
		int32 LineDivisions = 4;



	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
		bool bAlignLinePointsToDirection = true;

};

USTRUCT(BlueprintType)
struct PCGCUSTOM_API FPCGCRectangleSettings
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_NotOverridable))
		bool bCornerPointsOnly = false;

	//HL properties

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (ClampMin = "0.1", PCG_Overridable))
		double RectangleLenght = 400.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (ClampMin = "0.1", PCG_Overridable))
		double RectangleWidth = 400.0f;


	//Interpolation properties
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_NotOverridable, EditCondition = "!bCornerPointsOnly", EditConditionHides))
		EPCGCRectangleInterpolationMode Interpolation = EPCGCRectangleInterpolationMode::Step;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "!bCornerPointsOnly && Interpolation == EPCGCRectangleInterpolationMode::Step", EditConditionHides, ClampMin = "0.1", PCG_Overridable))
		double RectangleStep = 100.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "!bCornerPointsOnly && Interpolation == EPCGCRectangleInterpolationMode::StepLW", EditConditionHides, ClampMin = "0.1", PCG_Overridable))
		double RectangleLenghtStep = 100.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "!bCornerPointsOnly && Interpolation == EPCGCRectangleInterpolationMode::StepLW", EditConditionHides, ClampMin = "0.1", PCG_Overridable))
		double RectangleWidthStep = 100.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "!bCornerPointsOnly && Interpolation == EPCGCRectangleInterpolationMode::Subdivision", EditConditionHides, ClampMin = "1", PCG_Overridable))
		int32 RectangleSubdivisions = 4;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "!bCornerPointsOnly && Interpolation == EPCGCRectangleInterpolationMode::SubdivisionLW", EditConditionHides, ClampMin = "1", PCG_Overridable))
		int32 RectangleLenghtSubdivisions = 4;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "!bCornerPointsOnly && Interpolation == EPCGCRectangleInterpolationMode::SubdivisionLW", EditConditionHides, ClampMin = "1", PCG_Overridable))
		int32 RectangleWidthtSubdivisions = 4;


	//Points orientation properties
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (DisplayName = "OrientToDirection", PCG_NotOverridable))
		bool bOrientToCenter = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "bOrientToCenter", EditConditionHides, PCG_NotOverridable))
		bool bOrientCorners = false;


	//Merge sides in a single collection option
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "!bCornerPointsOnly", EditConditionHides, PCG_NotOverridable))
		bool bMergeSides = false;

};

USTRUCT(BlueprintType)
struct PCGCUSTOM_API FPCGCCircleleSettings
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (ClampMin = "0.1", PCG_Overridable))
		double CircleRadius = 200.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_NotOverridable, EditCondition = "!bLineEndPointsOnly", EditConditionHides))
		EPCGCInterpolationMode Interpolation = EPCGCInterpolationMode::Step;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "Interpolation == EPCGCInterpolationMode::Step", EditConditionHides, ClampMin = "0.1", PCG_Overridable))
		double CircleStep = 100.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "Interpolation == EPCGCInterpolationMode::Subdivision", EditConditionHides, ClampMin = "2", PCG_Overridable))
		int32 CircleSubdivisions = 16;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_NotOverridable))
		bool bOrientToCenter = true;

};


UCLASS()
class PCGCUSTOM_API UPCGCSimpleShapeSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
	//~Begin UPCGSettings interface
#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override;
	virtual FText GetDefaultNodeTitle() const override;
	virtual FName AdditionalTaskName() const override;
	virtual FText GetNodeTooltipText() const override { return NSLOCTEXT("PCGCSimpleShapeSettings", "NodeTooltip", "Outputs the points in a form of a certain shape"); }
	virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::Sampler; }
#endif


protected:
	virtual TArray<FPCGPinProperties> InputPinProperties() const override { return TArray<FPCGPinProperties>(); };
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;
	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings interface

public:

	//Declare Properties
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings)
		EPCGCSImpleShapePointLineMode  Shape = EPCGCSImpleShapePointLineMode::Point;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "Shape == EPCGCSImpleShapePointLineMode::Point", EditConditionHides, PCG_Overridable))
		FPCGCSinglePointSettings PointSettings;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "Shape == EPCGCSImpleShapePointLineMode::Line", EditConditionHides, PCG_Overridable))
		FPCGCLineSettings LineSettings;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "Shape == EPCGCSImpleShapePointLineMode::Rectangle", EditConditionHides, PCG_Overridable))
		FPCGCRectangleSettings RectangleSettings;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "Shape == EPCGCSImpleShapePointLineMode::Circle", EditConditionHides, PCG_Overridable))
		FPCGCCircleleSettings CircleSettings;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, AdvancedDisplay, meta = (ClampMin = "0.0", PCG_Overridable))
		FVector PointExtents = FVector(10.0f, 10.0f, 10.0f);

};

class UPCGCSimpleShapeElement : public FSimplePCGElement
{
protected:

	//Override main execution function
	virtual bool ExecuteInternal(FPCGContext* Context) const override;

	//Declare custom functions
	void CreatePoint(FPCGContext* Context, const UPCGCSimpleShapeSettings* Settings, TArray<FPCGTaggedData>& Outputs) const;
	void CreateLine(FPCGContext* Context, const UPCGCSimpleShapeSettings* Settings, TArray<FPCGTaggedData>& Outputs) const;
	void CreateRectangle(FPCGContext* Context, const UPCGCSimpleShapeSettings* Settings, TArray<FPCGTaggedData>& Outputs) const;
	void CreateCircle(FPCGContext* Context, const UPCGCSimpleShapeSettings* Settings, TArray<FPCGTaggedData>& Outputs) const;

private:

	TArray<FPCGPoint>& GetOutputPoints(TArray<FPCGTaggedData>& Outputs) const;

};

