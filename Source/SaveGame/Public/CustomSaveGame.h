// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "CustomSaveGame.generated.h"

UENUM()
enum class ESaveDataType : uint8
{
	NA,
	Bool,
	Int,
	Float,
	String,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDM_OnSaveGameBoolDataChanged, FName, _key, bool, _value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDM_OnSaveGameIntDataChanged, FName, _key, int32, _value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDM_OnSaveGameFloatDataChanged, FName, _key, float, _value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDM_OnSaveGameStringDataChanged, FName, _key, const FString&, _value);

UCLASS()
class SAVEGAME_API UCustomSaveGame : public USaveGame
{
	GENERATED_BODY()

private:
	UPROPERTY()
	TMap<FName, ESaveDataType> _KeyTypeMap;

protected:
	UPROPERTY()
	TMap<FName, bool> _BoolDataMap;

	UPROPERTY()
	TMap<FName, int32> _IntDataMap;

	UPROPERTY()
	TMap<FName, float> _FloatDataMap;
	
	UPROPERTY()
	TMap<FName, FString> _StringDataMap;

protected:
	bool CanSetKey(FName _key, ESaveDataType _target_type) const;
	void RegisterKeyType(FName _key, ESaveDataType _type);
	void UnregisterKeyTypeIfUnused(FName _key);

public:
	bool RemoveKey(FName _key);
	bool ContainsKey(FName _key) const;
	ESaveDataType GetKeyType(FName _key) const;

	virtual void ClearData();

public:
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	bool SaveBoolData(FName _key, bool _value);
	UFUNCTION(BlueprintPure, Category = "SaveGame")
	bool FindSavedBoolData(FName _key, bool& _out_value) const;

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	bool SaveIntData(FName _key, int32 _value);
	UFUNCTION(BlueprintPure, Category = "SaveGame")
	bool FindSavedIntData(FName _key, int32& _out_value) const;

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	bool SaveFloatData(FName _key, float _value);
	UFUNCTION(BlueprintPure, Category = "SaveGame")
	bool FindSavedFloatData(FName _key, float& _out_value) const;

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	bool SaveStringData(FName _key, const FString& _value);
	UFUNCTION(BlueprintPure, Category = "SaveGame")
	bool FindSavedStringData(FName _key, FString& _out_value) const;
};
