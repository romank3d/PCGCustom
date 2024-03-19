// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Templates/SubclassOf.h"
#include "UObject/NameTypes.h"
#include "Elements/PCGActorSelector.h"

#include "PCGCActorSelectorExtended.generated.h"

class AActor;
class UPCGComponent;
class UWorld;

UENUM()
enum class EPCGActorFilterExtended : uint8
{
	/** This actor (either the original PCG actor or the partition actor if partitioning is enabled). */
	Self,
	/** The parent of this actor in the hierarchy. */
	Parent,
	/** The top most parent of this actor in the hierarchy. */
	Root,
	/** All actors in world. */
	AllWorldActors,
	/** The source PCG actor (rather than the generated partition actor). */
	Original UMETA(Hidden),
};


struct PCGCUSTOM_API FPCGActorSelectionKeyExtended : FPCGActorSelectionKey
{
	FPCGActorSelectionKeyExtended() = default;

	//For all filters others than AllWorldActor. For AllWorldActors Filter, use the other constructors.
	explicit FPCGActorSelectionKeyExtended(EPCGActorFilter InFilter);
	
	explicit FPCGActorSelectionKeyExtended(FName InTag);
	explicit FPCGActorSelectionKeyExtended(TSubclassOf<AActor> InSelectionClass);
	
	//bool operator==(const FPCGActorSelectionKeyExtended& InOther) const;
	
	//friend uint32 GetTypeHash(const FPCGActorSelectionKeyExtended& In);
	bool IsMatching(const AActor* InActor, const UPCGComponent* InComponent) const;
	
	void SetExtraDependency(const UClass* InExtraDependency);

	//EPCGActorFilter ActorFilter = EPCGActorFilter::AllWorldActors;
	//EPCGActorSelection Selection = EPCGActorSelection::Unknown;
	//FName Tag = NAME_None;
	//TSubclassOf<AActor> ActorSelectionClass = nullptr;

	// If it should track a specific object dependency instead of an actor. For example, GetActorData with GetPCGComponent data.
	//const UClass* OptionalExtraDependency = nullptr;


};

USTRUCT(BlueprintType)
struct PCGCUSTOM_API FPCGCActorSelectorExtendedSettings
{
	GENERATED_BODY()

	/** Which actors to consider. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ActorSelectorSettings", meta = (DisplayName = "Actor Filter", EditCondition = "bShowActorFilter", EditConditionHides, HideEditConditionToggle, PCG_Overridable))
	EPCGActorFilterExtended ActorFilterCustom = EPCGActorFilterExtended::Self;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ActorSelectorSettings", meta = (EditCondition = false, EditConditionHides, HideEditConditionToggle, PCG_NotOverridable))
	EPCGActorFilter ActorFilter = EPCGActorFilter::Self;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ActorSelectorSettings", meta = (EditCondition = "ActorFilterCustom==EPCGActorFilterExtended::AllWorldActors", EditConditionHides, PCG_NotOverridable))
	bool bMustOverlapSelf = false;

	/** Whether to consider child actors. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ActorSelectorSettings", meta = (EditCondition = "bShowIncludeChildren && ActorFilterCustom!=EPCGActorFilterExtended::AllWorldActors", EditConditionHides, PCG_NotOverridable))
	bool bIncludeChildren = false;

	/** Enables/disables fine-grained actor filtering options. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ActorSelectorSettings", meta = (EditCondition = "ActorFilterCustom!=EPCGActorFilterExtended::AllWorldActors && bIncludeChildren", EditConditionHides, PCG_NotOverridable))
	bool bDisableFilter = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ActorSelectorSettings", meta = (EditCondition = "bShowActorSelection && (ActorFilterCustom==EPCGActorFilterExtended::AllWorldActors || (bIncludeChildren && !bDisableFilter))", EditConditionHides, PCG_NotOverridable))
	EPCGActorSelection ActorSelection = EPCGActorSelection::ByTag;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ActorSelectorSettings", meta = (EditCondition = "bShowActorSelection && (ActorFilterCustom==EPCGActorFilterExtended::AllWorldActors || (bIncludeChildren && !bDisableFilter)) && ActorSelection==EPCGActorSelection::ByTag", EditConditionHides, PCG_NotOverridable))
	FName ActorSelectionTag;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ActorSelectorSettings", meta = (EditCondition = "bShowActorSelectionClass && bShowActorSelection && (ActorFilterCustom==EPCGActorFilterExtended::AllWorldActors || (bIncludeChildren && !bDisableFilter)) && ActorSelection==EPCGActorSelection::ByClass", EditConditionHides, AllowAbstract = "true", PCG_NotOverridable))
	TSubclassOf<AActor> ActorSelectionClass;

	/** If true processes all matching actors, otherwise returns data from first match. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ActorSelectorSettings", meta = (EditCondition = "bShowSelectMultiple && ActorFilterCustom==EPCGActorFilterExtended::AllWorldActors && ActorSelection!=EPCGActorSelection::ByName", EditConditionHides, PCG_NotOverridable))
	bool bSelectMultiple = false;

	/** If true, ignores results found from within this actor's hierarchy */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ActorSelectorSettings", meta = (EditCondition = "ActorFilterCustom==EPCGActorFilterExtended::AllWorldActors", EditConditionHides, PCG_NotOverridable))
	bool bIgnoreSelfAndChildren = false;

	// Properties used to hide some fields when used in different contexts
	UPROPERTY(Transient, meta = (EditCondition = false, EditConditionHides, PCG_NotOverridable))
	bool bShowActorFilter = true;

	UPROPERTY(Transient, meta = (EditCondition = false, EditConditionHides, PCG_NotOverridable))
	bool bShowIncludeChildren = true;

	UPROPERTY(Transient, meta = (EditCondition = false, EditConditionHides, PCG_NotOverridable))
	bool bShowActorSelection = true;

	UPROPERTY(Transient, meta = (EditCondition = false, EditConditionHides, PCG_NotOverridable))
	bool bShowActorSelectionClass = true;

	UPROPERTY(Transient, meta = (EditCondition = false, EditConditionHides, PCG_NotOverridable))
	bool bShowSelectMultiple = true;

#if WITH_EDITOR	
	FText GetTaskNameSuffix() const;
	FName GetTaskName(const FText& Prefix) const;
#endif

	FPCGActorSelectionKeyExtended GetAssociatedKey() const;
	static FPCGCActorSelectorExtendedSettings ReconstructFromKey(const FPCGActorSelectionKey& InKey);
};

namespace PCGCActorSelectorExtended
{
	TArray<AActor*> FindActors(const FPCGCActorSelectorExtendedSettings& Settings, const UPCGComponent* InComponent, const TFunction<bool(const AActor*)>& BoundsCheck, const TFunction<bool(const AActor*)>& SelfIgnoreCheck);
	AActor* FindActor(const FPCGCActorSelectorExtendedSettings& InSettings, UPCGComponent* InComponent, const TFunction<bool(const AActor*)>& BoundsCheck, const TFunction<bool(const AActor*)>& SelfIgnoreCheck);
}
