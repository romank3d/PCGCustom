// Copyright Roman K. All Rights Reserved.

#include "PCGCSimpleShape.h"

#include "PCGContext.h"
#include "PCGPin.h"
#include "Data/PCGPointData.h"
#include "Helpers/PCGAsync.h"
#include "Helpers/PCGHelpers.h"
#include "Kismet/KismetMathLibrary.h"

#define LOCTEXT_NAMESPACE "PCGCSimpleShapeElement"

FPCGElementPtr UPCGCSimpleShapeSettings::CreateElement() const
{
	return MakeShared<UPCGCSimpleShapeElement>();
}

#if WITH_EDITOR
FName UPCGCSimpleShapeSettings::AdditionalTaskName() const
{
	//Set Dynamic Node Name depending on the selected shape
	if (const UEnum* EnumPtr = StaticEnum<EPCGCSImpleShapePointLineMode>())
	{
		return FName(FString("PCGC Simple Shape: ") + EnumPtr->GetNameStringByValue(static_cast<int>(Shape)));
	}
	else
	{
		return NAME_None;
	}
}


FName UPCGCSimpleShapeSettings::GetDefaultNodeName() const
{ 
	return FName(TEXT("PCGC Simple Shape")); 
}

FText UPCGCSimpleShapeSettings::GetDefaultNodeTitle() const
{
	return NSLOCTEXT("PCGCSimpleShapeSettings", "NodeTitle", "PCGC Simple Shape");
}
#endif

TArray<FPCGPinProperties> UPCGCSimpleShapeSettings::OutputPinProperties() const
{
	//Set Output Pin
	TArray<FPCGPinProperties> PinProperties;
	PinProperties.Emplace(PCGPinConstants::DefaultOutputLabel, EPCGDataType::Point);
	return PinProperties;
}

bool UPCGCSimpleShapeElement::ExecuteInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UPCGCSimpleShapeElement::Execute);

	check(Context);

	//Get the Settings
	const UPCGCSimpleShapeSettings* Settings = Context->GetInputSettings<UPCGCSimpleShapeSettings>();
	check(Settings);

	if (Settings->PointExtents.X < 0.0 || Settings->PointExtents.Y < 0.0 || Settings->PointExtents.Z < 0.0) {
		//Check if we have proper PointExtents (it's set to clamp to min in details settings, but can still be overriden to < 0)
		PCGE_LOG(Error, GraphAndLog, LOCTEXT("IllegalPointExtentsLenght", "Point Extents should be geater than 0"));
		//out
		return true;
	}

	//Get a reference to the output Collections Array (FPCGTaggedData) 
	TArray<FPCGTaggedData>& Outputs = Context->OutputData.TaggedData;

	//Launch function depending on the selected shape
	if (Settings->Shape == EPCGCSImpleShapePointLineMode::Point) {

		CreatePoint(Context, Settings, Outputs);
		return true;
	}
	else if ((Settings->Shape == EPCGCSImpleShapePointLineMode::Line)) {

		CreateLine(Context, Settings, Outputs);
		return true;
	}
	else if ((Settings->Shape == EPCGCSImpleShapePointLineMode::Rectangle)) {

		CreateRectangle(Context, Settings, Outputs);
		return true;
	}
	else if ((Settings->Shape == EPCGCSImpleShapePointLineMode::Circle)) {

		CreateCircle(Context, Settings, Outputs);
		return true;
	}

	//out
	return true;
}

TArray<FPCGPoint>& UPCGCSimpleShapeElement::GetOutputPoints(TArray<FPCGTaggedData>& Outputs) const {

	//Create point data, assign it to the output and get a handle to the points.

	//Initialize data element in collection array and get a reference to it
	FPCGTaggedData& Output = Outputs.Emplace_GetRef();

	//Create PointData
	UPCGPointData* OutputPointData = NewObject<UPCGPointData>();

	//Assign PointData to the output
	Output.Data = OutputPointData;

	//Specify the name of the Output Pin that will take this particular data collection (can be omitted in case of a single out pin)
	Output.Pin = PCGPinConstants::DefaultOutputLabel;

	//Get a refernce to points in PointData
	TArray<FPCGPoint>& Points = OutputPointData->GetMutablePoints();

	return Points;
}

