// Copyright Roman K., Epic Games, Inc. All Rights Reserved. 


#include "PCGCGetActorDataExtended.h"

#include "PCGComponent.h"
#include "PCGModule.h"
#include "GameFramework/Actor.h"
#include "Data/PCGPointData.h"
#include "Data/PCGSpatialData.h"
#include "Data/PCGVolumeData.h"
#include "Helpers/PCGHelpers.h"
#include "PCGSubsystem.h"
#include "Algo/AnyOf.h"
#include "PCGPin.h"
#include "UObject/Package.h"
#include "Helpers/PCGSettingsHelpersCustom.h"
#include "Metadata/Accessors/PCGAttributeAccessorHelpers.h"
#include "Components/BillboardComponent.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(PCGCGetActorDataExtended)


#define LOCTEXT_NAMESPACE "PCGCGetActorDataExtendedElement"

#if WITH_EDITOR
void UPCGCGetActorDataExtendedSettings::GetTrackedActorKeys(FPCGActorSelectionKeyToSettingsMap& OutKeysToSettings, TArray<TObjectPtr<const UPCGGraph>>& OutVisitedGraphs) const
{

	//FPCGActorSelectionKey Key = static_cast<FPCGActorSelectionKey>(ActorSelector.GetAssociatedKey());
	FPCGActorSelectionKeyExtended Key = ActorSelector.GetAssociatedKey();
	if (Mode == EPCGGetActorDataMode::GetDataFromPCGComponent || Mode == EPCGGetActorDataMode::GetDataFromPCGComponentOrParseComponents)
	{
		Key.SetExtraDependency(UPCGComponent::StaticClass());
	}

	OutKeysToSettings.FindOrAdd(Key).Emplace(this, bTrackActorsOnlyWithinBounds);
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

FName UPCGCGetActorDataExtendedSettings::AdditionalTaskName() const
{
#if WITH_EDITOR
	return ActorSelector.GetTaskName(GetDefaultNodeTitle());
#else
	return Super::AdditionalTaskName();
#endif
}

FPCGElementPtr UPCGCGetActorDataExtendedSettings::CreateElement() const
{
	return MakeShared<FPCGCGetActorDataExtendedElement>();
}

EPCGDataType UPCGCGetActorDataExtendedSettings::GetCurrentPinTypes(const UPCGPin* InPin) const
{
	check(InPin);

	if (InPin->IsOutputPin())
	{
		if (Mode == EPCGGetActorDataMode::GetSinglePoint)
		{
			return EPCGDataType::Point;
		}
		else if (Mode == EPCGGetActorDataMode::GetDataFromProperty)
		{
			return EPCGDataType::Param;
		}
	}

	return Super::GetCurrentPinTypes(InPin);
}

TArray<FPCGPinProperties> UPCGCGetActorDataExtendedSettings::OutputPinProperties() const
{

	TArray<FPCGPinProperties> Pins = {};

	if (bGetSpatialData)
	{
		Pins = Super::OutputPinProperties();

		if (Mode == EPCGGetActorDataMode::GetDataFromPCGComponent || Mode == EPCGGetActorDataMode::GetDataFromPCGComponentOrParseComponents)
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

FPCGContext* FPCGCGetActorDataExtendedElement::Initialize(const FPCGDataCollection& InputData, TWeakObjectPtr<UPCGComponent> SourceComponent, const UPCGNode* Node)
{
	FPCGDataFromActorContext* Context = new FPCGDataFromActorContext();
	Context->InputData = InputData;
	Context->SourceComponent = SourceComponent;
	Context->Node = Node;

	return Context;
}

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

		Context->FoundActors = PCGCActorSelectorExtended::FindActors(Settings->ActorSelector, Context->SourceComponent.Get(), BoundsCheck, SelfIgnoreCheck);
		Context->bPerformedQuery = true;

		if (Context->FoundActors.IsEmpty())
		{
			PCGE_LOG(Warning, GraphAndLog, LOCTEXT("NoActorFound", "No matching actor was found"));
			return true;
		}

		if (Settings->bGetSpatialData)
		{
			// If we're looking for PCG component data, we might have to wait for it.
			if (Settings->Mode == EPCGGetActorDataMode::GetDataFromPCGComponent || Settings->Mode == EPCGGetActorDataMode::GetDataFromPCGComponentOrParseComponents)
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

						Subsystem->ScheduleGeneric([Context]()
							{
								// Wake up the current task
								Context->bIsPaused = false;
						return true;
							}, Context->SourceComponent.Get(), WaitOnTaskIds);

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
		ProcessActors(Context, Settings, Context->FoundActors);
	}

	return true;
}

void FPCGCGetActorDataExtendedElement::GatherWaitTasks(AActor* FoundActor, FPCGContext* Context, TArray<FPCGTaskId>& OutWaitTasks) const
{
	if (!FoundActor)
	{
		return;
	}

	// We will prevent gathering the current execution - this task cannot wait on itself
	AActor* ThisOwner = ((Context && Context->SourceComponent.IsValid()) ? Context->SourceComponent->GetOwner() : nullptr);

	TInlineComponentArray<UPCGComponent*, 1> PCGComponents;
	FoundActor->GetComponents(PCGComponents);

	for (UPCGComponent* Component : PCGComponents)
	{
		if (Component->IsGenerating() && Component->GetOwner() != ThisOwner)
		{
			OutWaitTasks.Add(Component->GetGenerationTaskId());
		}
	}
}

void FPCGCGetActorDataExtendedElement::ProcessActors(FPCGContext* Context, const UPCGCGetActorDataExtendedSettings* Settings, const TArray<AActor*>& FoundActors) const
{

	if (Settings->bGetSpatialData)
	{
		// Special case:
		// If we're asking for single point with the merge single point data, we can do a more efficient process
		if (Settings->Mode == EPCGGetActorDataMode::GetSinglePoint && Settings->bMergeSinglePointData && FoundActors.Num() > 1)
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
	//const bool bContainsPartitionActors = Algo::AnyOf(FoundActors, [](const AActor* Actor) { return Cast<APCGPartitionActor>(Actor) != nullptr; });
	const bool bContainsPartitionActors = Algo::AnyOf(FoundActors, [](const AActor* Actor) {

		TInlineComponentArray<UPCGComponent*, 1> PCGComponents;
		Actor->GetComponents(PCGComponents);
		if (PCGComponents.IsEmpty()) {
			return false;
		}
		else {
			for (UPCGComponent* Component : PCGComponents)
			{
				if (Component->IsPartitioned())
				{
					return true;
				}
			}
		}
		return false; 
		});

	if (!bContainsPartitionActors)
	{
		UPCGPointData* Data = NewObject<UPCGPointData>();
		bool bHasData = false;

		for (AActor* Actor : FoundActors)
		{
			if (Actor)
			{
				Data->AddSinglePointFromActor(Actor);
				bHasData = true;
			}
		}
		
		if (bHasData)
		{
			FPCGTaggedData& TaggedData = Context->OutputData.TaggedData.Emplace_GetRef();
			TaggedData.Pin = PCGPinConstants::DefaultOutputLabel;
			TaggedData.Data = Data;
		}
	}
	else // Stripped-down version of the normal code path with bParseActor = false
	{

		PCGE_LOG(Error, GraphAndLog, LOCTEXT("NotSupportingPartitions", "Action Does Not Support Partition Actors"));

	//	FPCGDataCollection DataToMerge;
	//	const bool bParseActor = false;
	//
	//	for (AActor* Actor : FoundActors)
	//	{
	//		if (Actor)
	//		{
	//			FPCGDataCollection Collection = UPCGComponent::CreateActorPCGDataCollection(Actor, Context->SourceComponent.Get(), EPCGDataType::Any, bParseActor);
	//
	//			for (FPCGTaggedData& Output : Collection.TaggedData) {
	//				Output.Pin = PCGPinConstants::DefaultOutputLabel;
	//			}
	//
	//			DataToMerge.TaggedData += Collection.TaggedData;
	//		}
	//	}
	//
	//	 //Perform point data-to-point data merge
	//	if (DataToMerge.TaggedData.Num() > 1)
	//	{
	//		UPCGMergeSettings* MergeSettings = NewObject<UPCGMergeSettings>();
	//		FPCGMergeElement MergeElement;
	//		FPCGMergeElement MergeElement = FPCGMergeElement();
	//		FPCGContext* MergeContext = MergeElement.Initialize(DataToMerge, Context->SourceComponent, nullptr);
	//		MergeContext->AsyncState.NumAvailableTasks = Context->AsyncState.NumAvailableTasks;
	//		MergeContext->InputData.TaggedData.Emplace_GetRef().Data = MergeSettings;
	//	
	//		while (!MergeElement.Execute(MergeContext))
	//		
	//		
	//	
	//		Context->OutputData = MergeContext->OutputData;
	//		delete MergeContext;
	//	}
	//	else if (DataToMerge.TaggedData.Num() == 1)
	//	{
	//		Context->OutputData.TaggedData = DataToMerge.TaggedData;
	//	}
	}
}

void FPCGCGetActorDataExtendedElement::ProcessActor(FPCGContext* Context, const UPCGCGetActorDataExtendedSettings* Settings, AActor* FoundActor) const
{
	check(Context);
	check(Settings);

	if (!FoundActor || !IsValid(FoundActor))
	{
		return;
	}

	AActor* ThisOwner = ((Context && Context->SourceComponent.Get()) ? Context->SourceComponent->GetOwner() : nullptr);
	TInlineComponentArray<UPCGComponent*, 1> PCGComponents;
	bool bHasGeneratedPCGData = false;
	FProperty* FoundProperty = nullptr;

	const bool bCanGetDataFromComponent = (FoundActor != ThisOwner);

	if (bCanGetDataFromComponent && (Settings->Mode == EPCGGetActorDataMode::GetDataFromPCGComponent || Settings->Mode == EPCGGetActorDataMode::GetDataFromPCGComponentOrParseComponents))
	{
		FoundActor->GetComponents(PCGComponents);

		for (UPCGComponent* Component : PCGComponents)
		{
			bHasGeneratedPCGData |= !Component->GetGeneratedGraphOutput().TaggedData.IsEmpty();
		}
	}
	else if (Settings->Mode == EPCGGetActorDataMode::GetDataFromProperty)
	{
		if (Settings->PropertyName != NAME_None)
		{
			FoundProperty = FindFProperty<FProperty>(FoundActor->GetClass(), Settings->PropertyName);
		}
	}

	// Some additional validation
	if (Settings->Mode == EPCGGetActorDataMode::GetDataFromPCGComponent && !bHasGeneratedPCGData)
	{
		if (bCanGetDataFromComponent)
		{
			PCGE_LOG(Warning, GraphAndLog, FText::Format(LOCTEXT("ActorHasNoGeneratedData", "Actor '{0}' does not have any previously generated data"), FText::FromName(FoundActor->GetFName())));
		}
		else
		{
			PCGE_LOG(Error, GraphAndLog, FText::Format(LOCTEXT("ActorCannotGetOwnData", "Actor '{0}' cannot get its own generated data during generation"), FText::FromName(FoundActor->GetFName())));
		}

		return;
	}
	else if (Settings->Mode == EPCGGetActorDataMode::GetDataFromProperty && !FoundProperty)
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
				DuplicatedTaggedData.Data = Cast<UPCGData>(StaticDuplicateObject(TaggedData.Data, GetTransientPackage()));

				//FObjectDuplicationParameters DupParams = FObjectDuplicationParameters(Cast<UObject>(TaggedData.Data), Cast<UObject>(GetTransientPackage()));
				//DuplicatedTaggedData.Data = Cast<UPCGData>(StaticDuplicateObjectEx(DupParams));
			}
		//	//Outputs.Append(Component->GetGeneratedGraphOutput().TaggedData);
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
		const bool bParseActor = (Settings->Mode != EPCGGetActorDataMode::GetSinglePoint);
		FPCGDataCollection Collection = UPCGComponent::CreateActorPCGDataCollection(FoundActor, Context->SourceComponent.Get(), Settings->GetDataFilter(), bParseActor);
		
		Outputs += Collection.TaggedData;

		for (FPCGTaggedData& Output : Outputs) {
			Output.Pin = PCGPinConstants::DefaultOutputLabel;
		}
	}
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
					PCGSettingsHelpersCustom::FPCGGetAllOverridableParamsConfig Config;
					Config.bUseSeed = true;
					Config.bExcludeSuperProperties = true;
					Config.MaxStructDepth = 0;
					// Can only get exposed properties and visible
					Config.ExcludePropertyFlags = ExcludePropertyFlags;
					Config.IncludePropertyFlags = IncludePropertyFlags;

					TArray<FPCGSettingsOverridableParam> AllChildProperties = UnderlyingStruct ? PCGSettingsHelpersCustom::GetAllOverridableParams(UnderlyingStruct, Config) : PCGSettingsHelpersCustom::GetAllOverridableParams(UnderlyingClass, Config);


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
