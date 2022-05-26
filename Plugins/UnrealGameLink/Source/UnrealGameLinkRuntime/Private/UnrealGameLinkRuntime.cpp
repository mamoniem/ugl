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

#include "UnrealGameLinkRuntime.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameEngine.h"

//#define LOCTEXT_NAMESPACE "FUnrealGameLinkRuntimeModule"
DEFINE_LOG_CATEGORY_STATIC(LogUnrealGameLinkRuntime, Log, Log);

void FUnrealGameLinkRuntimeModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	/*
	[1] - Read the settings
	[2] - If Runtime Enabled, then register the auto-check
	*/

	//GetDefault<UUnrealGameLinkSettings>() vs GetMutableDefault<UUnrealGameLinkSettings>()
	if (UUnrealGameLinkSettings* UnrealGameLinkProjectSettings = GetMutableDefault<UUnrealGameLinkSettings>())
	{
		//Update obj values from the ini file
		bEnabledAtRuntime = UnrealGameLinkProjectSettings->bEnabledAtRuntime;
		bEnabledAtEditorRuntime = UnrealGameLinkProjectSettings->bEnabledAtEditorRuntime;
		EditorSyncInterval = UnrealGameLinkProjectSettings->EditorSyncInterval;
		bReloadActiveMapOnContentUpdates = UnrealGameLinkProjectSettings->bReloadActiveMapOnContentUpdates;
		PackagesToReloadPerPatch = UnrealGameLinkProjectSettings->PackagesToReloadPerPatch;
		bDebugEditorGeneralMessages = UnrealGameLinkProjectSettings->bDebugEditorGeneralMessages;
		bDebugRuntimeTicks = UnrealGameLinkProjectSettings->bDebugRuntimeTicks;
		bDebugRuntimePackagesReloading = UnrealGameLinkProjectSettings->bDebugRuntimePackagesReloading;

		//Update values in the ini, aka reset values
		UnrealGameLinkProjectSettings->MostRecentModifiedContent.Empty();
		UnrealGameLinkProjectSettings->SaveConfig(CPF_Config, *UnrealGameLinkProjectSettings->GetDefaultConfigFilename());
	}

	if (bDebugEditorGeneralMessages)
		UE_LOG(LogUnrealGameLinkRuntime, Log, TEXT("Note: \n*\n*\n*\n*\n*\nFUnrealGameLinkRuntimeModule::StartupModule, COMPLETED with bEnabledAtRuntime is set to [%s]\n*\n*\n*\n*\n*"), bEnabledAtRuntime ? *FString("True") : *FString("False"));

	//No need for the runtime hacky. We only need that in a running standalone (PC, Console,...etc.)
#if WITH_EDITOR
	if (!bEnabledAtEditorRuntime)
	{
		bEnabledAtRuntime = false;
		if (bDebugEditorGeneralMessages)
			UE_LOG(LogUnrealGameLinkRuntime, Log, TEXT("Note: \n*\n*\n*\n*\n*\nFUnrealGameLinkRuntimeModule::StartupModule, DISABLED for editor, with bEnabledAtRuntime is set to [%s]\n*\n*\n*\n*\n*"), bEnabledAtRuntime ? *FString("True") : *FString("False"));
	}
#endif

#if WITH_ENGINE
#endif

	//<3 delegates <3. Some were for testing, and let's keep them not totally removed for future updates and features, only cleaned up their body at the meantime.
	FWorldDelegates::OnWorldTickStart.AddRaw(this, &FUnrealGameLinkRuntimeModule::OnWorldTickStart);	
}

/*
* This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading, we call this function before unloading the module.
*/
void FUnrealGameLinkRuntimeModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	FWorldDelegates::OnWorldTickStart.RemoveAll(this);
}

