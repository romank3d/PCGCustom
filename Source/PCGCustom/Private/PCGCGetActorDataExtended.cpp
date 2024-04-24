// Copyright Roman K., Epic Games, Inc. All Rights Reserved. 


#include "PCGCGetActorDataExtended.h"

#include "PCGActorAndComponentMapping.h"
#include "PCGComponent.h"
#include "PCGModule.h"
#include "PCGCustomVersion.h"
#include "PCGSubsystem.h"
#include "Data/PCGPointData.h"
#include "PCGParamData.h"
#include "Data/PCGSpatialData.h"
#include "Data/PCGVolumeData.h"
#include "Elements/PCGMergeElement.h"
#include "Helpers/PCGHelpers.h"
#include "Helpers/PCGSettingsHelpers.h"
#include "Grid/PCGPartitionActor.h"

#include "Algo/AnyOf.h"
#include "GameFramework/Actor.h"
#include "UObject/Package.h"
#include "Internationalization/Text.h"

#include "Metadata/Accessors/PCGAttributeAccessorHelpers.h"
#include "Components/BillboardComponent.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(PCGCGetActorDataExtended)


#define LOCTEXT_NAMESPACE "PCGCGetActorDataExtendedElement"

namespace PCGDataFromActorConstants
{
	static const FName SinglePointPinLabel = TEXT("Single Point");
	static const FString PCGComponentDataGridSizeTagPrefix = TEXT("PCG_GridSize_");
	static const FText TagNamesSanitizedWarning = LOCTEXT("TagAttributeNamesSanitized", "One or more tag names contained invalid characters and were sanitized when creating the corresponding attributes.");
}

namespace PCGDataFromActorHelpers
{
	/**
	 * Get the PCG Components associated with an actor. Optionally, also search for any local components associated with components
	 * on the actor using the 'bGetLocalComponents' flag. By default, gets data on all grids, but alternatively you can provide a
	 * set of 'AllowedGrids' to match against.
	 *
	 * If 'bMustOverlap' is true, it will only collect components which overlap with the given 'OverlappingBounds'. Note that this
	 * overlap does not include bounds which are only touching, with no overlapping volume.
	 */
	static TInlineComponentArray<UPCGComponent*, 1> GetPCGComponentsFromActor(
		AActor* Actor,
		UPCGSubsystem* Subsystem,
		bool bGetLocalComponents = false,
		bool bGetAllGrids = true,
		int32 AllowedGrids = (int32)EPCGHiGenGrid::Uninitialized,
		bool bMustOverlap = false,
		const FBox& OverlappingBounds = FBox())
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FPCGDataFromActorElement::GetPCGComponentsFromActor);

		TInlineComponentArray<UPCGComponent*, 1> PCGComponents;

		if (!Actor || !Subsystem)
		{
			return PCGComponents;
		}

		Actor->GetComponents(PCGComponents);

		if (bMustOverlap)
		{
			// Remove actor components that do not overlap the source bounds.
			// Note: This assumes that a local component always lies inside the bounds of its original component,
			// which is true at the time of writing, but may not always be the case (e.g. "truly" unbounded execution).
			for (int I = PCGComponents.Num() - 1; I >= 0; I--)
			{
				const FBox ComponentBounds = PCGComponents[I]->GetGridBounds();

				// We reject overlaps with zero volume instead of simply checking Intersect(...) to avoid bounds which touch but do not overlap.
				if (OverlappingBounds.Overlap(ComponentBounds).GetVolume() <= 0)
				{
					PCGComponents.RemoveAtSwap(I);
				}
			}
		}

		TArray<UPCGComponent*> LocalComponents;

		if (bGetLocalComponents)
		{
			auto AddComponent = [&LocalComponents, bGetAllGrids, AllowedGrids](UPCGComponent* LocalComponent)
			{
				if (bGetAllGrids || (AllowedGrids & (int32)LocalComponent->GetGenerationGrid()))
				{
					LocalComponents.Add(LocalComponent);
				}
			};

			// Collect the local components for each actor PCG component.
			for (UPCGComponent* Component : PCGComponents)
			{
				if (Component && Component->IsPartitioned())
				{
					if (bMustOverlap)
					{
						Subsystem->ForAllRegisteredIntersectingLocalComponents(Component, OverlappingBounds, AddComponent);
					}
					else
					{
						Subsystem->ForAllRegisteredLocalComponents(Component, AddComponent);
					}
				}
			}
		}

		// Remove the actor's PCG components if they aren't on an allowed grid size.
		// Implementation note: We delay removing these components until now because they may have had local components on an allowed grid size.
		if (!bGetAllGrids)
		{
			for (int I = PCGComponents.Num() - 1; I >= 0; I--)
			{
				if (!(AllowedGrids & (int32)PCGComponents[I]->GetGenerationGridSize()))
				{
					PCGComponents.RemoveAtSwap(I);
				}
			}
		}

		if (bGetLocalComponents)
		{
			PCGComponents.Append(LocalComponents);
		}

		return PCGComponents;
	}
}


#if WITH_EDITOR
void UPCGCGetActorDataExtendedSettings::GetStaticTrackedKeys(FPCGSelectionKeyToSettingsMap& OutKeysToSettings, TArray<TObjectPtr<const UPCGGraph>>& OutVisitedGraphs) const
{
	FPCGSelectionKey Key = ActorSelector.GetAssociatedKey();
	if (Mode == EPCGGetDataFromActorModeExtended::GetDataFromPCGComponent || Mode == EPCGGetDataFromActorModeExtended::GetDataFromPCGComponentOrParseComponents)
	{
		Key.SetExtraDependency(UPCGComponent::StaticClass());
	}

	OutKeysToSettings.FindOrAdd(Key).Emplace(this, bTrackActorsOnlyWithinBounds);
}