void UPCGCSimpleShapeElement::CreatePoint(FPCGContext* Context, const UPCGCSimpleShapeSettings* Settings, TArray<FPCGTaggedData>& Outputs) const {

	//Create Single Point

	//Initialize and get Output Point Data
	TArray<FPCGPoint>& Points = UPCGCSimpleShapeElement::GetOutputPoints(Outputs);

	//Get Position from Settings
	FVector Position = Settings->PointSettings.PointOriginPosition;

	//Create and set the point
	FPCGPoint Point;
	Point.Transform.SetLocation(Position);
	Point.SetExtents(Settings->PointExtents);
	Point.Steepness = 0.5;
	Point.Density = 1.0;

	Point.Seed = PCGHelpers::ComputeSeed((int)Position.X, (int)Position.Y, (int)Position.Z);

	Point.Transform.SetRotation(FQuat(Settings->PointSettings.PointOrientation));

	//Add the point to the Output PointData
	Points.Add(Point);
}

void UPCGCSimpleShapeElement::CreateLine(FPCGContext* Context, const UPCGCSimpleShapeSettings* Settings, TArray<FPCGTaggedData>& Outputs) const {

	//Create A Line Of Points

	//Initialize and get Output Point Data
	TArray<FPCGPoint>& Points = UPCGCSimpleShapeElement::GetOutputPoints(Outputs);

	//Get Properties from Settings

	if (Settings->LineSettings.LineLenght <= 0.0) {
		//Check if we have proper Line Lenght (it's set to clamp to min in details settings, but can still be overriden to < 0)
		PCGE_LOG(Error, GraphAndLog, LOCTEXT("IllegalLineLenght", "Line Lenght should be geater than 0"));
		//out
		return;
	}

	const FVector PointExtents = Settings->PointExtents;
	const bool bIsCoordinateMode = Settings->LineSettings.Mode == EPCGCShapePointLineMode::SetPosition;
	const bool bAlignPointsToDirection = Settings->LineSettings.bAlignLinePointsToDirection;

	//Set end points positions and orientation
	FVector PointA = bIsCoordinateMode ? Settings->LineSettings.LineOriginPosition : FVector::Zero();
	FTransform PointATransforms = FTransform(Settings->LineSettings.LineDirection, PointA, FVector::Zero());
	FVector PointB = bIsCoordinateMode ? Settings->LineSettings.LineTargetPosition : UKismetMathLibrary::TransformDirection(PointATransforms, FVector(Settings->LineSettings.LineLenght, 0.0, 0.0));
	FQuat Orientation = FQuat(UKismetMathLibrary::MakeRotFromZ(PointB - PointA));

	//Function That will create points
	const auto CreatePoint = [Orientation, PointExtents, bAlignPointsToDirection](const FVector Position) {

		FPCGPoint Point;

		Point.Transform.SetLocation(Position);
		Point.SetExtents(PointExtents);
		Point.Steepness = 0.5;
		Point.Density = 1.0;

		Point.Seed = PCGHelpers::ComputeSeed((int)Position.X, (int)Position.Y, (int)Position.Z);

		//Should point be aligned to a line direction?
		if (bAlignPointsToDirection) {

			Point.Transform.SetRotation(Orientation);
		}

		return Point;
	};

	//If we need to output end points only
	if (Settings->LineSettings.bLineEndPointsOnly) {

		TArray<FVector> TwoPts = { PointA, PointB };
		for (FVector Pt : TwoPts) {

			Points.Emplace(CreatePoint(Pt));
		}

		//out
		return;
	}
	
	//Depending on interploation method
	double DistanceAB = bIsCoordinateMode ? FVector::Dist(PointA, PointB) : Settings->LineSettings.LineLenght;

	//Calculate number of points depending on interploation method
	double Step = Settings->LineSettings.LineStep;
	int32 Steps = Settings->LineSettings.LineDivisions;

	if (Settings->LineSettings.LineInterpolation == EPCGCInterpolationMode::Step) {

		//Check if step Lenght is <=0 (it's set to clamp to min in details settings, but can still be overriden to <0)
		if (Step < 0.1) {
			PCGE_LOG(Error, GraphAndLog, LOCTEXT("IllegalStepLenght", "Line Step Lenght should be geater than 0.1"));
			//out
			return;
		}

		Steps = DistanceAB / Step;
	}
	else {
		
		//Check if we have proper number of divisions (it's set to clamp to min in details settings, but can still be overriden to illegal values)
		if (Steps < 1) {
			PCGE_LOG(Error, GraphAndLog, LOCTEXT("IllegalSubDivsNumber", "Number of subdivisions should be geater than 0"));
			//out
			return;
		}

		Step = DistanceAB / Steps;
	}

	Steps++;

	//Process the points
	FPCGAsync::AsyncPointProcessing(Context, Steps, Points, [PointA, PointB, Step, DistanceAB, CreatePoint](int32 Index, FPCGPoint& OutPoint)
		{
			//Calculate position of the point, based on the interpolation value between end points
			double LerpAlpha = (Step * Index) / DistanceAB;
			FVector Position = FMath::Lerp(PointA, PointB, LerpAlpha);

			//Create and set the point
			OutPoint = CreatePoint(Position);

			return true;
		});
}