/*
* Delegate FWorldDelegates::OnWorldTickStart()
* 
* @param World			The world that is ticking (usually the current loaded one)
* @param TickType		What is ticking. The level time, the viewport, both or nothing.
* @param DeltaTime		The standard delta time.
*/
void FUnrealGameLinkRuntimeModule::OnWorldTickStart(UWorld* World, ELevelTick TickType, float DeltaTime)
{
	//UE_LOG(LogUnrealGameLinkRuntime, Log, TEXT("Note: \n*\n*\n*\n*\n*\n FUnrealGameLinkRuntimeModule::OnWorldTickStart =========>>>>> The world [%s] did start ticking, we can spawn the actor or start whatever we need in here \n*\n*\n*\n*\n*"), *World->GetName());
	if (!bEnabledAtRuntime)
		return;

	checkTimer += DeltaTime;
	if (checkTimer > EditorSyncInterval)
	{
		if (bDebugRuntimeTicks)
			UE_LOG(LogUnrealGameLinkRuntime, Log, TEXT("Note: \n*\n*\n*\n*\n*\n FUnrealGameLinkRuntimeModule::OnWorldTickStart =========>>>>> The world [%s] is TICKING and printing this every [%f] seconds \n*\n*\n*\n*\n*"), *World->GetName(), EditorSyncInterval);
		checkTimer = 0.f;

		//Update the running game
		if (CheckForModifiedPackages())
			ReloadModifiedPackages();
	}
}

/*
* Checks for any updates being sent from Unreal Editor to the current running game. Usually this will be in the form of modified packages list. This will be ticking based on the configs of UnrealGameLink that is set in the project settings.
*/
bool FUnrealGameLinkRuntimeModule::CheckForModifiedPackages()
{
	/*
	Keep in mind, using:
	GConfig->UnloadFile(), GConfig->LoadFile(), UnrealGameLinkProjectSettings->UpdateDefaultConfigFile(), UnrealGameLinkProjectSettings->LoadConfig() or UnrealGameLinkProjectSettings->ReloadConfig();
	will always fail.Due to reading the existing values from memory, but we need to re-read from disk.
	*/ 
	FConfigFile configFile;
	TArray<FString> foundArrayData;
	GConfig->LoadLocalIniFile(configFile, TEXT("UnrealGameLink"), true, ANSI_TO_TCHAR(FPlatformProperties::IniPlatformName()), true); //DefaultUnrealGameLink.ini
	configFile.GetArray(TEXT("/Script/UnrealGameLinkRuntime.UnrealGameLinkSettings"), TEXT("MostRecentModifiedContent"), foundArrayData);
	return foundArrayData.Num() > 0 ? true : false;

}