uint32 GetTypeHash(const FPCGSelectionKey& In)
{
	uint32 HashResult = HashCombine(GetTypeHash(In.ActorFilter), GetTypeHash(In.Selection));
	HashResult = HashCombine(HashResult, GetTypeHash(In.Tag));
	HashResult = HashCombine(HashResult, GetTypeHash(In.SelectionClass));
	HashResult = HashCombine(HashResult, GetTypeHash(In.OptionalExtraDependency));
	HashResult = HashCombine(HashResult, GetTypeHash(In.ObjectPath));

	return HashResult;
}

void UPCGCGetActorDataExtendedSettings::ApplyDeprecation(UPCGNode* InOutNode)
{
	if (DataVersion < FPCGCustomVersion::GetPCGComponentDataMustOverlapSourceComponentByDefault)
	{
		// Old versions of GetActorData did not require found components to overlap self, but going forward it's a more efficient default.
		bComponentsMustOverlapSelf = false;
	}

	Super::ApplyDeprecation(InOutNode);
}

FText UPCGCGetActorDataExtendedSettings::GetNodeTooltipText() const
{
	return LOCTEXT("DataFromActorTooltip", "Builds a collection of PCG-compatible data from the selected actors.");
}

void UPCGCGetActorDataExtendedSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(UPCGCGetActorDataExtendedSettings, ActorSelector))
	{
		if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(FPCGActorSelectorSettings, ActorSelection))
		{
			// Make sure that when switching away from the 'by class' selection, we actually break that data dependency
			if (ActorSelector.ActorSelection != EPCGActorSelection::ByClass)
			{
				ActorSelector.ActorSelectionClass = GetDefaultActorSelectorClass();
			}
		}
	}
}
#endif

TSubclassOf<AActor> UPCGCGetActorDataExtendedSettings::GetDefaultActorSelectorClass() const
{
	return TSubclassOf<AActor>();
}

void UPCGCGetActorDataExtendedSettings::PostLoad()
{
	Super::PostLoad();

	if (ActorSelector.ActorSelection != EPCGActorSelection::ByClass)
	{
		ActorSelector.ActorSelectionClass = GetDefaultActorSelectorClass();
	}
}

FString UPCGCGetActorDataExtendedSettings::GetAdditionalTitleInformation() const
{
#if WITH_EDITOR
	return ActorSelector.GetTaskNameSuffix().ToString();
#else
	return Super::GetAdditionalTitleInformation();
#endif
}

FPCGElementPtr UPCGCGetActorDataExtendedSettings::CreateElement() const
{
	return MakeShared<FPCGCGetActorDataExtendedElement>();
}


TArray<FPCGPinProperties> UPCGCGetActorDataExtendedSettings::OutputPinProperties() const
{

	TArray<FPCGPinProperties> Pins = {};

	if (bGetSpatialData)
	{

		if (Mode == EPCGGetDataFromActorModeExtended::GetSinglePoint)
		{
			Pins.Emplace(PCGPinConstants::DefaultOutputLabel, EPCGDataType::Point);
		}
		else if (Mode == EPCGGetDataFromActorModeExtended::GetDataFromProperty)
		{
			Pins.Emplace(PCGPinConstants::DefaultOutputLabel, EPCGDataType::Param);
		}
		else {
			Pins.Emplace(PCGPinConstants::DefaultOutputLabel, EPCGDataType::Spatial);
		}

		if (Mode == EPCGGetDataFromActorModeExtended::GetDataFromPCGComponent || Mode == EPCGGetDataFromActorModeExtended::GetDataFromPCGComponentOrParseComponents)
		{
			for (const FName& Pin : ExpectedPins)
			{
				Pins.Emplace(Pin);
			}
		}
	}

	if (bGetActorProperties) {
		Pins.Emplace(PropertiesPinName, EPCGDataType::Param);
	}

	if (bGetActorComponentsAsPoints) {
		Pins.Emplace(ComponentsPinName, EPCGDataType::Point);
	}

	return Pins;
}

FPCGContext* FPCGCGetActorDataExtendedElement::CreateContext()
{
	return new FPCGDataFromActorContext();
}

//FPCGContext* FPCGCGetActorDataExtendedElement::Initialize(const FPCGDataCollection& InputData, TWeakObjectPtr<UPCGComponent> SourceComponent, const UPCGNode* Node)
//{
//	FPCGDataFromActorContext* Context = new FPCGDataFromActorContext();
//	Context->InputData = InputData;
//	Context->SourceComponent = SourceComponent;
//	Context->Node = Node;
//
//	return Context;
//}

