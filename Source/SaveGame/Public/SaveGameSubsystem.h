// Copyright (c) 2026 장윤제. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CustomSaveGame.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SaveGameSubsystem.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDM_OnAsyncLoadGameStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDM_OnAsyncLoadGameFinished, bool, _load_success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDM_OnAsyncSaveGameStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDM_OnAsyncSaveGameFinished, bool, _save_success);

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

	UPROPERTY()
	bool _IsAsyncSaving = false;

	UPROPERTY()
	bool _IsAsyncLoading = false;

	UPROPERTY(Transient)
	bool _IsDeinitializing = false;

protected:
	void SetSaveGameCanModify(bool _can_modify);

public:
	UPROPERTY(BlueprintAssignable, Category = "SaveGame|Event")
	FDM_OnAsyncLoadGameStarted _OnAsyncLoadGameStarted;

	UPROPERTY(BlueprintAssignable, Category = "SaveGame|Event")
	FDM_OnAsyncLoadGameFinished _OnAsyncLoadGameFinished;

	UPROPERTY(BlueprintAssignable, Category = "SaveGame|Event")
	FDM_OnAsyncSaveGameStarted _OnAsyncSaveGameStarted;

	UPROPERTY(BlueprintAssignable, Category = "SaveGame|Event")
	FDM_OnAsyncSaveGameFinished _OnAsyncSaveGameFinished;

public:
	virtual void Initialize(FSubsystemCollectionBase& _collection) override;
	virtual void Deinitialize() override;

public:
	/*
	 * Load 정책:
	 * - 로드 성공 시 현재 메모리 SaveGame을 로드된 객체로 교체한다.
	 * - 로드 실패(슬롯 없음, 클래스 불일치, 파일 손상 등) 시 기존 메모리 SaveGame은 유지한다.
	 * - 호출자는 반환값 또는 완료 델리게이트로 실패를 판단하고 신규 게임/재시도/UI 갱신을 결정한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	bool LoadGame();
	
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void AsyncLoadGame();

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	bool SaveGame();

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void AsyncSaveGame();

	/*
	 * 현재 메모리 SaveGame 데이터만 초기화한다.
	 * 디스크 슬롯에 반영하려면 ResetGame() 이후 SaveGame()을 호출해야 한다.
	 * 슬롯 파일 자체를 삭제하려면 에디터 툴바의 Delete Save Slot 또는 DeleteGameInSlot을 사용한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void ResetGame();

public:
	UFUNCTION(BlueprintPure, Category = "SaveGame")
	bool IsAsyncLoading() const { return _IsAsyncLoading; }

	UFUNCTION(BlueprintPure, Category = "SaveGame")
	bool IsAsyncSaving() const { return _IsAsyncSaving; }

	UFUNCTION(BlueprintPure, Category = "SaveGame")
	bool CanModifySaveGame() const { return _IsAsyncLoading == false && _IsAsyncSaving == false; }

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
