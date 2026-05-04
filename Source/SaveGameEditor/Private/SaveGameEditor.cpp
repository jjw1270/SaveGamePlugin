// Copyright (c) 2026 장윤제. All rights reserved.

#include "SaveGameEditor.h"
#include "CommonUtils.h"
#include "SaveGame.h"
#include "MessageLogModule.h"
#include "ToolMenus.h"
#include "SaveGameEditorStyle.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "SaveGameDeveloperSettings.h"

#define LOCTEXT_NAMESPACE "FSaveGameEditorModule"

void FSaveGameEditorModule::StartupModule()
{
	// editor style
	FSaveGameEditorStyle::Initialize();

	// message log
	FMessageLogModule& message_log_module = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
	message_log_module.RegisterLogListing(SaveGameLog, FText::FromString(TEXT("Save Game")));

	// tool menu
	if (UToolMenus::IsToolMenuUIEnabled())
	{
		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FSaveGameEditorModule::RegisterMenus));
	}
}

void FSaveGameEditorModule::ShutdownModule()
{
	// editor style
	FSaveGameEditorStyle::Shutdown();

	// tool menu
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
}

void FSaveGameEditorModule::RegisterMenus()
{
	if (_IsMenuRegistered)
		return;

	AddSaveGameMenuEntry();
	_IsMenuRegistered = true;
}

void FSaveGameEditorModule::AddSaveGameMenuEntry()
{
	UToolMenu* toolbar_menu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.LevelEditorToolBar.User"));
	if (IsInvalid(toolbar_menu))
		return;

	FToolMenuSection& section = toolbar_menu->FindOrAddSection(TEXT("ResetGame"));

	section.AddEntry(FToolMenuEntry::InitToolBarButton(
		TEXT("ResetGame"),
		FUIAction(FExecuteAction::CreateRaw(this, &FSaveGameEditorModule::OnClicked_ResetGame)),
		LOCTEXT("ResetGame_Label", "Reset Game"),
		LOCTEXT("ResetGame_ToolTip", "Reset Game."),
		FSlateIcon(FSaveGameEditorStyle::GetStyleSetName(), "SaveGame.Reset")
	));
}

void FSaveGameEditorModule::OnClicked_ResetGame()
{
	const auto settings = GetDefault<USaveGameDeveloperSettings>();
	if (IsInvalid(settings))
		return;

	UGameplayStatics::DeleteGameInSlot(settings->_SaveGameSlotName, 0);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSaveGameEditorModule, SaveGameEditor)
