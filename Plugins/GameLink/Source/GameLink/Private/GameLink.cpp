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

#include "GameLink.h"
#include "GameLinkStyle.h"
#include "GameLinkCommands.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"
#include "ISettingsModule.h"
#include "GeneralProjectSettings.h"
#include "Misc/App.h"
#include "FileHelpers.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"

static const FName GameLinkTabName("GameLink");

#define LOCTEXT_NAMESPACE "FGameLinkModule"
DEFINE_LOG_CATEGORY_STATIC(LogGameLink, Log, Log);

void FGameLinkModule::StartupModule()
{
	FGameLinkStyle::Initialize();
	FGameLinkStyle::ReloadTextures();

	FGameLinkCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FGameLinkCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FGameLinkModule::GameLinkToolbarButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FGameLinkModule::RegisterMenus));

	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FGameLinkModule::RegisterProjectSettings);
	FEditorDelegates::OnShutdownPostPackagesSaved.AddRaw(this, &FGameLinkModule::OnShutdownPostPackagesSaved);

	FetchProjectName();
}

void FGameLinkModule::ShutdownModule()
{
	UnRegisterProjectSettings();

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FGameLinkStyle::Shutdown();

	FGameLinkCommands::Unregister();
}

void FGameLinkModule::GameLinkToolbarButtonClicked()
{
	if (IsTrackUsage)
	{
		TrackedUsageCount++;
		TrackedInfoToPush.Append(*FDateTime::Now().ToString());
		TrackedInfoToPush.Append("\n");
	}

	FetchUserCustomProjectSetting();

	CleanupCookingDirectory();

	bool bCookedSomething = CheckAndAutoCookIfNeeded();

	if (bCookedSomething)
	{
		if (bDebugEditorGeneralMessages)
			UE_LOG(LogGameLink, Log, TEXT("Note: \n\n<<<<<<<<<< FGameLinkModule::GameLinkToolbarButtonClicked(), User SUCCESS Find, Saving & Cooking some packages >>>>>>>>>>\n\n"));
	}
	else
	{
		FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("FGameLinkModule", "GameLinkToolbarButtonClicked", "Oh, crap, issue in saving or cooking!"));
	}

	if (IsTrackUsage)
	{
		TrackedInfoToPush.Append("============================================");
		TrackedInfoToPush.Append("\n");
	}

}

void FGameLinkModule::FetchProjectName()
{
	ProjectName = FApp::GetProjectName();
}

/*
* Look into the project settings, and make sure to read the required values from the ini file of UGL.
*/
void FGameLinkModule::FetchUserCustomProjectSetting()
{
	if (const UGameLinkSettings* GameLinkProjectSettings = GetDefault<UGameLinkSettings>())
	{
		bSaveBeforeCooking = GameLinkProjectSettings->bSaveBeforeCooking;
		bNotifyCookingResults = GameLinkProjectSettings->bNotifyCookingResults;
		bSaveConcurrent = GameLinkProjectSettings->bSaveConcurrent;
		bSaveAsync = GameLinkProjectSettings->bSaveAsync;
		bSaveUnversioned = GameLinkProjectSettings->bSaveUnversioned;
		TargetPlatforms = GameLinkProjectSettings->TargetPlatforms;
		bDebugEditorGeneralMessages = GameLinkProjectSettings->bDebugEditorGeneralMessages;
		bDebugEditorPackagesOperations = GameLinkProjectSettings->bDebugEditorPackagesOperations;
		bDebugEditorCooker = GameLinkProjectSettings->bDebugEditorCooker;
	}
}

/*
* Clean up the cooking directory. Entirely! For now there's no reason to keep anything, even the platforms that is not targeted in the current run.
*/
void FGameLinkModule::CleanupCookingDirectory()
{

	FString GameLinkCookDirectory = GetGameLinkParentCookingDirectory();

	if (FPaths::DirectoryExists(GameLinkCookDirectory))
	{
		IFileManager::Get().DeleteDirectory(*GameLinkCookDirectory, true, true);
	}
}

