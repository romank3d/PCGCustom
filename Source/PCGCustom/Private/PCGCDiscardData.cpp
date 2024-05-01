// Copyright Roman K. All Rights Reserved. 

#include "PCGCDiscardData.h"

#include "PCGContext.h"
#include "Data/PCGSpatialData.h"
#include "Data/PCGPointData.h"
#include "PCGParamData.h"
#include "Data/PCGIntersectionData.h"
#include "Data/PCGDifferenceData.h"
#include "Data/PCGUnionData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PCGCDiscardData)

#define LOCTEXT_NAMESPACE "PCGCDiscardDataElement"

#if WITH_EDITOR
FText UPCGCDiscardDataSettings::GetNodeTooltipText() const
{
	return LOCTEXT("NodeTooltip", "Discards data sets with no Points, no Attribute Entries, no Composite Data (Intersection, Difference, Union). Other data types will be passed through as is");
}

EPCGChangeType UPCGCDiscardDataSettings::GetChangeTypeForProperty(const FName& InPropertyName) const
{
	EPCGChangeType ChangeType = Super::GetChangeTypeForProperty(InPropertyName) | EPCGChangeType::Cosmetic;

	if (InPropertyName == GET_MEMBER_NAME_CHECKED(UPCGCDiscardDataSettings, bEnabled))
	{
		ChangeType |= EPCGChangeType::Structural;
	}

	return ChangeType;
}
#endif

FString UPCGCDiscardDataSettings::GetAdditionalTitleInformation() const
{
#if WITH_EDITOR
	return TEXT("PCG Custom");
#else
	return Super::GetAdditionalTitleInformation();
#endif
}

TArray<FPCGPinProperties> UPCGCDiscardDataSettings::OutputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;
	PinProperties.Emplace(PCGPinConstants::DefaultOutputLabel,
		EPCGDataType::Any,
		/*bInAllowMultipleConnections=*/true,
		/*bAllowMultipleData=*/true);

	return PinProperties;
}


FPCGElementPtr UPCGCDiscardDataSettings::CreateElement() const
{
	return MakeShared<FPCGCDiscardDataElement>();
}

bool FPCGCDiscardDataElement::ExecuteInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGCDiscardDataElement::Execute);

	const UPCGCDiscardDataSettings* Settings = Context->GetInputSettings<UPCGCDiscardDataSettings>();
	check(Settings);

	TArray<FPCGTaggedData> Inputs = Context->InputData.GetInputsByPin(PCGPinConstants::DefaultInputLabel);	
	TArray<FPCGTaggedData>& Outputs = Context->OutputData.TaggedData;

	for (const FPCGTaggedData& Input : Inputs) 
	{
		if (const UPCGPointData* PointData = Cast<UPCGPointData>(Input.Data.Get())) {
			if (!PointData->GetPoints().IsEmpty())
			{
				Outputs.Add(Input);
			}			
		}
		else if(const UPCGParamData* ParamData = Cast<UPCGParamData>(Input.Data.Get())){
			if (ParamData->ConstMetadata()->GetLocalItemCount() != 0)
			{
				Outputs.Add(Input);
			}
		}
		else if (Cast<UPCGIntersectionData>(Input.Data.Get()) || Cast<UPCGDifferenceData>(Input.Data.Get()) || Cast<UPCGUnionData>(Input.Data.Get()))
		{	
			const UPCGSpatialData* IntData = Cast<UPCGSpatialData>(Input.Data.Get());
			if (IntData->GetStrictBounds().IsValid) 
			{
				Outputs.Add(Input);	
			}	
		}
		else {
			Outputs.Add(Input);
		}		
	}

	return true;
}

#undef LOCTEXT_NAMESPACE
