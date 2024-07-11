// Copyright Roman K. All Rights Reserved. 

#include "PCGCCheckData.h"

#include "PCGContext.h"
#include "Data/PCGSpatialData.h"
#include "Data/PCGPointData.h"
#include "PCGParamData.h"
#include "Data/PCGIntersectionData.h"
#include "Data/PCGDifferenceData.h"
#include "Data/PCGUnionData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PCGCCheckData)

#define LOCTEXT_NAMESPACE "PCGCCheckDataElement"

namespace PCGCCheckDataSettings
{
	static const FName ValidationLabel = TEXT("IsValid");
}

#if WITH_EDITOR
FText UPCGCCheckDataSettings::GetNodeTooltipText() const
{
	return LOCTEXT("NodeTooltip", "Disables the output if the data count or elements count inside all data sets on the input is 0, can optionally discard empty data for points, attribute sets or composite data");
}

EPCGChangeType UPCGCCheckDataSettings::GetChangeTypeForProperty(const FName& InPropertyName) const
{
	EPCGChangeType ChangeType = Super::GetChangeTypeForProperty(InPropertyName) | EPCGChangeType::Cosmetic;

	if (InPropertyName == GET_MEMBER_NAME_CHECKED(UPCGCCheckDataSettings, bEnabled))
	{
		ChangeType |= EPCGChangeType::Structural;
	}

	return ChangeType;
}
#endif

FString UPCGCCheckDataSettings::GetAdditionalTitleInformation() const
{
#if WITH_EDITOR
	return TEXT("PCG Custom");
#else
	return Super::GetAdditionalTitleInformation();
#endif
}

EPCGDataType UPCGCCheckDataSettings::GetCurrentPinTypes(const UPCGPin* InPin) const
{
	check(InPin);
	// Non-dynamically-typed pins
	if (!InPin->IsOutputPin() || InPin->Properties.Label == PCGCCheckDataSettings::ValidationLabel)
	{
		return InPin->Properties.AllowedTypes;
	}

	// Output pin narrows to union of inputs on first pin
	const EPCGDataType InputTypeUnion = GetTypeUnionOfIncidentEdges(PCGPinConstants::DefaultInputLabel);
	return InputTypeUnion != EPCGDataType::None ? InputTypeUnion : EPCGDataType::Any;
}

TArray<FPCGPinProperties> UPCGCCheckDataSettings::InputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;
	FPCGPinProperties& InputPinProperty = PinProperties.Emplace_GetRef(PCGPinConstants::DefaultInputLabel, EPCGDataType::Any);
	InputPinProperty.SetRequiredPin();

	return PinProperties;
}

TArray<FPCGPinProperties> UPCGCCheckDataSettings::OutputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;
	PinProperties.Emplace(PCGPinConstants::DefaultOutputLabel,
		EPCGDataType::Any,
		/*bInAllowMultipleConnections=*/true,
		/*bAllowMultipleData=*/true);

	PinProperties.Emplace(
		PCGCCheckDataSettings::ValidationLabel,
		EPCGDataType::Param,
		/*bInAllowMultipleConnections=*/true,
		/*bInAllowMultipleData=*/true,
		LOCTEXT("OutParamTooltip", "Attribute set containing the data count from the input collection"));

	return PinProperties;
}


FPCGElementPtr UPCGCCheckDataSettings::CreateElement() const
{
	return MakeShared<FPCGCCheckDataElement>();
}

bool FPCGCCheckDataElement::ExecuteInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGCCheckDataElement::Execute);

	const UPCGCCheckDataSettings* Settings = Context->GetInputSettings<UPCGCCheckDataSettings>();
	check(Settings);

	TArray<FPCGTaggedData> Inputs = Context->InputData.GetInputsByPin(PCGPinConstants::DefaultInputLabel);	
	TArray<FPCGTaggedData>& Outputs = Context->OutputData.TaggedData;

	for (const FPCGTaggedData& Input : Inputs)
	{
		if (Settings->DiscardEmptyData) {
			if (const UPCGPointData* PointData = Cast<UPCGPointData>(Input.Data.Get())) {
				if (!PointData->GetPoints().IsEmpty())
				{
					FPCGTaggedData& Output1 = Outputs.Add_GetRef(Input);
					Output1.Pin = PCGPinConstants::DefaultOutputLabel;
				}
			}
			else if (const UPCGParamData* ParamData = Cast<UPCGParamData>(Input.Data.Get())) {
				if (ParamData->ConstMetadata()->GetLocalItemCount() != 0)
				{
					FPCGTaggedData& Output1 = Outputs.Add_GetRef(Input);
					Output1.Pin = PCGPinConstants::DefaultOutputLabel;
				}
			}
			else if (Cast<UPCGIntersectionData>(Input.Data.Get()) || Cast<UPCGDifferenceData>(Input.Data.Get()) || Cast<UPCGUnionData>(Input.Data.Get()))
			{
				const UPCGSpatialData* IntData = Cast<UPCGSpatialData>(Input.Data.Get());
				if (IntData->GetStrictBounds().IsValid)
				{
					FPCGTaggedData& Output1 = Outputs.Add_GetRef(Input);
					Output1.Pin = PCGPinConstants::DefaultOutputLabel;
				}
			}
			else {
				FPCGTaggedData& Output1 = Outputs.Add_GetRef(Input);
				Output1.Pin = PCGPinConstants::DefaultOutputLabel;
			}
		}
		else {
			FPCGTaggedData& Output1 = Outputs.Add_GetRef(Input);
			Output1.Pin = PCGPinConstants::DefaultOutputLabel;
		}
	}
	
	const bool IsFinalDataValid = Context->OutputData.GetInputCountByPin(PCGPinConstants::DefaultOutputLabel) != 0 ? true : false;

	UPCGParamData* OutputParamData = NewObject<UPCGParamData>();
	FPCGMetadataAttribute<bool>* NumAttribute = OutputParamData->Metadata->CreateAttribute<bool>(PCGCCheckDataSettings::ValidationLabel, IsFinalDataValid, /*bAllowInterpolation=*/false, /*bOverrideParent=*/false);
	OutputParamData->Metadata->AddEntry();

	FPCGTaggedData& Output2 = Outputs.Emplace_GetRef();
	Output2.Pin = PCGCCheckDataSettings::ValidationLabel;
	Output2.Data = OutputParamData;


	Context->OutputData.InactiveOutputPinBitmask = IsFinalDataValid ? 0 : 1;



	return true;
}

#undef LOCTEXT_NAMESPACE
