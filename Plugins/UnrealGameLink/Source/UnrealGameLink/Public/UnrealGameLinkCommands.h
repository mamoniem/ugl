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
#include "Framework/Commands/Commands.h"
#include "UnrealGameLinkStyle.h"

class FUnrealGameLinkCommands : public TCommands<FUnrealGameLinkCommands>
{
public:

	FUnrealGameLinkCommands()
		: TCommands<FUnrealGameLinkCommands>(TEXT("UnrealGameLink"), NSLOCTEXT("Contexts", "UnrealGameLink", "UnrealGameLink Plugin"), NAME_None, FUnrealGameLinkStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > PluginAction;
};