void UPCGCSimpleShapeElement::CreateRectangle(FPCGContext* Context, const UPCGCSimpleShapeSettings* Settings, TArray<FPCGTaggedData>& Outputs) const {

	//Create a Rectangle Of Points

	//Get Properties from Settings
	const FVector PointExtents = Settings->PointExtents;
	const bool bCornerPointsOnly = Settings->RectangleSettings.bCornerPointsOnly;
	const bool bOrientToDirection = Settings->RectangleSettings.bOrientToCenter;
	const bool bOrientCorners = Settings->RectangleSettings.bOrientCorners;
	const bool bMergeSides = Settings->RectangleSettings.bMergeSides;

	const double RightAngle = FMath::DegreesToRadians(90);

	if (Settings->RectangleSettings.RectangleLenght <= 0.0 || Settings->RectangleSettings.RectangleWidth <= 0.0) {
		PCGE_LOG(Error, GraphAndLog, LOCTEXT("IllegalDimensions", "Rectangle Dimensions should be geater than 0"));
		//out
		return;
	}

	//Set position for each corner point
	FVector P1 = FVector(Settings->RectangleSettings.RectangleLenght / 2.0, Settings->RectangleSettings.RectangleWidth / 2.0, 0.0);
	FVector P2 = FVector(-P1.X, P1.Y, 0.0);
	FVector P3 = -P1;
	FVector P4 = -P2;

	const TArray<FVector> Corners = { P1 , P2 , P3 , P4 };

	//Do for each side
	for (int32 Side = 0; Side < 4; Side++) {

		//Initialize and get Output Point Data
		TArray<FPCGPoint>& Points = UPCGCSimpleShapeElement::GetOutputPoints(Outputs);

		//Function That will create points
		const auto CreatePoint = [PointExtents, bOrientToDirection, RightAngle, &Side](const FVector Position, bool bOrientCorners) {

			FPCGPoint Point;

			Point.Transform.SetLocation(Position);
			Point.SetExtents(PointExtents);
			Point.Steepness = 0.5;
			Point.Density = 1.0;
			Point.Seed = PCGHelpers::ComputeSeed((int)Position.X, (int)Position.Y, (int)Position.Z);


			if (bOrientToDirection) {
				if (bOrientCorners) {
					Point.Transform.SetRotation(FQuat(FVector(0.0, 0.0, 1.0), (- RightAngle / 2) + (RightAngle * Side)));
				}
				else {
					Point.Transform.SetRotation(FQuat(FVector(0.0, 0.0, 1.0), RightAngle * Side));
				}
			}

			return Point;
		};

		//If we need to output corners only
		if (bCornerPointsOnly) {

			for (const FVector& Corner : Corners) {

				Points.Emplace(CreatePoint(Corner, bOrientCorners));
				Side++;
			}
			//out
			return;
		}

		//Tag the side output collection if sides are not merged
		if (!bMergeSides) {
			FPCGTaggedData& Output = Outputs.Last();
			Output.Tags.Emplace(FString("Side").Append(FString::FromInt(Side)));
		}

		//Define end points and calculate distance between them
		FVector PointA = Corners[Side];
		FVector PointB = Corners[(Side + 1) % 4];
		double DistanceAB = FVector::DistXY(PointA, PointB);

		//Check if it's "Lenght" or "Width" side
		bool bIsLenghtSide = Side == 0 || Side == 2 ? true : false;

		//Calculate steps based on interpolation mode
		int32 Steps = Settings->RectangleSettings.RectangleSubdivisions;
		int32 StepsL = Settings->RectangleSettings.RectangleLenghtSubdivisions;
		int32 StepsW = Settings->RectangleSettings.RectangleWidthtSubdivisions;
		double Step = Settings->RectangleSettings.RectangleStep;
		double StepL = Settings->RectangleSettings.RectangleLenghtStep;
		double StepW = Settings->RectangleSettings.RectangleWidthStep;

		if (Settings->RectangleSettings.Interpolation == EPCGCRectangleInterpolationMode::Step) {
			
			//Step mode
			
			//Check if step Lenght is <=0 (it's set to clamp to min in details settings, but can still be overriden to <0)
			if (Step < 0.1) {
				PCGE_LOG(Error, GraphAndLog, LOCTEXT("IllegalStepLenght", "Rectangle Step Lenght should be geater than 0.1"));
				//out
				return;
			}
			
			Steps = DistanceAB / Step;

			if (Step >= 1.0) {
				if (((int32)DistanceAB % (int32)Step) != 0 && ((int32)DistanceAB % (int32)Step) > (int32)Step / 3) {
					++Steps;
				}
			}
			
		}
		else if(Settings->RectangleSettings.Interpolation == EPCGCRectangleInterpolationMode::Subdivision){	
			
			//Subdivision mode

			//Check if we have proper number of divisions (it's set to clamp to min in details settings, but can still be overriden to illegal values)
			if (Steps < 1) {
				PCGE_LOG(Error, GraphAndLog, LOCTEXT("IllegalSubDivsNumber", "Number of Rectangle subdivisions should be geater than 0"));
				//out
				return;
			}

			Step = DistanceAB / Steps;
		}
		else if (Settings->RectangleSettings.Interpolation == EPCGCRectangleInterpolationMode::StepLW) {

			//Step LW mode

			if (bIsLenghtSide) {

				//Check if step Lenght is <=0 (it's set to clamp to min in details settings, but can still be overriden to <0)
				if (StepL < 0.1) {
					PCGE_LOG(Error, GraphAndLog, LOCTEXT("IllegalLenghtStep", "Rectangle Lenght Step should be geater than 0.1"));
					//out
					return;
				}

				Step = StepL;
				Steps = DistanceAB / StepL;

				if (Step >= 1.0) {
					if (((int32)DistanceAB % (int32)Step) != 0 && ((int32)DistanceAB % (int32)Step) > (int32)Step / 3) {
						++Steps;
					}
				}
			}
			else {

				//Check if step Lenght is <=0 (it's set to clamp to min in details settings, but can still be overriden to <0)
				if (StepW < 0.1) {
					PCGE_LOG(Error, GraphAndLog, LOCTEXT("IllegalWidthStep", "Rectangle Width Step should be geater than 0"));
					//out
					return;
				}

				Step = StepW;
				Steps = DistanceAB / StepW;

				if (Step >= 1.0) {
					if (((int32)DistanceAB % (int32)Step) != 0 && ((int32)DistanceAB % (int32)Step) > (int32)Step / 3) {
						++Steps;
					}
				}
			}
			
		}
		else if (Settings->RectangleSettings.Interpolation == EPCGCRectangleInterpolationMode::SubdivisionLW) {

			//Subdivision LW mode

			if (bIsLenghtSide) {

				//Check if we have proper number of divisions (it's set to clamp to min in details settings, but can still be overriden to illegal values)
				if (StepsL < 1) {
					PCGE_LOG(Error, GraphAndLog, LOCTEXT("IllegalSubDivsNumber", "Number of Rectangle Lenght subdivisions should be geater than 0"));
					//out
					return;
				}

				Step = DistanceAB / StepsL;
				Steps = StepsL;
			}
			else {

				//Check if we have proper number of divisions (it's set to clamp to min in details settings, but can still be overriden to illegal values)
				if (StepsW < 1) {
					PCGE_LOG(Error, GraphAndLog, LOCTEXT("IllegalSubDivsNumber", "Number of Rectangle Width subdivisions should be geater than 0"));
					//out
					return;
				}

				Step = DistanceAB / StepsW;
				Steps = StepsW;
			}
		}

		//Process the points
		FPCGAsync::AsyncPointProcessing(Context, Steps, Points, [CreatePoint, PointA, PointB, Step, DistanceAB, bOrientCorners](int32 Index, FPCGPoint& OutPoint)
			{
				double LerpAlpha = (double)(Step * Index) / DistanceAB;
				FVector Position = FMath::Lerp(PointA, PointB, LerpAlpha);

				OutPoint = CreatePoint(Position, Index == 0 && bOrientCorners);

				return true;
			});

	}

	if (bMergeSides) {

		//Merge sides in a single collection

		TArray<FPCGTaggedData> Sources = Context->OutputData.TaggedData;
		UPCGPointData* TargetPointData = nullptr;
		FPCGTaggedData* TargetTaggedData = nullptr;

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
	
}

void UPCGCSimpleShapeElement::CreateCircle(FPCGContext* Context, const UPCGCSimpleShapeSettings* Settings, TArray<FPCGTaggedData>& Outputs) const {

	//Create A circle Of Points

	//Initialize and get Output Point Data
	TArray<FPCGPoint>& Points = UPCGCSimpleShapeElement::GetOutputPoints(Outputs);

	//Get Properties from Settings
	const double Radius = Settings->CircleSettings.CircleRadius;

	//Check if Radius is <=0 (it's set to clamp to min in details settings, but can still be overriden to <0)
	if (Radius <= 0.0) {
		PCGE_LOG(Error, GraphAndLog, LOCTEXT("IllegalStepLenght", "Circle Radius should be geater than 0"));
		//out
		return;
	}

	const double Step = Settings->CircleSettings.CircleStep;
	const FVector PointExtents = Settings->PointExtents;
	const bool bOrientToCenter = Settings->CircleSettings.bOrientToCenter;
	const double RightAngle = FMath::DegreesToRadians(90);

	//Function That will create points
	const auto CreatePoint = [PointExtents, bOrientToCenter, RightAngle](const FVector Position, double Degree) {

		FPCGPoint Point;

		Point.Transform.SetLocation(Position);
		Point.SetExtents(PointExtents);
		Point.Steepness = 0.5;
		Point.Density = 1.0;

		Point.Seed = PCGHelpers::ComputeSeed((int)Position.X, (int)Position.Y, (int)Position.Z);

		//Should Point be oriented toward circle center
		if (bOrientToCenter) {
			Point.Transform.SetRotation(FQuat(FVector(0.0, 0.0, 1.0), Degree - RightAngle));
		}
		return Point;
	};

	//Calculate number of points
	double Steps;
	int32 Iterations;

	if (Settings->CircleSettings.Interpolation == EPCGCInterpolationMode::Step) {

		//If interpolation mode is - step

		//Check if step Lenght is <=0 (it's set to clamp to min in details settings, but can still be overriden to <0)
		if (Step < 0.1) {
			PCGE_LOG(Error, GraphAndLog, LOCTEXT("IllegalStepLenght", "Circle Step Lenght should be geater than 0.1"));
			//out
			return;
		}
	
		Steps = (Radius * 2 * PI) / Step;
		Iterations = Steps;
		Iterations++;
	}
	else {

		//If interpolation mode is - subdivisions
		Steps = Settings->CircleSettings.CircleSubdivisions;
		Iterations = Steps;

		//Check if we have proper number of divisions (it's set to clamp to min in details settings, but can still be overriden to illegal values)
		if (Iterations < 2) {
			PCGE_LOG(Error, GraphAndLog, LOCTEXT("IllegalSubDivsNumber", "Number of Circle subdivisions should be geater than 1"));
			//out
			return;
		}
	}

	double DegreeStep = FMath::DegreesToRadians(360.0 / Steps);

	//Process the points
	FPCGAsync::AsyncPointProcessing(Context, Iterations, Points, [CreatePoint, DegreeStep, Radius](int32 Index, FPCGPoint& OutPoint)
		{
			double Degree = DegreeStep * Index;
			FVector Position;

			Position.X = Radius * cos(Degree);
			Position.Y = Radius * sin(Degree);
			Position.Z = 0.0;

			//Create and set the point
			OutPoint = CreatePoint(Position, Degree);

			return true;
		});
}

#undef LOCTEXT_NAMESPACE