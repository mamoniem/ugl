/*
			$$\   $$\  $$$$$$\  $$\
			$$ |  $$ |$$  __$$\ $$ |
			$$ |  $$ |$$ /  \__|$$ |
			$$ |  $$ |$$ |$$$$\ $$ |
			$$ |  $$ |$$ |\_$$ |$$ |
			$$ |  $$ |$$ |  $$ |$$ |
			\$$$$$$  |\$$$$$$  |$$$$$$$$\
			 \______/  \______/ \________|

	Modify content in Editor & see them in a running
		game on a wide range of target platfomrs
		by:Muhammad A.Moniem(@_mamoniem)
				All rights reserved
						2019
				http://www.mamoniem.com/
*/

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/* Includes for Project Settings */
#include "Settings/UnrealGameLinkSettings.h"


class FUnrealGameLinkRuntimeModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	/*Delegates*/
	void OnWorldTickStart(UWorld* World, ELevelTick TickType, float DeltaTime);

	bool CheckForModifiedPackages();
	void ReloadModifiedPackages();
	void SortAndReloadModifiedPackages(TArray<UPackage*>& Packages, UWorld* World);

private:
	//read from the Project Settings
	bool bEnabledAtRuntime = false;
	bool bEnabledAtEditorRuntime = false;
	float EditorSyncInterval = 0.f;
	bool bReloadActiveMapOnContentUpdates = false;
	int32 PackagesToReloadPerPatch = 0;
	//mirror for the debug toggles
	bool bDebugEditorGeneralMessages = true;
	bool bDebugRuntimeTicks = true;
	bool bDebugRuntimePackagesReloading = true;

	float checkTimer = 0;
};