bool FPCGCGetActorDataExtendedElement::ExecuteInternal(FPCGContext* InContext) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGCGetActorDataExtendedElement::Execute);

	check(InContext);
	FPCGDataFromActorContext* Context = static_cast<FPCGDataFromActorContext*>(InContext);

	const UPCGCGetActorDataExtendedSettings* Settings = Context->GetInputSettings<UPCGCGetActorDataExtendedSettings>();
	check(Settings);

	if (!Context->bPerformedQuery)
	{
		TFunction<bool(const AActor*)> BoundsCheck = [](const AActor*) -> bool { return true; };
		const UPCGComponent* PCGComponent = Context->SourceComponent.IsValid() ? Context->SourceComponent.Get() : nullptr;
		const AActor* Self = PCGComponent ? PCGComponent->GetOwner() : nullptr;
		if (Self && Settings->ActorSelector.bMustOverlapSelf)
		{
			// Capture ActorBounds by value because it goes out of scope
			const FBox ActorBounds = PCGHelpers::GetActorBounds(Self);
			BoundsCheck = [Settings, ActorBounds, PCGComponent](const AActor* OtherActor) -> bool
			{
				const FBox OtherActorBounds = OtherActor ? PCGHelpers::GetGridBounds(OtherActor, PCGComponent) : FBox(EForceInit::ForceInit);
				return ActorBounds.Intersect(OtherActorBounds);
			};
		}

		TFunction<bool(const AActor*)> SelfIgnoreCheck = [](const AActor*) -> bool { return true; };
		if (Self && Settings->ActorSelector.bIgnoreSelfAndChildren)
		{
			SelfIgnoreCheck = [Self](const AActor* OtherActor) -> bool
			{
				// Check if OtherActor is a child of self
				const AActor* CurrentOtherActor = OtherActor;
				while (CurrentOtherActor)
				{
					if (CurrentOtherActor == Self)
					{
						return false;
					}

					CurrentOtherActor = CurrentOtherActor->GetParentActor();
				}

				// Check if Self is a child of OtherActor
				const AActor* CurrentSelfActor = Self;
				while (CurrentSelfActor)
				{
					if (CurrentSelfActor == OtherActor)
					{
						return false;
					}

					CurrentSelfActor = CurrentSelfActor->GetParentActor();
				}

				return true;
			};
		}

		// When gathering PCG data on any world actor, we can leverage the octree kept by the Tracking system, and get all intersecting components if we need to overlap self
		// or just gather all registered components (which is way faster than going through all actors in the world).
		if (Settings->Mode == EPCGGetDataFromActorModeExtended::GetDataFromPCGComponent && Settings->ActorSelector.ActorFilter == EPCGActorFilter::AllWorldActors)
		{
			UPCGSubsystem* Subsystem = Context->SourceComponent.IsValid() ? Context->SourceComponent->GetSubsystem() : nullptr;
			if (Subsystem)
			{
				TRACE_CPUPROFILER_EVENT_SCOPE(FPCGDataFromActorElement::Execute::FindPCGComponents);

				const FPCGSelectionKey Key = Settings->ActorSelector.GetAssociatedKey();

				// TODO: Perhaps move the logic into the selector.
				if (Settings->ActorSelector.bMustOverlapSelf)
				{
					FBox ActorBounds = PCGHelpers::GetGridBounds(Self, PCGComponent);
					for (UPCGComponent* Component : Subsystem->GetAllIntersectingComponents(ActorBounds))
					{
						if (AActor* Actor = Component->GetOwner())
						{
							if (Key.IsMatching(Actor, Component))
							{
								Context->FoundActors.Add(Actor);
							}
						}
					}
				}
				else
				{
					for (UPCGComponent* Component : Subsystem->GetAllRegisteredComponents())
					{
						if (AActor* Actor = Component->GetOwner())
						{
							if (Key.IsMatching(Actor, Component))
							{
								Context->FoundActors.Add(Actor);
							}
						}
					}
				}

				Context->bPerformedQuery = true;
			}
		}

		if (!Context->bPerformedQuery)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(FPCGCGetActorDataExtendedElement::Execute::FindActors);
			Context->FoundActors = PCGActorSelector::FindActors(Settings->ActorSelector, Context->SourceComponent.Get(), BoundsCheck, SelfIgnoreCheck);
			Context->bPerformedQuery = true;
		}

		if (Context->FoundActors.IsEmpty())
		{
			PCGE_LOG(Verbose, LogOnly, LOCTEXT("NoActorFound", "No matching actor was found"));
			return true;
		}

		if (Settings->bGetSpatialData)
		{
			// If we're looking for PCG component data, we might have to wait for it.
			if (Settings->Mode == EPCGGetDataFromActorModeExtended::GetDataFromPCGComponent || Settings->Mode == EPCGGetDataFromActorModeExtended::GetDataFromPCGComponentOrParseComponents)
			{
				TArray<FPCGTaskId> WaitOnTaskIds;
				for (AActor* Actor : Context->FoundActors)
				{
					GatherWaitTasks(Actor, Context, WaitOnTaskIds);
				}

				if (!WaitOnTaskIds.IsEmpty())
				{
					UPCGSubsystem* Subsystem = Context->SourceComponent.IsValid() ? Context->SourceComponent->GetSubsystem() : nullptr;
					if (Subsystem)
					{
						// Add a trivial task after these generations that wakes up this task
						Context->bIsPaused = true;

						Subsystem->ScheduleGeneric(
							[Context]() // Normal execution: Wake up the current task
							{
								Context->bIsPaused = false;
						return true;
							},
							[Context]() // On Abort: Wake up on abort, clear all results and mark as cancelled
							{
								Context->bIsPaused = false;
							Context->FoundActors.Reset();
							Context->OutputData.bCancelExecution = true;
							return true;
							},
								Context->SourceComponent.Get(),
								WaitOnTaskIds);

						return false;
					}
					else
					{
						PCGE_LOG(Error, GraphAndLog, LOCTEXT("UnableToWaitForGenerationTasks", "Unable to wait for end of generation tasks"));
					}
				}
			}
		}
	}

	if (Context->bPerformedQuery)
	{
#if WITH_EDITOR
		// Remove ignored change origins now that we've completed the wait tasks.
		UPCGComponent* OriginalComponent = Context->SourceComponent->GetOriginalComponent();
		for (TObjectKey<UObject>& IgnoredChangeOriginKey : Context->IgnoredChangeOrigins)
		{
			if (UObject* IgnoredChangeOrigin = IgnoredChangeOriginKey.ResolveObjectPtr())
			{
				OriginalComponent->StopIgnoringChangeOriginDuringGeneration(IgnoredChangeOrigin);
			}
		}
#endif

		ProcessActors(Context, Settings, Context->FoundActors);
	}

	return true;
}

