// Copyright Roman K., Epic Games, Inc. All Rights Reserved. 

#pragma once

#include "PCGSettings.h"
#include "PCGContext.h"
#include "Elements/PCGActorSelector.h"
#include "PCGPin.h"

#include "UObject/ObjectKey.h"

#include "PCGCGetActorDataExtended.generated.h"



UENUM()
enum class EPCGGetDataFromActorModeExtended : uint8
{
	ParseActorComponents UMETA(Tooltip = "Parse the found actor(s) for relevant components such as Primitives, Splines, and Volumes."),
	GetSinglePoint UMETA(Tooltip = "Produces a single point per actor with the actor transform and bounds."),
	GetDataFromProperty UMETA(Tooltip = "Gets a data collection from an actor property."),
	GetDataFromPCGComponent UMETA(Tooltip = "Copy generated output from other PCG components on the found actor(s)."),
	GetDataFromPCGComponentOrParseComponents UMETA(Tooltip = "Attempts to copy generated output from other PCG components on the found actor(s), otherwise, falls back to parsing actor components.")
};


UCLASS(BlueprintType, ClassGroup = (Procedural))
class PCGCUSTOM_API UPCGCGetActorDataExtendedSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
	//~Begin UPCGSettings interface
#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override { return FName(TEXT("GetActorDataExtended")); }
	virtual FText GetDefaultNodeTitle() const override { return NSLOCTEXT("PCGCGetActorDataExtendedSettings", "NodeTitle", "PCGC Get Actor Data Extended"); }
	virtual FText GetNodeTooltipText() const override;
	virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::Spatial; }
	virtual void GetStaticTrackedKeys(FPCGSelectionKeyToSettingsMap& OutKeysToSettings, TArray<TObjectPtr<const UPCGGraph>>& OutVisitedGraphs) const override;
	virtual bool HasDynamicPins() const override { return true; }
	virtual void ApplyDeprecation(UPCGNode* InOutNode) override;
#endif

	virtual FString GetAdditionalTitleInformation() const override;

protected:
#if WITH_EDITOR
	virtual EPCGChangeType GetChangeTypeForProperty(const FName& InPropertyName) const override { return Super::GetChangeTypeForProperty(InPropertyName) | EPCGChangeType::Cosmetic; }
#endif
	virtual TArray<FPCGPinProperties> InputPinProperties() const override { return TArray<FPCGPinProperties>(); }
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;

	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings

public:
	//~Begin UObject interface
	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~End UObject interface

public:
	/** Override this to filter what kinds of data should be retrieved from the actor(s). */
	virtual EPCGDataType GetDataFilter() const { return EPCGDataType::Any; }

	/** Override this to change the default value the selector will revert to when changing the actor selection type */
	virtual TSubclassOf<AActor> GetDefaultActorSelectorClass() const;

	const FName PropertiesPinName = TEXT("Properties");
	const FName ComponentsPinName = TEXT("Components");

	/** Describes which actors to select for data collection. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ActorSelectorSettings", meta = (ShowOnlyInnerProperties, PCG_Overridable))
		FPCGActorSelectorSettings ActorSelector;

#if WITH_EDITORONLY_DATA
	/** If this is checked, found actors that are outside component bounds will not trigger a refresh. Only works for tags for now in editor. */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Actor Selector Settings")
		bool bTrackActorsOnlyWithinBounds = true;
