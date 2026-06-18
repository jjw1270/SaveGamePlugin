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

UCLASS()
class SAVEGAME_API USaveGameSlotRegistry : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TMap<int32, FString> _SaveGameSlotMap;

	UPROPERTY()
	int32 _LastSaveGameSlotIndex = 0;
};

/*
 * 생성된 슬롯 목록을 registry SaveGame에 보관한다.
 * 메모리 SaveGame 객체는 활성 슬롯 하나만 보관한다.
 */
UCLASS()
class SAVEGAME_API USaveGameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

protected:
	UPROPERTY()
	TObjectPtr<UCustomSaveGame> _SaveGame = nullptr;

	UPROPERTY()
	TObjectPtr<USaveGameSlotRegistry> _SaveGameSlotRegistry = nullptr;

	UPROPERTY()
	int32 _ActiveSaveGameSlotIndex = INDEX_NONE;

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
	UCustomSaveGame* CreateConfiguredSaveGameObject() const;
	USaveGameSlotRegistry* CreateSaveGameSlotRegistryObject() const;
	USaveGameSlotRegistry* LoadOrCreateSaveGameSlotRegistry() const;
	bool SaveSaveGameSlotRegistry() const;
	int32 AllocateSaveGameSlotIndex();
	FString MakeSaveGameSlotName(int32 _slot_index) const;

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
	bool LoadGameSlot(int32 _slot_index);

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void AsyncLoadGame();

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void AsyncLoadGameSlot(int32 _slot_index);

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	bool SaveGame();

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void AsyncSaveGame();

	/*
	 * 현재 메모리 SaveGame 데이터만 초기화한다.
	 * 디스크 슬롯에 반영하려면 ResetGame() 이후 SaveGame()을 호출해야 한다.
	 * 슬롯 파일 자체를 삭제하려면 DeleteSaveGameSlot(), 에디터 툴바의 Delete Save Slots 또는 DeleteGameInSlot을 사용한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void ResetGame();

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	bool CreateNewGame(int32 _slot_index = -1);

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	bool DeleteSaveGameSlot(int32 _slot_index);

public:
	UFUNCTION(BlueprintPure, Category = "SaveGame")
	int32 GetSaveGameSlotCount() const;

	UFUNCTION(BlueprintPure, Category = "SaveGame")
	TArray<int32> GetSaveGameSlotIndices() const;

	UFUNCTION(BlueprintPure, Category = "SaveGame")
	FString GetSaveGameSlotName(int32 _slot_index) const;

	UFUNCTION(BlueprintPure, Category = "SaveGame")
	int32 GetActiveSaveGameSlotIndex() const { return _ActiveSaveGameSlotIndex; }

	UFUNCTION(BlueprintPure, Category = "SaveGame")
	bool IsValidSaveGameSlotIndex(int32 _slot_index) const;

	UFUNCTION(BlueprintPure, Category = "SaveGame")
	bool DoesSaveGameExist(int32 _slot_index) const;

	UFUNCTION(BlueprintPure, Category = "SaveGame")
	bool IsAsyncLoading() const { return _IsAsyncLoading; }

	UFUNCTION(BlueprintPure, Category = "SaveGame")
	bool IsAsyncSaving() const { return _IsAsyncSaving; }

	UFUNCTION(BlueprintPure, Category = "SaveGame")
	bool CanModifySaveGame() const { return _IsAsyncLoading == false && _IsAsyncSaving == false; }

	UFUNCTION(BlueprintPure, Category = "SaveGame")
	UCustomSaveGame* GetSaveGame() const { return _SaveGame; }

public:
	static FString GetConfiguredSaveGameSlotNamePrefix();
	static FString MakeConfiguredSaveGameSlotName(int32 _slot_index);
	static FString MakeConfiguredSaveGameSlotRegistryName();
	static USaveGameSlotRegistry* LoadConfiguredSaveGameSlotRegistry();
	static bool SaveConfiguredSaveGameSlotRegistry(USaveGameSlotRegistry* _slot_registry);

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

	UFUNCTION(BlueprintPure, meta = (WorldContext = "_world_ctx"), Category = "SaveGame")
	static int32 GetSaveGameSlotCount(const UObject* _world_ctx);

	UFUNCTION(BlueprintPure, meta = (WorldContext = "_world_ctx"), Category = "SaveGame")
	static TArray<int32> GetSaveGameSlotIndices(const UObject* _world_ctx);

	UFUNCTION(BlueprintPure, meta = (WorldContext = "_world_ctx"), Category = "SaveGame")
	static FString GetSaveGameSlotName(const UObject* _world_ctx, int32 _slot_index);

	UFUNCTION(BlueprintPure, meta = (WorldContext = "_world_ctx"), Category = "SaveGame")
	static int32 GetActiveSaveGameSlotIndex(const UObject* _world_ctx);

	UFUNCTION(BlueprintPure, meta = (WorldContext = "_world_ctx"), Category = "SaveGame")
	static bool DoesSaveGameExist(const UObject* _world_ctx, int32 _slot_index);

private:
	static UCustomSaveGame* GetSaveGame_Internal(const UObject* _world_ctx);

public:
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "_world_ctx"), Category = "SaveGame")
	static void SaveGame(const UObject* _world_ctx);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "_world_ctx"), Category = "SaveGame")
	static bool LoadGameSlot(const UObject* _world_ctx, int32 _slot_index);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "_world_ctx"), Category = "SaveGame")
	static void AsyncLoadGameSlot(const UObject* _world_ctx, int32 _slot_index);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "_world_ctx"), Category = "SaveGame")
	static bool CreateNewGame(const UObject* _world_ctx, int32 _slot_index = -1);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "_world_ctx"), Category = "SaveGame")
	static bool DeleteSaveGameSlot(const UObject* _world_ctx, int32 _slot_index);
};
