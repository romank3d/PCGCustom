// Copyright Roman K. All Rights Reserved.

#pragma once

#include "PCGSettings.h"

#include "PCGCSimpleShape.generated.h"

UENUM()
enum class EPCGCSImpleShapePointLineMode : uint16
{
	//Shape Modes
	//Shapes UMETA(Hidden),
	Point,
	Line,
	Rectangle,
	Circle,
	Grid
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

UENUM()
enum class EPCGCGridCreationMode : uint8
{
	Rows,
	Size
};

USTRUCT(BlueprintType)
struct PCGCUSTOM_API FPCGCSinglePointSettings
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
		FRotator PointOrientation = FRotator(0.0, 0.0, 0.0);

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
		double LineLenght = 400.0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "Mode == EPCGCShapePointLineMode::Direction", EditConditionHides, PCG_Overridable))
		FRotator LineDirection = FRotator(0.0, 0.0, 0.0);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "Mode == EPCGCShapePointLineMode::SetPosition", EditConditionHides, PCG_Overridable))
		FVector LineOriginPosition = FVector(0.0, 0.0, 0.0);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "Mode == EPCGCShapePointLineMode::SetPosition", EditConditionHides, PCG_Overridable))
		FVector LineTargetPosition = FVector(400.0, 0.0, 0.0);



	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_NotOverridable, EditCondition = "!bLineEndPointsOnly", EditConditionHides))
		EPCGCInterpolationMode LineInterpolation = EPCGCInterpolationMode::Step;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "!bLineEndPointsOnly && LineInterpolation == EPCGCInterpolationMode::Step", EditConditionHides, ClampMin = "0.1", PCG_Overridable))
		double LineStep = 100.0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "!bLineEndPointsOnly && LineInterpolation == EPCGCInterpolationMode::Subdivision", EditConditionHides, ClampMin = "1", PCG_Overridable))
		int32 LineDivisions = 4;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
		bool bAlignLinePointsToDirection = false;

};

USTRUCT(BlueprintType)
struct PCGCUSTOM_API FPCGCRectangleSettings
{
	GENERATED_BODY()

public:


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_NotOverridable))
		bool bCornerPointsOnly = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (ClampMin = "0.1", PCG_Overridable))
		double RectangleLenght = 400.0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (ClampMin = "0.1", PCG_Overridable))
		double RectangleWidth = 400.0;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_NotOverridable, EditCondition = "!bCornerPointsOnly", EditConditionHides))
		EPCGCRectangleInterpolationMode Interpolation = EPCGCRectangleInterpolationMode::Step;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "!bCornerPointsOnly && Interpolation == EPCGCRectangleInterpolationMode::Step", EditConditionHides, ClampMin = "0.1", PCG_Overridable))
		double RectangleStep = 100.0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "!bCornerPointsOnly && Interpolation == EPCGCRectangleInterpolationMode::StepLW", EditConditionHides, ClampMin = "0.1", PCG_Overridable))
		double RectangleLenghtStep = 100.0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "!bCornerPointsOnly && Interpolation == EPCGCRectangleInterpolationMode::StepLW", EditConditionHides, ClampMin = "0.1", PCG_Overridable))
		double RectangleWidthStep = 100.0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "!bCornerPointsOnly && Interpolation == EPCGCRectangleInterpolationMode::Subdivision", EditConditionHides, ClampMin = "1", PCG_Overridable))
		int32 RectangleSubdivisions = 4;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "!bCornerPointsOnly && Interpolation == EPCGCRectangleInterpolationMode::SubdivisionLW", EditConditionHides, ClampMin = "1", PCG_Overridable))
		int32 RectangleLenghtSubdivisions = 4;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "!bCornerPointsOnly && Interpolation == EPCGCRectangleInterpolationMode::SubdivisionLW", EditConditionHides, ClampMin = "1", PCG_Overridable))
		int32 RectangleWidthtSubdivisions = 4;


	//Points orientation
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (DisplayName = "OrientToDirection", PCG_NotOverridable))
		bool bOrientToCenter = true;

	//Orient corner points to the center of the shape
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "bOrientToCenter", EditConditionHides, PCG_NotOverridable))
		bool bOrientCorners = false;


	//Merge sides in a single Point Set
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "!bCornerPointsOnly", EditConditionHides, PCG_NotOverridable))
		bool bMergeSides = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_NotOverridable))
		bool bCenterPivot = true;
};

USTRUCT(BlueprintType)
struct PCGCUSTOM_API FPCGCCircleSettings
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (ClampMin = "0.1", PCG_Overridable))
		double CircleRadius = 200.0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_NotOverridable))
		EPCGCInterpolationMode Interpolation = EPCGCInterpolationMode::Step;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "Interpolation == EPCGCInterpolationMode::Step", EditConditionHides, ClampMin = "0.1", PCG_Overridable))
		double CircleStep = 100.0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "Interpolation == EPCGCInterpolationMode::Subdivision", EditConditionHides, ClampMin = "2", PCG_Overridable))
		int32 CircleSubdivisions = 16;

	//Orient points to the center of the shape
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_NotOverridable))
		bool bOrientToCenter = true;

};

