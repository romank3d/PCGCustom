// Copyright Roman K. All Rights Reserved.

#include "PCGCDifferenceByTag.h"

#include "Data/PCGSpatialData.h"
#include "Data/PCGPointData.h"
#include "PCGContext.h"
#include "Containers/Set.h"
#include "Helpers/PCGHelpers.h"
#include "PCGPin.h"


#define LOCTEXT_NAMESPACE "PCGCDifferenceByTagElement"

#if WITH_EDITOR
FText UPCGCDifferenceByTagSettings::GetNodeTooltipText() const
{
	return LOCTEXT("NodeTooltip", "Spatially subtracts data sets based on Priority and ID tags.");
}
#endif

FString UPCGCDifferenceByTagSettings::GetAdditionalTitleInformation() const
{
#if WITH_EDITOR
	return TEXT("PCG Custom");
#else
	return Super::GetAdditionalTitleInformation();
#endif
}

TArray<FPCGPinProperties> UPCGCDifferenceByTagSettings::InputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;
	FPCGPinProperties& InputPinProperty = PinProperties.Emplace_GetRef(PCGPinConstants::DefaultInputLabel, EPCGDataType::Spatial);
	InputPinProperty.SetRequiredPin();

	return PinProperties;
}

TArray<FPCGPinProperties> UPCGCDifferenceByTagSettings::OutputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;
	PinProperties.Emplace(PCGPinConstants::DefaultOutputLabel, EPCGDataType::Spatial);

	return PinProperties;
}

FPCGElementPtr UPCGCDifferenceByTagSettings::CreateElement() const
{
	return MakeShared<FPCGCDifferenceByTagElement>();
}

bool FPCGCDifferenceByTagElement::ExecuteInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGCDifferenceByTagElement::Execute);

	const UPCGCDifferenceByTagSettings* Settings = Context->GetInputSettings<UPCGCDifferenceByTagSettings>();
	check(Settings);

	//Preparing IO
	const TArray<FPCGTaggedData> Inputs = Context->InputData.GetInputs();
	TArray<FPCGTaggedData>& Outputs = Context->OutputData.TaggedData;

	//Checking If custom tags are used and getting their number
	int32 NumCustomTags = Settings->bUsingCustomTags ? Settings->NumCustomTags : 0;

	//For each set
	for (const FPCGTaggedData& Input : Inputs)
	{
		const UPCGSpatialData* InputSpatialData = Cast<UPCGSpatialData>(Input.Data);

		//Pass set as is, if there are no tags, not enough tags, or data is not spatial
		if (Input.Tags.IsEmpty() || Input.Tags.Num() < 2 + NumCustomTags || !InputSpatialData) {
			Outputs.Add(Input);
			continue;
		}

		

		bool bHasPointsInSource = false;
		bool bHasPointsInDifferences = false;

		bHasPointsInSource |= Input.Data->IsA<UPCGPointData>();

		//Extract ID tag and Priority 
		TArray<FString> InputTags = Input.Tags.Array();
		
		const TArray<FString> Tags = PCGHelpers::GetStringArrayFromCommaSeparatedString(Settings->ExcludeTags);
		
		if (!Tags.IsEmpty())
		{
			bool bHasCommonTags = false;
			for (FString Tag : Tags) {
				if (InputTags.Contains(Tag)) {
					bHasCommonTags = true;
					break;
				}
			}
			if (bHasCommonTags) {
				Outputs.Add(Input);
				continue;
			}
		}

		FString IdTag = InputTags[InputTags.Num() - 1 - NumCustomTags];
		int32 DataSetPriority = FCString::Atoi(*InputTags[(InputTags.Num()-2 - NumCustomTags)]);

		UPCGDifferenceData* DifferenceData = nullptr;
		

		for (const FPCGTaggedData& InputInner : Inputs) {

			const UPCGSpatialData* InputInnerSpatialData = Cast<UPCGSpatialData>(InputInner.Data);

			//Pass this set, if there are no tags, not enough tags, or data is not spatial
			if (InputInner.Tags.IsEmpty() || InputInner.Tags.Num() < 2 + NumCustomTags || !InputInnerSpatialData) {
				continue;
			}

			TArray<FString> InputInnerTags = InputInner.Tags.Array();
			
			if (!Tags.IsEmpty())
			{
				bool bHasCommonTags = false;
				for (FString Tag : Tags) {
					if (InputInnerTags.Contains(Tag)) {
						bHasCommonTags = true;
						break;
					}
				}
				if (bHasCommonTags) {
					continue;
				}
			}

			if (IdTag != InputInnerTags[InputInnerTags.Num() - 1 - NumCustomTags]) {
				if (DataSetPriority < FCString::Atoi(*InputInnerTags[(InputInnerTags.Num() - 2 - NumCustomTags)]))
				{
					bHasPointsInDifferences |= InputInner.Data->IsA<UPCGPointData>();

					if (!DifferenceData) {

						DifferenceData = NewObject<UPCGDifferenceData>();
						DifferenceData->Initialize(InputSpatialData);

						DifferenceData->AddDifference(InputInnerSpatialData);
					}
					else {

						DifferenceData->AddDifference(InputInnerSpatialData);
					}
				}
			}
		}

		if (!DifferenceData) {

			Outputs.Add(Input);
			continue;
		}

		FPCGTaggedData& Output = Outputs.Add_GetRef(Input);
		Output.Data = DifferenceData;

		if (DifferenceData &&
			(Settings->Mode == EPCGDifferenceMode::Discrete ||
			(Settings->Mode == EPCGDifferenceMode::Inferred && bHasPointsInSource && bHasPointsInDifferences)))
		{

			DifferenceData->SetDensityFunction(Settings->DensityFunction);
			DifferenceData->bDiffMetadata = Settings->bDiffMetadata;
			Output.Data = DifferenceData->ToPointData(Context);
		}

	}

	return true;

}


#undef LOCTEXT_NAMESPACE