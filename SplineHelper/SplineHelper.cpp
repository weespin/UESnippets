#include "SplineHelper.h"

#include "AdaptiveSplineDetails.h"
#include "Modules/ModuleManager.h"


void FSplineHelper::StartupModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout(
		"AdaptiveSplineComponent",
		FOnGetDetailCustomizationInstance::CreateStatic(&FAdaptiveSplineDetails::MakeInstance)
	);
}

void FSplineHelper::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomClassLayout("AdaptiveSplineComponent");
	}
}
IMPLEMENT_MODULE(FSplineHelper, SplineHelper)