/*
* Check for modified files, and see if we need/can cook them. This is taking place at the order below.
*/
bool FGameLinkModule::CheckAndAutoCookIfNeeded()
{
	
	bool bSuccess = false;

	TArray<UPackage*> PackagesToSave;
	bSuccess  = GetAllModifiedPackages(true, true, PackagesToSave);

	TArray<UPackage*> PackagesThatSaved;
	bSuccess = FEditorFileUtils::SaveDirtyPackages(true, true, true, false, false, false);

	if (bSuccess)
	{
		if (bDebugEditorCooker)
			UE_LOG(LogGameLink, Log, TEXT("Note: FGameLinkModule::CheckAndAutoCookIfNeeded(), User SUCCESS saving packages, will proceed in cooking them shortly"));
	}
	else
	{
		FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("FGameLinkModule", "GameLinkToolbarButtonClickedExec", "Oh, crap, issue in saving (cancelled may be), we can't cook data at this state!"));
		return bSuccess;
	}

	if (PackagesToSave.Num() != 0)
	{
		for (UPackage* package : PackagesToSave)
		{
			if (!package->IsDirty())
				PackagesThatSaved.AddUnique(package);
		}
	}

	if (bDebugEditorCooker)
		UE_LOG(LogGameLink, Log, TEXT("Note: FGameLinkModule::CheckAndAutoCookIfNeeded(), Found total of [%i] modified packages. But as the user wanted to process only [%i] packages. Skipping [%i] packages."), PackagesToSave.Num(), PackagesThatSaved.Num(), PackagesToSave.Num() - PackagesThatSaved.Num());

	TArray<ITargetPlatform*> TargetPackagingPlatforms;
	bSuccess = GetAllTargetPackagingPlatforms(TargetPackagingPlatforms);
	if (bSuccess)
	{
		if (bDebugEditorCooker)
			UE_LOG(LogGameLink, Log, TEXT("Note: FGameLinkModule::CheckAndAutoCookIfNeeded().GetAllTargetPackagingPlatforms(), Successfully got [%d] Target platforms."), TargetPackagingPlatforms.Num());

	}
	else
	{
		FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("FGameLinkModule", "GameLinkToolbarButtonClickedExec", "FAILED to get any target platforms from the project setting! Make sure to put at least one"));
		return bSuccess;
	}

	bSuccess = CookModifiedPackages(PackagesThatSaved, TargetPackagingPlatforms);
	if (bSuccess)
	{
		if (bDebugEditorCooker)
			UE_LOG(LogGameLink, Log, TEXT("Note: FGameLinkModule::CheckAndAutoCookIfNeeded().CookModifiedPackages, Successfully cooked [%d] packages for [%d] platforms"), PackagesThatSaved.Num(), TargetPackagingPlatforms.Num());
	}
	else
	{
		FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("FGameLinkModule", "GameLinkToolbarButtonClickedExec", "FAILED to Cook one or more packages, Check log for detailed errors"));
		return bSuccess;
	}

	bSuccess = CopyModifiedPackages();
	if (bSuccess)
	{
		if (bDebugEditorCooker)
			UE_LOG(LogGameLink, Log, TEXT("Note: FGameLinkModule::CheckAndAutoCookIfNeeded().CopyModifiedPackages, Successfully Copied all modified [%d] packages for [%d] platforms"), PackagesThatSaved.Num(), TargetPackagingPlatforms.Num());
	}
	else
	{
		FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("FGameLinkModule", "GameLinkToolbarButtonClickedExec", "FAILED to Copy modified packages, Check log for detailed errors"));
		return bSuccess;
	}

	return true;
}

void FGameLinkModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FGameLinkCommands::Get().PluginAction, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FGameLinkCommands::Get().PluginAction));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

void FGameLinkModule::RegisterProjectSettings()
{
	if (ISettingsModule* settingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		settingsModule->RegisterSettings
		(
			"Project", "Plugins", "GameLink",
			LOCTEXT("GameLinkPluginName", "GameLink"),
			LOCTEXT("GameLinkPluginTooltip", "Global settings for GameLink"),
			GetMutableDefault<UGameLinkSettings>()
		);
	}
}

