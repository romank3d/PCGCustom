// Copyright Roman K. All Rights Reserved.

#pragma once

#include "PCGSettings.h"
#include "Data/PCGDifferenceData.h"

#include "PCGCDifferenceByTag.generated.h"

/**
 * 
 */
UCLASS()
class PCGCUSTOM_API UPCGCDifferenceByTagSettings : public UPCGSettings
{
	GENERATED_BODY()
	
public:
	//~Begin UPCGSettings interface
#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override { return FName(TEXT("DifferenceByTag")); }
	virtual FText GetDefaultNodeTitle() const override { return NSLOCTEXT("PCGCDifferenceByTag", "NodeTitle", "PCGC Difference By Actor Tag"); }
	virtual FText GetNodeTooltipText() const override;
	virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::Spatial; }
#endif

	virtual TArray<FPCGPinProperties> InputPinProperties() const override;
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;

protected:
	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings interface

public:

	/** The density function to use when recalculating the density after the operation. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
		EPCGDifferenceDensityFunction DensityFunction = EPCGDifferenceDensityFunction::Binary;

	/** Describes how the difference operation will treat the output data:
	 * Continuous - Non-destructive data output will be maintained.
	 * Discrete - Output data will be discrete points, or explicitly converted to points.
	 * Inferred - Output data will choose from Continuous or Discrete, based on the source and operation.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings)
		EPCGDifferenceMode Mode = EPCGDifferenceMode::Inferred;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings)
		bool bDiffMetadata = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (InlineEditConditionToggle, PCG_Overridable))
		bool bUsingCustomTags = false;

	//Number of tags, specified after mandatory "Priority" and "ActorID" tags
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, DisplayName = "Number Of Custom Tags", meta = (EditCondition = "bUsingCustomTags", ClampMin = "0", PCG_Overridable))
		int32 NumCustomTags = 0;
};

class FPCGCDifferenceByTagElement : public IPCGElement
{
protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const;
};
