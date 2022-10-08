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

#include "UnrealGameLinkCommands.h"

#define LOCTEXT_NAMESPACE "FUnrealGameLinkModule"

void FUnrealGameLinkCommands::RegisterCommands()
{
	UI_COMMAND(PluginAction, "UnrealGameLink", "Update and link the currently modified asset[s] to the running game build[s]", EUserInterfaceActionType::Button, /*FInputGesture()*/FInputChord());
}

#undef LOCTEXT_NAMESPACE
