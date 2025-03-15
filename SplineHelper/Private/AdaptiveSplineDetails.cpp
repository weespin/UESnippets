#include "AdaptiveSplineDetails.h"
#include "InputCoreTypes.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "MultiMeshSpline.h"
#include "UnrealEdGlobals.h"
#include "Components/SplineComponent.h"
#include "Editor/UnrealEdEngine.h"
#include "SplineComponentVisualizer.h"
#include "Widgets/Input/SNumericEntryBox.h"


TSharedRef<IDetailCustomization> FAdaptiveSplineDetails::MakeInstance()
{
	return MakeShareable(new FAdaptiveSplineDetails());
}

void FAdaptiveSplineDetails::SubdivComponent(USplineComponent* Spline, int32 Subdivisions = 2)
{
	if (!Spline || Subdivisions < 1)
	{
		return;
	}

	int32 OriginalPoints = Spline->GetNumberOfSplinePoints();

	TArray<FVector> NewPoints;
	TArray<FVector> NewTangents;
	TArray<FVector> NewLeaveTangents;
	TArray<FVector> NewArriveTangents;
	TArray<float> NewPointDistances;

	for (int32 i = 0; i < OriginalPoints - 1; i++)
	{
		FVector StartPoint = Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::Local);

		NewPoints.Add(StartPoint);

		for (int32 j = 1; j <= Subdivisions; j++)
		{
			float Alpha = static_cast<float>(j) / (Subdivisions + 1);
			float Distance = Spline->GetDistanceAlongSplineAtSplinePoint(i);
			float NextDistance = Spline->GetDistanceAlongSplineAtSplinePoint(i + 1);
			float InterpDistance = FMath::Lerp(Distance, NextDistance, Alpha);
			FVector NewPoint = Spline->GetLocationAtDistanceAlongSpline(InterpDistance, ESplineCoordinateSpace::Local);
			NewPoints.Add(NewPoint);
		}
	}

	NewPoints.Add(Spline->GetLocationAtSplinePoint(OriginalPoints - 1, ESplineCoordinateSpace::Local));

	Spline->ClearSplinePoints();

	for (int32 i = 0; i < NewPoints.Num(); i++)
	{
		Spline->AddSplinePoint(NewPoints[i], ESplineCoordinateSpace::Local, true);
	}

	Spline->UpdateSpline();
}

void FAdaptiveSplineDetails::SubdivComponentsBetweenKeys(USplineComponent* SplineComponent, const TSet<int32> SelectedPoints)
{
	if (!SplineComponent || SelectedPoints.Num() < 2)
	{
		return;
	}

	TArray<int32> SortedPoints = SelectedPoints.Array();
	SortedPoints.Sort();

	TArray<TPair<int32, int32>> SegmentsToSubdivide;
	for (int32 i = 0; i < SortedPoints.Num() - 1; i++)
	{
		for (int32 j = SortedPoints[i]; j < SortedPoints[i + 1]; j++)
		{
			if (j + 1 < SplineComponent->GetNumberOfSplinePoints())
			{
				SegmentsToSubdivide.Add(TPair<int32, int32>(j, j + 1));
			}
		}
	}

	if (SegmentsToSubdivide.Num() == 0)
	{
		return;
	}

	int32 OriginalPoints = SplineComponent->GetNumberOfSplinePoints();
	TArray<FVector> NewPoints;

	for (int32 i = 0; i < OriginalPoints; i++)
	{
		NewPoints.Add(SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::Local));
	}

	int32 PointsAdded = 0;
	for (const TPair<int32, int32>& Segment : SegmentsToSubdivide)
	{
		int32 StartIndex = Segment.Key;
		int32 EndIndex = Segment.Value;

		float StartDistance = SplineComponent->GetDistanceAlongSplineAtSplinePoint(StartIndex);
		float EndDistance = SplineComponent->GetDistanceAlongSplineAtSplinePoint(EndIndex);

		constexpr int32 Subdivisions = 2;
		for (int32 j = 1; j <= Subdivisions; j++)
		{
			float Alpha = static_cast<float>(j) / (Subdivisions + 1);
			float InterpDistance = FMath::Lerp(StartDistance, EndDistance, Alpha);
			FVector NewPoint = SplineComponent->GetLocationAtDistanceAlongSpline(
				InterpDistance, ESplineCoordinateSpace::Local);

			NewPoints.Insert(NewPoint, StartIndex + j + PointsAdded);
		}

		PointsAdded += Subdivisions;
	}

	SplineComponent->ClearSplinePoints();
	for (int32 i = 0; i < NewPoints.Num(); i++)
	{
		SplineComponent->AddSplinePoint(NewPoints[i], ESplineCoordinateSpace::Local, true);
	}

	SplineComponent->UpdateSpline();
}


