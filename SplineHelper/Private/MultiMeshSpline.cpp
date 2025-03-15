// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiMeshSpline.h"

#include "Components/SplineMeshComponent.h"

AMultiMeshSpline::AMultiMeshSpline()
{
	PrimaryActorTick.bCanEverTick = false;

	Spline = CreateDefaultSubobject<UAdaptiveSplineComponent>(TEXT("Spline Component"));
	Spline->SetMobility(EComponentMobility::Static);

	RootComponent = Spline;
}

void AMultiMeshSpline::GenerateMeshesByPoints()
{
	const int32 MaxPoints = Spline->GetNumberOfSplinePoints();

	for (int32 Point = 0; Point < MaxPoints - 1; ++Point)
	{
		const float NextPoint = Point + 1;
		const float Start = ConvertPointToTime(Point);
		const float End = ConvertPointToTime(NextPoint);
		CreateSplineMeshSegment(Start, End);
	}
}

void AMultiMeshSpline::GenerateMeshesByTime()
{
	const float Start = 0;
	const float End = Spline->Duration;

	for (float nCurrentPosition = Start; nCurrentPosition < End; nCurrentPosition += TimeInterval)
	{
		CreateSplineMeshSegment(nCurrentPosition, nCurrentPosition + TimeInterval);
	}
}

void AMultiMeshSpline::GenerateMeshesBySteepness()
{
	const float Start = 0;
	const float End = Spline->Duration;
	TArray<float> TimePoints;

	TimePoints.Add(Start);

	for (float nCurrentPosition = Start; nCurrentPosition < End; nCurrentPosition += TimeInterval)
	{
		const float NextPosition = FMath::Min(nCurrentPosition + TimeInterval, End);
		FindSteepnessPoints(nCurrentPosition, NextPosition, MaxSteepnessThreshold, TimePoints);
	}

	if (!TimePoints.Contains(End))
	{
		TimePoints.Add(End);
	}

	TimePoints.Sort();

	for (int32 i = 0; i < TimePoints.Num() - 1; i++)
	{
		CreateSplineMeshSegment(TimePoints[i], TimePoints[i + 1]);
	}
}

void AMultiMeshSpline::FindSteepnessPoints(float StartTime, float EndTime, float InMaxSteepness, TArray<float>& TimePoints)
{
	if (StartTime == EndTime)
	{
		return;
	}

	const FVector StartTangent = Spline->GetTangentAtTime(StartTime, ESplineCoordinateSpace::Local).GetSafeNormal();
	const FVector EndTangent = Spline->GetTangentAtTime(EndTime, ESplineCoordinateSpace::Local).GetSafeNormal();

	const float Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(StartTangent, EndTangent)));

	if (Angle > InMaxSteepness)
	{
		const float MidTime = (StartTime + EndTime) * 0.5f;

		TimePoints.AddUnique(MidTime);

		FindSteepnessPoints(StartTime, MidTime, InMaxSteepness, TimePoints);
		FindSteepnessPoints(MidTime, EndTime, InMaxSteepness, TimePoints);
	}
}

void AMultiMeshSpline::CreateSplineMeshSegment(float StartTime, float EndTime)
{
	USplineMeshComponent* Component = Cast<USplineMeshComponent>(AddComponentByClass(USplineMeshComponent::StaticClass(), true, GetActorTransform(), false));

	if(!IsValid(Component))
	{
		return;
	}

	CreatedMeshes.Add(Component);
	Component->SetStaticMesh(Mesh);
	Component->SetMobility(EComponentMobility::Static);
	const FVector StartPoint = Spline->GetLocationAtTime(StartTime, ESplineCoordinateSpace::Local);
	const FVector EndPoint = Spline->GetLocationAtTime(EndTime, ESplineCoordinateSpace::Local);
	const FVector StartTangent = Spline->GetTangentAtTime(StartTime, ESplineCoordinateSpace::Local);
	const FVector EndTangent = Spline->GetTangentAtTime(EndTime, ESplineCoordinateSpace::Local);
	const FVector StartScale = Spline->GetScaleAtTime(StartTime);
	const FVector EndScale = Spline->GetScaleAtTime(EndTime);
	const float StartRoll = Spline->GetRollAtTime(StartTime, ESplineCoordinateSpace::Local);
	const float EndRoll = Spline->GetRollAtTime(EndTime, ESplineCoordinateSpace::Local);

	Component->SetStartAndEnd(StartPoint, StartTangent, EndPoint, EndTangent);
	Component->SetStartRoll(FMath::DegreesToRadians(StartRoll));
	Component->SetEndRoll(FMath::DegreesToRadians(EndRoll));
	Component->SetStartScale(FVector2D{ StartScale.Y, StartScale.Z });
	Component->SetEndScale(FVector2D{ EndScale.Y, EndScale.Z });
	Component->BodyInstance.CopyRuntimeBodyInstancePropertiesFrom(&this->BodyInstance);
	Component->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
}

struct FPositionRange
{
	float Start;
	float End;
};