void FPCGCGetActorDataExtendedElement::GatherWaitTasks(AActor* FoundActor, FPCGContext* InContext, TArray<FPCGTaskId>& OutWaitTasks) const
{
	if (!FoundActor)
	{
		return;
	}

	FPCGDataFromActorContext* Context = static_cast<FPCGDataFromActorContext*>(InContext);
	check(Context);

	const UPCGCGetActorDataExtendedSettings* Settings = Context->GetInputSettings<UPCGCGetActorDataExtendedSettings>();
	check(Settings);

	UPCGComponent* SourceComponent = Context->SourceComponent.IsValid() ? Context->SourceComponent.Get() : nullptr;
	const UPCGComponent* SourceOriginalComponent = SourceComponent ? SourceComponent->GetOriginalComponent() : nullptr;

	if (!SourceOriginalComponent)
	{
		return;
	}

	// We will prevent gathering the current execution - this task cannot wait on itself
	const AActor* SourceOwner = SourceOriginalComponent->GetOwner();

	TInlineComponentArray<UPCGComponent*, 1> PCGComponents = PCGDataFromActorHelpers::GetPCGComponentsFromActor(
		FoundActor,
		SourceComponent->GetSubsystem(),
		/*bGetLocalComponents=*/true,
		Settings->bGetDataOnAllGrids,
		Settings->AllowedGrids,
		Settings->bComponentsMustOverlapSelf,
		Settings->bComponentsMustOverlapSelf ? SourceComponent->GetGridBounds() : FBox());

	for (UPCGComponent* Component : PCGComponents)
	{
		const UPCGComponent* OriginalComponent = Component ? Component->GetOriginalComponent() : nullptr;

		// Avoid waiting on our own execution (including local components).
		if (!OriginalComponent || OriginalComponent == SourceOriginalComponent || (Settings->ActorSelector.bIgnoreSelfAndChildren && OriginalComponent->GetOwner() == SourceOwner))
		{
			continue;
		}

		if (Component->IsGenerating())
		{
			OutWaitTasks.Add(Component->GetGenerationTaskId());
		}
		else if (!Component->bGenerated && Component->bActivated && (Component->GetSerializedEditingMode() == EPCGEditorDirtyMode::Preview) && Component->GetOwner())
		{
#if WITH_EDITOR
			// Signal that any change notifications from generating upstream component should not trigger re-executions of this component.
			// Such change notifications can cancel the current execution.
			// Note: Uses owner because FPCGActorAndComponentMapping::OnPCGGraphGeneratedOrCleaned reports change on owner.
			SourceComponent->GetOriginalComponent()->StartIgnoringChangeOriginDuringGeneration(Component->GetOwner());
			Context->IgnoredChangeOrigins.Add(Component->GetOwner());
#endif

			const FPCGTaskId GenerateTask = Component->GenerateLocalGetTaskId(EPCGComponentGenerationTrigger::GenerateOnDemand, /*bForce=*/false);
			if (GenerateTask != InvalidPCGTaskId)
			{
				//PCGGraphExecutionLogging::LogGraphScheduleDependency(Component, Context->Stack);

				OutWaitTasks.Add(GenerateTask);
			}
			else
			{
				//PCGGraphExecutionLogging::LogGraphScheduleDependencyFailed(Component, Context->Stack);
			}
		}
	}
}

void FPCGCGetActorDataExtendedElement::ProcessActors(FPCGContext* Context, const UPCGCGetActorDataExtendedSettings* Settings, const TArray<AActor*>& FoundActors) const
{

	if (Settings->bGetSpatialData)
	{
		// Special case:
		// If we're asking for single point with the merge single point data, we can do a more efficient process
		if (Settings->Mode == EPCGGetDataFromActorModeExtended::GetSinglePoint && Settings->bMergeSinglePointData && FoundActors.Num() > 1)
		{
			MergeActorsIntoPointData(Context, Settings, FoundActors);
		}
		else
		{
			for (AActor* Actor : FoundActors)
			{
				ProcessActor(Context, Settings, Actor);
			}
		}
	}
	if (Settings->bGetActorProperties)
	{
		for (AActor* Actor : FoundActors)
		{
			GetActorProperties(Context, Settings, Actor);
		}
	}
	if (Settings->bGetActorComponentsAsPoints)
	{
		for (AActor* Actor : FoundActors)
		{
			GetActorComponentsAsPoints(Context, Settings, Actor);
		}
	}

}

