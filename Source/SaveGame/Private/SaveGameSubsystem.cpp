// Copyright (c) 2026 장윤제. All rights reserved.


#include "SaveGameSubsystem.h"
#include "CommonUtils.h"
#include "Kismet/GameplayStatics.h"
#include "SaveGameDeveloperSettings.h"


void USaveGameSubsystem::NewGame()
{
	const auto settings = GetDefault<USaveGameDeveloperSettings>();
	if (IsInvalid(settings) || settings->_SaveGameClass.IsNull())
	{
		TRACE_ERROR(TEXT("SaveGame Dev Setting Error"));
		return;
	}

	_SaveGame = Cast<UCustomSaveGame>(UGameplayStatics::CreateSaveGameObject(settings->_SaveGameClass.LoadSynchronous()));
}

bool USaveGameSubsystem::CanLoadGame() const
{
	const auto settings = GetDefault<USaveGameDeveloperSettings>();
	if (IsInvalid(settings) || settings->_SaveGameClass.IsNull())
	{
		TRACE_ERROR(TEXT("SaveGame Dev Setting Error"));
		return false;
	}

	return IsValid(UGameplayStatics::LoadGameFromSlot(settings->_SaveGameSlotName, 0));
}

void USaveGameSubsystem::LoadGame()
{
	const auto settings = GetDefault<USaveGameDeveloperSettings>();
	if (IsInvalid(settings) || settings->_SaveGameClass.IsNull())
	{
		TRACE_ERROR(TEXT("SaveGame Dev Setting Error"));
		return;
	}

	_SaveGame = Cast<UCustomSaveGame>(UGameplayStatics::LoadGameFromSlot(settings->_SaveGameSlotName, 0));
}

void USaveGameSubsystem::SaveGame()
{
	if (IsInvalid(_SaveGame))
	{
		TRACE_ERROR(TEXT("SaveGame Is Invalid"));
		return;
	}

	const auto settings = GetDefault<USaveGameDeveloperSettings>();
	if (IsInvalid(settings))
		return;

	UGameplayStatics::SaveGameToSlot(_SaveGame, settings->_SaveGameSlotName, 0);
}

void USaveGameSubsystem::ClearSaveGameData()
{
	if (IsInvalid(_SaveGame))
	{
		TRACE_ERROR(TEXT("SaveGame Is Invalid"));
		return;
	}

	_SaveGame->ClearData();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

UCustomSaveGame* USaveGameHelper::GetSaveGame_Internal(const UObject* _world_ctx)
{
	auto save_game_subsys = UCommonUtils::GetGameInstanceSubsystem<USaveGameSubsystem>(_world_ctx);
	if (IsValid(save_game_subsys))
	{
		return save_game_subsys->GetSaveGame();
	}

	return nullptr;
}

void USaveGameHelper::SaveGame(const UObject* _world_ctx)
{
	auto save_game_subsys = UCommonUtils::GetGameInstanceSubsystem<USaveGameSubsystem>(_world_ctx);
	if (IsInvalid(save_game_subsys))
		return;

	save_game_subsys->SaveGame();
}
