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
#include "SaveGameSubsystem.h"

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

	AddDeleteSaveSlotMenuEntry();
	_IsMenuRegistered = true;
}

void FSaveGameEditorModule::AddDeleteSaveSlotMenuEntry()
{
	UToolMenu* toolbar_menu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.LevelEditorToolBar.User"));
	if (IsInvalid(toolbar_menu))
		return;

	FToolMenuSection& section = toolbar_menu->FindOrAddSection(TEXT("DeleteSaveSlot"));

	section.AddEntry(FToolMenuEntry::InitComboButton(
		TEXT("DeleteSaveSlot"),
		FUIAction(),
		FNewToolMenuDelegate::CreateRaw(this, &FSaveGameEditorModule::FillDeleteSaveSlotMenu),
		LOCTEXT("DeleteSaveSlot_Label", "Delete Save Slot"),
		LOCTEXT("DeleteSaveSlot_ToolTip", "Delete all configured SaveGame slots or select a specific slot to delete."),
		FSlateIcon(FSaveGameEditorStyle::GetStyleSetName(), "SaveGame.DeleteSaveSlot")
	));
}

void FSaveGameEditorModule::FillDeleteSaveSlotMenu(UToolMenu* _menu)
{
	if (IsInvalid(_menu))
		return;

	auto slot_registry = USaveGameSubsystem::LoadConfiguredSaveGameSlotRegistry();
	TArray<int32> slot_indices;
	if (IsValid(slot_registry))
	{
		slot_registry->_SaveGameSlotMap.GetKeys(slot_indices);
		slot_indices.Sort();
	}

	FToolMenuSection& all_slot_section = _menu->AddSection(TEXT("DeleteAllSaveSlots"), LOCTEXT("DeleteAllSaveSlots_Section", "All Slots"));
	all_slot_section.AddMenuEntry(
		TEXT("DeleteAllSaveSlots"),
		LOCTEXT("DeleteAllSaveSlots_Label", "Delete All Save Slots"),
		LOCTEXT("DeleteAllSaveSlots_ToolTip", "Delete all registered SaveGame slots."),
		FSlateIcon(FSaveGameEditorStyle::GetStyleSetName(), "SaveGame.DeleteSaveSlot"),
		FUIAction(FExecuteAction::CreateRaw(this, &FSaveGameEditorModule::OnClicked_DeleteAllSaveSlots))
	);

	FToolMenuSection& slot_section = _menu->AddSection(TEXT("DeleteSelectedSaveSlot"), LOCTEXT("DeleteSelectedSaveSlot_Section", "Selected Slot"));
	for (const int32 slot_index : slot_indices)
	{
		slot_section.AddMenuEntry(
			FName(*FString::Printf(TEXT("DeleteSaveSlot_%d"), slot_index)),
			FText::Format(LOCTEXT("DeleteSaveSlot_LabelFormat", "Delete Save Slot {0}"), FText::AsNumber(slot_index)),
			FText::Format(LOCTEXT("DeleteSaveSlot_ToolTipFormat", "Delete only SaveGame slot {0}."), FText::AsNumber(slot_index)),
			FSlateIcon(FSaveGameEditorStyle::GetStyleSetName(), "SaveGame.DeleteSaveSlot"),
			FUIAction(FExecuteAction::CreateRaw(this, &FSaveGameEditorModule::OnClicked_DeleteSaveSlot, slot_index))
		);
	}
}

void FSaveGameEditorModule::OnClicked_DeleteAllSaveSlots()
{
	auto slot_registry = USaveGameSubsystem::LoadConfiguredSaveGameSlotRegistry();
	if (IsInvalid(slot_registry))
		return;

	TArray<int32> slot_indices;
	slot_registry->_SaveGameSlotMap.GetKeys(slot_indices);
	for (const int32 slot_index : slot_indices)
	{
		if (const FString* slot_name = slot_registry->_SaveGameSlotMap.Find(slot_index))
		{
			UGameplayStatics::DeleteGameInSlot(*slot_name, 0);
		}
	}

	slot_registry->_SaveGameSlotMap.Empty();
	slot_registry->_LastSaveGameSlotIndex = 0;
	USaveGameSubsystem::SaveConfiguredSaveGameSlotRegistry(slot_registry);
}

void FSaveGameEditorModule::OnClicked_DeleteSaveSlot(int32 _slot_index)
{
	auto slot_registry = USaveGameSubsystem::LoadConfiguredSaveGameSlotRegistry();
	if (IsInvalid(slot_registry))
		return;

	const FString* slot_name = slot_registry->_SaveGameSlotMap.Find(_slot_index);
	if (slot_name == nullptr)
		return;

	UGameplayStatics::DeleteGameInSlot(*slot_name, 0);
	slot_registry->_SaveGameSlotMap.Remove(_slot_index);
	USaveGameSubsystem::SaveConfiguredSaveGameSlotRegistry(slot_registry);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSaveGameEditorModule, SaveGameEditor)
