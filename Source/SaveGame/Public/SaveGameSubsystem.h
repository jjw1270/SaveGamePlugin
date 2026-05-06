// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CustomSaveGame.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SaveGameSubsystem.generated.h"

DECLARE_DYNAMIC_DELEGATE_OneParam(FD_OnLoadGameFinished, bool, _load_success);
DECLARE_DYNAMIC_DELEGATE_OneParam(FD_OnSaveGameFinished, bool, _save_success);

/*
 * 오직 하나의 슬롯만 존재.
 */
UCLASS()
class SAVEGAME_API USaveGameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
protected:
	UPROPERTY()
	TObjectPtr<UCustomSaveGame> _SaveGame = nullptr;

	UPROPERTY()
	TObjectPtr<class UWidgetBase> _AsyncSaveGameWidget = nullptr;

public:
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	bool LoadGame();
	
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void AsyncLoadGame(FD_OnLoadGameFinished _on_load_game_finished_event);

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	bool SaveGame();

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void AsyncSaveGame(FD_OnSaveGameFinished _on_save_game_finished_event);

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void ResetGame();

public:
	UFUNCTION(BlueprintPure, Category = "SaveGame")
	UCustomSaveGame* GetSaveGame() const { return _SaveGame; }

};

template<typename T>
concept CONCEPT_SaveGame = TIsDerivedFrom<T, UCustomSaveGame>::IsDerived;

UCLASS()
class SAVEGAME_API USaveGameHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	template<CONCEPT_SaveGame T = UCustomSaveGame>
	static T* GetSaveGame(const UObject* _world_ctx)
	{
		return Cast<T>(GetSaveGame_Internal(_world_ctx));
	}

	UFUNCTION(BlueprintPure, meta = (WorldContext = "_world_ctx"))
	static UCustomSaveGame* GetSaveGame_Editable(const UObject* _world_ctx)
	{
		return GetSaveGame_Internal(_world_ctx);
	}

	UFUNCTION(BlueprintPure, meta = (WorldContext = "_world_ctx"))
	static const UCustomSaveGame* GetSaveGame_ReadOnly(const UObject* _world_ctx)
	{
		return GetSaveGame_Internal(_world_ctx);
	}

private:
	static UCustomSaveGame* GetSaveGame_Internal(const UObject* _world_ctx);

public:
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "_world_ctx"), Category = "SaveGame")
	static void SaveGame(const UObject* _world_ctx);
};
