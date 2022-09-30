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

#include "UnrealGameLink.h"
#include "UnrealGameLinkStyle.h"
#include "UnrealGameLinkCommands.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"

/* Includes for Project Settings */
#include "ISettingsModule.h"
#include "GeneralProjectSettings.h"
#include "Misc/App.h"

#include "FileHelpers.h"

/* Includes for slate notifications */
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"

/* Includes for the track*/
#include "ISourceControlModule.h"
#include "SourceControl/Public/SourceControlHelpers.h"

/* for the FSavePackageArgs*/
#include "UObject/SavePackage.h"
#include "PackageHelperFunctions.h"
#include "CookOnTheSide/CookOnTheFlyServer.h"

/*
#include "UnrealEd/Private/Cooker/LooseCookedPackageWriter.h"
#include "UnrealEd/Private/Cooker/CookTypes.h"
#include "UnrealEd/Private/Cooker/CookedSavePackageValidator.h"
*/

static const FName UnrealGameLinkTabName("UnrealGameLink");

#define LOCTEXT_NAMESPACE "FUnrealGameLinkModule"
DEFINE_LOG_CATEGORY_STATIC(LogUnrealGameLink, Log, Log);

void FUnrealGameLinkModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FUnrealGameLinkStyle::Initialize();
	FUnrealGameLinkStyle::ReloadTextures();

	FUnrealGameLinkCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FUnrealGameLinkCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FUnrealGameLinkModule::UnrealGameLinkToolbarButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FUnrealGameLinkModule::RegisterMenus));

	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FUnrealGameLinkModule::RegisterProjectSettings);
	FEditorDelegates::OnShutdownPostPackagesSaved.AddRaw(this, &FUnrealGameLinkModule::OnShutdownPostPackagesSaved);

	//we need to get the project name one time only, as it is not possible to be changed while the editor is running
	FetchProjectName();
}

void FUnrealGameLinkModule::ShutdownModule()
{
	UnRegisterProjectSettings();

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FUnrealGameLinkStyle::Shutdown();

	FUnrealGameLinkCommands::Unregister();
}

void FUnrealGameLinkModule::UnrealGameLinkToolbarButtonClicked()
{
	// Put your "OnButtonClicked" stuff here

	//store track info
	if (IsTrackUsage)
	{
		TrackedUsageCount++;
		TrackedInfoToPush.Append(*FDateTime::Now().ToString());
		TrackedInfoToPush.Append("\n");
	}

	//Always make sure to re-read the project settings, just in case the user did change some settings between different blinks.
	FetchUserCustomProjectSetting();

	//Clean up the cooking directory before we do anything
	CleanupCookingDirectory();

	bool bCookedSomething = CheckAndAutoCookIfNeeded();

	if (bCookedSomething)
	{
		if (bDebugEditorGeneralMessages)
			UE_LOG(LogUnrealGameLink, Log, TEXT("Note: \n\n<<<<<<<<<< FUnrealGameLinkModule::UnrealGameLinkToolbarButtonClicked(), User SUCCESS Find, Saving & Cooking some packages >>>>>>>>>>\n\n"));
	}
	else
	{
		FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("FUnrealGameLinkModule", "UnrealGameLinkToolbarButtonClicked", "Oh, crap, issue in saving or cooking!"));
	}

	//store track info
	if (IsTrackUsage)
	{
		TrackedInfoToPush.Append("============================================");
		TrackedInfoToPush.Append("\n");
	}

}

void FUnrealGameLinkModule::FetchProjectName()
{
	//this way will get the project name from the project's default settings, not the *.uproject file name
	/*
	const UGeneralProjectSettings& ProjectSettings = *GetDefault<UGeneralProjectSettings>();
	ProjectName = ProjectSettings.ProjectName;
	*/

	//get the *.uproject name
	ProjectName = FApp::GetProjectName();
}

