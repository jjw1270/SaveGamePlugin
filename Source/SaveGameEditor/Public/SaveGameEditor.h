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
	void AddSaveGameMenuEntry();

	void OnClicked_ClearSaveGameData();
};
