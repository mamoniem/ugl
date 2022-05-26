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
#include "UObject/Object.h"

#include "UnrealGameLinkSettings.generated.h"
/**
 * 
 */

UENUM()
enum class ESupportedPlatforms : uint8
{
	Windows				= 0,	//maybe name it Win64
	Mac					= 1,
	Linux				= 2,
	PS4					= 3,
	XB1					= 4,
	WinGDK				= 5,
	XboxOneGDK			= 6,
	XSX					= 7,
	Switch				= 8
};

USTRUCT()
struct FUnrealGameLinkTargetPlatform
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Cooker Settings")
	ESupportedPlatforms Platfrom;

	UPROPERTY(EditAnywhere, Category = "Cooker Settings")
	FDirectoryPath StreamingBuildRootDirectory;

	FUnrealGameLinkTargetPlatform()
	{
		Platfrom = ESupportedPlatforms::Windows;
		StreamingBuildRootDirectory.Path = "../";
	}
};

UCLASS(config=UnrealGameLink, defaultConfig)
class UNREALGAMELINKRUNTIME_API UUnrealGameLinkSettings : public UObject
{
	GENERATED_BODY()

public:		
	////////////////////////////////////
	// 	   Runtime Settings	////////////
	////////////////////////////////////

	/* This need to be enabled before packaging & cooking the project, otherwise, the running standalone will not be linked with the editor */
	UPROPERTY(EditAnywhere, config, meta = (ConfigRestartRequired=true), Category = "Runtime Settings")
		bool bEnabledAtRuntime = false;

	/* This shall be disabled, as it make no sense to Enable UnrealGameLink cooking-streaming for editor. Note that changing this value would need the editor to restart [Editing is meant to be disabled at the meantime]*/
	UPROPERTY(EditAnywhere, config, meta = (ConfigRestartRequired = true), Category = "Runtime Settings")
		bool bEnabledAtEditorRuntime = false;

	/* Time in Seconds where checks for editor content updates will be made (this impacts in running build only) */
	UPROPERTY(EditAnywhere, config, meta = (ConfigRestartRequired = true), Category = "Runtime Settings")
		float EditorSyncInterval = 2;

	/* Either to reload the current running map on the game running instance, or to update the level without reloading */
	UPROPERTY(EditAnywhere, config, Category = "Runtime Settings")
		bool bReloadActiveMapOnContentUpdates = false;

	////////////////////////////////////
	// 	   Cooker Settings	////////////
	////////////////////////////////////

	/* Save the files before cooking. This is essential step, in order to make sure the user gets the correct result cooked to the running game */
	UPROPERTY(EditAnywhere, config, Category = "Cooker Settings")
		bool bSaveBeforeCooking = true;

	/* Display notification when the cooking is completed. Regardless success or failure, the notification will show accordingly with proper output */
	UPROPERTY(EditAnywhere, config, Category = "Cooker Settings")
		bool bNotifyCookingResults = true;
	
	/* This to enable the flag SAVE_Concurrent when cooking a package. More info about save & cook flags are in ObjectMacros.h, but in general, this exact flag description is [ We are save packages in multiple threads at once and should not call non-threadsafe functions or rely on globals. GIsSavingPackage should be set and PreSave/Postsave functions should be called before/after the entire concurrent save.] */
	UPROPERTY(EditAnywhere, config, Category = "Cooker Settings")
		bool bSaveConcurrent = true;

	/* This to enable the flag SAVE_Async when cooking a package. More info about save & cook flags are in ObjectMacros.h, but in general, this exact flag description is [Save to a memory writer, then actually write to disk async] */
	UPROPERTY(EditAnywhere, config, Category = "Cooker Settings")
		bool bSaveAsync = true;

	/* This to enable the flag SAVE_Unversioned when cooking a package. More info about save & cook flags are in ObjectMacros.h, but in general, this exact flag description is [Save all versions as zero. Upon load this is changed to the current version. This is only reasonable to use with full cooked builds for distribution.] */
	UPROPERTY(EditAnywhere, config, Category = "Cooker Settings")
		bool bSaveUnversioned = false;
		
	/* This is the list of the platform to cook for. Single platform is recommended for the fastest result, but always can have mutliple platforms at the same time */
	UPROPERTY(EditAnywhere, config, meta = (TitleProperty = "Platfrom"), Category = "Cooker Settings")
		TArray<FUnrealGameLinkTargetPlatform> TargetPlatforms;

	////////////////////////////////////
	// 	   Debug Settings	////////////
	////////////////////////////////////

	/* Debug general messages at Editor or Runtime (modules startup, shutdown, delegates register, unregister,...etc.) */
	UPROPERTY(EditAnywhere, config, meta = (ConfigRestartRequired = true), Category = "Debug Settings")
		bool bDebugEditorGeneralMessages = true;

	/* Editor Debug messages for operations taking place on packages */
	UPROPERTY(EditAnywhere, config, meta = (ConfigRestartRequired = true), Category = "Debug Settings")
		bool bDebugEditorPackagesOperations = true;

	/* Editor Debug messages for the cooker */
	UPROPERTY(EditAnywhere, config, meta = (ConfigRestartRequired = true), Category = "Debug Settings")
		bool bDebugEditorCooker = true;

	/* Runtime Client Debug messages for the runtime tick checks [aka not recommended until you need it] */
	UPROPERTY(EditAnywhere, config, Category = "Debug Settings")
		bool bDebugRuntimeTicks = true;

	/* Runtime Client Debug messages for the packages reloading commands */
	UPROPERTY(EditAnywhere, config, Category = "Debug Settings")
		bool bDebugRuntimePackagesReloading = true;

	////////////////////////////////////
	// 	   Info	////////////////////////
	////////////////////////////////////

	/* The total number of packages to put in a single reloading patch at runtime */
	UPROPERTY(VisibleAnywhere, config, Category = "Info")
		int32 PackagesToReloadPerPatch = 200;

	/* A read only list of the most recent modified assets (including all platforms per asset). This list will be filled whenever you UnrealGameLink something to a running game instance[s].
	and will automatically empty itself frequently when needed, or at the start-up of the editor/engine. */
	UPROPERTY(VisibleAnywhere, config, Category = "Info")
		TArray<FString> MostRecentModifiedContent;
};
