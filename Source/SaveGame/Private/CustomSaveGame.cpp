// Copyright (c) 2026 장윤제. All rights reserved.


#include "CustomSaveGame.h"
#include "CommonUtils.h"
#include "Internationalization/BreakIterator.h"


bool UCustomSaveGame::CanSetKey(FName _key, ESaveDataType _target_type) const
{
	if (_key.IsNone())
		return false;

	const auto found_type_ptr = _KeyTypeMap.Find(_key);
	return IsInvalid(found_type_ptr) || *found_type_ptr == _target_type;
}

void UCustomSaveGame::RegisterKeyType(FName _key, ESaveDataType _type)
{
	_KeyTypeMap.Add(_key, _type);
}

void UCustomSaveGame::UnregisterKeyTypeIfUnused(FName _key)
{
	const ESaveDataType key_type = GetKeyType(_key);
	switch (key_type)
	{
	case ESaveDataType::Bool:
		if (_BoolDataMap.Contains(_key))
			return;
		break;
	case ESaveDataType::Int:
		if (_IntDataMap.Contains(_key))
			return;
		break;
	case ESaveDataType::Float:
		if (_FloatDataMap.Contains(_key))
			return;
		break;
	case ESaveDataType::String:
		if (_StringDataMap.Contains(_key))
			return;
		break;
	default:
		break;
	}

	_KeyTypeMap.Remove(_key);
}

bool UCustomSaveGame::RemoveKey(FName _key)
{
	bool is_removed = false;

	const ESaveDataType key_type = GetKeyType(_key);
	switch (key_type)
	{
	case ESaveDataType::Bool:
		is_removed = _BoolDataMap.Remove(_key) > 0;
		break;
	case ESaveDataType::Int:
		is_removed = _IntDataMap.Remove(_key) > 0;
		break;
	case ESaveDataType::Float:
		is_removed = _FloatDataMap.Remove(_key) > 0;
		break;
	case ESaveDataType::String:
		is_removed = _StringDataMap.Remove(_key) > 0;
		break;
	default:
		break;
	}
	
	if (is_removed)
	{
		UnregisterKeyTypeIfUnused(_key);
	}

	return is_removed;
}

bool UCustomSaveGame::ContainsKey(FName _key) const
{
	return _KeyTypeMap.Contains(_key);
}

ESaveDataType UCustomSaveGame::GetKeyType(FName _key) const
{
	return  _KeyTypeMap.FindRef(_key);
}

void UCustomSaveGame::ClearData()
{
	_KeyTypeMap.Empty();

	_BoolDataMap.Empty();
	_IntDataMap.Empty();
	_FloatDataMap.Empty();
	_StringDataMap.Empty();
}

bool UCustomSaveGame::IsEmpty() const
{
	return _KeyTypeMap.IsEmpty() &&
				_BoolDataMap.IsEmpty() &&
				_IntDataMap.IsEmpty() &&
				_FloatDataMap.IsEmpty() &&
				_StringDataMap.IsEmpty();
}

bool UCustomSaveGame::SaveBoolData(FName _key, bool _value)
{
	if (CanSetKey(_key, ESaveDataType::Bool) == false)
	{
		TRACE_WARNING(TEXT("저장 실패! Key : %s"), *_key.ToString());
		return false;
	}

	RegisterKeyType(_key, ESaveDataType::Bool);
	_BoolDataMap.Add(_key, _value);

	return true;
}

bool UCustomSaveGame::FindSavedBoolData(FName _key, bool& _out_value) const
{
	const ESaveDataType key_type = GetKeyType(_key);
	if (key_type != ESaveDataType::Bool)
	{
		if (key_type != ESaveDataType::NA)
		{
			TRACE_WARNING(TEXT("잘못된 키 타입입니다. Key : %s, Type : %s"), *_key.ToString(), *TEnumToString(key_type));
		}

		return false;
	}

	const auto data_ptr = _BoolDataMap.Find(_key);
	if (IsInvalid(data_ptr))
		return false;

	_out_value = *data_ptr;
	return true;
}

bool UCustomSaveGame::SaveIntData(FName _key, int32 _value)
{
	if (CanSetKey(_key, ESaveDataType::Int) == false)
	{
		TRACE_WARNING(TEXT("저장 실패! Key : %s"), *_key.ToString());
		return false;
	}

	RegisterKeyType(_key, ESaveDataType::Int);
	_IntDataMap.Add(_key, _value);

	return true;
}

bool UCustomSaveGame::FindSavedIntData(FName _key, int32& _out_value) const
{
	const ESaveDataType key_type = GetKeyType(_key);
	if (key_type != ESaveDataType::Int)
	{
		if (key_type != ESaveDataType::NA)
		{
			TRACE_WARNING(TEXT("잘못된 키 타입입니다. Key : %s, Type : %s"), *_key.ToString(), *TEnumToString(key_type));
		}

		return false;
	}

	const auto data_ptr = _IntDataMap.Find(_key);
	if (IsInvalid(data_ptr))
		return false;

	_out_value = *data_ptr;
	return true;
}

bool UCustomSaveGame::SaveFloatData(FName _key, float _value)
{
	if (CanSetKey(_key, ESaveDataType::Float) == false)
	{
		TRACE_WARNING(TEXT("저장 실패! Key : %s"), *_key.ToString());
		return false;
	}

	RegisterKeyType(_key, ESaveDataType::Float);
	_FloatDataMap.Add(_key, _value);

	return true;
}

bool UCustomSaveGame::FindSavedFloatData(FName _key, float& _out_value) const
{
	const ESaveDataType key_type = GetKeyType(_key);
	if (key_type != ESaveDataType::Float)
	{
		if (key_type != ESaveDataType::NA)
		{
			TRACE_WARNING(TEXT("잘못된 키 타입입니다. Key : %s, Type : %s"), *_key.ToString(), *TEnumToString(key_type));
		}

		return false;
	}

	const auto data_ptr = _FloatDataMap.Find(_key);
	if (IsInvalid(data_ptr))
		return false;

	_out_value = *data_ptr;
	return true;
}

bool UCustomSaveGame::SaveStringData(FName _key, const FString& _value)
{
	if (CanSetKey(_key, ESaveDataType::String) == false)
	{
		TRACE_WARNING(TEXT("저장 실패! Key : %s"), *_key.ToString());
		return false;
	}

	RegisterKeyType(_key, ESaveDataType::String);
	_StringDataMap.Add(_key, _value);

	return true;
}

bool UCustomSaveGame::FindSavedStringData(FName _key, FString& _out_value) const
{
	const ESaveDataType key_type = GetKeyType(_key);
	if (key_type != ESaveDataType::String)
	{
		if(key_type != ESaveDataType::NA)
		{
			TRACE_WARNING(TEXT("잘못된 키 타입입니다. Key : %s, Type : %s"), *_key.ToString(), *TEnumToString(key_type));
		}

		return false;
	}

	const auto data_ptr = _StringDataMap.Find(_key);
	if (IsInvalid(data_ptr))
		return false;

	_out_value = *data_ptr;
	return true;
}

