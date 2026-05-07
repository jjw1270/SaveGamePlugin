# SaveGame

`SaveGame`는 Unreal Engine 5 프로젝트에서 **단일 저장 슬롯**을 기준으로 SaveGame 객체 생성, 동기/비동기 저장·로드, Blueprint 친화적인 key-value 저장, 저장 중 수정 잠금, 에디터 슬롯 삭제 버튼을 제공하는 저장 플러그인이다.

이 문서는 `Plugins/SaveGamePlugin`의 **현재 코드 기준**으로 유지되는 기술 README다. 실제 사용 절차 중심 문서는 [UserGuide.md](./UserGuide.md)를 참고한다.

---

## 핵심 요약

- 플러그인은 프로젝트당 하나의 SaveGame 슬롯을 기본 전제로 한다.
- SaveGame 클래스와 슬롯명은 `USaveGameDeveloperSettings`에서 설정한다.
- `USaveGameSubsystem`은 `UGameInstanceSubsystem`으로 런타임 SaveGame 객체를 보관하고 저장/로드 API를 제공한다.
- `UCustomSaveGame`은 `Bool`, `Int`, `Float`, `String` key-value 데이터를 저장한다.
- 한 key는 하나의 타입만 가질 수 있다.
- 비동기 저장/로드 중에는 SaveGame 수정이 잠긴다.
- Load 실패 시 기존 메모리 SaveGame을 유지한다.
- `ResetGame()`은 메모리 데이터만 초기화한다. 디스크 반영은 `SaveGame()` 호출이 필요하다.
- 에디터 툴바의 `Delete Save Slot`은 디스크 저장 슬롯만 삭제한다.

---

## 모듈 구성

플러그인은 `CommonLibrary`, `CustomUI` 플러그인에 의존한다.

| 모듈 | 타입 | 역할 |
| --- | --- | --- |
| `SaveGame` | Runtime | SaveGame 데이터 타입, DeveloperSettings, GameInstanceSubsystem, Blueprint/C++ helper |
| `SaveGameEditor` | Editor | 에디터 메시지 로그 등록, 툴바 `Delete Save Slot` 버튼, Slate 아이콘 스타일 |

---

## 주요 런타임 타입

### `UCustomSaveGame`

파일: `Source/SaveGame/Public/CustomSaveGame.h`

`USaveGame`을 상속한 기본 저장 객체다.

지원 데이터 타입:

| 타입 | 저장 함수 | 조회 함수 |
| --- | --- | --- |
| `bool` | `SaveBoolData` | `FindSavedBoolData` |
| `int32` | `SaveIntData` | `FindSavedIntData` |
| `float` | `SaveFloatData` | `FindSavedFloatData` |
| `FString` | `SaveStringData` | `FindSavedStringData` |

내부 데이터:

| 멤버 | 의미 |
| --- | --- |
| `_KeyTypeMap` | key별 저장 타입 기록 |
| `_BoolDataMap` | bool 데이터 |
| `_IntDataMap` | int32 데이터 |
| `_FloatDataMap` | float 데이터 |
| `_StringDataMap` | FString 데이터 |
| `_CanModify` | 저장/로드 중 수정을 막기 위한 transient 플래그 |

주요 규칙:

- `NAME_None` key는 저장할 수 없다.
- 이미 `Bool`로 등록된 key에 `String` 값을 저장할 수 없다.
- `RemoveKey`는 key 타입에 맞는 map에서 데이터를 제거한다.
- `ClearData`는 모든 key-value map과 type map을 비운다.
- `FindSaved...` 함수는 실패 시 out parameter를 변경하지 않는다.

### `ESaveDataType`

파일: `Source/SaveGame/Public/CustomSaveGame.h`

key에 저장된 값의 타입을 나타낸다.

| 값 | 의미 |
| --- | --- |
| `NA` | key 없음 또는 타입 미지정 |
| `Bool` | bool 데이터 |
| `Int` | int32 데이터 |
| `Float` | float 데이터 |
| `String` | FString 데이터 |

### `USaveGameDeveloperSettings`

파일: `Source/SaveGame/Public/SaveGameDeveloperSettings.h`

Project Settings에 저장되는 전역 SaveGame 설정이다.

| 설정 | 의미 |
| --- | --- |
| `_SaveGameClass` | 생성/로드할 `UCustomSaveGame` 파생 클래스 |
| `_SaveGameSlotName` | `UGameplayStatics` 저장 슬롯명. 기본값은 `SaveGameSlot` |
| `_AsyncSaveGameWidgetClass` | async save 중 화면 최상단에 표시할 `UWidgetBase` 위젯 클래스 |

현재 프로젝트 예시:

