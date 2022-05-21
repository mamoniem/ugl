// Copyright Epic Games, Inc. All Rights Reserved.

#include "testUGLGameMode.h"
#include "testUGLCharacter.h"
#include "UObject/ConstructorHelpers.h"

AtestUGLGameMode::AtestUGLGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