void FGameLinkModule::UnRegisterProjectSettings()
{
	if (ISettingsModule* settingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		if (bDebugEditorGeneralMessages)
			UE_LOG(LogGameLink, Log, TEXT("Note: FGameLinkModule::UnRegisterProjectSettings(), UNREGISTER PROJECT SETTINGS =============>>>>>>>>>> "));

		settingsModule->UnregisterSettings
		(
			"Project", "Plugins", "GameLink"
		);
	}
}

void FGameLinkModule::OnShutdownPostPackagesSaved()
{
	if (IsTrackUsage)
	{
		UE_LOG(LogGameLink, Log, TEXT("Note: FGameLinkModule::OnShutdownPostPackagesSaved(), GameLink shall be saving the tracking info now."));

		if (!TrackedInfoToPush.IsEmpty())
		{
			TrackedInfoDir = GetGameLinkLogingDirectory();

			if (IFileManager::Get().DirectoryExists(*TrackedInfoDir))
			{
				UE_LOG(LogGameLink, Log, TEXT("Note: FGameLinkModule::OnShutdownPostPackagesSaved(), Tracking DIR found."));
			}
			else
			{
				UE_LOG(LogGameLink, Log, TEXT("Note: FGameLinkModule::OnShutdownPostPackagesSaved(), no DIR for tracking found!!!! We defianlty creating one"));
				IFileManager::Get().MakeDirectory(*TrackedInfoDir);
			}

			FString fileName;
			FString fullFilePathName;
			fileName = *FDateTime::Now().ToString();
			fileName.Append(".log");
			fullFilePathName = FPaths::Combine(TrackedInfoDir, fileName);

			FArchive* writer = IFileManager::Get().CreateFileWriter(*fullFilePathName);
			FString fileContent = "============================================\n";
			fileContent.Append(FString::Printf(TEXT("GameLink used [%d] times during this session"), TrackedUsageCount));
			fileContent.Append("\n");
			fileContent.Append("============================================\n");
			fileContent.Append(TrackedInfoToPush);
			auto writerInfo = StringCast<ANSICHAR>(*fileContent, fileContent.Len());
			writer->Serialize((ANSICHAR*)writerInfo.Get(), writerInfo.Length() * sizeof(ANSICHAR));
			writer->Close();
		}

	}
}

/*
* Fill the OutPackages array with all the packages owners of the currently modified assets.
* 
* @param CheckMaps				Either consider all maps (*.umap) to be saved and cooked
* @param CheckAssets			Either consider all assets (*.uasset) to be saved and cooked
* @param OutPackages			Reference to the packages array that we fill with found results
*/
bool FGameLinkModule::GetAllModifiedPackages(const bool CheckMaps, const bool CheckAssets, TArray<UPackage*>& OutPackages)
{
	if (CheckMaps)
	{
		FEditorFileUtils::GetDirtyWorldPackages(OutPackages);
	}

	if (CheckAssets)
	{
		FEditorFileUtils::GetDirtyContentPackages(OutPackages);
	}

	if (OutPackages.Num() != 0)
	{
		if (bDebugEditorPackagesOperations)
			UE_LOG(LogGameLink, Log, TEXT("Note: FGameLinkModule::GetAllModifiedPackages(), found [%d] dirty packages that need to be saved."), OutPackages.Num());
		return true;
	}
	else
	{
		FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("FGameLinkModule", "GetAllModifiedPackagesExec", "NOTHIN FOUND!!!"));
		return false;
	}
}