```ini
[/Script/SaveGame.SaveGameDeveloperSettings]
_SaveGameClass=/Script/RulesHorror.RulesHorrorSaveGame
_AsyncSaveGameWidgetClass=/Script/UMG.WidgetBlueprintGeneratedClass'/Game/UI/Common/UI_SavingGame_BP.UI_SavingGame_BP_C'
```

### `USaveGameSubsystem`

파일: `Source/SaveGame/Public/SaveGameSubsystem.h`

`UGameInstanceSubsystem` 기반 런타임 저장 관리자다.

주요 데이터:

| 멤버 | 의미 |
| --- | --- |
| `_SaveGame` | 현재 메모리에 보관 중인 SaveGame 객체 |
| `_AsyncSaveGameWidget` | async save 중 표시하는 위젯 인스턴스 |
| `_IsAsyncSaving` | async save 진행 여부 |
| `_IsAsyncLoading` | async load 진행 여부 |
| `_IsDeinitializing` | subsystem 종료 중 뒤늦은 async callback 처리 방지 |

주요 함수:

| 함수 | 설명 |
| --- | --- |
| `LoadGame()` | 슬롯에서 동기 로드. 성공 시 `_SaveGame` 교체 |
| `AsyncLoadGame()` | 슬롯에서 비동기 로드. 완료 시 delegate broadcast |
| `SaveGame()` | 현재 `_SaveGame`을 슬롯에 동기 저장 |
| `AsyncSaveGame()` | 현재 `_SaveGame`을 슬롯에 비동기 저장 |
| `ResetGame()` | 현재 메모리 SaveGame 데이터만 초기화 |
| `GetSaveGame()` | 현재 SaveGame 객체 반환 |
| `CanModifySaveGame()` | async 저장/로드가 아니면 true |

이벤트:

| 이벤트 | 시점 |
| --- | --- |
| `_OnAsyncLoadGameStarted` | async load 시작 직후 |
| `_OnAsyncLoadGameFinished(bool)` | async load 완료 또는 즉시 실패 |
| `_OnAsyncSaveGameStarted` | async save 시작 직후 |
| `_OnAsyncSaveGameFinished(bool)` | async save 완료 또는 즉시 실패 |

### `USaveGameHelper`

파일: `Source/SaveGame/Public/SaveGameSubsystem.h`

Blueprint/C++에서 SaveGame 객체와 저장 API를 쉽게 호출하기 위한 `UBlueprintFunctionLibrary`다.

| 함수 | 설명 |
| --- | --- |
| `GetSaveGame<T>(WorldContext)` | C++ 템플릿 helper. 원하는 `UCustomSaveGame` 파생 타입으로 cast |
| `GetSaveGame_Editable(WorldContext)` | Blueprint용 수정 가능 SaveGame 반환 |
| `GetSaveGame_ReadOnly(WorldContext)` | Blueprint용 read-only SaveGame 반환 |
| `SaveGame(WorldContext)` | 현재 SaveGame 동기 저장 |

---

## 저장 데이터 정책

### key 타입 정책

`UCustomSaveGame`은 key별 타입을 `_KeyTypeMap`에 기록한다.

예:

```cpp
SaveGame->SaveStringData(TEXT("Nickname"), TEXT("Player"));
SaveGame->SaveIntData(TEXT("Nickname"), 10); // 실패. 이미 String key
```

타입이 다른 key 접근은 실패하며, `TRACE_WARNING` 로그를 남긴다.

### Find 정책

`FindSaved...` 계열 함수는 `TMap::Find`와 유사하게 동작한다.

- 성공: `true` 반환, `_out_value` 갱신
- 실패: `false` 반환, `_out_value` 유지

따라서 호출자는 기본값이 필요하면 호출 전에 직접 세팅해야 한다.

```cpp
FString Nickname;
SaveGame->FindSavedStringData(TEXT("Nickname"), Nickname);
```

또는:

```cpp
FString Nickname = TEXT("DefaultName");
if (SaveGame->FindSavedStringData(TEXT("Nickname"), Nickname) == false)
{
	// Nickname은 여전히 DefaultName
}
```

### 수정 잠금 정책

`AsyncSaveGame()` 또는 `AsyncLoadGame()`이 시작되면 `_SaveGame->SetCanModify(false)`가 호출된다.

- 기본 key-value 저장 함수는 `CanModifySaveGameData`를 통해 수정을 차단한다.
- 파생 SaveGame 클래스가 자체 필드를 수정한다면 직접 `CanModify()`를 확인해야 한다.
- async 완료 후에는 `SetCanModify(true)`가 호출된다.

파생 클래스 권장 패턴:

```cpp
void UMySaveGame::SaveProgress(int32 _value)
{
	if (CanModify() == false)
		return;

	_Progress = _value;
}
```

---

## 저장 / 로드 흐름

### 초기화

`USaveGameSubsystem::Initialize`에서 다음 순서로 메모리 SaveGame을 만든다.

