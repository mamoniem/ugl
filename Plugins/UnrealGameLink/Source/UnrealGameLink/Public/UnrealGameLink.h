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

class FToolBarBuilder;
class FMenuBuilder;

class FUnrealGameLinkModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	void UnrealGameLinkToolbarButtonClicked();
	
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
	//bool CookModifiedPackage(UPackage* Package, ITargetPlatform* TargetPlatform, const FString& CookDir, const FString& CookedFileNamePath);
	bool CookAllModifiedPackages(TArray<UPackage*> Packages, ITargetPlatform* TargetPlatform, const FString& CookDir, TArray<FString>& CookedFileNamePaths);
	bool CopyModifiedPackages();
	
	//Helpers
	void GetPackagesCookingDirectory(UPackage* Package, const FString& UnrealGameLinkCookedDirectory, const FString& TargetPlatformName, FString& OutPackageCookDirectory);
	FString GetUnrealGameLinkParentCookingDirectory();
	FString GetUnrealGameLinkLogingDirectory();

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
	TArray<FUnrealGameLinkTargetPlatform> TargetPlatforms;
	//read from the Project Settings [debug toggles]
	bool bDebugEditorGeneralMessages = true;
	bool bDebugEditorPackagesOperations = true;
	bool bDebugEditorCooker = true;
	bool bDebugRuntimeTicks = true;
	bool bDebugRuntimePackagesReloading = true;
};
