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
	_ActiveSaveGameSlotIndex = INDEX_NONE;
	_SaveGameSlotRegistry = LoadOrCreateSaveGameSlotRegistry();
	_SaveGame = CreateConfiguredSaveGameObject();
	if (IsInvalid(_SaveGame))
	{
		TRACE_ERROR(TEXT("SaveGame Create Failed"));
	}
}

UCustomSaveGame* USaveGameSubsystem::CreateConfiguredSaveGameObject() const
{
	const auto settings = GetDefault<USaveGameDeveloperSettings>();
	if (IsInvalid(settings) || settings->_SaveGameClass.IsNull())
	{
		TRACE_ERROR(TEXT("SaveGame Dev Setting Error"));
		return nullptr;
	}

	UClass* save_game_class = settings->_SaveGameClass.LoadSynchronous();
	if (IsInvalid(save_game_class))
	{
		TRACE_ERROR(TEXT("SaveGame Class Load Failed"));
		return nullptr;
	}

	return Cast<UCustomSaveGame>(UGameplayStatics::CreateSaveGameObject(save_game_class));
}

USaveGameSlotRegistry* USaveGameSubsystem::CreateSaveGameSlotRegistryObject() const
{
	return Cast<USaveGameSlotRegistry>(UGameplayStatics::CreateSaveGameObject(USaveGameSlotRegistry::StaticClass()));
}

USaveGameSlotRegistry* USaveGameSubsystem::LoadOrCreateSaveGameSlotRegistry() const
{
	auto loaded_slot_registry = LoadConfiguredSaveGameSlotRegistry();
	if (IsValid(loaded_slot_registry))
	{
		return loaded_slot_registry;
	}

	return CreateSaveGameSlotRegistryObject();
}

bool USaveGameSubsystem::SaveSaveGameSlotRegistry() const
{
	return SaveConfiguredSaveGameSlotRegistry(_SaveGameSlotRegistry);
}

