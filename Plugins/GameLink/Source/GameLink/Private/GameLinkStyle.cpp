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

#include "GameLinkStyle.h"
#include "GameLink.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"

TSharedPtr< FSlateStyleSet > FGameLinkStyle::StyleInstance = NULL;

void FGameLinkStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FGameLinkStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FGameLinkStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("GameLinkStyle"));
	return StyleSetName;
}

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon40x40(40.0f, 40.0f);

TSharedRef< FSlateStyleSet > FGameLinkStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("GameLinkStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("GameLink")->GetBaseDir() / TEXT("Resources"));

	Style->Set("GameLink.PluginAction", new IMAGE_BRUSH(TEXT("ButtonIcon_40x"), Icon40x40));

	return Style;
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT

void FGameLinkStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FGameLinkStyle::Get()
{
	return *StyleInstance;
}
