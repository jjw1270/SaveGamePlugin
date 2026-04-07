// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SaveGameDeveloperSettings.generated.h"


UCLASS(Config = Game, DefaultConfig)
class SAVEGAME_API USaveGameDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Config, Category = "SaveGame")
	TSoftClassPtr<class UCustomSaveGame> _SaveGameClass = nullptr;

	UPROPERTY(EditAnywhere, Config, Category = "SaveGame")
	FString _SaveGameSlotName = TEXT("SaveGameSlot");
};
