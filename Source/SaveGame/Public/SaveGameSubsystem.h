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
	static bool IsInConfiguredSaveGameSlotRange(int32 _slot_index);

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
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	bool LoadGameSlot(int32 _slot_index);

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void AsyncLoadGameSlot(int32 _slot_index);

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	bool SaveGameSlot(int32 _slot_index);

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void AsyncSaveGameSlot(int32 _slot_index);

	/*
	 * 지정 슬롯의 SaveGame 데이터를 초기화하고 디스크에 저장한다.
	 * 슬롯 파일 자체를 삭제하려면 DeleteSaveGameSlot(), 에디터 툴바의 Delete Save Slots 또는 DeleteGameInSlot을 사용한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	bool ResetGameSlot(int32 _slot_index);

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	bool CreateNewGameSlot(int32 _slot_index = -1);

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
	UCustomSaveGame* GetCurrentSaveGame() const { return _SaveGame; }

	UFUNCTION(BlueprintPure, Category = "SaveGame")
	UCustomSaveGame* GetSaveGameFromSlot(int32 _slot_index) const;

public:
	static FString GetConfiguredSaveGameSlotNamePrefix();
	static int32 GetConfiguredMaxSaveGameSlotCount();
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
	UFUNCTION(BlueprintPure, meta = (WorldContext = "_world_ctx"))
	static UCustomSaveGame* GetCurrentSaveGame_Editable(const UObject* _world_ctx)
	{
		return GetCurrentSaveGame_Internal(_world_ctx);
	}

	UFUNCTION(BlueprintPure, meta = (WorldContext = "_world_ctx"))
	static const UCustomSaveGame* GetCurrentSaveGame_ReadOnly(const UObject* _world_ctx)
	{
		return GetCurrentSaveGame_Internal(_world_ctx);
	}

	UFUNCTION(BlueprintPure, meta = (WorldContext = "_world_ctx"), Category = "SaveGame")
	static UCustomSaveGame* GetSaveGameFromSlot_Editable(const UObject* _world_ctx, int32 _slot_index)
	{
		return GetSaveGameFromSlot_Internal(_world_ctx, _slot_index);
	}

	UFUNCTION(BlueprintPure, meta = (WorldContext = "_world_ctx"), Category = "SaveGame")
	static const UCustomSaveGame* GetSaveGameFromSlot_ReadOnly(const UObject* _world_ctx, int32 _slot_index)
	{
		return GetSaveGameFromSlot_Internal(_world_ctx, _slot_index);
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
	static UCustomSaveGame* GetCurrentSaveGame_Internal(const UObject* _world_ctx);
	static UCustomSaveGame* GetSaveGameFromSlot_Internal(const UObject* _world_ctx, int32 _slot_index);

public:
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "_world_ctx"), Category = "SaveGame")
	static bool LoadGameSlot(const UObject* _world_ctx, int32 _slot_index);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "_world_ctx"), Category = "SaveGame")
	static void AsyncLoadGameSlot(const UObject* _world_ctx, int32 _slot_index);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "_world_ctx"), Category = "SaveGame")
	static bool SaveGameSlot(const UObject* _world_ctx, int32 _slot_index);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "_world_ctx"), Category = "SaveGame")
	static void AsyncSaveGameSlot(const UObject* _world_ctx, int32 _slot_index);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "_world_ctx"), Category = "SaveGame")
	static bool ResetGameSlot(const UObject* _world_ctx, int32 _slot_index);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "_world_ctx"), Category = "SaveGame")
	static bool CreateNewGameSlot(const UObject* _world_ctx, int32 _slot_index = -1);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "_world_ctx"), Category = "SaveGame")
	static bool DeleteSaveGameSlot(const UObject* _world_ctx, int32 _slot_index);
};