USTRUCT(BlueprintType)
struct PCGCUSTOM_API FPCGCGridSettings
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = false, PCG_NotOverridable))
		EPCGCGridCreationMode Mode = EPCGCGridCreationMode::Rows;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, DisplayName = "Grid Lenght Rows", meta = (EditCondition = "Mode == EPCGCGridCreationMode::Rows", EditConditionHides, ClampMin = "1", PCG_Overridable))
		int32 LenghtRows = 5;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, DisplayName = "Grid Width Rows", meta = (EditCondition = "Mode == EPCGCGridCreationMode::Rows", EditConditionHides, ClampMin = "1", PCG_Overridable))
		int32 WidthRows = 5;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, DisplayName = "Grid Height Rows", meta = (EditCondition = "Mode == EPCGCGridCreationMode::Rows", EditConditionHides, ClampMin = "1", PCG_Overridable))
		int32 HeightRows = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, DisplayName = "Grid Row Step", meta = (EditCondition = "Mode == EPCGCGridCreationMode::Rows", EditConditionHides, ClampMin = "0.1", PCG_Overridable))
		double RowStep = 100.0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_NotOverridable))
		bool bCenterPivotXY = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "bCenterPivotXY && HeightRows > 1", PCG_NotOverridable))
		bool bCenterPivotZ = false;
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
	virtual FText GetNodeTooltipText() const override { return NSLOCTEXT("PCGCSimpleShapeSettings", "NodeTooltip", "Create points in a form of a certain shape"); }
	virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::Sampler; }
	virtual bool HasFlippedTitleLines() const override { return true; }
	virtual TArray<FPCGPreConfiguredSettingsInfo> GetPreconfiguredInfo() const override;
	virtual bool OnlyExposePreconfiguredSettings() const override { return true; }
#endif

//	virtual FName AdditionalTaskName() const override;
	virtual FString GetAdditionalTitleInformation() const override;
	virtual void ApplyPreconfiguredSettings(const FPCGPreConfiguredSettingsInfo& PreconfigureInfo) override;

protected:

#if WITH_EDITOR
	virtual EPCGChangeType GetChangeTypeForProperty(const FName& InPropertyName) const override { return Super::GetChangeTypeForProperty(InPropertyName) | EPCGChangeType::Cosmetic; }
#endif

	virtual TArray<FPCGPinProperties> InputPinProperties() const override { return TArray<FPCGPinProperties>(); };
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;
	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings interface

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings)
		EPCGCSImpleShapePointLineMode  Shape = EPCGCSImpleShapePointLineMode::Point;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "Shape == EPCGCSImpleShapePointLineMode::Point", EditConditionHides, PCG_Overridable))
		FPCGCSinglePointSettings PointSettings;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "Shape == EPCGCSImpleShapePointLineMode::Line", EditConditionHides, PCG_Overridable))
		FPCGCLineSettings LineSettings;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "Shape == EPCGCSImpleShapePointLineMode::Rectangle", EditConditionHides, PCG_Overridable))
		FPCGCRectangleSettings RectangleSettings;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "Shape == EPCGCSImpleShapePointLineMode::Circle", EditConditionHides, PCG_Overridable))
		FPCGCCircleSettings CircleSettings;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (EditCondition = "Shape == EPCGCSImpleShapePointLineMode::Grid", EditConditionHides, PCG_Overridable))
		FPCGCGridSettings GridSettings;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, AdvancedDisplay, meta = (PCG_Overridable))
		FVector OriginLocation = FVector();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, AdvancedDisplay, meta = (ClampMin = "0.0", PCG_Overridable))
		FVector PointExtents = FVector(10.0, 10.0, 10.0);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, AdvancedDisplay, meta = (ClampMin = "0.0", PCG_Overridable))
		double Density = 1.0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, AdvancedDisplay, meta = (ClampMin = "0.0", ClampMax = "1.0", PCG_Overridable))
		double Steepness = 0.5;

	//Actor location will only update automatically if node caching ("Is Cacheable") is turned off, otherwise, requires force regen
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, AdvancedDisplay, meta = (PCG_Overridable))
		bool bLocal = false;

	//Toggle node caching
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, AdvancedDisplay, meta = (PCG_Overridable))
		bool bIsCacheable = false;



};

class UPCGCSimpleShapeElement : public IPCGElement
{
protected:

	//Override main execution function
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
	virtual bool IsCacheable(const UPCGSettings* InSettings) const override;

	//Declare custom functions
	void CreatePoint(FPCGContext* Context, const UPCGCSimpleShapeSettings* Settings, TArray<FPCGTaggedData>& Outputs, FVector LocalOffset) const;
	void CreateLine(FPCGContext* Context, const UPCGCSimpleShapeSettings* Settings, TArray<FPCGTaggedData>& Outputs, FVector LocalOffset) const;
	void CreateRectangle(FPCGContext* Context, const UPCGCSimpleShapeSettings* Settings, TArray<FPCGTaggedData>& Outputs, FVector LocalOffset) const;
	void CreateCircle(FPCGContext* Context, const UPCGCSimpleShapeSettings* Settings, TArray<FPCGTaggedData>& Outputs, FVector LocalOffset) const;
	void CreateGrid(FPCGContext* Context, const UPCGCSimpleShapeSettings* Settings, TArray<FPCGTaggedData>& Outputs, FVector LocalOffset) const;

private:

	TArray<FPCGPoint>& GetOutputPoints(TArray<FPCGTaggedData>& Outputs) const;

};