/*
* Look thought the project settings of the GameLink, and grab all the platforms that need to be targeted.
* 
* @param OutPackagingPlatforms		reference to the packaging platforms array to fill and send back.
*/
bool FGameLinkModule::GetAllTargetPackagingPlatforms(TArray<ITargetPlatform*>& OutPackagingPlatforms)
{
	TArray<FString> UserSettingsTargetPackaingPlaftormNames;
	
	if (const UGameLinkSettings* GameLinkProjectSettings = GetDefault<UGameLinkSettings>())
	{
		for (const FGameLinkTargetPlatform platform : GameLinkProjectSettings->TargetPlatforms)
		{
			FString platformName = UEnum::GetValueAsString(platform.Platfrom);
			platformName.RemoveAt(0, 21);

			UserSettingsTargetPackaingPlaftormNames.AddUnique(platformName);
		}

		if (UserSettingsTargetPackaingPlaftormNames.Num() <= 0)
			return false;

		ITargetPlatformManagerModule& TargetPlatformManagerModuleRef = GetTargetPlatformManagerRef();
		const TArray<ITargetPlatform*>& EngineTargetPlatforms = TargetPlatformManagerModuleRef.GetTargetPlatforms();

		for (ITargetPlatform* EngineTarget : EngineTargetPlatforms)
		{
			if (UserSettingsTargetPackaingPlaftormNames.Contains(EngineTarget->PlatformName()))
				OutPackagingPlatforms.AddUnique(EngineTarget);
		}
		
	}
	else
	{
		return false;
	}

	return true;
}

/*
* Cook the entire PackagesList array for the given TargetPlaformsList. That could take quite sometime, it depends on the number of packages and the number of the target platforms.
* 
* @param PackagesList			The list of packages to go through and cook
* @param TargetPlatforms		The platforms we want to cook and update right now, usually single platform is recommended, but multiple are welcomed indeed
*/
bool FGameLinkModule::CookModifiedPackages(const TArray<UPackage*> PackagesList, const TArray<ITargetPlatform*> TargetPlatformsList)
{
	bool bSuccess = false;
	FString GameLinkCookDirectory = GetGameLinkParentCookingDirectory();

	if (UGameLinkSettings* GameLinkProjectSettings = GetMutableDefault<UGameLinkSettings>())
	{
		GameLinkProjectSettings->MostRecentModifiedContent.Empty();
		GameLinkProjectSettings->SaveConfig(CPF_Config, *GameLinkProjectSettings->GetDefaultConfigFilename());
	}

	for (ITargetPlatform* TargetPlatform : TargetPlatformsList)
	{
		if (bDebugEditorPackagesOperations)
			UE_LOG(LogGameLink, Log, TEXT("Note: FGameLinkModule::CookModifiedPackages(), Started cooking packages for the [%s] target plaftorm."), *TargetPlatform->PlatformName());

		for (UPackage* package : PackagesList)
		{
			FString CookingDirectory;
			GetPackagesCookingDirectory(package, GameLinkCookDirectory, TargetPlatform->PlatformName(), CookingDirectory);
			if (bDebugEditorPackagesOperations)
				UE_LOG(LogGameLink, Log, TEXT("Cooking Dir for Packag [%s] is :%s"), *package->GetName(), *CookingDirectory);

			if (CookingDirectory.IsEmpty())
			{
				if (bDebugEditorPackagesOperations)
					UE_LOG(LogGameLink, Error, TEXT("Faile to get or create Cooking Dir for the Packag [%s]"), *package->GetName());
				return false;
			}

			bSuccess = CookModifiedPackage(package, TargetPlatform, CookingDirectory);
		}
	}

	return true;
}