void FPCGCGetActorDataExtendedElement::MergeActorsIntoPointData(FPCGContext* Context, const UPCGCGetActorDataExtendedSettings* Settings, const TArray<AActor*>& FoundActors) const
{
	check(Context);

	// At this point in time, the partition actors behave slightly differently, so if we are in the case where
	// we have one or more partition actors, we'll go through the normal process and do post-processing to merge the point data instead.
	const bool bContainsPartitionActors = Algo::AnyOf(FoundActors, [](const AActor* Actor) { return Cast<APCGPartitionActor>(Actor) != nullptr; });

//	const bool bContainsPartitionActors = Algo::AnyOf(FoundActors, [](const AActor* Actor) {
//
//		TInlineComponentArray<UPCGComponent*, 1> PCGComponents;
//		Actor->GetComponents(PCGComponents);
//		if (PCGComponents.IsEmpty()) {
//			return false;
//		}
//		else {
//			for (UPCGComponent* Component : PCGComponents)
//			{
//				if (Component->IsPartitioned())
//				{
//					return true;
//				}
//			}
//		}
//		return false; 
//		});

	if (!bContainsPartitionActors)
	{
		UPCGPointData* Data = NewObject<UPCGPointData>();
		bool bHasData = false;
		bool bAnyAttributeNameWasSanitized = false;

		for (AActor* Actor : FoundActors)
		{
			if (Actor)
			{
				bool bAttributeNameWasSanitized = false;
				Data->AddSinglePointFromActor(Actor, &bAttributeNameWasSanitized);

				bAnyAttributeNameWasSanitized |= bAttributeNameWasSanitized;

				bHasData = true;
			}
		}

		if (bAnyAttributeNameWasSanitized && !Settings->bSilenceSanitizedAttributeNameWarnings)
		{
			PCGE_LOG(Warning, GraphAndLog, PCGDataFromActorConstants::TagNamesSanitizedWarning);
		}

		if (bHasData)
		{
			FPCGTaggedData& TaggedData = Context->OutputData.TaggedData.Emplace_GetRef();
			TaggedData.Data = Data;
		}
	}
	else // Stripped-down version of the normal code path with bParseActor = false
	{

		FPCGDataCollection DataToMerge;
		const bool bParseActor = false;
		bool bAnyAttributeNameWasSanitized = false;

		for (AActor* Actor : FoundActors)
		{
			if (Actor)
			{
				bool bAttributeNameWasSanitized = false;
				FPCGDataCollection Collection = UPCGComponent::CreateActorPCGDataCollection(Actor, Context->SourceComponent.Get(), EPCGDataType::Any, bParseActor, &bAttributeNameWasSanitized);

				bAnyAttributeNameWasSanitized |= bAttributeNameWasSanitized;

				DataToMerge.TaggedData += Collection.TaggedData;
			}
		}

		if (bAnyAttributeNameWasSanitized && !Settings->bSilenceSanitizedAttributeNameWarnings)
		{
			PCGE_LOG(Warning, GraphAndLog, PCGDataFromActorConstants::TagNamesSanitizedWarning);
		}
	
		 //Perform point data-to-point data merge
		if (DataToMerge.TaggedData.Num() > 1)
		{
			TArray<FPCGTaggedData>& Sources = DataToMerge.TaggedData;
			UPCGPointData* TargetPointData = nullptr;
			FPCGTaggedData* TargetTaggedData = nullptr;

			TArray<FPCGTaggedData>& Outputs = Context->OutputData.TaggedData;

			Outputs.SetNumUninitialized(0);

			const FPCGTaggedData& Source = Sources[0];

			TargetPointData = NewObject<UPCGPointData>();
			TargetTaggedData = &(Outputs.Emplace_GetRef(Source));
			TargetTaggedData->Data = TargetPointData;

			TArray<FPCGPoint>& TargetPoints = TargetPointData->GetMutablePoints();

			for (int32 SourceIndex = 0; SourceIndex < Sources.Num(); ++SourceIndex)
			{
				const UPCGPointData* SourcePointData = Cast<const UPCGPointData>(Sources[SourceIndex].Data);

				TargetPoints.Append(SourcePointData->GetPoints());
			}
		}
		else if (DataToMerge.TaggedData.Num() == 1)
		{
			Context->OutputData.TaggedData = DataToMerge.TaggedData;
		}

	}
}

