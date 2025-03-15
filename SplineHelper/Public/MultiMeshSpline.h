// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AdaptiveSplineComponent.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Actor.h"
#include "MultiMeshSpline.generated.h"

class USplineMeshComponent;

USTRUCT(Blueprintable)
struct FSplinedMeshRange
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry")
	float RangeStart;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry")
	float RangeEnd;
};

UENUM(Blueprintable)
enum ESplineMeshType
{
	Point,
	TimeBased,
	Steepness
};


UENUM(Blueprintable)
enum ERepetitionType
{
	Always,
	BetweenPoints,
	BetweenTimeIntervals
};

USTRUCT(Blueprintable)
struct FAdditionalMeshRepetitionParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repetition")
	float Repetition = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repetition")
	TEnumAsByte<ERepetitionType> Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Repetition", meta = (EditCondition = "RepeatenceType != ERepeatenceType::Always"))
	TArray<FSplinedMeshRange> Ranges;
};

USTRUCT(Blueprintable)
struct FAdditionalMeshInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UStaticMeshComponent> MeshClass = UStaticMeshComponent::StaticClass(); 

	UPROPERTY(EditAnywhere)
	FVector Scale = FVector(1,1,1);

	UPROPERTY(EditAnywhere)
	FVector LocationOffset;

	UPROPERTY(EditAnywhere)
	FRotator RotationOffset;

	UPROPERTY(EditAnywhere)
	bool bAdjustByBounds;
};

USTRUCT(Blueprintable)
struct FAdditionalMesh
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FAdditionalMeshInfo InstanceInfo;

	UPROPERTY(EditAnywhere)
	FAdditionalMeshRepetitionParams RepetitionInfo;

	UPROPERTY(EditAnywhere)
	FName Identifier;

	UPROPERTY(EditAnywhere)
	bool bTriggerCreationEvent;
};

UCLASS(Blueprintable)
class SPLINEHELPER_API AMultiMeshSpline : public AActor
{
	GENERATED_BODY()
	
public:	
	AMultiMeshSpline();
	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	void GenerateMeshesByPoints();
	void GenerateMeshesByTime();
	void GenerateMeshesBySteepness();
	void GenerateAdditionalMeshes();

	void FindSteepnessPoints(float StartTime, float EndTime, float InMaxSteepness, TArray<float>& TimePoints);
	void CreateSplineMeshSegment(float StartTime, float EndTime);
	UStaticMeshComponent* CreateMeshAtPosition(float Position, const FAdditionalMeshInfo& MeshInfo);
	FORCEINLINE float ConvertPointToTime(const int32 Point) const;

public:
	UFUNCTION(BlueprintImplementableEvent)
	void OnAdditionalMeshCreated(int32 Index, int32 MaxIndex, const FName& Name, UStaticMeshComponent* CreatedMesh);

	UFUNCTION(BlueprintCallable)
	void UpdateCollisionInfo();

	void Refresh();

	virtual void Tick(float DeltaTime) override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UAdaptiveSplineComponent* Spline;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Collision, meta = (ShowOnlyInnerProperties, SkipUCSModifiedProperties))
	FBodyInstance BodyInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FAdditionalMesh> AdditionalMeshSettings;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<ESplineMeshType> SplineType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "SplineType != ESplineMeshType::Point", ClampMin = "0.001", UIMin = "0.001"))
	float TimeInterval = 0.1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "SplineType == ESplineMeshType::Steepness", ClampMin = "0.5", UIMin = "0.5"))
	float MaxSteepnessThreshold = 20;

private:
	UPROPERTY()
	TArray<USplineMeshComponent*> CreatedMeshes;

	UPROPERTY()
	TArray<UStaticMeshComponent*> CreatedAdditionalMeshes;
};
