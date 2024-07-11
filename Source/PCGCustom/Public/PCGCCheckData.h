// Copyright Roman K. All Rights Reserved. 
#pragma once

#include "PCGSettings.h"

#include "PCGCCheckData.generated.h"


UCLASS(BlueprintType, ClassGroup = (Procedural))
class PCGCUSTOM_API UPCGCCheckDataSettings : public UPCGSettings
{
	GENERATED_BODY()

public:

	//~Begin UPCGSettings interface
#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override { return FName(TEXT("CheckData")); }
	virtual FText GetDefaultNodeTitle() const override { return NSLOCTEXT("PCGCCheckDataElement", "NodeTitle", "PCGC Check Data"); }
	virtual FText GetNodeTooltipText() const override;
	virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::ControlFlow;}
	virtual bool HasDynamicPins() const override { return true; }
#endif
	virtual bool OutputPinsCanBeDeactivated() const override { return true; }
	virtual FString GetAdditionalTitleInformation() const override;
	virtual EPCGDataType GetCurrentPinTypes(const UPCGPin* InPin) const override;
	

public:

	//Will discard empty data sets for points, attribute sets or composite data
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
	bool DiscardEmptyData = true;

protected:
#if WITH_EDITOR
	virtual EPCGChangeType GetChangeTypeForProperty(const FName& InPropertyName) const override;
#endif
	virtual TArray<FPCGPinProperties> InputPinProperties() const override;
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;
	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings interface
};

class FPCGCCheckDataElement : public IPCGElement
{
protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};