bool USaveGameSubsystem::IsInConfiguredSaveGameSlotRange(int32 _slot_index)
{
	return _slot_index >= 0 && _slot_index < GetConfiguredMaxSaveGameSlotCount();
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

FString USaveGameSubsystem::GetConfiguredSaveGameSlotNamePrefix()
{
	const auto settings = GetDefault<USaveGameDeveloperSettings>();
	if (IsInvalid(settings) || settings->_SaveGameSlotName.IsEmpty())
	{
		return TEXT("SaveGameSlot");
	}

	return settings->_SaveGameSlotName;
}

int32 USaveGameSubsystem::GetConfiguredMaxSaveGameSlotCount()
{
	const auto settings = GetDefault<USaveGameDeveloperSettings>();
	if (IsInvalid(settings))
	{
		return 1;
	}

	return FMath::Max(1, settings->_MaxSaveGameSlotCount);
}

FString USaveGameSubsystem::MakeConfiguredSaveGameSlotName(int32 _slot_index)
{
	return FString::Printf(TEXT("%s_%d"), *GetConfiguredSaveGameSlotNamePrefix(), _slot_index);
}

FString USaveGameSubsystem::MakeConfiguredSaveGameSlotRegistryName()
{
	return FString::Printf(TEXT("%s_Registry"), *GetConfiguredSaveGameSlotNamePrefix());
}

USaveGameSlotRegistry* USaveGameSubsystem::LoadConfiguredSaveGameSlotRegistry()
{
	return Cast<USaveGameSlotRegistry>(UGameplayStatics::LoadGameFromSlot(MakeConfiguredSaveGameSlotRegistryName(), 0));
}

bool USaveGameSubsystem::SaveConfiguredSaveGameSlotRegistry(USaveGameSlotRegistry* _slot_registry)
{
	if (IsInvalid(_slot_registry))
	{
		TRACE_ERROR(TEXT("SaveGame Slot Registry Is Invalid"));
		return false;
	}

	return UGameplayStatics::SaveGameToSlot(_slot_registry, MakeConfiguredSaveGameSlotRegistryName(), 0);
}

FString USaveGameSubsystem::MakeSaveGameSlotName(int32 _slot_index) const
{
	if (IsValid(_SaveGameSlotRegistry))
	{
		if (const FString* slot_name = _SaveGameSlotRegistry->_SaveGameSlotMap.Find(_slot_index))
		{
			return *slot_name;
		}
	}

	return FString();
}

int32 USaveGameSubsystem::AllocateSaveGameSlotIndex()
{
	if (IsInvalid(_SaveGameSlotRegistry))
	{
		_SaveGameSlotRegistry = LoadOrCreateSaveGameSlotRegistry();
	}

	if (IsInvalid(_SaveGameSlotRegistry))
	{
		return INDEX_NONE;
	}

	for (int32 new_slot_index = 0; new_slot_index < GetConfiguredMaxSaveGameSlotCount(); ++new_slot_index)
	{
		if (_SaveGameSlotRegistry->_SaveGameSlotMap.Contains(new_slot_index))
			continue;

		_SaveGameSlotRegistry->_LastSaveGameSlotIndex = new_slot_index;
		return new_slot_index;
	}

	TRACE_WARNING(TEXT("SaveGame Slot Count Limit Exceeded : %d"), GetConfiguredMaxSaveGameSlotCount());
	return INDEX_NONE;
}

int32 USaveGameSubsystem::GetSaveGameSlotCount() const
{
	return GetSaveGameSlotIndices().Num();
}

TArray<int32> USaveGameSubsystem::GetSaveGameSlotIndices() const
{
	TArray<int32> slot_indices;
	if (IsValid(_SaveGameSlotRegistry))
	{
		_SaveGameSlotRegistry->_SaveGameSlotMap.GetKeys(slot_indices);
		slot_indices.RemoveAll([](int32 _slot_index)
		{
			return USaveGameSubsystem::IsInConfiguredSaveGameSlotRange(_slot_index) == false;
		});
		slot_indices.Sort();
	}

	return slot_indices;
}

FString USaveGameSubsystem::GetSaveGameSlotName(int32 _slot_index) const
{
	if (IsValidSaveGameSlotIndex(_slot_index) == false)
	{
		return FString();
	}

	return MakeSaveGameSlotName(_slot_index);
}

bool USaveGameSubsystem::IsValidSaveGameSlotIndex(int32 _slot_index) const
{
	return IsInConfiguredSaveGameSlotRange(_slot_index)
		&& IsValid(_SaveGameSlotRegistry)
		&& _SaveGameSlotRegistry->_SaveGameSlotMap.Contains(_slot_index);
}

bool USaveGameSubsystem::DoesSaveGameExist(int32 _slot_index) const
{
	if (IsValidSaveGameSlotIndex(_slot_index) == false)
		return false;

	return UGameplayStatics::DoesSaveGameExist(MakeSaveGameSlotName(_slot_index), 0);
}

UCustomSaveGame* USaveGameSubsystem::GetSaveGameFromSlot(int32 _slot_index) const
{
	if (IsValidSaveGameSlotIndex(_slot_index) == false)
	{
		TRACE_WARNING(TEXT("Invalid SaveGame Slot Index : %d"), _slot_index);
		return nullptr;
	}

	if (_ActiveSaveGameSlotIndex == _slot_index && IsValid(_SaveGame))
	{
		return _SaveGame;
	}

	return Cast<UCustomSaveGame>(UGameplayStatics::LoadGameFromSlot(MakeSaveGameSlotName(_slot_index), 0));
}

bool USaveGameSubsystem::LoadGameSlot(int32 _slot_index)
{
	if (_IsAsyncSaving || _IsAsyncLoading)
	{
		TRACE_WARNING(TEXT("Async save/load is in progress."));
		return false;
	}

	if (IsValidSaveGameSlotIndex(_slot_index) == false)
	{
		TRACE_WARNING(TEXT("Invalid SaveGame Slot Index : %d"), _slot_index);
		return false;
	}

	auto loaded_save_game = Cast<UCustomSaveGame>(UGameplayStatics::LoadGameFromSlot(MakeSaveGameSlotName(_slot_index), 0));

	const bool load_success = IsValid(loaded_save_game);
	if (load_success)
	{
		_SaveGame = loaded_save_game;
		_ActiveSaveGameSlotIndex = _slot_index;
		SetSaveGameCanModify(true);
	}

	return load_success;
}

void USaveGameSubsystem::AsyncLoadGameSlot(int32 _slot_index)
{
	if (IsValidSaveGameSlotIndex(_slot_index) == false)
	{
		TRACE_WARNING(TEXT("Invalid SaveGame Slot Index : %d"), _slot_index);
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
	TWeakObjectPtr<UCustomSaveGame> locked_save_game = _SaveGame;
	SetSaveGameCanModify(false);
	_OnAsyncLoadGameStarted.Broadcast();

	UGameplayStatics::AsyncLoadGameFromSlot(MakeSaveGameSlotName(_slot_index), 0,
		FAsyncLoadGameFromSlotDelegate::CreateWeakLambda(this,
			[this, locked_save_game, _slot_index](const FString& _slot_name, const int32 _user_index, USaveGame* _save_game)
			{
				if (_IsDeinitializing)
					return;

				auto loaded_save_game = Cast<UCustomSaveGame>(_save_game);

				const bool load_success = IsValid(loaded_save_game);
				if (load_success)
				{
					if (locked_save_game.IsValid())
					{
						locked_save_game->SetCanModify(true);
					}

					_SaveGame = loaded_save_game;
					_ActiveSaveGameSlotIndex = _slot_index;
				}

				_IsAsyncLoading = false;
				SetSaveGameCanModify(true);

				_OnAsyncLoadGameFinished.Broadcast(load_success);
			}
		)
	);
}

bool USaveGameSubsystem::SaveGameSlot(int32 _slot_index)
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

	if (IsValidSaveGameSlotIndex(_slot_index) == false)
	{
		TRACE_WARNING(TEXT("Invalid SaveGame Slot Index : %d"), _slot_index);
		return false;
	}

	const bool save_success = UGameplayStatics::SaveGameToSlot(_SaveGame, MakeSaveGameSlotName(_slot_index), 0);
	if (save_success)
	{
		_ActiveSaveGameSlotIndex = _slot_index;
	}

	return save_success;
}

void USaveGameSubsystem::AsyncSaveGameSlot(int32 _slot_index)
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

	if (IsValidSaveGameSlotIndex(_slot_index) == false)
	{
		TRACE_WARNING(TEXT("Invalid SaveGame Slot Index : %d"), _slot_index);
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

	return UGameplayStatics::AsyncSaveGameToSlot(_SaveGame, MakeSaveGameSlotName(_slot_index), 0,
		FAsyncSaveGameToSlotDelegate::CreateWeakLambda(this,
			[this, _slot_index](const FString& _slot_name, const int32 _user_index, bool _is_success)
			{
				if (_IsDeinitializing)
					return;

				if (_is_success)
				{
					_ActiveSaveGameSlotIndex = _slot_index;
				}

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

bool USaveGameSubsystem::ResetGameSlot(int32 _slot_index)
{
	if (_IsAsyncSaving || _IsAsyncLoading)
	{
		TRACE_WARNING(TEXT("Async save/load is in progress."));
		return false;
	}

	if (IsValidSaveGameSlotIndex(_slot_index) == false)
	{
		TRACE_WARNING(TEXT("Invalid SaveGame Slot Index : %d"), _slot_index);
		return false;
	}

	TObjectPtr<UCustomSaveGame> previous_save_game = _SaveGame;
	const int32 previous_active_save_game_slot_index = _ActiveSaveGameSlotIndex;

	auto new_save_game = CreateConfiguredSaveGameObject();
	if (IsInvalid(new_save_game))
	{
		TRACE_ERROR(TEXT("SaveGame Create Failed"));
		return false;
	}

	_SaveGame = new_save_game;
	_ActiveSaveGameSlotIndex = _slot_index;
	SetSaveGameCanModify(true);

	if (SaveGameSlot(_slot_index))
	{
		return true;
	}

	_SaveGame = previous_save_game;
	_ActiveSaveGameSlotIndex = previous_active_save_game_slot_index;
	SetSaveGameCanModify(true);
	return false;
}

bool USaveGameSubsystem::CreateNewGameSlot(int32 _slot_index)
{
	if (_IsAsyncSaving || _IsAsyncLoading)
	{
		TRACE_WARNING(TEXT("Async save/load is in progress."));
		return false;
	}

	if (IsInvalid(_SaveGameSlotRegistry))
	{
		_SaveGameSlotRegistry = LoadOrCreateSaveGameSlotRegistry();
	}

	if (IsInvalid(_SaveGameSlotRegistry))
	{
		TRACE_ERROR(TEXT("SaveGame Slot Registry Is Invalid"));
		return false;
	}

	TObjectPtr<UCustomSaveGame> previous_save_game = _SaveGame;
	const int32 previous_last_save_game_slot_index = _SaveGameSlotRegistry->_LastSaveGameSlotIndex;
	const int32 previous_active_save_game_slot_index = _ActiveSaveGameSlotIndex;

	const int32 max_save_game_slot_count = GetConfiguredMaxSaveGameSlotCount();
	if (GetSaveGameSlotCount() >= max_save_game_slot_count)
	{
		TRACE_WARNING(TEXT("SaveGame Slot Count Limit Exceeded : %d"), max_save_game_slot_count);
		return false;
	}

	if (_slot_index < 0)
	{
		_slot_index = AllocateSaveGameSlotIndex();
	}

	if (IsInConfiguredSaveGameSlotRange(_slot_index) == false)
	{
		TRACE_WARNING(TEXT("Invalid SaveGame Slot Index : %d"), _slot_index);
		return false;
	}

	if (_SaveGameSlotRegistry->_SaveGameSlotMap.Contains(_slot_index))
	{
		TRACE_WARNING(TEXT("SaveGame Slot Already Exists : %d"), _slot_index);
		_SaveGameSlotRegistry->_LastSaveGameSlotIndex = previous_last_save_game_slot_index;
		return false;
	}

	auto new_save_game = CreateConfiguredSaveGameObject();
	if (IsInvalid(new_save_game))
	{
		TRACE_ERROR(TEXT("SaveGame Create Failed"));
		_SaveGameSlotRegistry->_LastSaveGameSlotIndex = previous_last_save_game_slot_index;
		return false;
	}

	_SaveGameSlotRegistry->_LastSaveGameSlotIndex = FMath::Max(_SaveGameSlotRegistry->_LastSaveGameSlotIndex, _slot_index);
	_SaveGameSlotRegistry->_SaveGameSlotMap.Add(_slot_index, MakeConfiguredSaveGameSlotName(_slot_index));

	_SaveGame = new_save_game;
	_ActiveSaveGameSlotIndex = _slot_index;
	SetSaveGameCanModify(true);

	const bool save_game_success = SaveGameSlot(_slot_index);
	const bool save_registry_success = SaveSaveGameSlotRegistry();
	if (save_game_success == false || save_registry_success == false)
	{
		UGameplayStatics::DeleteGameInSlot(MakeConfiguredSaveGameSlotName(_slot_index), 0);
		_SaveGameSlotRegistry->_SaveGameSlotMap.Remove(_slot_index);
		_SaveGameSlotRegistry->_LastSaveGameSlotIndex = previous_last_save_game_slot_index;
		_SaveGame = previous_save_game;
		_ActiveSaveGameSlotIndex = previous_active_save_game_slot_index;
		SetSaveGameCanModify(true);
		SaveSaveGameSlotRegistry();
		return false;
	}

	return true;
}

bool USaveGameSubsystem::DeleteSaveGameSlot(int32 _slot_index)
{
	if (_IsAsyncSaving || _IsAsyncLoading)
	{
		TRACE_WARNING(TEXT("Async save/load is in progress."));
		return false;
	}

	if (IsValidSaveGameSlotIndex(_slot_index) == false)
	{
		TRACE_WARNING(TEXT("Invalid SaveGame Slot Index : %d"), _slot_index);
		return false;
	}

	const FString slot_name = MakeSaveGameSlotName(_slot_index);
	const bool delete_success = UGameplayStatics::DoesSaveGameExist(slot_name, 0) == false || UGameplayStatics::DeleteGameInSlot(slot_name, 0);
	if (delete_success == false)
	{
		return false;
	}

	_SaveGameSlotRegistry->_SaveGameSlotMap.Remove(_slot_index);
	const bool save_registry_success = SaveSaveGameSlotRegistry();
	if (save_registry_success == false)
	{
		_SaveGameSlotRegistry->_SaveGameSlotMap.Add(_slot_index, slot_name);
		return false;
	}

	if (_ActiveSaveGameSlotIndex == _slot_index)
	{
		_SaveGame = CreateConfiguredSaveGameObject();
		_ActiveSaveGameSlotIndex = INDEX_NONE;
		SetSaveGameCanModify(true);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

UCustomSaveGame* USaveGameHelper::GetCurrentSaveGame_Internal(const UObject* _world_ctx)
{
	auto save_game_subsys = UCommonUtils::GetGameInstanceSubsystem<USaveGameSubsystem>(_world_ctx);
	if (IsValid(save_game_subsys))
	{
		return save_game_subsys->GetCurrentSaveGame();
	}

	return nullptr;
}

UCustomSaveGame* USaveGameHelper::GetSaveGameFromSlot_Internal(const UObject* _world_ctx, int32 _slot_index)
{
	auto save_game_subsys = UCommonUtils::GetGameInstanceSubsystem<USaveGameSubsystem>(_world_ctx);
	if (IsValid(save_game_subsys))
	{
		return save_game_subsys->GetSaveGameFromSlot(_slot_index);
	}

	return nullptr;
}

int32 USaveGameHelper::GetSaveGameSlotCount(const UObject* _world_ctx)
{
	auto save_game_subsys = UCommonUtils::GetGameInstanceSubsystem<USaveGameSubsystem>(_world_ctx);
	if (IsValid(save_game_subsys))
	{
		return save_game_subsys->GetSaveGameSlotCount();
	}

	return 0;
}

TArray<int32> USaveGameHelper::GetSaveGameSlotIndices(const UObject* _world_ctx)
{
	auto save_game_subsys = UCommonUtils::GetGameInstanceSubsystem<USaveGameSubsystem>(_world_ctx);
	if (IsValid(save_game_subsys))
	{
		return save_game_subsys->GetSaveGameSlotIndices();
	}

	return TArray<int32>();
}

FString USaveGameHelper::GetSaveGameSlotName(const UObject* _world_ctx, int32 _slot_index)
{
	auto save_game_subsys = UCommonUtils::GetGameInstanceSubsystem<USaveGameSubsystem>(_world_ctx);
	if (IsValid(save_game_subsys))
	{
		return save_game_subsys->GetSaveGameSlotName(_slot_index);
	}

	return FString();
}

int32 USaveGameHelper::GetActiveSaveGameSlotIndex(const UObject* _world_ctx)
{
	auto save_game_subsys = UCommonUtils::GetGameInstanceSubsystem<USaveGameSubsystem>(_world_ctx);
	if (IsValid(save_game_subsys))
	{
		return save_game_subsys->GetActiveSaveGameSlotIndex();
	}

	return INDEX_NONE;
}

bool USaveGameHelper::DoesSaveGameExist(const UObject* _world_ctx, int32 _slot_index)
{
	auto save_game_subsys = UCommonUtils::GetGameInstanceSubsystem<USaveGameSubsystem>(_world_ctx);
	if (IsValid(save_game_subsys))
	{
		return save_game_subsys->DoesSaveGameExist(_slot_index);
	}

	return false;
}

bool USaveGameHelper::LoadGameSlot(const UObject* _world_ctx, int32 _slot_index)
{
	auto save_game_subsys = UCommonUtils::GetGameInstanceSubsystem<USaveGameSubsystem>(_world_ctx);
	if (IsValid(save_game_subsys))
	{
		return save_game_subsys->LoadGameSlot(_slot_index);
	}

	return false;
}

void USaveGameHelper::AsyncLoadGameSlot(const UObject* _world_ctx, int32 _slot_index)
{
	auto save_game_subsys = UCommonUtils::GetGameInstanceSubsystem<USaveGameSubsystem>(_world_ctx);
	if (IsInvalid(save_game_subsys))
		return;

	save_game_subsys->AsyncLoadGameSlot(_slot_index);
}

bool USaveGameHelper::SaveGameSlot(const UObject* _world_ctx, int32 _slot_index)
{
	auto save_game_subsys = UCommonUtils::GetGameInstanceSubsystem<USaveGameSubsystem>(_world_ctx);
	if (IsValid(save_game_subsys))
	{
		return save_game_subsys->SaveGameSlot(_slot_index);
	}

	return false;
}

void USaveGameHelper::AsyncSaveGameSlot(const UObject* _world_ctx, int32 _slot_index)
{
	auto save_game_subsys = UCommonUtils::GetGameInstanceSubsystem<USaveGameSubsystem>(_world_ctx);
	if (IsInvalid(save_game_subsys))
		return;

	save_game_subsys->AsyncSaveGameSlot(_slot_index);
}

bool USaveGameHelper::ResetGameSlot(const UObject* _world_ctx, int32 _slot_index)
{
	auto save_game_subsys = UCommonUtils::GetGameInstanceSubsystem<USaveGameSubsystem>(_world_ctx);
	if (IsValid(save_game_subsys))
	{
		return save_game_subsys->ResetGameSlot(_slot_index);
	}

	return false;
}

bool USaveGameHelper::CreateNewGameSlot(const UObject* _world_ctx, int32 _slot_index)
{
	auto save_game_subsys = UCommonUtils::GetGameInstanceSubsystem<USaveGameSubsystem>(_world_ctx);
	if (IsValid(save_game_subsys))
	{
		return save_game_subsys->CreateNewGameSlot(_slot_index);
	}

	return false;
}

bool USaveGameHelper::DeleteSaveGameSlot(const UObject* _world_ctx, int32 _slot_index)
{
	auto save_game_subsys = UCommonUtils::GetGameInstanceSubsystem<USaveGameSubsystem>(_world_ctx);
	if (IsValid(save_game_subsys))
	{
		return save_game_subsys->DeleteSaveGameSlot(_slot_index);
	}

	return false;
}
