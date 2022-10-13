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

/* Includes for Project Settings */
#include "Settings/GameLinkSettings.h"

class FToolBarBuilder;
class FMenuBuilder;

class FGameLinkModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	void GameLinkToolbarButtonClicked();
	
private:
	void RegisterMenus();
	void RegisterProjectSettings();
	void UnRegisterProjectSettings();
	void OnShutdownPostPackagesSaved();

	void FetchProjectName();
	void FetchUserCustomProjectSetting();
	void CleanupCookingDirectory();
	bool CheckAndAutoCookIfNeeded();
	bool GetAllModifiedPackages(const bool CheckMaps, const bool CheckAssets, TArray<UPackage*>& OutPackages);
	bool GetAllTargetPackagingPlatforms(TArray<ITargetPlatform*>& OutPackagingPlatforms);
	bool CookModifiedPackages(const TArray<UPackage*> PackagesList, const TArray<ITargetPlatform*> TargetPlatformsList);
	bool CookModifiedPackage(UPackage* Package, ITargetPlatform* TargetPlatform, const FString& CookedFileNamePath);
	bool CopyModifiedPackages();
	
	//Helpers
	void GetPackagesCookingDirectory(UPackage* Package, const FString& GameLinkCookedDirectory, const FString& TargetPlatformName, FString& OutPackageCookDirectory);
	FString GetGameLinkParentCookingDirectory();
	FString GetGameLinkLogingDirectory();

private:
	TSharedPtr<class FUICommandList> PluginCommands;

	FString ProjectName = "";

	//tracking info
	bool IsTrackUsage = true;
	int32 TrackedUsageCount = 0;
	FString TrackedInfoToPush = "";
	FString TrackedInfoDir = "";

	//read from the Project Settings
	bool bSaveBeforeCooking = true;
	bool bNotifyCookingResults = true;
	bool bSaveConcurrent = true;
	bool bSaveAsync = true;
	bool bSaveUnversioned = false;
	TArray<FGameLinkTargetPlatform> TargetPlatforms;
	//read from the Project Settings [debug toggles]
	bool bDebugEditorGeneralMessages = true;
	bool bDebugEditorPackagesOperations = true;
	bool bDebugEditorCooker = true;
	bool bDebugRuntimeTicks = true;
	bool bDebugRuntimePackagesReloading = true;
};
