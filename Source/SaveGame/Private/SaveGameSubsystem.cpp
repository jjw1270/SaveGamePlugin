// Copyright (c) 2026 장윤제. All rights reserved.


#include "SaveGameSubsystem.h"
#include "CommonUtils.h"
#include "Kismet/GameplayStatics.h"
#include "SaveGameDeveloperSettings.h"



bool USaveGameSubsystem::LoadGame()
{
	const auto settings = GetDefault<USaveGameDeveloperSettings>();
	if (IsInvalid(settings) || settings->_SaveGameClass.IsNull())
	{
		TRACE_ERROR(TEXT("SaveGame Dev Setting Error"));
		return false;
	}

	_SaveGame = Cast<UCustomSaveGame>(UGameplayStatics::LoadGameFromSlot(settings->_SaveGameSlotName, 0));

	return IsValid(_SaveGame);
}

void USaveGameSubsystem::AsyncLoadGame(F_OnLoadGameFinished _on_load_game_finished_event)
{
	const auto settings = GetDefault<USaveGameDeveloperSettings>();
	if (IsInvalid(settings) || settings->_SaveGameClass.IsNull())
	{
		TRACE_ERROR(TEXT("SaveGame Dev Setting Error"));
		return;
	}

	UGameplayStatics::AsyncLoadGameFromSlot(settings->_SaveGameSlotName, 0, 
		FAsyncLoadGameFromSlotDelegate::CreateWeakLambda(this,
			[this, _on_load_game_finished_event](const FString& _slot_name, const int32 _user_index, USaveGame* _save_game)
			{
				_SaveGame = Cast<UCustomSaveGame>(_save_game);
				_on_load_game_finished_event.ExecuteIfBound(IsValid(_SaveGame));
			}
		)
	);
}

bool USaveGameSubsystem::SaveGame()
{
	if (IsInvalid(_SaveGame))
	{
		TRACE_ERROR(TEXT("SaveGame Is Invalid"));
		return false;
	}

	const auto settings = GetDefault<USaveGameDeveloperSettings>();
	if (IsInvalid(settings))
		return false;

	return UGameplayStatics::SaveGameToSlot(_SaveGame, settings->_SaveGameSlotName, 0);
}

void USaveGameSubsystem::ResetGame()
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
