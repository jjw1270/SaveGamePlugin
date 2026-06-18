// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FSaveGameEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	bool _IsMenuRegistered = false;

	void RegisterMenus();
	void AddDeleteSaveSlotMenuEntry();
	void FillDeleteSaveSlotMenu(class UToolMenu* _menu);

	void OnClicked_DeleteAllSaveSlots();
	void OnClicked_DeleteSaveSlot(int32 _slot_index);
};
