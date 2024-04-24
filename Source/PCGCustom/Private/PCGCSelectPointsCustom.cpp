// Copyright Roman K., Epic Games, Inc. All Rights Reserved. 


#include "PCGCSelectPointsCustom.h"

#include "PCGContext.h"
#include "Data/PCGSpatialData.h"
#include "Data/PCGPointData.h"
#include "Helpers/PCGAsync.h"
#include "Helpers/PCGHelpers.h"

#include "Math/RandomStream.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PCGCSelectPointsCustom)

#define LOCTEXT_NAMESPACE "PCGSelectPointsCustomElement"

namespace PCGCSelectPointsCustomSettings
{
	static const FName ChosenPointsLabel = TEXT("SelectedPoints");
	static const FName DiscardedPointsLabel = TEXT("DiscardedPoints");
}

UPCGCSelectPointsCustomSettings::UPCGCSelectPointsCustomSettings()
{
	bUseSeed = true;
}

FString UPCGCSelectPointsCustomSettings::GetAdditionalTitleInformation() const
{
#if WITH_EDITOR
	return TEXT("PCG Custom");
#else
	return Super::GetAdditionalTitleInformation();
#endif
}

TArray<FPCGPinProperties> UPCGCSelectPointsCustomSettings::OutputPinProperties() const
{
	TArray<FPCGPinProperties> Properties;

	Properties.Emplace(PCGCSelectPointsCustomSettings::ChosenPointsLabel, EPCGDataType::Point);
	Properties.Emplace(PCGCSelectPointsCustomSettings::DiscardedPointsLabel, EPCGDataType::Point);

	return Properties;
}

#if WITH_EDITOR
FText UPCGCSelectPointsCustomSettings::GetNodeTooltipText() const
{
	return LOCTEXT("NodeTooltip", "Selects a stable random subset of the input points.");
}
#endif

FPCGElementPtr UPCGCSelectPointsCustomSettings::CreateElement() const
{
	return MakeShared<FPCGCSelectPointsCustomElement>();
}

bool FPCGCSelectPointsCustomElement::ExecuteInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGCSelectPointsCustomElement::Execute);
	// TODO: make time-sliced implementation
	const UPCGCSelectPointsCustomSettings* Settings = Context->GetInputSettings<UPCGCSelectPointsCustomSettings>();
	check(Settings);

	TArray<FPCGTaggedData> Inputs = Context->InputData.GetInputs();

	TArray<FPCGTaggedData>& Outputs = Context->OutputData.TaggedData;

	float Ratio = Settings->InvertSelection ? (1 - Settings->Ratio) : Settings->Ratio;

	const int Seed = Context->GetSeed();

	const bool bNoSampling = (Ratio <= 0.0f);
	const bool bTrivialSampling = (Ratio >= 1.0f);


	for (const FPCGTaggedData& Input : Inputs)
	{
		if (bNoSampling) {
			PCGE_LOG(Verbose, LogOnly, LOCTEXT("SkippedNoSampling", "Skipped - Nothing to sample"));
			FPCGTaggedData& DiscardedOutput = Outputs.Add_GetRef(Input);
			DiscardedOutput.Pin = PCGCSelectPointsCustomSettings::DiscardedPointsLabel;

			if (!Input.Data || Cast<UPCGSpatialData>(Input.Data) == nullptr)
			{
				PCGE_LOG(Error, GraphAndLog, LOCTEXT("InvalidInputData", "Invalid input data"));
				continue;
			}

			continue;
		}

		FPCGTaggedData& SelectedOutput = Outputs.Add_GetRef(Input);
		SelectedOutput.Pin = PCGCSelectPointsCustomSettings::ChosenPointsLabel;
	
		if (!Input.Data || Cast<UPCGSpatialData>(Input.Data) == nullptr)
		{
			PCGE_LOG(Error, GraphAndLog, LOCTEXT("InvalidInputData", "Invalid input data"));
			continue;
		}

		// Skip processing if the transformation would be trivial
		if (bTrivialSampling)
		{
			PCGE_LOG(Verbose, LogOnly, LOCTEXT("SkippedTrivialSampling", "Skipped - trivial sampling"));
			continue;
		}

		const UPCGPointData* OriginalData = Cast<UPCGSpatialData>(Input.Data)->ToPointData(Context);

		if (!OriginalData)
		{
			PCGE_LOG(Error, GraphAndLog, LOCTEXT("NoPointDataInInput", "Unable to get point data from input"));
			continue;
		}


		FPCGTaggedData& DiscardedOutput = Outputs.Add_GetRef(Input);
		DiscardedOutput.Pin = PCGCSelectPointsCustomSettings::DiscardedPointsLabel;

		const TArray<FPCGPoint>& Points = OriginalData->GetPoints();
		const int OriginalPointCount = Points.Num();

		// Early out
		if (OriginalPointCount == 0)
		{
			PCGE_LOG(Verbose, LogOnly, LOCTEXT("SkippedAllPointsRejected", "Skipped - all points rejected"));
			continue;
		}

		UPCGPointData* SampledData = NewObject<UPCGPointData>();
		SampledData->InitializeFromData(OriginalData);
		TArray<FPCGPoint>& SampledPoints = SampledData->GetMutablePoints();

		UPCGPointData* DiscardedData = NewObject<UPCGPointData>();
		DiscardedData->InitializeFromData(OriginalData);
		TArray<FPCGPoint>& DiscardedPoints = DiscardedData->GetMutablePoints();

		SelectedOutput.Data = SampledData;
		DiscardedOutput.Data = DiscardedData;

		TRACE_CPUPROFILER_EVENT_SCOPE(FPCGSelectPointsCustomElement::Execute::SelectPoints);

		FPCGAsync::AsyncPointFilterProcessing(Context, OriginalPointCount, SampledPoints, DiscardedPoints, [&Points, Seed, Ratio](int32 Index, FPCGPoint& SelectedPoint, FPCGPoint& DiscardedPoint)
		{
			const FPCGPoint& Point = Points[Index];

			// Apply a high-pass filter based on selected ratio
			FRandomStream RandomSource(PCGHelpers::ComputeSeed(Seed, Point.Seed));
			float Chance = RandomSource.FRand();

			if (Chance < Ratio)
			{
				SelectedPoint = Point;
				return true;
			}
			else
			{
				DiscardedPoint = Point;
				return false;
			}

		});

		PCGE_LOG(Verbose, LogOnly, FText::Format(LOCTEXT("GenerationInfo", "Generated {0} points from {1} source points"), SampledPoints.Num(), OriginalPointCount));
		
	}

	return true;
}

#undef LOCTEXT_NAMESPACE