void FAdaptiveSplineDetails::SimplifyComponent(USplineComponent* Spline, int32 Simplifications)
{
	if (!Spline || Simplifications < 1)
	{
		return;
	}

	int32 OriginalPoints = Spline->GetNumberOfSplinePoints();

	if (OriginalPoints <= 2 || OriginalPoints <= Simplifications + 1)
	{
		return;
	}

	TArray<FVector> Points;
	for (int32 i = 0; i < OriginalPoints; i++)
	{
		Points.Add(Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::Local));
	}

	TArray<FVector> SimplifiedPoints;
	SimplifiedPoints.Add(Points[0]);

	int32 PointsToKeep = OriginalPoints - Simplifications;
	if (PointsToKeep < 2)
	{
		PointsToKeep = 2;
	}

	float Step = static_cast<float>(OriginalPoints - 1) / static_cast<float>(PointsToKeep - 1);

	for (int32 i = 1; i < PointsToKeep - 1; i++)
	{
		float Index = i * Step;
		int32 LowerIndex = FMath::FloorToInt(Index);
		int32 UpperIndex = FMath::CeilToInt(Index);

		if (LowerIndex == UpperIndex)
		{
			SimplifiedPoints.Add(Points[LowerIndex]);
		}
		else
		{
			float Alpha = Index - LowerIndex;
			FVector InterpPoint = FMath::Lerp(Points[LowerIndex], Points[UpperIndex], Alpha);
			SimplifiedPoints.Add(InterpPoint);
		}
	}

	SimplifiedPoints.Add(Points[OriginalPoints - 1]);

	Spline->ClearSplinePoints();
	for (int32 i = 0; i < SimplifiedPoints.Num(); i++)
	{
		Spline->AddSplinePoint(SimplifiedPoints[i], ESplineCoordinateSpace::Local, true);
	}

	Spline->UpdateSpline();
}

void FAdaptiveSplineDetails::SimplifyComponentsBetweenKeys(USplineComponent* SplineComponent, const TSet<int32> SelectedPoints)
{
	if (!SplineComponent || SelectedPoints.Num() < 2)
	{
		return;
	}

	TArray<int32> SortedPoints = SelectedPoints.Array();
	SortedPoints.Sort();

	TArray<TPair<int32, int32>> SegmentsToSimplify;
	for (int32 i = 0; i < SortedPoints.Num() - 1; i++)
	{
		int32 StartIndex = SortedPoints[i];
		int32 EndIndex = SortedPoints[i + 1];

		if (EndIndex - StartIndex > 1)
		{
			SegmentsToSimplify.Add(TPair<int32, int32>(StartIndex, EndIndex));
		}
	}

	if (SegmentsToSimplify.Num() == 0)
	{
		return;
	}

	int32 OriginalPoints = SplineComponent->GetNumberOfSplinePoints();
	TArray<FVector> AllPoints;
	for (int32 i = 0; i < OriginalPoints; i++)
	{
		AllPoints.Add(SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::Local));
	}

	int32 PointsRemoved = 0;
	for (const TPair<int32, int32>& Segment : SegmentsToSimplify)
	{
		int32 StartIndex = Segment.Key - PointsRemoved;
		int32 EndIndex = Segment.Value - PointsRemoved;
		int32 PointsToRemove = EndIndex - StartIndex - 1;

		if (PointsToRemove > 0)
		{
			AllPoints.RemoveAt(StartIndex + 1, PointsToRemove);
			PointsRemoved += PointsToRemove;
		}
	}

	SplineComponent->ClearSplinePoints();
	for (int32 i = 0; i < AllPoints.Num(); i++)
	{
		SplineComponent->AddSplinePoint(AllPoints[i], ESplineCoordinateSpace::Local, true);
	}

	SplineComponent->UpdateSpline();
}

FSplineComponentVisualizer* GetFirstValidSplineVisualizer()
{
	if (GUnrealEd)
	{
		TSharedPtr<FComponentVisualizer> Visualizer = GUnrealEd->FindComponentVisualizer(
			USplineComponent::StaticClass()->GetFName());
		if (Visualizer.IsValid())
		{
			return static_cast<FSplineComponentVisualizer*>(Visualizer.Get());
		}
	}
	return nullptr;
}

