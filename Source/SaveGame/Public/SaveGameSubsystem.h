// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CustomSaveGame.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SaveGameSubsystem.generated.h"

/*
 * save game 하나만 존재.
 */
UCLASS()
class SAVEGAME_API USaveGameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
protected:
	UPROPERTY()
	TObjectPtr<UCustomSaveGame> _SaveGame = nullptr;

public:
	bool NewGame();

	bool CanLoadGame() const;
	bool LoadGame();

	bool SaveGame();

public:
	UCustomSaveGame* GetSaveGame() const { return _SaveGame; }

	void ClearSaveGameData();

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
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "_world_ctx"))
	static void SaveGame(const UObject* _world_ctx);
};
