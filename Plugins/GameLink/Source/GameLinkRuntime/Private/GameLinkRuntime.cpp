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

#include "GameLinkRuntime.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameEngine.h"
#include "Misc/ConfigCacheIni.h"
#include "ComponentReregisterContext.h"


DEFINE_LOG_CATEGORY_STATIC(LogGameLinkRuntime, Log, Log);

void FGameLinkRuntimeModule::StartupModule()
{

	ReadAllSettingFromConfig();
	ResetMostRecentInConfig();

	if (bDebugEditorGeneralMessages)
		UE_LOG(LogGameLinkRuntime, Log, TEXT("Note: \n*\n*\n*\n*\n*\nFGameLinkRuntimeModule::StartupModule, COMPLETED with bEnabledAtRuntime is set to [%s]\n*\n*\n*\n*\n*"), bEnabledAtRuntime ? *FString("True") : *FString("False"));

#if WITH_EDITOR
	if (!bEnabledAtEditorRuntime)
	{
		bEnabledAtRuntime = false;
		if (bDebugEditorGeneralMessages)
			UE_LOG(LogGameLinkRuntime, Log, TEXT("Note: \n*\n*\n*\n*\n*\nFGameLinkRuntimeModule::StartupModule, DISABLED for editor, with bEnabledAtRuntime is set to [%s]\n*\n*\n*\n*\n*"), bEnabledAtRuntime ? *FString("True") : *FString("False"));
	}
#else
	
#endif

#if WITH_ENGINE
#endif

	FWorldDelegates::OnWorldTickStart.AddRaw(this, &FGameLinkRuntimeModule::OnWorldTickStart);	
}

/*
* This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading, we call this function before unloading the module.
*/
void FGameLinkRuntimeModule::ShutdownModule()
{
	FWorldDelegates::OnWorldTickStart.RemoveAll(this);
}

/*
* Delegate FWorldDelegates::OnWorldTickStart()
* 
* @param World			The world that is ticking (usually the current loaded one)
* @param TickType		What is ticking. The level time, the viewport, both or nothing.
* @param DeltaTime		The standard delta time.
*/
void FGameLinkRuntimeModule::OnWorldTickStart(UWorld* World, ELevelTick TickType, float DeltaTime)
{
	if (!bEnabledAtRuntime)
		return;

	checkTimer += DeltaTime;
	if (checkTimer > EditorSyncInterval)
	{
		if (bDebugRuntimeTicks)
			UE_LOG(LogGameLinkRuntime, Log, TEXT("Note: \n*\n*\n*\n*\n*\n FGameLinkRuntimeModule::OnWorldTickStart =========>>>>> The world [%s] is TICKING and printing this every [%f] seconds \n*\n*\n*\n*\n*"), *World->GetName(), EditorSyncInterval);
		checkTimer = 0.f;

		if (CheckForModifiedPackages())
			ReloadModifiedPackages();
	}
}

void FGameLinkRuntimeModule::ReadAllSettingFromConfig()
{
	FConfigFile configFile;
	GConfig->LoadLocalIniFile(configFile, TEXT("GameLink"), true, ANSI_TO_TCHAR(FPlatformProperties::IniPlatformName()), true); //DefaultGameLink.ini

	bool foundEnabledAtRuntime;
	bool foundEnabledAtEditorRuntime;
	float foundEditorSyncInterval;
	bool fountReloadActiveMapOnContentUpdates;
	int32 foundPackagesToReloadPerPatch;
	bool foundDebugEditorGeneralMessages;
	bool foundDebugRuntimeTicks;
	bool foundDebugRuntimePackagesReloading;

	configFile.GetBool(TEXT("/Script/GameLinkRuntime.GameLinkSettings"), TEXT("bEnabledAtRuntime"), foundEnabledAtRuntime);
	configFile.GetBool(TEXT("/Script/GameLinkRuntime.GameLinkSettings"), TEXT("bEnabledAtEditorRuntime"), foundEnabledAtEditorRuntime);
	configFile.GetFloat(TEXT("/Script/GameLinkRuntime.GameLinkSettings"), TEXT("EditorSyncInterval"), foundEditorSyncInterval);
	configFile.GetBool(TEXT("/Script/GameLinkRuntime.GameLinkSettings"), TEXT("bReloadActiveMapOnContentUpdates"), fountReloadActiveMapOnContentUpdates);
	configFile.GetInt(TEXT("/Script/GameLinkRuntime.GameLinkSettings"), TEXT("PackagesToReloadPerPatch"), foundPackagesToReloadPerPatch);
	configFile.GetBool(TEXT("/Script/GameLinkRuntime.GameLinkSettings"), TEXT("bDebugEditorGeneralMessages"), foundDebugEditorGeneralMessages);
	configFile.GetBool(TEXT("/Script/GameLinkRuntime.GameLinkSettings"), TEXT("bDebugRuntimeTicks"), foundDebugRuntimeTicks);
	configFile.GetBool(TEXT("/Script/GameLinkRuntime.GameLinkSettings"), TEXT("bDebugRuntimePackagesReloading"), foundDebugRuntimePackagesReloading);
	
	bEnabledAtRuntime = true;
	bEnabledAtEditorRuntime = foundEnabledAtEditorRuntime;
	EditorSyncInterval = foundEditorSyncInterval;
	bReloadActiveMapOnContentUpdates = fountReloadActiveMapOnContentUpdates;
	PackagesToReloadPerPatch = foundPackagesToReloadPerPatch;
	bDebugEditorGeneralMessages = foundDebugEditorGeneralMessages;
	bDebugRuntimeTicks = foundDebugRuntimeTicks;
	bDebugRuntimePackagesReloading = foundDebugRuntimePackagesReloading;
}