void FAdaptiveSplineDetails::RequestWork(bool bSubdiv)
{
	if (CachedDetailBuilder)
	{
		TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized;
		CachedDetailBuilder->GetObjectsBeingCustomized(ObjectsBeingCustomized);

		if (ObjectsBeingCustomized.IsEmpty())
		{
			return;
		}

		if (USplineComponent* SplineComponent = Cast<USplineComponent>(ObjectsBeingCustomized[0].Get()))
		{
			const FSplineComponentVisualizer* SplineVisualizer = GetFirstValidSplineVisualizer();
			const TSet<int32>& SelectedKeys = SplineVisualizer->GetSelectedKeys();

			if (SelectedKeys.Num() < 2)
			{
				if (bSubdiv)
				{
					SubdivComponent(SplineComponent, StepsValue);
				}
				else
				{
					SimplifyComponent(SplineComponent, StepsValue);
				}
			}
			else
			{
				if (bSubdiv)
				{
					SubdivComponentsBetweenKeys(SplineComponent, SelectedKeys);
				}
				else
				{
					SimplifyComponentsBetweenKeys(SplineComponent, SelectedKeys);
				}
			}

			if (AMultiMeshSpline* MultiMeshSpline = Cast<AMultiMeshSpline>(SplineComponent->GetOwner()))
			{
				MultiMeshSpline->Refresh();
			}
		}
	}
}

FReply FAdaptiveSplineDetails::OnSimplifyClicked()
{
	RequestWork(false);
	return FReply::Handled();
}

FReply FAdaptiveSplineDetails::OnSubdivideClicked()
{
	RequestWork(true);
	return FReply::Handled();
}

void FAdaptiveSplineDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	CachedDetailBuilder = &DetailBuilder;
	IDetailCategoryBuilder& CustomSection = DetailBuilder.EditCategory("SplineUtilsSection",FText::FromString("Spline Utilities Section"),ECategoryPriority::TypeSpecific);
	IDetailCategoryBuilder& CustomCategory = CustomSection.GetParentLayout().EditCategory(
		"SplineUtilsTab", FText::FromString("Spline Utilities"), ECategoryPriority::TypeSpecific);

	CustomCategory.AddCustomRow(FText::FromString("Simplify Row"))
		.NameContent()[
			SNew(STextBlock)
			.Text(FText::FromString("Simplify"))
		]
		.ValueContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.0f, 0.0f, 5.0f, 0.0f)
			[
				SNew(SButton)
				.Text(FText::FromString("Start"))
				.OnClicked(FOnClicked::CreateRaw(this, &FAdaptiveSplineDetails::OnSimplifyClicked))
			]

		];
	CustomCategory.AddCustomRow(FText::FromString("Subdivide Row"))
	.NameContent()[
			SNew(STextBlock)
			.Text(FText::FromString("Subdivide"))
		]
		.ValueContent()
		[

			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			  .AutoWidth()
			  .Padding(0.0f, 0.0f, 5.0f, 0.0f)
			[
				SNew(SButton)
						.Text(FText::FromString("Start"))
						.OnClicked(FOnClicked::CreateRaw(this, &FAdaptiveSplineDetails::OnSubdivideClicked))
			]


		];
	CustomCategory.AddCustomRow(FText::FromString("Steps Row"))
	.NameContent()[
			SNew(STextBlock)
			.Text(FText::FromString("Steps"))
		]
		.ValueContent()[
			SNew(SHorizontalBox) +
			SHorizontalBox::Slot()
			.HAlign(HAlign_Fill)
			[
				SNew(SNumericEntryBox<int32>)
						.Value(this, &FAdaptiveSplineDetails::GetStepsValue)
						.OnValueChanged(this, &FAdaptiveSplineDetails::OnStepsValueChanged)
						.MinValue(2)
						.MaxValue(20)
						.MinSliderValue(2)
						.MaxSliderValue(20)
						.AllowSpin(true)
			]
		];
}

TOptional<int32> FAdaptiveSplineDetails::GetStepsValue() const
{
	return StepsValue;
}

void FAdaptiveSplineDetails::OnStepsValueChanged(int32 NewValue)
{
	StepsValue = NewValue;
}
