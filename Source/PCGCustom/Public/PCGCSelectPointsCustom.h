// Copyright Roman K., Epic Games, Inc. All Rights Reserved. 


#pragma once

#include "PCGSettings.h"

#include "PCGCSelectPointsCustom.generated.h"

UCLASS(BlueprintType, ClassGroup = (Procedural))
class PCGCUSTOM_API UPCGCSelectPointsCustomSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
	UPCGCSelectPointsCustomSettings();

	//~Begin UPCGSettings interface
#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override { return FName(TEXT("SplitPoints")); }
	virtual FText GetDefaultNodeTitle() const override { return NSLOCTEXT("PCGCSplitPointsElement", "NodeTitle", "PCGC Split Points"); }
	virtual FText GetNodeTooltipText() const override;
	virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::Sampler; }
#endif

	virtual FString GetAdditionalTitleInformation() const override;

protected:
	virtual TArray<FPCGPinProperties> InputPinProperties() const override { return Super::DefaultPointInputPinProperties(); }
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;
	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings interface

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(ClampMin="0", ClampMax="1", PCG_Overridable))
		float Ratio = 0.1f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
		bool InvertSelection = false;
};

class FPCGCSelectPointsCustomElement : public IPCGElement
{
protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};