/*
* Look into the project settings, and make sure to read the required values from the ini file of UGL.
*/
void FUnrealGameLinkModule::FetchUserCustomProjectSetting()
{
	if (const UUnrealGameLinkSettings* UnrealGameLinkProjectSettings = GetDefault<UUnrealGameLinkSettings>())
	{
		bSaveBeforeCooking = UnrealGameLinkProjectSettings->bSaveBeforeCooking;
		bNotifyCookingResults = UnrealGameLinkProjectSettings->bNotifyCookingResults;
		bSaveConcurrent = UnrealGameLinkProjectSettings->bSaveConcurrent;
		bSaveAsync = UnrealGameLinkProjectSettings->bSaveAsync;
		bSaveUnversioned = UnrealGameLinkProjectSettings->bSaveUnversioned;
		TargetPlatforms = UnrealGameLinkProjectSettings->TargetPlatforms;
		bDebugEditorGeneralMessages = UnrealGameLinkProjectSettings->bDebugEditorGeneralMessages;
		bDebugEditorPackagesOperations = UnrealGameLinkProjectSettings->bDebugEditorPackagesOperations;
		bDebugEditorCooker = UnrealGameLinkProjectSettings->bDebugEditorCooker;
	}
}

/*
* Clean up the cooking directory. Entirely! For now there's no reason to keep anything, even the platforms that is not targeted in the current run.
*/
void FUnrealGameLinkModule::CleanupCookingDirectory()
{

	FString UnrealGameLinkCookDirectory = GetUnrealGameLinkParentCookingDirectory();

	if (FPaths::DirectoryExists(UnrealGameLinkCookDirectory))
	{
		IFileManager::Get().DeleteDirectory(*UnrealGameLinkCookDirectory, true, true);
	}
}

/*
* Check for modified files, and see if we need/can cook them. This is taking place at the order below.
*/
bool FUnrealGameLinkModule::CheckAndAutoCookIfNeeded()
{
	
	bool bSuccess = false;

	/*
	1 - Get all modified assets [done]
	2 - Prompt to save assets [done]
		2 - Skip what the user would like to skip, and re-arrange the list [done]
	3 - Get targeted platforms to cook to (otherwise we don't need to cook) [done]
	4 - Cook those assets based on the project settings [done]
		4 - Notify the user same way targeting animation is doing [done]
	5 - Copy the entire folder over to the build, regardless it is deployed or streaming [done]
	6 - Refresh the running game runtime [UnrealGameLinkRuntimeModule]
	*/

	//[1]
	TArray<UPackage*> PackagesToSave;
	bSuccess  = GetAllModifiedPackages(true, true, PackagesToSave);

	//[2]
	TArray<UPackage*> PackagesThatSaved;
	bSuccess = FEditorFileUtils::SaveDirtyPackages(true, true, true, false, false, false);

	if (bSuccess)
	{
		if (bDebugEditorCooker)
			UE_LOG(LogUnrealGameLink, Log, TEXT("Note: FUnrealGameLinkModule::CheckAndAutoCookIfNeeded(), User SUCCESS saving packages, will proceed in cooking them shortly"));
	}
	else
	{
		FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("FUnrealGameLinkModule", "UnrealGameLinkToolbarButtonClickedExec", "Oh, crap, issue in saving (cancelled may be), we can't cook data at this state!"));
		return bSuccess;
	}

	//let's fill the final list that is of the user's choice
	if (PackagesToSave.Num() != 0)
	{
		for (UPackage* package : PackagesToSave)
		{
			//it's saved already
			if (!package->IsDirty())
				PackagesThatSaved.AddUnique(package);
		}
	}

	if (bDebugEditorCooker)
		UE_LOG(LogUnrealGameLink, Log, TEXT("Note: FUnrealGameLinkModule::CheckAndAutoCookIfNeeded(), Found total of [%i] modified packages. But as the user wanted to process only [%i] packages. Skipping [%i] packages."), PackagesToSave.Num(), PackagesThatSaved.Num(), PackagesToSave.Num() - PackagesThatSaved.Num());

	//[3]
	TArray<ITargetPlatform*> TargetPackagingPlatforms;
	bSuccess = GetAllTargetPackagingPlatforms(TargetPackagingPlatforms);
	if (bSuccess)
	{
		if (bDebugEditorCooker)
			UE_LOG(LogUnrealGameLink, Log, TEXT("Note: FUnrealGameLinkModule::CheckAndAutoCookIfNeeded().GetAllTargetPackagingPlatforms(), Successfully got [%d] Target platforms."), TargetPackagingPlatforms.Num());

	}
	else
	{
		FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("FUnrealGameLinkModule", "UnrealGameLinkToolbarButtonClickedExec", "FAILED to get any target platforms from the project setting! Make sure to put at least one"));
		return bSuccess;
	}

	//[4]
	bSuccess = CookModifiedPackages(PackagesThatSaved, TargetPackagingPlatforms);
	if (bSuccess)
	{
		if (bDebugEditorCooker)
			UE_LOG(LogUnrealGameLink, Log, TEXT("Note: FUnrealGameLinkModule::CheckAndAutoCookIfNeeded().CookModifiedPackages, Successfully cooked [%d] packages for [%d] platforms"), PackagesThatSaved.Num(), TargetPackagingPlatforms.Num());
	}
	else
	{
		FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("FUnrealGameLinkModule", "UnrealGameLinkToolbarButtonClickedExec", "FAILED to Cook one or more packages, Check log for detailed errors"));
		return bSuccess;
	}

	//[5]
	bSuccess = CopyModifiedPackages();
	if (bSuccess)
	{
		if (bDebugEditorCooker)
			UE_LOG(LogUnrealGameLink, Log, TEXT("Note: FUnrealGameLinkModule::CheckAndAutoCookIfNeeded().CopyModifiedPackages, Successfully Copied all modified [%d] packages for [%d] platforms"), PackagesThatSaved.Num(), TargetPackagingPlatforms.Num());
	}
	else
	{
		FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("FUnrealGameLinkModule", "UnrealGameLinkToolbarButtonClickedExec", "FAILED to Copy modified packages, Check log for detailed errors"));
		return bSuccess;
	}

	return true;
}

void FUnrealGameLinkModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FUnrealGameLinkCommands::Get().PluginAction, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FUnrealGameLinkCommands::Get().PluginAction));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

void FUnrealGameLinkModule::RegisterProjectSettings()
{
	if (ISettingsModule* settingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		settingsModule->RegisterSettings
		(
			"Project", "Plugins", "UnrealGameLink",
			LOCTEXT("UnrealGameLinkPluginName", "UnrealGameLink"),
			LOCTEXT("UnrealGameLinkPluginTooltip", "Global settings for UnrealGameLink"),
			GetMutableDefault<UUnrealGameLinkSettings>()
		);
	}
}

void FUnrealGameLinkModule::UnRegisterProjectSettings()
{
	if (ISettingsModule* settingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		if (bDebugEditorGeneralMessages)
			UE_LOG(LogUnrealGameLink, Log, TEXT("Note: FUnrealGameLinkModule::UnRegisterProjectSettings(), UNREGISTER PROJECT SETTINGS =============>>>>>>>>>> "));

		settingsModule->UnregisterSettings
		(
			"Project", "Plugins", "UnrealGameLink"
		);
	}
}

void FUnrealGameLinkModule::OnShutdownPostPackagesSaved()
{
	if (IsTrackUsage)
	{
		UE_LOG(LogUnrealGameLink, Log, TEXT("Note: FUnrealGameLinkModule::OnShutdownPostPackagesSaved(), UnrealGameLink shall be saving the tracking info now."));

		if (!TrackedInfoToPush.IsEmpty())
		{
			//get or make dir
			if (IFileManager::Get().DirectoryExists(*TrackedInfoDir))
			{
				UE_LOG(LogUnrealGameLink, Log, TEXT("Note: FUnrealGameLinkModule::OnShutdownPostPackagesSaved(), Tracking DIR found."));
			}
			else
			{
				UE_LOG(LogUnrealGameLink, Log, TEXT("Note: FUnrealGameLinkModule::OnShutdownPostPackagesSaved(), no DIR for tracking found!!!! We defianlty creating one"));
				IFileManager::Get().MakeDirectory(*TrackedInfoDir);
			}
		

			//which name will be used
			if (ISourceControlModule::Get().IsEnabled())
			{
				UE_LOG(LogUnrealGameLink, Log, TEXT("Note: FUnrealGameLinkModule::OnShutdownPostPackagesSaved(), Src Ctrl is in use...Good!"));
				
				FString sourceControlSettingsFile = SourceControlHelpers::GetSettingsIni();
				FString username;
				GConfig->GetString(TEXT("PerforceSourceControl.PerforceSourceControlSettings"), TEXT("UserName"), username, sourceControlSettingsFile);

				UE_LOG(LogUnrealGameLink, Log, TEXT("Note: FUnrealGameLinkModule::OnShutdownPostPackagesSaved(), Current P4 user is [%s]"), *username);

				//If there isn't a user dir, let's make one!
				FString currentUserDirectory = FPaths::Combine(TrackedInfoDir, username);
				if (IFileManager::Get().DirectoryExists(*currentUserDirectory))
				{
					//do nothing
				}
				else
				{
					IFileManager::Get().MakeDirectory(*currentUserDirectory);
				}

				//now let's make a file & fill it with all the info
				FString fileName;
				FString fullFilePathName;
				fileName = username;
				fileName.Append("-");
				fileName.Append(FDateTime::Now().ToString());
				fileName.Append(".ini");
				fullFilePathName = FPaths::Combine(currentUserDirectory, fileName);

				FArchive* writer = IFileManager::Get().CreateFileWriter(*fullFilePathName);
				FString fileContent = "============================================\n";
				fileContent.Append(FString::Printf(TEXT("UnrealGameLink used [%d] times during this session"), TrackedUsageCount));
				fileContent.Append("\n");
				fileContent.Append("============================================\n");
				fileContent.Append(TrackedInfoToPush);
				auto writerInfo = StringCast<ANSICHAR>(*fileContent, fileContent.Len());
				writer->Serialize((ANSICHAR*)writerInfo.Get(), writerInfo.Length() * sizeof(ANSICHAR));
				writer->Close();
		
			}
			else
			{
				UE_LOG(LogUnrealGameLink, Log, TEXT("Note: FUnrealGameLinkModule::OnShutdownPostPackagesSaved(), No Src Ctrl in use, we can't get a proper user name to count on, we might need to handle this by PC name later"));
			}

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
bool FUnrealGameLinkModule::GetAllModifiedPackages(const bool CheckMaps, const bool CheckAssets, TArray<UPackage*>& OutPackages)
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
			UE_LOG(LogUnrealGameLink, Log, TEXT("Note: FUnrealGameLinkModule::GetAllModifiedPackages(), found [%d] dirty packages that need to be saved."), OutPackages.Num());
		return true;
	}
	else
	{
		FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("FUnrealGameLinkModule", "GetAllModifiedPackagesExec", "NOTHIN FOUND!!!"));
		return false;
	}
}