#endif // WITH_EDITORONLY_DATA

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SpatialData")
		bool bGetSpatialData = true;

	/** Describes what kind of data we will collect from the found actor(s). */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SpatialData", meta = (EditCondition = "bDisplayModeSettings && bGetSpatialData", EditConditionHides, HideEditConditionToggle))
		EPCGGetDataFromActorModeExtended Mode = EPCGGetDataFromActorModeExtended::ParseActorComponents;

	/** Also produces a single point data at the actor location. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SpatialData", meta = (EditCondition = "(Mode == EPCGGetDataFromActorModeExtended::GetDataFromPCGComponent || Mode == EPCGGetDataFromActorModeExtended::GetDataFromPCGComponentOrParseComponents) && bGetSpatialData", EditConditionHides))
		bool bAlsoOutputSinglePointData = false;

	/** Only get data from components which overlap with the bounds of your source component. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SpatialData", meta = (EditCondition = "(Mode == EPCGGetDataFromActorModeExtended::GetDataFromPCGComponent || Mode == EPCGGetDataFromActorModeExtended::GetDataFromPCGComponentOrParseComponents) && bGetSpatialData", EditConditionHides))
		bool bComponentsMustOverlapSelf = true;

	/** Get data from all grid sizes if there is a partitioned PCG component on the actor, instead of a specific set of grid sizes. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SpatialData", meta = (EditCondition = "(Mode == EPCGGetDataFromActorModeExtended::GetDataFromPCGComponent || Mode == EPCGGetDataFromActorModeExtended::GetDataFromPCGComponentOrParseComponents) && bGetSpatialData", EditConditionHides))
		bool bGetDataOnAllGrids = true;

	/** Select which grid sizes to consider when collecting data from partitioned PCG components. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SpatialData", meta = (Bitmask, BitmaskEnum = "/Script/PCG.EPCGHiGenGrid", EditCondition = "(!bGetDataOnAllGrids && Mode == EPCGGetDataFromActorModeExtended::GetDataFromPCGComponent || Mode == EPCGGetDataFromActorModeExtended::GetDataFromPCGComponentOrParseComponents) && bGetSpatialData", EditConditionHides))
		int32 AllowedGrids = int32(EPCGHiGenGrid::Uninitialized);

	/** Merges all the single point data outputs into a single point data. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SpatialData", meta = (EditCondition = "Mode == EPCGGetDataFromActorModeExtended::GetSinglePoint && bGetSpatialData", EditConditionHides))
		bool bMergeSinglePointData = false;

	// This can be set false by inheriting nodes to hide the 'Mode' property.
	UPROPERTY(Transient, meta = (EditCondition = false, EditConditionHides))
		bool bDisplayModeSettings = true;

	/** Provide pin names to match against the found component output pins. Data will automatically be wired to the expected pin if the name comparison succeeds. All unmatched pins will go into the standard out pin. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SpatialData", meta = (EditCondition = "(Mode == EPCGGetDataFromActorModeExtended::GetDataFromPCGComponent || Mode == EPCGGetDataFromActorModeExtended::GetDataFromPCGComponentOrParseComponents) && bGetSpatialData", EditConditionHides))
		TArray<FName> ExpectedPins;

	/** The property name on the found actor to create a data collection from. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SpatialData", meta = (EditCondition = "Mode == EPCGGetDataFromActorModeExtended::GetDataFromProperty && bGetSpatialData", EditConditionHides))
		FName PropertyName = NAME_None;

	/** Silence warnings that attribute names were sanitized to replace invalid characters. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SpatialData|Advanced")
		bool bSilenceSanitizedAttributeNameWarnings = false;





	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ActorProperties")
		bool bGetActorProperties = false;

	/** Specify the names of the properties, structs or objects, exposed on the Actor */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ActorProperties", meta = (EditCondition = "bGetActorProperties"))
		TArray<FName> PropertiesNames = {};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ActorComponents")
		bool bGetActorComponentsAsPoints = false;

	// Specify the names of the Attributes which will contain tags, mapped from component tags.
	// Mapping is done by the tag index, for example: ComponentTagAttributeName with index 0 will contain ComponentTag with index 0
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ActorComponents", meta = (EditCondition = "bGetActorComponentsAsPoints"))
		TArray<FName> ComponentTagAttributeNames = {};

	/** Specify the types of component objects that you want to be excluded from the point set*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ActorComponents", meta = (EditCondition = "bGetActorComponentsAsPoints"))
		TArray<TSubclassOf<UPrimitiveComponent>> ExclusionClasses;

};

class FPCGDataFromActorContext : public FPCGContext
{
public:
	TArray<AActor*> FoundActors;
	bool bPerformedQuery = false;

#if WITH_EDITOR
	/** Any change origin ignores we added, to solve dependency issues (like upstream execution cancelling downstream graph). */
	TArray<TObjectKey<UObject>> IgnoredChangeOrigins;
#endif
};

class FPCGCGetActorDataExtendedElement : public IPCGElement
{
public:
//	virtual FPCGContext* Initialize(const FPCGDataCollection& InputData, TWeakObjectPtr<UPCGComponent> SourceComponent, const UPCGNode* Node) override;
	virtual bool CanExecuteOnlyOnMainThread(FPCGContext* Context) const override { return true; }
	virtual void GetDependenciesCrc(const FPCGDataCollection& InInput, const UPCGSettings* InSettings, UPCGComponent* InComponent, FPCGCrc& OutCrc) const override;
	virtual bool ShouldComputeFullOutputDataCrc(FPCGContext* Context) const override { return true; }
//	virtual bool IsCacheable(const UPCGSettings* InSettings) const override { return false; }

protected:
	virtual FPCGContext* CreateContext() override;
	virtual bool ExecuteInternal(FPCGContext* Context) const;
	void GatherWaitTasks(AActor* FoundActor, FPCGContext* InContext, TArray<FPCGTaskId>& OutWaitTasks) const;
	virtual void ProcessActors(FPCGContext* Context, const UPCGCGetActorDataExtendedSettings* Settings, const TArray<AActor*>& FoundActors) const;
	virtual void ProcessActor(FPCGContext* Context, const UPCGCGetActorDataExtendedSettings* Settings, AActor* FoundActor) const;

	void MergeActorsIntoPointData(FPCGContext* Context, const UPCGCGetActorDataExtendedSettings* Settings, const TArray<AActor*>& FoundActors) const;

	void GetActorProperties(FPCGContext* Context, const UPCGCGetActorDataExtendedSettings* Settings, AActor* FoundActor) const;
	void GetActorComponentsAsPoints(FPCGContext* Context, const UPCGCGetActorDataExtendedSettings* Settings, AActor* FoundActor) const;
};