void FPCGCGetActorDataExtendedElement::ProcessActor(FPCGContext* Context, const UPCGCGetActorDataExtendedSettings* Settings, AActor* FoundActor) const
{
	check(Context);
	check(Settings);

	UPCGComponent* SourceComponent = Context->SourceComponent.IsValid() ? Context->SourceComponent.Get() : nullptr;
	const UPCGComponent* SourceOriginalComponent = SourceComponent ? SourceComponent->GetOriginalComponent() : nullptr;

	if (!FoundActor || !IsValid(FoundActor) || !SourceOriginalComponent)
	{
		return;
	}

	const AActor* SourceOwner = SourceOriginalComponent->GetOwner();
	TInlineComponentArray<UPCGComponent*, 1> PCGComponents;
	bool bHasGeneratedPCGData = false;
	FProperty* FoundProperty = nullptr;

	if (Settings->Mode == EPCGGetDataFromActorModeExtended::GetDataFromPCGComponent || Settings->Mode == EPCGGetDataFromActorModeExtended::GetDataFromPCGComponentOrParseComponents)
	{
		PCGComponents = PCGDataFromActorHelpers::GetPCGComponentsFromActor(
			FoundActor,
			SourceComponent->GetSubsystem(),
			/*bGetLocalComponents=*/true,
			Settings->bGetDataOnAllGrids,
			Settings->AllowedGrids,
			Settings->bComponentsMustOverlapSelf,
			Settings->bComponentsMustOverlapSelf ? SourceComponent->GetGridBounds() : FBox());

		// Remove any PCG components that don't belong to an external execution context (i.e. share the same original component), or that share a common root actor.
		PCGComponents.RemoveAllSwap([Settings, SourceOwner, SourceOriginalComponent](UPCGComponent* Component)
			{
				const UPCGComponent* OriginalComponent = Component ? Component->GetOriginalComponent() : nullptr;

		return !OriginalComponent
			|| OriginalComponent == SourceOriginalComponent
			|| (Settings->ActorSelector.bIgnoreSelfAndChildren && OriginalComponent->GetOwner() == SourceOwner);
			});

		for (UPCGComponent* Component : PCGComponents)
		{
			bHasGeneratedPCGData |= !Component->GetGeneratedGraphOutput().TaggedData.IsEmpty();
		}
	}
	else if (Settings->Mode == EPCGGetDataFromActorModeExtended::GetDataFromProperty)
	{
		if (Settings->PropertyName != NAME_None)
		{
			FoundProperty = FindFProperty<FProperty>(FoundActor->GetClass(), Settings->PropertyName);
		}
	}

	// Some additional validation
	if (Settings->Mode == EPCGGetDataFromActorModeExtended::GetDataFromPCGComponent && !bHasGeneratedPCGData)
	{
		if (!PCGComponents.IsEmpty())
		{
			PCGE_LOG(Log, GraphAndLog, FText::Format(LOCTEXT("ActorHasNoGeneratedData", "Actor '{0}' does not have any previously generated data"), FText::FromName(FoundActor->GetFName())));
		}

		return;
	}
	else if (Settings->Mode == EPCGGetDataFromActorModeExtended::GetDataFromProperty && !FoundProperty)
	{
		PCGE_LOG(Warning, GraphAndLog, FText::Format(LOCTEXT("ActorHasNoProperty", "Actor '{0}' does not have a property name '{1}'"), FText::FromName(FoundActor->GetFName()), FText::FromName(Settings->PropertyName)));
		return;
	}

	TArray<FPCGTaggedData>& Outputs = Context->OutputData.TaggedData;

	if (bHasGeneratedPCGData)
	{
		for (UPCGComponent* Component : PCGComponents)
		{
		//	// TODO - Temporary behavior
		//	// At the moment, intersections that reside in the transient package can hold on to a reference on this data
		//	// which prevents proper garbage collection on map change, hence why we duplicate here. Normally, we would expect
		//	// this not to be a problem, as these intersections should be garbage collected, but this requires more investigation
			for (const FPCGTaggedData& TaggedData : Component->GetGeneratedGraphOutput().TaggedData)
			{
				FPCGTaggedData& DuplicatedTaggedData = Outputs.Add_GetRef(TaggedData);
				//DuplicatedTaggedData.Data = Cast<UPCGData>(StaticDuplicateObject(TaggedData.Data, GetTransientPackage()));

				//FObjectDuplicationParameters DupParams = FObjectDuplicationParameters(Cast<UObject>(TaggedData.Data), Cast<UObject>(GetTransientPackage()));
				//DuplicatedTaggedData.Data = Cast<UPCGData>(StaticDuplicateObjectEx(DupParams));
			}
			//Outputs.Append(Component->GetGeneratedGraphOutput().TaggedData);
		}
	}
	else if (FoundProperty)
	{
		bool bAbleToGetProperty = false;
		const void* PropertyAddressData = FoundProperty->ContainerPtrToValuePtr<void>(FoundActor);
		// TODO: support more property types here
		// Pointer to UPCGData
		// Soft object pointer to UPCGData
		// Array of pcg data -> all on the default pin
		// Map of pcg data -> use key for pin? might not be robust
		// FPCGDataCollection
		if (const FStructProperty* StructProperty = CastField<FStructProperty>(FoundProperty))
		{
			if (StructProperty->Struct == FPCGDataCollection::StaticStruct())
			{
				const FPCGDataCollection* CollectionInProperty = reinterpret_cast<const FPCGDataCollection*>(PropertyAddressData);
				Outputs.Append(CollectionInProperty->TaggedData);

				bAbleToGetProperty = true;
			}
		}

		if (!bAbleToGetProperty)
		{
			PCGE_LOG(Warning, GraphAndLog, FText::Format(LOCTEXT("PropertyTypeUnsupported", "Actor '{0}' property '{1}' does not have a supported type"), FText::FromName(FoundActor->GetFName()), FText::FromName(Settings->PropertyName)));
		}
	}
	else
	{
		const bool bParseActor = (Settings->Mode != EPCGGetDataFromActorModeExtended::GetSinglePoint);
		bool bAttributeNameWasSanitized = false;
		FPCGDataCollection Collection = UPCGComponent::CreateActorPCGDataCollection(FoundActor, SourceComponent, Settings->GetDataFilter(), bParseActor);
		
		if (bAttributeNameWasSanitized && !Settings->bSilenceSanitizedAttributeNameWarnings)
		{
			PCGE_LOG(Warning, GraphAndLog, PCGDataFromActorConstants::TagNamesSanitizedWarning);
		}

		Outputs += Collection.TaggedData;

		for (FPCGTaggedData& Output : Outputs) {
			Output.Pin = PCGPinConstants::DefaultOutputLabel;
		}
	}

	// Finally, if we're in a case where we need to output the single point data too, let's do it now.
	if (Settings->bAlsoOutputSinglePointData && (Settings->Mode == EPCGGetDataFromActorModeExtended::GetDataFromPCGComponent || Settings->Mode == EPCGGetDataFromActorModeExtended::GetDataFromPCGComponentOrParseComponents))
	{
		const bool bParseActor = false;
		bool bAttributeNameWasSanitized = false;
		FPCGDataCollection Collection = UPCGComponent::CreateActorPCGDataCollection(FoundActor, SourceComponent, EPCGDataType::Any, bParseActor, &bAttributeNameWasSanitized);

		if (bAttributeNameWasSanitized && !Settings->bSilenceSanitizedAttributeNameWarnings)
		{
			PCGE_LOG(Warning, GraphAndLog, PCGDataFromActorConstants::TagNamesSanitizedWarning);
		}

		for (const FPCGTaggedData& SinglePointData : Collection.TaggedData)
		{
			FPCGTaggedData& OutSinglePoint = Outputs.Add_GetRef(SinglePointData);
			OutSinglePoint.Pin = PCGDataFromActorConstants::SinglePointPinLabel;
		}
	}
}

