// Copyright Roman K. All Rights Reserved. 
#pragma once

#include "PCGSettings.h"

#include "PCGCDiscardData.generated.h"

UCLASS(BlueprintType, ClassGroup = (Procedural))
class PCGCUSTOM_API UPCGCDiscardDataSettings : public UPCGSettings
{
	GENERATED_BODY()

public:

	//~Begin UPCGSettings interface
#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override { return FName(TEXT("DiscardEmptyDataSets")); }
	virtual FText GetDefaultNodeTitle() const override { return NSLOCTEXT("PCGCDiscardDataElement", "NodeTitle", "PCGC Discard Empty Data Sets"); }
	virtual FText GetNodeTooltipText() const override;
	virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::Generic;}
#endif

	virtual FString GetAdditionalTitleInformation() const override;

	virtual bool HasDynamicPins() const override { return true; }

protected:
#if WITH_EDITOR
	virtual EPCGChangeType GetChangeTypeForProperty(const FName& InPropertyName) const override;
#endif
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;
	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings interface
};

class FPCGCDiscardDataElement : public IPCGElement
{
protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};
