// Copyright (c) 2026 장윤제. All rights reserved.


#include "SaveGameSubsystem.h"
#include "CommonUtils.h"
#include "Kismet/GameplayStatics.h"
#include "SaveGameDeveloperSettings.h"
#include "Widgets/WidgetBase.h"


void USaveGameSubsystem::Initialize(FSubsystemCollectionBase& _collection)
{
	Super::Initialize(_collection);

	_IsDeinitializing = false;

	const auto settings = GetDefault<USaveGameDeveloperSettings>();
	if (IsInvalid(settings) || settings->_SaveGameClass.IsNull())
	{
		TRACE_ERROR(TEXT("SaveGame Dev Setting Error"));
		return;
	}

	UClass* save_game_class = settings->_SaveGameClass.LoadSynchronous();
	if (IsInvalid(save_game_class))
	{
		TRACE_ERROR(TEXT("SaveGame Class Load Failed"));
		return;
	}

	_SaveGame = Cast<UCustomSaveGame>(UGameplayStatics::CreateSaveGameObject(save_game_class));
	if (IsInvalid(_SaveGame))
	{
		TRACE_ERROR(TEXT("SaveGame Create Failed"));
	}
}

void USaveGameSubsystem::Deinitialize()
{
	_IsDeinitializing = true;

	SetSaveGameCanModify(true);

	_IsAsyncSaving = false;
	_IsAsyncLoading = false;

	if (IsValid(_AsyncSaveGameWidget))
	{
		_AsyncSaveGameWidget->RemoveFromParent();
		_AsyncSaveGameWidget = nullptr;
	}

	Super::Deinitialize();
}

void USaveGameSubsystem::SetSaveGameCanModify(bool _can_modify)
{
	if (IsValid(_SaveGame))
	{
		_SaveGame->SetCanModify(_can_modify);
	}
}

bool USaveGameSubsystem::LoadGame()
{
	if (_IsAsyncSaving || _IsAsyncLoading)
	{
		TRACE_WARNING(TEXT("Async save/load is in progress."));
		return false;
	}

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

void USaveGameSubsystem::AsyncLoadGame()
{
	const auto settings = GetDefault<USaveGameDeveloperSettings>();
	if (IsInvalid(settings))
	{
		TRACE_ERROR(TEXT("SaveGame Dev Setting Error"));
		_OnAsyncLoadGameFinished.Broadcast(false);
		return;
	}

	if (_IsAsyncLoading)
	{
		TRACE_WARNING(TEXT("SaveGame is already loading."));
		_OnAsyncLoadGameFinished.Broadcast(false);
		return;
	}

	if (_IsAsyncSaving)
	{
		TRACE_WARNING(TEXT("SaveGame is already saving."));
		_OnAsyncLoadGameFinished.Broadcast(false);
		return;
	}

	_IsAsyncLoading = true;
	SetSaveGameCanModify(false);
	_OnAsyncLoadGameStarted.Broadcast();

	UGameplayStatics::AsyncLoadGameFromSlot(settings->_SaveGameSlotName, 0, 
		FAsyncLoadGameFromSlotDelegate::CreateWeakLambda(this,
			[this](const FString& _slot_name, const int32 _user_index, USaveGame* _save_game)
			{
				if (_IsDeinitializing)
					return;

				auto loaded_save_game = Cast<UCustomSaveGame>(_save_game);

				const bool load_success = IsValid(loaded_save_game);
				if (load_success)
				{
					_SaveGame = loaded_save_game;
				}

				_IsAsyncLoading = false;
				SetSaveGameCanModify(true);

				_OnAsyncLoadGameFinished.Broadcast(load_success);
			}
		)
	);
}

bool USaveGameSubsystem::SaveGame()
{
	if (_IsAsyncSaving || _IsAsyncLoading)
	{
		TRACE_WARNING(TEXT("Async save/load is in progress."));
		return false;
	}

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

void USaveGameSubsystem::AsyncSaveGame()
{
	if (IsInvalid(_SaveGame))
	{
		TRACE_ERROR(TEXT("SaveGame Is Invalid"));
		_OnAsyncSaveGameFinished.Broadcast(false);
		return;
	}

	const auto settings = GetDefault<USaveGameDeveloperSettings>();
	if (IsInvalid(settings))
	{
		_OnAsyncSaveGameFinished.Broadcast(false);
		return;
	}

	if (_IsAsyncSaving)
	{
		TRACE_WARNING(TEXT("SaveGame is already saving."));
		_OnAsyncSaveGameFinished.Broadcast(false);
		return;
	}

	if (_IsAsyncLoading)
	{
		TRACE_WARNING(TEXT("SaveGame is already loading."));
		_OnAsyncSaveGameFinished.Broadcast(false);
		return;
	}

	_IsAsyncSaving = true;
	SetSaveGameCanModify(false);
	_OnAsyncSaveGameStarted.Broadcast();

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
			[this](const FString& _slot_name, const int32 _user_index, bool _is_success)
			{
				if (_IsDeinitializing)
					return;

				_IsAsyncSaving = false;
				SetSaveGameCanModify(true);

				if (IsValid(_AsyncSaveGameWidget))
				{
					_AsyncSaveGameWidget->RemoveFromParent();
				}

				_OnAsyncSaveGameFinished.Broadcast(_is_success);
			}
		)
	);
}

void USaveGameSubsystem::ResetGame()
{
	if (_IsAsyncSaving || _IsAsyncLoading)
	{
		TRACE_WARNING(TEXT("Async save/load is in progress."));
		return;
	}

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