void FPCGCGetActorDataExtendedElement::GetDependenciesCrc(const FPCGDataCollection& InInput, const UPCGSettings* InSettings, UPCGComponent* InComponent, FPCGCrc& OutCrc) const
{
	FPCGCrc Crc;
	IPCGElement::GetDependenciesCrc(InInput, InSettings, InComponent, Crc);

	// If we track self or original, we are dependent on the actor data
	if (const UPCGCGetActorDataExtendedSettings* Settings = Cast<const UPCGCGetActorDataExtendedSettings>(InSettings))
	{
		const bool bDependsOnSelfOrHierarchy = (Settings->ActorSelector.ActorFilter == EPCGActorFilter::Self || Settings->ActorSelector.ActorFilter == EPCGActorFilter::Original);
		const bool bDependsOnSelfBounds = Settings->ActorSelector.bMustOverlapSelf;

		if (InComponent && (bDependsOnSelfOrHierarchy || bDependsOnSelfBounds))
		{
			UPCGComponent* ComponentToCheck = (Settings->ActorSelector.ActorFilter == EPCGActorFilter::Original) ? InComponent->GetOriginalComponent() : InComponent;
			const UPCGData* ActorData = ComponentToCheck ? ComponentToCheck->GetActorPCGData() : nullptr;

			if (ActorData)
			{
				Crc.Combine(ActorData->GetOrComputeCrc(/*bFullDataCrc=*/false));
			}
		}

		const bool bDependsOnComponentData = Settings->Mode == EPCGGetDataFromActorModeExtended::GetDataFromPCGComponent || Settings->Mode == EPCGGetDataFromActorModeExtended::GetDataFromPCGComponentOrParseComponents;
		const bool bDependsOnLocalComponentBounds = Settings->bComponentsMustOverlapSelf || !Settings->bGetDataOnAllGrids;

		if (InComponent && bDependsOnComponentData && bDependsOnLocalComponentBounds)
		{
			if (const UPCGData* LocalActorData = InComponent->GetActorPCGData())
			{
				Crc.Combine(LocalActorData->GetOrComputeCrc(/*bFullDataCrc=*/false));
			}
		}
	}

	OutCrc = Crc;
}