1. `USaveGameDeveloperSettings` 조회
2. `_SaveGameClass` 동기 로드
3. `UGameplayStatics::CreateSaveGameObject`로 새 SaveGame 객체 생성
4. `_SaveGame`에 보관

중요:

- 초기화는 새 빈 SaveGame 객체를 만든다.
- 디스크 슬롯을 자동 로드하지 않는다.
- 저장된 슬롯을 쓰려면 프로젝트 초기 흐름에서 `LoadGame()` 또는 `AsyncLoadGame()`을 호출해야 한다.

### Load 정책

`LoadGame()` / `AsyncLoadGame()`은 로드 성공 시 현재 메모리 SaveGame을 로드된 객체로 교체한다.

로드 실패 조건 예:

- 슬롯 파일 없음
- 파일 손상
- 클래스 불일치
- `USaveGame` cast 실패

로드 실패 시 정책:

- 기존 메모리 SaveGame은 유지한다.
- 신규 게임 상태로 강제 초기화하지 않는다.
- 호출자는 반환값 또는 완료 delegate로 실패를 판단한다.

### Save 정책

`SaveGame()` / `AsyncSaveGame()`은 현재 `_SaveGame` 객체를 설정된 슬롯명에 저장한다.

동기 저장:

```cpp
SaveGameSubsystem->SaveGame();
```

비동기 저장:

```cpp
SaveGameSubsystem->AsyncSaveGame();
```

비동기 저장 중 `_AsyncSaveGameWidgetClass`가 설정되어 있으면 해당 위젯을 생성해 `AddToViewport(999)`로 표시하고, 완료 후 제거한다.

### 동시 실행 정책

다음 중 하나가 진행 중이면 다른 저장/로드/리셋 작업은 실패한다.

- `_IsAsyncSaving == true`
- `_IsAsyncLoading == true`

대상 함수:

- `LoadGame()`
- `SaveGame()`
- `ResetGame()`
- `AsyncLoadGame()`
- `AsyncSaveGame()`

### Deinitialize 정리

Subsystem 종료 시:

1. `_IsDeinitializing = true`
2. SaveGame 수정 가능 상태 복구
3. async 상태 플래그 초기화
4. async save 위젯 제거
5. 뒤늦은 async callback은 즉시 return

종료 중에는 완료 이벤트 broadcast를 기대하지 않는 것이 안전하다.

---

## 에디터 기능

### Delete Save Slot 버튼

`SaveGameEditor` 모듈은 Level Editor toolbar에 `Delete Save Slot` 버튼을 추가한다.

동작:

```cpp
UGameplayStatics::DeleteGameInSlot(settings->_SaveGameSlotName, 0);
```

주의:

- 디스크 슬롯 파일만 삭제한다.
- 이미 메모리에 로드된 `_SaveGame`은 자동 초기화하지 않는다.
- PIE 중 메모리 SaveGame까지 초기화하려면 런타임에서 `ResetGame()`을 호출해야 한다.

### 메시지 로그

`SaveGameEditor`는 `SaveGameLog` 이름으로 `MessageLog` listing을 등록한다.

---

## Content / Resources

플러그인은 `CanContainContent=true`다.

현재 포함 자원:

| 경로 | 내용 |
| --- | --- |
| `Content/UI_SampleAsyncSaveGame_BP.uasset` | async save 표시용 샘플 UI |
| `Resources/Icon128.png` | 플러그인 아이콘 |
| `Resources/DeleteSaveSlot_40.png` | 에디터 툴바 버튼 아이콘 |

---

## 현재 한계 / 주의사항

- 저장 슬롯은 하나만 사용하는 구조다.
- UserIndex는 항상 `0`을 사용한다.
- 초기화 시 디스크 슬롯을 자동 로드하지 않는다.
- `ResetGame()`은 디스크 슬롯을 삭제하거나 저장하지 않는다.
- `FindSaved...` 실패 시 out parameter를 초기화하지 않는다.
- key-value 저장 타입은 `Bool`, `Int`, `Float`, `String`만 제공한다.
- 파생 SaveGame 필드는 파생 클래스가 직접 `CanModify()` 정책을 지켜야 한다.
- async save/load 작업 자체는 취소하지 않는다. subsystem 종료 시 callback 후처리만 무시한다.

---

## 추천 코드 읽기 순서

1. `SaveGame.Build.cs`
2. `CustomSaveGame.h/.cpp`
3. `SaveGameDeveloperSettings.h/.cpp`
4. `SaveGameSubsystem.h/.cpp`
5. `SaveGame.h/.cpp`
6. `SaveGameEditor.Build.cs`
7. `SaveGameEditor.cpp`
8. `SaveGameEditorStyle.cpp`

---

## 관련 문서

- [User Guide](./UserGuide.md) — 플러그인을 사용하는 디자이너/개발자용 작업 가이드
