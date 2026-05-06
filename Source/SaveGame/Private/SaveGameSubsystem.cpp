// Copyright (c) 2026 장윤제. All rights reserved.


#include "SaveGameSubsystem.h"
#include "CommonUtils.h"
#include "Kismet/GameplayStatics.h"
#include "SaveGameDeveloperSettings.h"
#include "Widgets/WidgetBase.h"


void USaveGameSubsystem::Initialize(FSubsystemCollectionBase& _collection)
{
	Super::Initialize(_collection);

	const auto settings = GetDefault<USaveGameDeveloperSettings>();
	if (IsInvalid(settings) || settings->_SaveGameClass.IsNull())
	{
		TRACE_ERROR(TEXT("SaveGame Dev Setting Error"));
		return;
	}

	_SaveGame = Cast<UCustomSaveGame>(UGameplayStatics::CreateSaveGameObject(settings->_SaveGameClass.LoadSynchronous()));
}

bool USaveGameSubsystem::LoadGame()
{
	const auto settings = GetDefault<USaveGameDeveloperSettings>();
	if (IsInvalid(settings))
	{
		TRACE_ERROR(TEXT("SaveGame Dev Setting Error"));
		return false;
	}

	auto loaded_save_game = Cast<UCustomSaveGame>(UGameplayStatics::LoadGameFromSlot(settings->_SaveGameSlotName, 0));
	
	const bool load_success = IsValid(loaded_save_game);
	if (load_success)
	{
		_SaveGame = loaded_save_game;
	}

	return load_success;
}

void USaveGameSubsystem::AsyncLoadGame(FD_OnLoadGameFinished _on_load_game_finished_event)
{
	const auto settings = GetDefault<USaveGameDeveloperSettings>();
	if (IsInvalid(settings))
	{
		TRACE_ERROR(TEXT("SaveGame Dev Setting Error"));
		return;
	}

	UGameplayStatics::AsyncLoadGameFromSlot(settings->_SaveGameSlotName, 0, 
		FAsyncLoadGameFromSlotDelegate::CreateWeakLambda(this,
			[this, _on_load_game_finished_event](const FString& _slot_name, const int32 _user_index, USaveGame* _save_game)
			{
				auto loaded_save_game = Cast<UCustomSaveGame>(_save_game);

				const bool load_success = IsValid(loaded_save_game);
				if (load_success)
				{
					_SaveGame = loaded_save_game;
				}

				_on_load_game_finished_event.ExecuteIfBound(load_success);
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

void USaveGameSubsystem::AsyncSaveGame(FD_OnSaveGameFinished _on_save_game_finished_event)
{
	if (IsInvalid(_SaveGame))
	{
		TRACE_ERROR(TEXT("SaveGame Is Invalid"));
		_on_save_game_finished_event.ExecuteIfBound(false);
		return;
	}

	const auto settings = GetDefault<USaveGameDeveloperSettings>();
	if (IsInvalid(settings))
	{
		_on_save_game_finished_event.ExecuteIfBound(false);
		return;
	}

	if (_IsAsyncSaving)
	{
		TRACE_WARNING(TEXT("SaveGame is already saving."));
		_on_save_game_finished_event.ExecuteIfBound(false);
		return;
	}

	_IsAsyncSaving = true;

	if (IsInvalid(_AsyncSaveGameWidget))
	{
		if (IsValid(settings->_AsyncSaveGameWidgetClass))
		{
			_AsyncSaveGameWidget = CreateWidget<UWidgetBase>(GetGameInstance(), settings->_AsyncSaveGameWidgetClass);
		}
		else
		{
			TRACE_WARNING(TEXT("Save Game Dev setting : _AsyncSaveGameWidgetClass is invalid."));
		}
	}

	if (IsValid(_AsyncSaveGameWidget))
	{
		// 최상단에 와야 함!
		_AsyncSaveGameWidget->AddToViewport(999);
	}

	return UGameplayStatics::AsyncSaveGameToSlot(_SaveGame, settings->_SaveGameSlotName, 0,
		FAsyncSaveGameToSlotDelegate::CreateWeakLambda(this,
			[this, _on_save_game_finished_event](const FString& _slot_name, const int32 _user_index, bool _is_success)
			{
				_IsAsyncSaving = false;

				if (IsValid(_AsyncSaveGameWidget))
				{
					_AsyncSaveGameWidget->RemoveFromParent();
				}

				_on_save_game_finished_event.ExecuteIfBound(_is_success);
			}
		)
	);
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