/*
* Look thought the project settings of the UnrealGameLink, and grab all the platforms that need to be targeted.
* 
* @param OutPackagingPlatforms		reference to the packaging platforms array to fill and send back.
*/
bool FUnrealGameLinkModule::GetAllTargetPackagingPlatforms(TArray<ITargetPlatform*>& OutPackagingPlatforms)
{
	//The values user set in the project settings, we just grab them as names/string
	TArray<FString> UserSettingsTargetPackaingPlaftormNames;
	
	//get platforms list from the user setting of the project
	if (const UUnrealGameLinkSettings* UnrealGameLinkProjectSettings = GetDefault<UUnrealGameLinkSettings>())
	{
		for (const FUnrealGameLinkTargetPlatform platform : UnrealGameLinkProjectSettings->TargetPlatforms)
		{
			FString platformName = UEnum::GetValueAsString(platform.Platfrom);
			platformName.RemoveAt(0, 21);

			UserSettingsTargetPackaingPlaftormNames.AddUnique(platformName);
		}

		//User didn't set anything, the arry in the project settings is empty
		if (UserSettingsTargetPackaingPlaftormNames.Num() <= 0)
			return false;

		ITargetPlatformManagerModule& TargetPlatformManagerModuleRef = GetTargetPlatformManagerRef();
		const TArray<ITargetPlatform*>& EngineTargetPlatforms = TargetPlatformManagerModuleRef.GetTargetPlatforms();

		for (ITargetPlatform* EngineTarget : EngineTargetPlatforms)
		{
			if (bDebugEditorPackagesOperations)
				UE_LOG(LogUnrealGameLink, Log, TEXT("Note: FUnrealGameLinkModule::GetAllTargetPackagingPlatforms, ITargetPlatformManagerModule supports platform [%s]"), *EngineTarget->PlatformName());

			if (UserSettingsTargetPackaingPlaftormNames.Contains(EngineTarget->PlatformName()))
				OutPackagingPlatforms.AddUnique(EngineTarget);
		}
		
	}
	else
	{
		//error in settings! can't find project setting!!!
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
bool FUnrealGameLinkModule::CookModifiedPackages(const TArray<UPackage*> PackagesList, const TArray<ITargetPlatform*> TargetPlatformsList)
{
	bool bSuccess = false;
	FString UnrealGameLinkCookDirectory = GetUnrealGameLinkParentCookingDirectory();

	//Let's clean up the temp list before we do anything
	if (UUnrealGameLinkSettings* UnrealGameLinkProjectSettings = GetMutableDefault<UUnrealGameLinkSettings>())
	{
		UnrealGameLinkProjectSettings->MostRecentModifiedContent.Empty();
		UnrealGameLinkProjectSettings->SaveConfig(CPF_Config, *UnrealGameLinkProjectSettings->GetDefaultConfigFilename());
	}

	//get platforms list from the user setting of the project
	for (ITargetPlatform* TargetPlatform : TargetPlatformsList)
	{
		if (bDebugEditorPackagesOperations)
			UE_LOG(LogUnrealGameLink, Log, TEXT("Note: FUnrealGameLinkModule::CookModifiedPackages(), Started cooking packages for the [%s] target plaftorm."), *TargetPlatform->PlatformName());

		for (UPackage* package : PackagesList)
		{
			//[1] Get the directory for cooking
			FString CookingDirectory;
			GetPackagesCookingDirectory(package, UnrealGameLinkCookDirectory, TargetPlatform->PlatformName(), CookingDirectory);
			if (bDebugEditorPackagesOperations)
				UE_LOG(LogUnrealGameLink, Log, TEXT("Cooking Dir for Packag [%s] is :%s"), *package->GetName(), *CookingDirectory);

			if (CookingDirectory.IsEmpty())
			{
				if (bDebugEditorPackagesOperations)
					UE_LOG(LogUnrealGameLink, Error, TEXT("Faile to get or create Cooking Dir for the Packag [%s]"), *package->GetName());
				return false;
			}

			//[2] cook the current package in the loop
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
bool FUnrealGameLinkModule::CookModifiedPackage(UPackage* Package, ITargetPlatform* TargetPlatform, const FString& CookedFileNamePath)
{
	bool bSuccess = false;

	//TODO::Expose more flags to the project settings
	uint32 SaveFlags = 
		(bSaveConcurrent ? SAVE_Concurrent : 0) |
		(bSaveAsync ? SAVE_Async : 0) |
		(bSaveUnversioned ? SAVE_Unversioned : 0) |
		SAVE_ComputeHash | 
		SAVE_KeepGUID;

	//TODO::Expose more flags to the project settings
	EObjectFlags TopLevelFlags = RF_Public;
	if (Cast<UWorld>(Package))
		TopLevelFlags = RF_NoFlags;

	//Check if the target have editor only, if so, let's set it. But make sure at the end before we exit, we clear that flag again
	if (!TargetPlatform->HasEditorOnlyData())
		Package->SetPackageFlags(PKG_FilterEditorOnly);
	else
		Package->ClearPackageFlags(PKG_FilterEditorOnly);

	//Before we cook a package, let's check and delte if exist, just in case the folder clean failed at start
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

	/*
	//bSuccess = UE::EditorDomain::TrySavePackage(Package);

	TUniquePtr<ICookedPackageWriter> PackageWriter = MakeUnique<ICookedPackageWriter>(CookedFileNamePath);
	//IPackageWriter* PackageWriter;
	IPackageWriter::FBeginPackageInfo BeginInfo;
	BeginInfo.PackageName = Package->GetFName();
	PackageWriter->BeginPackage(BeginInfo);
	FSavePackageContext SavePackageContext(TargetPlatform, PackageWriter.Get());
	*/

	/*
	FSavePackageArgs SaveArgs;
	//SaveArgs.SavePackageContext = &SavePackageContext;
	SaveArgs.TopLevelFlags = TopLevelFlags;
	SaveArgs.Error = GError;
	SaveArgs.bForceByteSwapping = false;
	SaveArgs.bWarnOfLongFilename = false;
	SaveArgs.SaveFlags = SaveFlags;
	SaveArgs.TargetPlatform = TargetPlatform;
	SaveArgs.FinalTimeStamp = FDateTime::MinValue();
	SaveArgs.bSlowTask = false;
	SaveArgs.SavePackageContext = nullptr;
	FSavePackageResultStruct SaveResult = GEditor->Save
	(
		Package,
		nullptr,
		*CookedFileNamePath,
		SaveArgs
	);
	*/

	//possible method
	/*
	LoadPackage(Package, *CookedFileNamePath, LOAD_None);
	*/

	//this seems good method, but flags is enum!!
	/*
	bSuccess = SavePackageHelper(Package, CookedFileNamePath, TopLevelFlags, GError, SAVE_Concurrent);
	*/

	//this method use the body of the SavePackageHelper() function
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = TopLevelFlags;
	SaveArgs.Error = GError;
	SaveArgs.SaveFlags = SaveFlags; //SAVE_Concurrent | SAVE_Async | SAVE_Unversioned | SAVE_ComputeHash | SAVE_KeepGUID;
	SaveArgs.TargetPlatform = TargetPlatform;
	bSuccess =  GEditor->SavePackage(Package, nullptr, *CookedFileNamePath, SaveArgs);



	//UE4.x, this will be deprecated sometime in UE5.x, in UE5.0 it warns about future deprecation
	/*
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
		false, //TODO::can show a slowtask progressbar if needed
		nullptr,
		nullptr
	);
	*/

	GIsCookerLoadingPackage = false;

	//bSuccess = SaveResult == ESavePackageResult::Success ? true : false;

	//Notify if user set the option
	if (bNotifyCookingResults)
	{
		FText notificationMessage = FText::Format
		(
			LOCTEXT("FUnrealGameLinkModuleNotification", "{0} Cooking package [{1}] for platform [{2}]"),
			bSuccess ? FText::FromString(ANSI_TO_TCHAR("Success")) : FText::FromString(ANSI_TO_TCHAR("Failure")),
			FText::FromString(Package->GetName()),
			FText::FromString(TargetPlatform->PlatformName())
		);
		FNotificationInfo notificationInfo(notificationMessage);
		notificationInfo.ExpireDuration = 4.f; //fade in 1 sec, out 1 sec, and remains for 2 sec
		notificationInfo.FadeInDuration = 1.f;
		notificationInfo.FadeOutDuration = 1.f;
		notificationInfo.bFireAndForget = true;
		notificationInfo.bUseLargeFont = false;
		notificationInfo.bUseSuccessFailIcons = true;
		notificationInfo.HyperlinkText = FText::FromString(CookedFileNamePath);
		notificationInfo.Hyperlink = FSimpleDelegate::CreateLambda([](FString FilePath) { FPlatformProcess::ExploreFolder(*FilePath);}, CookedFileNamePath);
		FSlateNotificationManager::Get().AddNotification(notificationInfo)->SetCompletionState(bSuccess ? SNotificationItem::ECompletionState::CS_Success : SNotificationItem::ECompletionState::CS_Fail);
	}

	//last step, if success, we print to keep tracking in log, but also update the temp list in the project settings
	if (bSuccess)
	{
		if (bDebugEditorCooker)
			UE_LOG(LogUnrealGameLink, Log, TEXT("Note: FUnrealGameLinkModule::CookModifiedPackage, Success cooking the package [%s] for the target platform [%s]"), *Package->GetName(), *TargetPlatform->PlatformName());

		if (UUnrealGameLinkSettings* UnrealGameLinkProjectSettings = GetMutableDefault<UUnrealGameLinkSettings>())
		{
			UnrealGameLinkProjectSettings->MostRecentModifiedContent.AddUnique(CookedFileNamePath);
			UnrealGameLinkProjectSettings->SaveConfig(CPF_Config, *UnrealGameLinkProjectSettings->GetDefaultConfigFilename());

			if (IsTrackUsage)
			{
				TrackedInfoToPush.Append(CookedFileNamePath);
				TrackedInfoToPush.Append("\n");
			}
		}
	}

	//reset things to normal before we exit that package
	if (!TargetPlatform->HasEditorOnlyData())
		Package->ClearPackageFlags(PKG_FilterEditorOnly);

	return true;
}

/*
* Copy the modified packages from the UnrealGameLink temp cooking directory, to their final residence place based on their platform values set in the project settings.
*/
bool FUnrealGameLinkModule::CopyModifiedPackages()
{
	bool bSuccess = false;

	/*
	1 - Copy all the modified packages
	2 - Copy the UnrealGameLink config
	*/

	if (const UUnrealGameLinkSettings* UnrealGameLinkProjectSettings = GetDefault<UUnrealGameLinkSettings>())
	{
		for (const FUnrealGameLinkTargetPlatform platform : UnrealGameLinkProjectSettings->TargetPlatforms)
		{
			FString platformName = UEnum::GetValueAsString(platform.Platfrom);
			platformName.RemoveAt(0, 21);

			FDirectoryPath runningGameRootDirectory = platform.StreamingBuildRootDirectory;

			//Just in case the user put a none valid directory, or even it was valid but been deleted or so..
			if (IFileManager::Get().DirectoryExists(*runningGameRootDirectory.Path))
			{
				IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
				FString CopyFrom = FPaths::Combine(GetUnrealGameLinkParentCookingDirectory(), platformName, TEXT("Content")); //COOKING_DIR+PLATFORM_NAME+CONTENT (this is where all the sub-content directory files we need to copy)
				FString CopyTo = FPaths::Combine(*runningGameRootDirectory.Path, ProjectName, TEXT("Content")); //BUILD_ROOT+GAME_NAME+CONTENT (this is where Content folder content located)
			
				//[1]
				bSuccess = PlatformFile.CopyDirectoryTree(*CopyTo, *CopyFrom, true);

				//[2]
				FString CopyFileFrom = FPaths::Combine(*FPaths::ProjectConfigDir(), TEXT("DefaultUnrealGameLink.ini")); //PROJECT_CONFIG_DIR+CONFIG_FILE_NAME
				FString CopyFileTo = FPaths::Combine(*runningGameRootDirectory.Path, ProjectName, TEXT("Config"), TEXT("DefaultUnrealGameLink.ini")); //BUILD_ROOT+GAME_NAME+CONFIG_DIR+FILE_NAME
				IFileManager::Get().Copy(*CopyFileTo, *CopyFileFrom, true, true, true);
			}
			else
			{
				if (bDebugEditorPackagesOperations)
					UE_LOG(LogUnrealGameLink, Warning, TEXT("SKIPPED FUnrealGameLinkModule::CopyModifiedPackages() for the none-valid directory [%s]"), *runningGameRootDirectory.Path);
			}

		}
	}

	return true;
}

/*
* Pretty close to UCookOnTheFlyServer::CreateSaveContext(const ITargetPlatform* TargetPlatform) in the CookOnTheFlyServer.h/cpp
*/
/*
UE::Cook::FCookSavePackageContext* FUnrealGameLinkModule::CreateSaveContext(const ITargetPlatform* TargetPlatform)
{
	using namespace UE::Cook;

	const FString RootPathSandbox = ConvertToFullSandboxPath(FPaths::RootDir(), true);
	FString MetadataPathSandbox;
	MetadataPathSandbox = ConvertToFullSandboxPath(FPaths::ProjectDir() / "Metadata", true);

	const FString PlatformString = TargetPlatform->PlatformName();
	const FString ResolvedRootPath = RootPathSandbox.Replace(TEXT("[Platform]"), *PlatformString);
	const FString ResolvedMetadataPath = MetadataPathSandbox.Replace(TEXT("[Platform]"), *PlatformString);

	ICookedPackageWriter* PackageWriter = nullptr;
	FString WriterDebugName;

	PackageWriter = new FLooseCookedPackageWriter(ResolvedRootPath, ResolvedMetadataPath, TargetPlatform,
	GetAsyncIODelete(), *PackageDatas, PluginsToRemap);
	WriterDebugName = TEXT("DirectoryWriter");

	DiffModeHelper->InitializePackageWriter(PackageWriter);

	FConfigFile PlatformEngineIni;
	FConfigCacheIni::LoadLocalIniFile(PlatformEngineIni, TEXT("Engine"), true, *TargetPlatform->IniPlatformName());

	bool bLegacyBulkDataOffsets = false;
	PlatformEngineIni.GetBool(TEXT("Core.System"), TEXT("LegacyBulkDataOffsets"), bLegacyBulkDataOffsets);
	if (bLegacyBulkDataOffsets)
	{
		UE_LOG(LogCook, Warning, TEXT("Engine.ini:[Core.System]:LegacyBulkDataOffsets is no longer supported in UE5. The intended use was to reduce patch diffs, but UE5 changed cooked bytes in every package for other reasons, so removing support for this flag does not cause additional patch diffs."));
	}

	FCookSavePackageContext* Context = new FCookSavePackageContext(TargetPlatform, PackageWriter, WriterDebugName);
	Context->SaveContext.SetValidator(MakeUnique<FCookedSavePackageValidator>(TargetPlatform, *this));
	return Context;
}
*/

/*
* Get the cooking directory of a given package based on a given target platform & parent root cooking directory.
* 
* @param Package						The target package to get a cooking directory for
* @param UnrealGameLinkCookedDirectory			The main cooking directory of the plugin, usually shall be at "PROJECT/Save/UnrealGameLinkCooked"
* @param TargetPlatformName				The platform we cooking for, will be used to make a sub-folder of the cooking main directory
* @param OutPackageCookDirectory		The value to send back to the caller, this shall be a full path including the filename + format
*/
void FUnrealGameLinkModule::GetPackagesCookingDirectory(UPackage* Package, const FString& UnrealGameLinkCookedDirectory, const FString& TargetPlatformName, FString& OutPackageCookDirectory)
{
	//remember that Package->FileName is "None" for the newly added packages! EPIC!!

	if (bDebugEditorPackagesOperations)
		UE_LOG(LogUnrealGameLink, Log, TEXT("FUnrealGameLinkModule::GetPackagesCookingDirectory() for the package: [%s]"), *Package->GetName());

	FString PackageFullFileNamePath;

	//Get full path
	bool FoundPackage = false;
	FoundPackage = FPackageName::DoesPackageExist(Package->GetName(), &PackageFullFileNamePath, false);

	//remove the /../../../PROJECT_NAME/Content and leave only the asset name and path inside the content folder
	int32 strFoundAt = PackageFullFileNamePath.Find("/Content", ESearchCase::IgnoreCase, ESearchDir::FromStart);
	check(strFoundAt != -1);
	FString FilePathRelativeToContent = PackageFullFileNamePath.RightChop(strFoundAt);

	//combine all (CookingDirInSavedFolder + Platform + InContentDirAndFileName)
	OutPackageCookDirectory = FPaths::Combine(UnrealGameLinkCookedDirectory, TargetPlatformName, FilePathRelativeToContent);
	if (bDebugEditorPackagesOperations)
		UE_LOG(LogUnrealGameLink, Log, TEXT("FINAL COOKING PATH:%s"), *OutPackageCookDirectory);

}

/*
* Returns the main cooking directory for UnrealGameLink. Usually should be .../[PROJECT]/Saved/UnrealGameLinkCooked/...
*/
FString FUnrealGameLinkModule::GetUnrealGameLinkParentCookingDirectory()
{
	FString UnrealGameLinkCookDirectory = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("UnrealGameLinkCooked")));
	return UnrealGameLinkCookDirectory;
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUnrealGameLinkModule, UnrealGameLink)