/*
* Execute a reload for the packages that been modified in the unreal Editor and been sent to the current running game instance.
*/
void FUnrealGameLinkRuntimeModule::ReloadModifiedPackages()
{
	/*
	Note that we can't use the GetMutableDefault<UUnrealGameLinkSettings>() method now, it will read from MEMORY. 
	And we need to re-read from disk. But using LoadLocalIniFile() is 100% guranteed.
	*/

	bool bPostCleanupReloadWorld = false;

	FConfigFile ConfigFile;
	TArray<FString> FoundArrayData;
	TArray<UPackage*> FoundArrayDataAsPackages;
	GConfig->LoadLocalIniFile(ConfigFile, TEXT("UnrealGameLink"), true, ANSI_TO_TCHAR(FPlatformProperties::IniPlatformName()), true); //DefaultUnrealGameLink.ini
	ConfigFile.GetArray(TEXT("/Script/UnrealGameLinkRuntime.UnrealGameLinkSettings"), TEXT("MostRecentModifiedContent"), FoundArrayData);

	int32 capacity = FoundArrayData.Num();

	if (bDebugRuntimePackagesReloading)
		UE_LOG(LogUnrealGameLinkRuntime, Log, TEXT("Note: \n*\n*\n FUnrealGameLinkRuntimeModule::ReloadModifiedPackages =========>>>>> STARTED [%d] <<<<<========= \n*\n*"), capacity);

	for (FString FoundItem : FoundArrayData)
	{
		if (FoundItem.Contains(FPlatformProperties::PlatformName(), ESearchCase::CaseSensitive))
		{
			int32 strFoundAt = FoundItem.Find("/Content", ESearchCase::IgnoreCase, ESearchDir::FromStart);
			check(strFoundAt != -1);
			FString packageRelativePath = FoundItem.RightChop(strFoundAt); //"/Content/..../..../..../...."
			packageRelativePath = packageRelativePath.Replace(*FString("Content"), *FString("Game"));
			if (bDebugRuntimePackagesReloading)
				UE_LOG(LogUnrealGameLinkRuntime, Log, TEXT("Note: FUnrealGameLinkRuntimeModule::ReloadModifiedPackages =========>>>>> TargetFile [%s] <<<<<========="), *packageRelativePath);


			UPackage* package = LoadPackage(nullptr, *packageRelativePath, 0);
			FoundArrayDataAsPackages.AddUnique(package);

		}
	}

	//Check if any package is the current running world, if so, let's cut it short, and reload the world right away (don't wait to reload it as package as this would most likely fail), if not let's reload the packages as expected
	TWeakObjectPtr<UWorld> RunningWorld;
	if (UGameEngine* GameEngine = Cast<UGameEngine>(GEngine))
	{
		if (UWorld* GameWorld = GameEngine->GetGameWorld())
		{
			RunningWorld = GameWorld;
		}
	}

	if (FoundArrayDataAsPackages.Num() != 0)
	{
		if (bDebugRuntimePackagesReloading)
			UE_LOG(LogUnrealGameLinkRuntime, Warning, TEXT("Note: FUnrealGameLinkRuntimeModule::ReloadModifiedPackages =========>>>>> World Context 0 World map name is [%s] and name [%s] <<<<<========="), *GEngine->GetWorldContexts()[0].World()->GetMapName(), *GEngine->GetWorldContexts()[0].World()->GetName()); //context 0 is the same as the main world. In running build there is only one context world.
		//If a map is one of those packages, let's cut it short (TODO::Only if matching the currently running map, not just map in general)
		for (UPackage* package : FoundArrayDataAsPackages)
		{
			if (bDebugRuntimePackagesReloading)
				UE_LOG(LogUnrealGameLinkRuntime, Warning, TEXT("Note: FUnrealGameLinkRuntimeModule::ReloadModifiedPackages =========>>>>> Will compare Package [%s] against Package [%s] <<<<<========="), *package->GetName(), *RunningWorld.Get()->GetOutermost()->GetName());
			
			if (package->HasAnyPackageFlags(PKG_ContainsMap | PKG_ContainsMapData))
			{
				if (bDebugRuntimePackagesReloading)
					UE_LOG(LogUnrealGameLinkRuntime, Warning, TEXT("Note: FUnrealGameLinkRuntimeModule::ReloadModifiedPackages =========>>>>> Package [%s] is flagged PKG_ContainsMap or PKG_ContainsMapData <<<<<========="), *package->GetName());
				
				bPostCleanupReloadWorld = true;
				break;
			}
			else
			{
				//do nothing
			}
		}

		//Only if the current opened map is NOT part of the packages, we proceed to reload the packages
		if (!bPostCleanupReloadWorld)
		{
			if (UWorld* RunningWorldPtr = RunningWorld.Get())
			{
				SortAndReloadModifiedPackages(FoundArrayDataAsPackages, RunningWorldPtr);
			}
		}
	}
	else
	{
		if (bDebugRuntimePackagesReloading)
			UE_LOG(LogUnrealGameLinkRuntime, Log, TEXT("Note: FUnrealGameLinkRuntimeModule::ReloadModifiedPackages =========>>>>> NO PACKAGES CAN BE RETRIVED FROM GIVEN DIRECTORIES <<<<<========="));
	}

	//If the user set that value, we then make sure to use it to force reload the world
	if (bReloadActiveMapOnContentUpdates)
	{
		bPostCleanupReloadWorld = true;
	}

	//clean up the most recent modified packages list, so we don't update every sync
	if (UUnrealGameLinkSettings* UnrealGameLinkProjectSettings = GetMutableDefault<UUnrealGameLinkSettings>())
	{
		if (bDebugRuntimePackagesReloading)
			UE_LOG(LogUnrealGameLinkRuntime, Log, TEXT("Note: FUnrealGameLinkRuntimeModule::ReloadModifiedPackages =========>>>>> POST RELOADING CLEANUP <<<<<========="));
		UnrealGameLinkProjectSettings->MostRecentModifiedContent.Empty();
		UnrealGameLinkProjectSettings->SaveConfig(CPF_Config, *UnrealGameLinkProjectSettings->GetDefaultConfigFilename()); //TODO::Test with UpdateDefaultConfigFile() instead, to only modify certain part
	}

	//if world need to be reloaded, then let's do it. Let's always assume we're in world 0
	if (bPostCleanupReloadWorld)
	{
		if (bDebugRuntimePackagesReloading)
			UE_LOG(LogUnrealGameLinkRuntime, Log, TEXT("Note: FUnrealGameLinkRuntimeModule::ReloadModifiedPackages =========>>>>> POST CLEANUP IS RELOADING WORLD [%s] NOW <<<<<========="), *RunningWorld.Get()->GetName());
		UGameplayStatics::OpenLevel(RunningWorld.Get(), *RunningWorld.Get()->GetMapName());
	}
}

