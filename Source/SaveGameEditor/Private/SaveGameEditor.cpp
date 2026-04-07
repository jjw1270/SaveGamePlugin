// Copyright Epic Games, Inc. All Rights Reserved.

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

	FToolMenuSection& section = toolbar_menu->FindOrAddSection(TEXT("ClearSaveGameData"));

	section.AddEntry(FToolMenuEntry::InitToolBarButton(
		TEXT("ClearSaveGameData"),
		FUIAction(FExecuteAction::CreateRaw(this, &FSaveGameEditorModule::OnClicked_ClearSaveGameData)),
		LOCTEXT("ClearSaveGameData_Label", "Clear Save Game Data"),
		LOCTEXT("ClearSaveGameData_ToolTip", "Clear Save Game Data."),
		FSlateIcon(FSaveGameEditorStyle::GetStyleSetName(), "SaveGame.Clear")
	));
}

void FSaveGameEditorModule::OnClicked_ClearSaveGameData()
{
	const auto settings = GetDefault<USaveGameDeveloperSettings>();
	if (IsInvalid(settings))
		return;

	UGameplayStatics::DeleteGameInSlot(settings->_SaveGameSlotName, 0);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSaveGameEditorModule, SaveGameEditor)