/*
* Cook the given Package for the specified given TargetPlatform and put the final cooked package in the CookedFileNamePath.
* 
* @param Package						The exact package that need to be cooked
* @param TargetPlatform					The platform we gonna cook for (usually win64, XbGDK or XbX. But others are okay)
* @param CookedFileNamePath				The name and location for the resulted cooked package
*/
bool FGameLinkModule::CookModifiedPackage(UPackage* Package, ITargetPlatform* TargetPlatform, const FString& CookedFileNamePath)
{
	bool bSuccess = false;

	uint32 SaveFlags = 
		(bSaveConcurrent ? SAVE_Concurrent : 0) |
		(bSaveAsync ? SAVE_Async : 0) |
		(bSaveUnversioned ? SAVE_Unversioned : 0) |
		SAVE_ComputeHash | 
		SAVE_KeepGUID;

	EObjectFlags TopLevelFlags = RF_Public;
	if (Cast<UWorld>(Package))
		TopLevelFlags = RF_NoFlags;

	if (!TargetPlatform->HasEditorOnlyData())
		Package->SetPackageFlags(PKG_FilterEditorOnly);
	else
		Package->ClearPackageFlags(PKG_FilterEditorOnly);

	if (FPaths::FileExists(CookedFileNamePath))
	{
		IFileManager::Get().Delete(*CookedFileNamePath);
	}

	Package->FullyLoad();
	TArray<UObject*> PackageContent;
	GetObjectsWithOuter(Package, PackageContent);
	for (const auto& item : PackageContent)
	{
		item->BeginCacheForCookedPlatformData(TargetPlatform);
	}

	GIsCookerLoadingPackage = true;
	FSavePackageResultStruct SaveResult = GEditor->Save
	(
		Package,
		nullptr,
		TopLevelFlags,
		*CookedFileNamePath,
		GError,
		nullptr,
		false,
		false,
		SaveFlags,
		TargetPlatform,
		FDateTime::MinValue(),
		false,
		nullptr,
		nullptr
	);
	GIsCookerLoadingPackage = false;

	bSuccess = SaveResult == ESavePackageResult::Success ? true : false;

	if (bNotifyCookingResults)
	{
		FText notificationMessage = FText::Format
		(
			LOCTEXT("FGameLinkModuleNotification", "{0} Cooking package [{1}] for platform [{2}]"),
			bSuccess ? FText::FromString(ANSI_TO_TCHAR("Success")) : FText::FromString(ANSI_TO_TCHAR("Failure")),
			FText::FromString(Package->GetName()),
			FText::FromString(TargetPlatform->PlatformName())
		);
		FNotificationInfo notificationInfo(notificationMessage);
		notificationInfo.ExpireDuration = 4.f;
		notificationInfo.FadeInDuration = 1.f;
		notificationInfo.FadeOutDuration = 1.f;
		notificationInfo.bFireAndForget = true;
		notificationInfo.bUseLargeFont = false;
		notificationInfo.bUseSuccessFailIcons = true;
		notificationInfo.HyperlinkText = FText::FromString(CookedFileNamePath);
		notificationInfo.Hyperlink = FSimpleDelegate::CreateLambda([](FString FilePath) { FPlatformProcess::ExploreFolder(*FilePath);}, CookedFileNamePath);
		FSlateNotificationManager::Get().AddNotification(notificationInfo)->SetCompletionState(bSuccess ? SNotificationItem::ECompletionState::CS_Success : SNotificationItem::ECompletionState::CS_Fail);
	}

	if (bSuccess)
	{
		if (bDebugEditorCooker)
			UE_LOG(LogGameLink, Log, TEXT("Note: FGameLinkModule::CookModifiedPackage, Success cooking the package [%s] for the target platform [%s]"), *Package->GetName(), *TargetPlatform->PlatformName());

		if (UGameLinkSettings* GameLinkProjectSettings = GetMutableDefault<UGameLinkSettings>())
		{
			GameLinkProjectSettings->MostRecentModifiedContent.AddUnique(CookedFileNamePath);
			GameLinkProjectSettings->SaveConfig(CPF_Config, *GameLinkProjectSettings->GetDefaultConfigFilename());

			if (IsTrackUsage)
			{
				TrackedInfoToPush.Append(CookedFileNamePath);
				TrackedInfoToPush.Append("\n");
			}
		}
	}

	if (!TargetPlatform->HasEditorOnlyData())
		Package->ClearPackageFlags(PKG_FilterEditorOnly);

	return true;
}

