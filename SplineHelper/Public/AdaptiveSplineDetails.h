// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "DetailCategoryBuilder.h"
#include "IDetailCustomization.h"
#include "Editor\DetailCustomizations\Private\SplineComponentDetails.h"
class USplineComponent;

class FAdaptiveSplineDetails : public IDetailCustomization
{
public:
    static TSharedRef<IDetailCustomization> MakeInstance();

    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
    void SubdivComponent(USplineComponent* Spline, int32 Subdivisions);
    void SubdivComponentsBetweenKeys(USplineComponent* SplineComponent, const TSet<int32> Keys);
    void SimplifyComponent(USplineComponent* Spline, int32 Simplifications);
    void SimplifyComponentsBetweenKeys(USplineComponent* SplineComponent, const TSet<int32> Keys);

private:
    FReply OnSimplifyClicked();
    FReply OnSubdivideClicked();
    void RequestWork(bool bSubdiv);

private:
    TOptional<int32> GetStepsValue() const;
    void OnStepsValueChanged(int32 NewValue);
    int32 StepsValue = 2;

    IDetailLayoutBuilder* CachedDetailBuilder;

};