void AMultiMeshSpline::GenerateAdditionalMeshes()
{
	const int32 AdditionalMeshesNum = AdditionalMeshSettings.Num();
	for (int32 nAdditionalMesh = 0; nAdditionalMesh < AdditionalMeshesNum; ++nAdditionalMesh)
	{
		const FAdditionalMesh& CurrentAdditionalMesh = AdditionalMeshSettings[nAdditionalMesh];
		const FAdditionalMeshInfo& CurrentMeshInfo = CurrentAdditionalMesh.InstanceInfo;
		const FAdditionalMeshRepetitionParams& CurrentRepetitionInfo = CurrentAdditionalMesh.RepetitionInfo;

		if(FMath::IsNearlyZero(CurrentRepetitionInfo.Repetition) || !IsValid(CurrentMeshInfo.Mesh) || !IsValid(CurrentMeshInfo.MeshClass))
		{
			continue;
		}

		TArray<FPositionRange> PositionRanges;
		switch (CurrentRepetitionInfo.Type)
		{
			case ERepetitionType::Always:
			{
				FPositionRange Range;
				Range.Start = 0.0f;
				Range.End = Spline->Duration;
				PositionRanges.Add(Range);
				break;
			}
			case ERepetitionType::BetweenTimeIntervals:
			{
				const int32 nRanges = CurrentRepetitionInfo.Ranges.Num();
				for (int32 nRange = 0; nRange < nRanges; ++nRange)
				{
					const FSplinedMeshRange& CurrentRange = CurrentRepetitionInfo.Ranges[nRange];
					FPositionRange Range;
					Range.Start = FMath::Max(CurrentRange.RangeStart, 0.0f);
					Range.End = FMath::Min(Spline->Duration, CurrentRange.RangeEnd);
					PositionRanges.Add(Range);
				}
				break;
			}
			case ERepetitionType::BetweenPoints:
			{
				const int32 nRanges = CurrentRepetitionInfo.Ranges.Num();
				for (int32 nRange = 0; nRange < nRanges; ++nRange)
				{
					const FSplinedMeshRange& CurrentRange = CurrentRepetitionInfo.Ranges[nRange];
					FPositionRange Range;
					Range.Start = ConvertPointToTime(FMath::Max(CurrentRange.RangeStart, 0));
					Range.End = ConvertPointToTime(FMath::Min(Spline->GetNumberOfSplinePoints() - 1, CurrentRange.RangeEnd));
					PositionRanges.Add(Range);
				}
				break;
			}
		}

		const float Repetition = FMath::Max(CurrentRepetitionInfo.Repetition, 0.009f);
		for (const FPositionRange& Range : PositionRanges)
		{
			for (float CurrentPosition = Range.Start; CurrentPosition < Range.End; CurrentPosition += Repetition)
			{
				const int32 Index = FMath::FloorToInt((CurrentPosition - Range.Start) / Repetition);
				const int32 MaxIndex = FMath::FloorToInt((Range.End - Range.Start) / Repetition);
				UStaticMeshComponent* NewComponent = CreateMeshAtPosition(CurrentPosition, CurrentMeshInfo);

				if (CurrentAdditionalMesh.bTriggerCreationEvent)
				{
					OnAdditionalMeshCreated(Index, MaxIndex, CurrentAdditionalMesh.Identifier, NewComponent);
				}
			}
		}
	}
}

UStaticMeshComponent* AMultiMeshSpline::CreateMeshAtPosition(float Position, const FAdditionalMeshInfo& MeshInfo)
{
	const FVector Location = Spline->GetLocationAtTime(Position, ESplineCoordinateSpace::Local);
	const FVector Tangent = Spline->GetTangentAtTime(Position, ESplineCoordinateSpace::Local);

	UStaticMeshComponent* Component = Cast<UStaticMeshComponent>(AddComponentByClass(MeshInfo.MeshClass, false, Spline->GetComponentTransform(), false));
	if(!IsValid(Component))
	{
		return nullptr;
	}

	CreatedAdditionalMeshes.Add(Component);

	Component->SetStaticMesh(MeshInfo.Mesh);
	Component->SetRelativeLocation(Location + MeshInfo.LocationOffset);
	Component->SetRelativeScale3D(MeshInfo.Scale);

	if (MeshInfo.bAdjustByBounds)
	{
		FVector BoundsMin;
		FVector BoundsMax;
		Component->GetLocalBounds(BoundsMin, BoundsMax);
		const FVector BoundsCenter = ((BoundsMin + BoundsMax) / 2.0f) * MeshInfo.Scale;
		Component->AddRelativeLocation(-BoundsCenter);
	}

	Component->SetRelativeRotation(FRotationMatrix::MakeFromX(Tangent).Rotator() + MeshInfo.RotationOffset);
	Component->AttachToComponent(Spline, FAttachmentTransformRules::KeepRelativeTransform);
	return Component;

}

FORCEINLINE float AMultiMeshSpline::ConvertPointToTime(const int32 Point) const
{
	return Spline->GetDistanceAlongSplineAtSplinePoint(Point) / Spline->GetSplineLength() * Spline->Duration;
}

void AMultiMeshSpline::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Refresh();
}

void AMultiMeshSpline::UpdateCollisionInfo()
{
	for (USplineMeshComponent* CreatedMesh : CreatedMeshes)
	{
		if (CreatedMesh)
		{
			CreatedMesh->BodyInstance.CopyRuntimeBodyInstancePropertiesFrom(&this->BodyInstance);
		}
	}
}

void AMultiMeshSpline::Refresh()
{
	for (USplineMeshComponent* CreatedMesh : CreatedMeshes)
	{
		if (CreatedMesh)
		{
			CreatedMesh->DestroyComponent();
		}
	}
	for (UStaticMeshComponent* CreatedMesh : CreatedAdditionalMeshes)
	{
		if (CreatedMesh)
		{
			CreatedMesh->DestroyComponent();
		}
	}
	CreatedAdditionalMeshes.Empty();
	CreatedMeshes.Empty();

	switch (SplineType)
	{
		case Point: GenerateMeshesByPoints(); break;
		case TimeBased: GenerateMeshesByTime(); break;
		case Steepness: GenerateMeshesBySteepness(); break;
	}
	
	GenerateAdditionalMeshes();
}

void AMultiMeshSpline::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