/*
* Copy the modified packages from the GameLink temp cooking directory, to their final residence place based on their platform values set in the project settings.
*/
bool FGameLinkModule::CopyModifiedPackages()
{
	bool bSuccess = false;

	if (const UGameLinkSettings* GameLinkProjectSettings = GetDefault<UGameLinkSettings>())
	{
		for (const FGameLinkTargetPlatform platform : GameLinkProjectSettings->TargetPlatforms)
		{
			FString platformName = UEnum::GetValueAsString(platform.Platfrom);
			platformName.RemoveAt(0, 21);

			FDirectoryPath runningGameRootDirectory = platform.StreamingBuildRootDirectory;

			if (IFileManager::Get().DirectoryExists(*runningGameRootDirectory.Path))
			{
				IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
				FString CopyFrom = FPaths::Combine(GetGameLinkParentCookingDirectory(), platformName, TEXT("Content"));
				FString CopyTo = FPaths::Combine(*runningGameRootDirectory.Path, ProjectName, TEXT("Content"));
			
				bSuccess = PlatformFile.CopyDirectoryTree(*CopyTo, *CopyFrom, true);

				FString CopyFileFrom = FPaths::Combine(*FPaths::ProjectConfigDir(), TEXT("DefaultGameLink.ini"));
				FString CopyFileTo = FPaths::Combine(*runningGameRootDirectory.Path, ProjectName, TEXT("Config"), TEXT("DefaultGameLink.ini"));
				IFileManager::Get().Copy(*CopyFileTo, *CopyFileFrom, true, true, true);
			}
			else
			{
				if (bDebugEditorPackagesOperations)
					UE_LOG(LogGameLink, Warning, TEXT("SKIPPED FGameLinkModule::CopyModifiedPackages() for the none-valid directory [%s]"), *runningGameRootDirectory.Path);
			}

		}
	}

	return true;
}

/*
* Get the cooking directory of a given package based on a given target platform & parent root cooking directory.
* 
* @param Package						The target package to get a cooking directory for
* @param GameLinkCookedDirectory			The main cooking directory of the plugin, usually shall be at "PROJECT/Save/GameLinkCooked"
* @param TargetPlatformName				The platform we cooking for, will be used to make a sub-folder of the cooking main directory
* @param OutPackageCookDirectory		The value to send back to the caller, this shall be a full path including the filename + format
*/
void FGameLinkModule::GetPackagesCookingDirectory(UPackage* Package, const FString& GameLinkCookedDirectory, const FString& TargetPlatformName, FString& OutPackageCookDirectory)
{
	if (bDebugEditorPackagesOperations)
		UE_LOG(LogGameLink, Log, TEXT("FGameLinkModule::GetPackagesCookingDirectory() for the package: [%s]"), *Package->GetName());

	FString PackageFullFileNamePath;

	bool FoundPackage = false;
	FoundPackage = FPackageName::DoesPackageExist(Package->GetName(), NULL, &PackageFullFileNamePath, false);

	int32 strFoundAt = PackageFullFileNamePath.Find("/Content", ESearchCase::IgnoreCase, ESearchDir::FromStart);
	check(strFoundAt != -1);
	FString FilePathRelativeToContent = PackageFullFileNamePath.RightChop(strFoundAt);

	OutPackageCookDirectory = FPaths::Combine(GameLinkCookedDirectory, TargetPlatformName, FilePathRelativeToContent);
	if (bDebugEditorPackagesOperations)
		UE_LOG(LogGameLink, Log, TEXT("FINAL COOKING PATH:%s"), *OutPackageCookDirectory);

}

/*
* Returns the main cooking directory for GameLink. Usually should be .../[PROJECT]/Saved/GameLinkCooked/...
*/
FString FGameLinkModule::GetGameLinkParentCookingDirectory()
{
	FString GameLinkCookDirectory = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("GameLinkCooked")));
	return GameLinkCookDirectory;
}

/*
* Returns the main logs directory for GameLink. Usually should be .../[PROJECT]/Saved/GameLinkLogs/...
*/
FString FGameLinkModule::GetGameLinkLogingDirectory()
{
	FString GameLinkLogsDirectory = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("GameLinkLogs")));
	return GameLinkLogsDirectory;
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FGameLinkModule, GameLink)