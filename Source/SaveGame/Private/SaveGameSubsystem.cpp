// Copyright (c) 2026 장윤제. All rights reserved.


#include "SaveGameSubsystem.h"
#include "CommonUtils.h"
#include "Kismet/GameplayStatics.h"
#include "SaveGameDeveloperSettings.h"

void USaveGameSubsystem::Initialize(FSubsystemCollectionBase& _collection)
{
	Super::Initialize(_collection);

	const auto settings = GetDefault<USaveGameDeveloperSettings>();
	if (IsInvalid(settings))
		return;

	_SaveGame = Cast<UCustomSaveGame>(UGameplayStatics::LoadGameFromSlot(settings->_SaveGameSlotName, 0));

	if (IsInvalid(_SaveGame))
	{
		if (settings->_SaveGameClass.IsNull())
		{
			TRACE_ERROR(TEXT("Dev setting에서 _SaveGameClass 를 설정하세요."));
			return;
		}

		_SaveGame = Cast<UCustomSaveGame>(UGameplayStatics::CreateSaveGameObject(settings->_SaveGameClass.LoadSynchronous()));
	}

	if (IsInvalid(_SaveGame))
	{
		TRACE_ERROR(TEXT("SaveGame 초기화 실패"));
		return;
	}
}

void USaveGameSubsystem::SaveGameToSlot()
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

void USaveGameHelper::SaveGameToSlot(const UObject* _world_ctx)
{
	auto save_game_subsys = UCommonUtils::GetGameInstanceSubsystem<USaveGameSubsystem>(_world_ctx);
	if (IsInvalid(save_game_subsys))
		return;

	save_game_subsys->SaveGameToSlot();
}
