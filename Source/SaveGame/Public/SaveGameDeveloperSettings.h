// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SaveGameDeveloperSettings.generated.h"


UCLASS(Config = Game, DefaultConfig)
class SAVEGAME_API USaveGameDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Config)
	TSoftClassPtr<class UCustomSaveGame> _SaveGameClass = nullptr;

	UPROPERTY(EditAnywhere, Config)
	FString _SaveGameSlotName = TEXT("SaveGameSlot");
};
