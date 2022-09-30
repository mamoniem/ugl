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

using UnrealBuildTool;

public class UnrealGameLink : ModuleRules
{
	public UnrealGameLink(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"InputCore",
				"UnrealEd",
				"ToolMenus",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"EngineSettings",
				"SourceControl",
				//"DerivedDataCache", //needed for EditorDomainUtils.h in order to include DerivedDataCache.h
				"UnrealGameLinkRuntime",
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
