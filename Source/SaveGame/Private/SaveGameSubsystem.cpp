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

	int32 new_slot_index = FMath::Max(0, _SaveGameSlotRegistry->_LastSaveGameSlotIndex) + 1;
	while (_SaveGameSlotRegistry->_SaveGameSlotMap.Contains(new_slot_index))
	{
		++new_slot_index;
	}

	_SaveGameSlotRegistry->_LastSaveGameSlotIndex = new_slot_index;
	return new_slot_index;
}

int32 USaveGameSubsystem::GetSaveGameSlotCount() const
{
	if (IsInvalid(_SaveGameSlotRegistry))
	{
		return 0;
	}

	return _SaveGameSlotRegistry->_SaveGameSlotMap.Num();
}

TArray<int32> USaveGameSubsystem::GetSaveGameSlotIndices() const
{
	TArray<int32> slot_indices;
	if (IsValid(_SaveGameSlotRegistry))
	{
		_SaveGameSlotRegistry->_SaveGameSlotMap.GetKeys(slot_indices);
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
	return IsValid(_SaveGameSlotRegistry) && _SaveGameSlotRegistry->_SaveGameSlotMap.Contains(_slot_index);
}

bool USaveGameSubsystem::DoesSaveGameExist(int32 _slot_index) const
{
	if (IsValidSaveGameSlotIndex(_slot_index) == false)
		return false;

	return UGameplayStatics::DoesSaveGameExist(MakeSaveGameSlotName(_slot_index), 0);
}

bool USaveGameSubsystem::LoadGame()
{
	return LoadGameSlot(_ActiveSaveGameSlotIndex);
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

void USaveGameSubsystem::AsyncLoadGame()
{
	AsyncLoadGameSlot(_ActiveSaveGameSlotIndex);
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

	if (IsValidSaveGameSlotIndex(_ActiveSaveGameSlotIndex) == false)
	{
		TRACE_WARNING(TEXT("Invalid SaveGame Slot Index : %d"), _ActiveSaveGameSlotIndex);
		return false;
	}

	return UGameplayStatics::SaveGameToSlot(_SaveGame, MakeSaveGameSlotName(_ActiveSaveGameSlotIndex), 0);
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

	if (IsValidSaveGameSlotIndex(_ActiveSaveGameSlotIndex) == false)
	{
		TRACE_WARNING(TEXT("Invalid SaveGame Slot Index : %d"), _ActiveSaveGameSlotIndex);
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

	return UGameplayStatics::AsyncSaveGameToSlot(_SaveGame, MakeSaveGameSlotName(_ActiveSaveGameSlotIndex), 0,
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

bool USaveGameSubsystem::CreateNewGame(int32 _slot_index)
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

	if (_slot_index <= 0)
	{
		_slot_index = AllocateSaveGameSlotIndex();
	}

	if (_slot_index <= 0)
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

	const bool save_game_success = SaveGame();
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

bool USaveGameHelper::CreateNewGame(const UObject* _world_ctx, int32 _slot_index)
{
	auto save_game_subsys = UCommonUtils::GetGameInstanceSubsystem<USaveGameSubsystem>(_world_ctx);
	if (IsValid(save_game_subsys))
	{
		return save_game_subsys->CreateNewGame(_slot_index);
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