void FGameLinkRuntimeModule::ResetMostRecentInConfig()
{
	if (bDebugRuntimePackagesReloading)
		UE_LOG(LogGameLinkRuntime, Log, TEXT("Note: FGameLinkRuntimeModule::ResetMostRecentInConfig =========>>>>> POST RELOADING/STARTINGUP CLEANUP <<<<<========="));

	if (UGameLinkSettings* GameLinkProjectSettings = GetMutableDefault<UGameLinkSettings>())
	{
		GameLinkProjectSettings->MostRecentModifiedContent.Empty();
		GameLinkProjectSettings->SaveConfig(CPF_Config, *GameLinkProjectSettings->GetDefaultConfigFilename());
	}
}

/*
* Checks for any updates being sent from Unreal Editor to the current running game. Usually this will be in the form of modified packages list. This will be ticking based on the configs of GameLink that is set in the project settings.
*/
bool FGameLinkRuntimeModule::CheckForModifiedPackages()
{
	FConfigFile configFile;
	TArray<FString> foundArrayData;
	GConfig->LoadLocalIniFile(configFile, TEXT("GameLink"), true, ANSI_TO_TCHAR(FPlatformProperties::IniPlatformName()), true); //DefaultGameLink.ini
	configFile.GetArray(TEXT("/Script/GameLinkRuntime.GameLinkSettings"), TEXT("MostRecentModifiedContent"), foundArrayData);
	return foundArrayData.Num() > 0 ? true : false;

}

/*
* Execute a reload for the packages that been modified in the unreal Editor and been sent to the current running game instance.
*/
void FGameLinkRuntimeModule::ReloadModifiedPackages()
{
	bool bPostCleanupReloadWorld = false;

	FConfigFile ConfigFile;
	TArray<FString> FoundArrayData;
	TArray<UPackage*> FoundArrayDataAsPackages;
	GConfig->LoadLocalIniFile(ConfigFile, TEXT("GameLink"), true, ANSI_TO_TCHAR(FPlatformProperties::IniPlatformName()), true); //DefaultGameLink.ini
	ConfigFile.GetArray(TEXT("/Script/GameLinkRuntime.GameLinkSettings"), TEXT("MostRecentModifiedContent"), FoundArrayData);

	int32 capacity = FoundArrayData.Num();

	if (bDebugRuntimePackagesReloading)
		UE_LOG(LogGameLinkRuntime, Log, TEXT("Note: \n*\n*\n FGameLinkRuntimeModule::ReloadModifiedPackages =========>>>>> STARTED [%d] <<<<<========= \n*\n*"), capacity);

	for (FString FoundItem : FoundArrayData)
	{
		if (FoundItem.Contains(FPlatformProperties::PlatformName(), ESearchCase::CaseSensitive))
		{
			int32 strFoundAt = FoundItem.Find("/Content", ESearchCase::IgnoreCase, ESearchDir::FromStart);
			check(strFoundAt != -1);
			FString packageRelativePath = FoundItem.RightChop(strFoundAt); //"/Content/..../..../..../...."
			packageRelativePath = packageRelativePath.Replace(*FString("Content"), *FString("Game"));
			if (bDebugRuntimePackagesReloading)
				UE_LOG(LogGameLinkRuntime, Log, TEXT("Note: FGameLinkRuntimeModule::ReloadModifiedPackages =========>>>>> TargetFile [%s] <<<<<========="), *packageRelativePath);


			UPackage* package = LoadPackage(nullptr, *packageRelativePath, 0);
			FoundArrayDataAsPackages.AddUnique(package);

		}
	}

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
			UE_LOG(LogGameLinkRuntime, Warning, TEXT("Note: FGameLinkRuntimeModule::ReloadModifiedPackages =========>>>>> World Context 0 World map name is [%s] and name [%s] <<<<<========="), *GEngine->GetWorldContexts()[0].World()->GetMapName(), *GEngine->GetWorldContexts()[0].World()->GetName()); //context 0 is the same as the main world. In running build there is only one context world.

		for (UPackage* package : FoundArrayDataAsPackages)
		{
			if (bDebugRuntimePackagesReloading)
				UE_LOG(LogGameLinkRuntime, Warning, TEXT("Note: FGameLinkRuntimeModule::ReloadModifiedPackages =========>>>>> Will compare Package [%s] against Package [%s] <<<<<========="), *package->GetName(), *RunningWorld.Get()->GetOutermost()->GetName());
			
			if (package->HasAnyPackageFlags(PKG_ContainsMap | PKG_ContainsMapData))
			{
				if (bDebugRuntimePackagesReloading)
					UE_LOG(LogGameLinkRuntime, Warning, TEXT("Note: FGameLinkRuntimeModule::ReloadModifiedPackages =========>>>>> Package [%s] is flagged PKG_ContainsMap or PKG_ContainsMapData <<<<<========="), *package->GetName());
				
				bPostCleanupReloadWorld = true;
				break;
			}
			else
			{
				//do nothing
			}
		}

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
			UE_LOG(LogGameLinkRuntime, Log, TEXT("Note: FGameLinkRuntimeModule::ReloadModifiedPackages =========>>>>> NO PACKAGES CAN BE RETRIVED FROM GIVEN DIRECTORIES <<<<<========="));
	}

	if (bReloadActiveMapOnContentUpdates)
	{
		bPostCleanupReloadWorld = true;
	}

	ResetMostRecentInConfig();

	if (bPostCleanupReloadWorld)
	{
		if (bDebugRuntimePackagesReloading)
			UE_LOG(LogGameLinkRuntime, Log, TEXT("Note: FGameLinkRuntimeModule::ReloadModifiedPackages =========>>>>> POST CLEANUP IS RELOADING WORLD [%s] NOW <<<<<========="), *RunningWorld.Get()->GetName());
		UGameplayStatics::OpenLevel(RunningWorld.Get(), *RunningWorld.Get()->GetMapName());
	}
}