void FPCGCGetActorDataExtendedElement::GetActorProperties(FPCGContext* Context, const UPCGCGetActorDataExtendedSettings* Settings, AActor* FoundActor) const
{
	check(Context);
	check(Settings);

	if (!FoundActor || !IsValid(FoundActor) || Settings->PropertiesNames.IsEmpty())
	{
		return;
	}

	UObject* ObjectToInspect = FoundActor;
	using ExtractablePropertyTuple = TTuple<FName, const void*, const FProperty*>;
	TArray<ExtractablePropertyTuple> ExtractableProperties;

	const uint64 ExcludePropertyFlags = CPF_DisableEditOnInstance;
	const uint64 IncludePropertyFlags = CPF_BlueprintVisible;

	if (!Settings->PropertiesNames.IsEmpty()) {

		//For each property name
		for(FName PropertyName : Settings->PropertiesNames)
		{
			if (PropertyName != NAME_None) {

				//Try to find property
				FProperty* Property = FindFProperty<FProperty>(ObjectToInspect->GetClass(), PropertyName);
				if (!Property)
				{
					PCGE_LOG(Error, GraphAndLog, FText::Format(LOCTEXT("PropertyDoesNotExist", "Property '{0}' does not exist in the found actor"), FText::FromName(PropertyName)));
					return;
				}
			
				//Check property flags
				if (Property->HasAnyPropertyFlags(ExcludePropertyFlags) || !Property->HasAnyPropertyFlags(IncludePropertyFlags))
				{
					PCGE_LOG(Error, GraphAndLog, FText::Format(LOCTEXT("PropertyExistsButNotVisible", "Property '{0}' does exist in the found actor, but is not visible."), FText::FromName(PropertyName)));
					return;
				}

				//Process property and add it to the the list of extractable properties
				if (!PCGAttributeAccessorHelpers::IsPropertyAccessorSupported(Property) && (Property->IsA<FStructProperty>() || Property->IsA<FObjectProperty>())) 
				{
					//If property is a struct or Object
					UScriptStruct* UnderlyingStruct = nullptr;
					UClass* UnderlyingClass = nullptr;
					const void* ObjectAddress = nullptr;

					if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
					{
						UnderlyingStruct = StructProperty->Struct;
						ObjectAddress = StructProperty->ContainerPtrToValuePtr<void>(ObjectToInspect);
					}
					else if (FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
					{
						UnderlyingClass = ObjectProperty->PropertyClass;
						ObjectAddress = ObjectProperty->GetObjectPropertyValue_InContainer(ObjectToInspect);
					}

					check(UnderlyingStruct || UnderlyingClass);
					check(ObjectAddress);

					// Re-use code from overridable params
					// Limit ourselves to not recurse into more structs.
					PCGSettingsHelpers::FPCGGetAllOverridableParamsConfig Config;
					Config.bUseSeed = true;
					Config.bExcludeSuperProperties = true;
					Config.MaxStructDepth = 0;
					// Can only get exposed properties and visible
					Config.ExcludePropertyFlags = ExcludePropertyFlags;
					Config.IncludePropertyFlags = IncludePropertyFlags;

					TArray<FPCGSettingsOverridableParam> AllChildProperties = UnderlyingStruct ? PCGSettingsHelpers::GetAllOverridableParams(UnderlyingStruct, Config) : PCGSettingsHelpers::GetAllOverridableParams(UnderlyingClass, Config);


					for (const FPCGSettingsOverridableParam& Param : AllChildProperties)
					{
						if (ensure(!Param.PropertiesNames.IsEmpty()))
						{
							const FName ChildPropertyName = Param.PropertiesNames[0];
							if (const FProperty* ChildProperty = (UnderlyingStruct ? UnderlyingStruct->FindPropertyByName(ChildPropertyName) : UnderlyingClass->FindPropertyByName(ChildPropertyName)))
							{
								// We use authored name as attribute name to avoid issue with noisy property names, like in UUserDefinedStructs, where some random number is appended to the property name.
								// By default, it will just return the property name anyway.
								const FString AuthoredName = UnderlyingStruct ? UnderlyingStruct->GetAuthoredNameForField(ChildProperty) : UnderlyingClass->GetAuthoredNameForField(ChildProperty);
								ExtractableProperties.Emplace(FName(AuthoredName), ObjectAddress, ChildProperty);
							}
						}
					}

				}
				else {

					//If property is a regular type

					const FName AttributeName = Property->GetFName();
					ExtractableProperties.Emplace(AttributeName, ObjectToInspect, Property);
				}
			}
		}

	}

	if (ExtractableProperties.IsEmpty())
	{
		PCGE_LOG(Error, GraphAndLog, LOCTEXT("NoPropertiesFound", "No properties found to extract"));
		return;
	}

	UPCGParamData* ParamData = NewObject<UPCGParamData>();
	UPCGMetadata* Metadata = ParamData->MutableMetadata();
	check(Metadata);
	PCGMetadataEntryKey EntryKey = Metadata->AddEntry();
	bool bValidOperation = false;

	for (ExtractablePropertyTuple& ExtractableProperty : ExtractableProperties)
	{
		const FName AttributeName = ExtractableProperty.Get<0>();
		const void* ContainerPtr = ExtractableProperty.Get<1>();
		const FProperty* FinalProperty = ExtractableProperty.Get<2>();

		if (!Metadata->SetAttributeFromDataProperty(AttributeName, EntryKey, ContainerPtr, FinalProperty, /*bCreate=*/ true))
		{
			PCGE_LOG(Error, GraphAndLog, FText::Format(LOCTEXT("ErrorCreatingAttribute", "Error while creating an attribute for property '{0}'. Either the property type is not supported by PCG or attribute creation failed."), FText::FromString(FinalProperty->GetName())));
			continue;
		}

		bValidOperation = true;
	}

	if (bValidOperation)
	{
		TArray<FPCGTaggedData>& Outputs = Context->OutputData.TaggedData;
		FPCGTaggedData& Output = Outputs.Emplace_GetRef();
		Output.Pin = Settings->PropertiesPinName;
		Output.Data = ParamData;
	}


	
}

void FPCGCGetActorDataExtendedElement::GetActorComponentsAsPoints(FPCGContext* Context, const UPCGCGetActorDataExtendedSettings* Settings, AActor* FoundActor) const
{
	check(Context);
	check(Settings);

	if (!FoundActor || !IsValid(FoundActor))
	{
		return;
	}

	auto NameTagsToStringTags = [](const FName& InName) { return InName.ToString(); };
	TSet<FString> ActorTags;
	Algo::Transform(FoundActor->Tags, ActorTags, NameTagsToStringTags);
	

	using PrimitiveComponentArray = TInlineComponentArray<UPrimitiveComponent*, 4>;
	PrimitiveComponentArray Primitives;


	FoundActor->GetComponents(Primitives);


	UPCGPointData* PointData = NewObject<UPCGPointData>();
	TArray<FPCGPoint>& Points = PointData->GetMutablePoints();

	TArray<FPCGTaggedData>& Outputs = Context->OutputData.TaggedData;
	FPCGTaggedData& Output = Outputs.Emplace_GetRef();
	Output.Pin = Settings->ComponentsPinName;
	Output.Tags = ActorTags;
	Output.Data = PointData;

	TArray<FName> AttributeNames = Settings->ComponentTagAttributeNames;

	bool bParseTags = !AttributeNames.IsEmpty() ? true : false;

	TArray<FPCGMetadataAttribute<FName>*> Attributes;

	if (bParseTags) {

		for (int32 TagIndex = 0; TagIndex < AttributeNames.Num(); TagIndex++)
		{
			if (AttributeNames[TagIndex] == NAME_None) {
				continue;
			}

			Attributes.Add(PointData->Metadata->FindOrCreateAttribute<FName>(AttributeNames[TagIndex], NAME_None, /*bAllowInterpolation=*/false, /*bOverrideParent=*/false, /*bOverwriteIfTypeMismatch=*/ false));
			
		}

	}
		

	for (UPrimitiveComponent* PrimitiveComponent : Primitives)
	{
		// Exception: skip the billboard component
		if (Cast<UBillboardComponent>(PrimitiveComponent))
		{
			continue;
		}
		if (!Settings->ExclusionClasses.IsEmpty()) {
			bool bIsExcluded = false;
			for (TSubclassOf<UPrimitiveComponent> Class : Settings->ExclusionClasses) {

				if (PrimitiveComponent->IsA(Class)) {
					bIsExcluded = true;
					break;
				}
			}
			if (bIsExcluded) {
				continue;
			}
		}
		
		FPCGPoint Point;
		Point.Transform = PrimitiveComponent->GetComponentTransform();
		Point.SetLocalBounds(PrimitiveComponent->GetLocalBounds().GetBox());		
		Point.Steepness = 0.5;
		Point.Density = 1.0;

		FVector Position = Point.Transform.GetLocation();
		Point.Seed = PCGHelpers::ComputeSeed((int)Position.X, (int)Position.Y, (int)Position.Z);


		if (bParseTags && !Attributes.IsEmpty()) {

			TArray<FName> ComponentParsedTags;

			for (int32 TagIndex = 0; TagIndex < AttributeNames.Num(); TagIndex++)
			{
				if(AttributeNames[TagIndex] == NAME_None){
					continue;
				}
				ComponentParsedTags.Add(!PrimitiveComponent->ComponentTags.IsEmpty() && PrimitiveComponent->ComponentTags.IsValidIndex(TagIndex) ? PrimitiveComponent->ComponentTags[TagIndex] : NAME_None);
			}


			Point.MetadataEntry = PointData->Metadata->AddEntry();

			for (int32 AttributeIndex = 0; AttributeIndex < Attributes.Num(); AttributeIndex++)
			{
				Attributes[AttributeIndex]->SetValue(Point.MetadataEntry, ComponentParsedTags[AttributeIndex]);
			}	
		}

		Points.Add(Point);
		
	}

}

#undef LOCTEXT_NAMESPACE