/*
* Sore and reload all the modified packages that are passed to the function, and make sure to clean and reload them in the current/passed world
* 
* @param Packages		The list of packages to process
* @param World			The current running world
*/
void FUnrealGameLinkRuntimeModule::SortAndReloadModifiedPackages(TArray<UPackage*>& Packages, UWorld* World)
{
	//For the sake of info, this is very useful
	if (bDebugRuntimePackagesReloading)
	{
		UE_LOG(LogUnrealGameLinkRuntime, Log, TEXT("Note: FUnrealGameLinkRuntimeModule::SortAndReloadModifiedPackages =========>>>>> Will perform [%d] Packages per patch <<<<<========="), PackagesToReloadPerPatch);

		for (UPackage* package : Packages)
		{
			if (package->HasAnyPackageFlags(PKG_InMemoryOnly))
			{
				UE_LOG(LogUnrealGameLinkRuntime, Warning, TEXT("Note: FUnrealGameLinkRuntimeModule::SortAndReloadModifiedPackages =========>>>>> Package [%s] is flagged PKG_InMemoryOnly, this shall NOT be touched or force reloaded <<<<<========="), *package->GetName());
			}
			else
			{
				UE_LOG(LogUnrealGameLinkRuntime, Log, TEXT("Note: FUnrealGameLinkRuntimeModule::SortAndReloadModifiedPackages =========>>>>> Package [%s] is SAFE for RELOAD <<<<<========="), *package->GetName());
			}
		}
	}

	//sort packages for reload, so dependencies reloaded earlier than dependents
	SortPackagesForReload(Packages);

	//Flush rendering commands & update the primitive scene info.
	FGlobalComponentReregisterContext ComponentRegisterContext;

	TArray<FReloadPackageData> PackagesToReloadData;
	PackagesToReloadData.Reserve(Packages.Num());
	for (UPackage* package : Packages)
	{
		PackagesToReloadData.Emplace(package, LOAD_None);
	}

	TArray<UPackage*> AlreadyReloadedPackages;
	ReloadPackages(PackagesToReloadData, AlreadyReloadedPackages, PackagesToReloadPerPatch);

	//For the sake of info, this is very useful
	if (bDebugRuntimePackagesReloading)
	{
		TArray<UPackage*> FailedPackages;
		for (int32 i = 0; i < Packages.Num(); ++i)
		{
			UPackage* ExistingPackage = Packages[i];
			UPackage* ReloadedPackage = AlreadyReloadedPackages[i];

			if (ReloadedPackage)
			{
				//Well, it's reloaded, nothing need to be stored.
			}
			else
			{
				FailedPackages.Add(ExistingPackage);
			}
		}

		if (FailedPackages.Num() > 0)
		{
			for (UPackage* FailedPackage : FailedPackages)
				UE_LOG(LogUnrealGameLinkRuntime, Log, TEXT("Note: FUnrealGameLinkRuntimeModule::SortAndReloadModifiedPackages =========>>>>> FAILED for the Package [%s] <<<<<========="), *FailedPackage->GetFName().ToString());
		}
	}

	if (bDebugRuntimePackagesReloading)
		UE_LOG(LogUnrealGameLinkRuntime, Log, TEXT("Note: FUnrealGameLinkRuntimeModule::SortAndReloadModifiedPackages =========>>>>> Going to PINGPONG per level resources <<<<<========="));

	for (int32 i = 0; i < World->GetNumLevels(); ++i)
	{
		if (bDebugRuntimePackagesReloading)
			UE_LOG(LogUnrealGameLinkRuntime, Log, TEXT("Note: FUnrealGameLinkRuntimeModule::SortAndReloadModifiedPackages =========>>>>> World [%s] is PINGPONG-ING for level index [%i] which called [%s]  <<<<<========="), *World->GetName(), i, *World->GetLevel(i)->GetName());

		ULevel* level = World->GetLevel(i);
		level->ReleaseRenderingResources();
		level->InitializeRenderingResources();
	}
}

// #undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUnrealGameLinkRuntimeModule, UnrealGameLinkRuntime)