/*
* Sore and reload all the modified packages that are passed to the function, and make sure to clean and reload them in the current/passed world
* 
* @param Packages		The list of packages to process
* @param World			The current running world
*/
void FGameLinkRuntimeModule::SortAndReloadModifiedPackages(TArray<UPackage*>& Packages, UWorld* World)
{
	//For the sake of info, this is very useful
	if (bDebugRuntimePackagesReloading)
	{
		UE_LOG(LogGameLinkRuntime, Log, TEXT("Note: FGameLinkRuntimeModule::SortAndReloadModifiedPackages =========>>>>> Will perform [%d] Packages per patch <<<<<========="), PackagesToReloadPerPatch);

		for (UPackage* package : Packages)
		{
			if (package->HasAnyPackageFlags(PKG_InMemoryOnly))
			{
				UE_LOG(LogGameLinkRuntime, Warning, TEXT("Note: FGameLinkRuntimeModule::SortAndReloadModifiedPackages =========>>>>> Package [%s] is flagged PKG_InMemoryOnly, this shall NOT be touched or force reloaded <<<<<========="), *package->GetName());
			}
			else
			{
				UE_LOG(LogGameLinkRuntime, Log, TEXT("Note: FGameLinkRuntimeModule::SortAndReloadModifiedPackages =========>>>>> Package [%s] is SAFE for RELOAD <<<<<========="), *package->GetName());
			}
		}
	}

	SortPackagesForReload(Packages);

	FGlobalComponentReregisterContext ComponentRegisterContext;

	TArray<FReloadPackageData> PackagesToReloadData;
	PackagesToReloadData.Reserve(Packages.Num());
	for (UPackage* package : Packages)
	{
		PackagesToReloadData.Emplace(package, LOAD_None);
	}

	TArray<UPackage*> AlreadyReloadedPackages;
	ReloadPackages(PackagesToReloadData, AlreadyReloadedPackages, PackagesToReloadPerPatch);

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
				UE_LOG(LogGameLinkRuntime, Log, TEXT("Note: FGameLinkRuntimeModule::SortAndReloadModifiedPackages =========>>>>> FAILED for the Package [%s] <<<<<========="), *FailedPackage->GetFName().ToString());
		}
	}

	if (bDebugRuntimePackagesReloading)
		UE_LOG(LogGameLinkRuntime, Log, TEXT("Note: FGameLinkRuntimeModule::SortAndReloadModifiedPackages =========>>>>> Going to PINGPONG per level resources <<<<<========="));

	for (int32 i = 0; i < World->GetNumLevels(); ++i)
	{
		if (bDebugRuntimePackagesReloading)
			UE_LOG(LogGameLinkRuntime, Log, TEXT("Note: FGameLinkRuntimeModule::SortAndReloadModifiedPackages =========>>>>> World [%s] is PINGPONG-ING for level index [%i] which called [%s]  <<<<<========="), *World->GetName(), i, *World->GetLevel(i)->GetName());

		ULevel* level = World->GetLevel(i);
		level->ReleaseRenderingResources();
		level->InitializeRenderingResources();
	}
}
	
IMPLEMENT_MODULE(FGameLinkRuntimeModule, GameLinkRuntime)