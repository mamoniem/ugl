/*

 $$$$$$\                                    $$\       $$\           $$\
$$  __$$\                                   $$ |      \__|          $$ |
$$ /  \__| $$$$$$\  $$$$$$\$$$$\   $$$$$$\  $$ |      $$\ $$$$$$$\  $$ |  $$\
$$ |$$$$\  \____$$\ $$  _$$  _$$\ $$  __$$\ $$ |      $$ |$$  __$$\ $$ | $$  |
$$ |\_$$ | $$$$$$$ |$$ / $$ / $$ |$$$$$$$$ |$$ |      $$ |$$ |  $$ |$$$$$$  /
$$ |  $$ |$$  __$$ |$$ | $$ | $$ |$$   ____|$$ |      $$ |$$ |  $$ |$$  _$$<
\$$$$$$  |\$$$$$$$ |$$ | $$ | $$ |\$$$$$$$\ $$$$$$$$\ $$ |$$ |  $$ |$$ | \$$\
 \______/  \_______|\__| \__| \__| \_______|\________|\__|\__|  \__|\__|  \__|

				Modify content in Editor & see them in a running
					game on a wide range of target platfomrs
					by:Muhammad A.Moniem(@_mamoniem)
							All rights reserved
								2002-2022
						http://www.mamoniem.com/
*/

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Engine/EngineBaseTypes.h"
#include "Settings/GameLinkSettings.h"


class FGameLinkRuntimeModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void OnWorldTickStart(UWorld* World, ELevelTick TickType, float DeltaTime);

	void ReadAllSettingFromConfig();
	void ResetMostRecentInConfig();
	bool CheckForModifiedPackages();
	void ReloadModifiedPackages();
	void SortAndReloadModifiedPackages(TArray<UPackage*>& Packages, UWorld* World);

private:
	bool bEnabledAtRuntime = false;
	bool bEnabledAtEditorRuntime = false;
	float EditorSyncInterval = 0.f;
	bool bReloadActiveMapOnContentUpdates = false;
	int32 PackagesToReloadPerPatch = 0;

	bool bDebugEditorGeneralMessages = true;
	bool bDebugRuntimeTicks = true;
	bool bDebugRuntimePackagesReloading = true;

	float checkTimer = 0;
};
