# SaveGame User Guide

이 문서는 SaveGame 플러그인을 실제 프로젝트에서 사용하는 사람을 위한 작업 가이드다. 코드 구조 설명보다 **무엇을 만들고, 어디에 설정하고, 언제 로드/저장하고, 어떤 정책을 지켜야 하는지**에 초점을 둔다.

---

## 1. 기본 개념

SaveGame 플러그인은 다음 단위로 저장 시스템을 구성한다.

| 이름 | 설명 |
| --- | --- |
| CustomSaveGame | key-value 데이터와 프로젝트별 저장 필드를 담는 SaveGame 객체 |
| SaveGameSubsystem | GameInstance 단위로 현재 SaveGame 객체를 보관하고 저장/로드를 수행하는 Subsystem |
| SaveGameDeveloperSettings | SaveGame 클래스, 슬롯명, async save UI를 지정하는 Project Settings |
| SaveGameHelper | Blueprint/C++에서 현재 SaveGame과 저장 함수를 쉽게 호출하는 helper |
| Delete Save Slot | 에디터 툴바에서 디스크 저장 슬롯을 삭제하는 버튼 |

가장 흔한 흐름은 다음과 같다.

```text
SaveGame 클래스 만들기
-> Project Settings에 SaveGameClass/SlotName 설정
-> 게임 시작 또는 로비에서 LoadGame/AsyncLoadGame 호출
-> 플레이 중 SaveGame 데이터 수정
-> 필요한 시점에 SaveGame/AsyncSaveGame 호출
```

---

## 2. 처음 설정하기

### 2.1 SaveGame 클래스 만들기

프로젝트용 SaveGame 클래스는 `UCustomSaveGame`을 상속한다.

예:

```cpp
#pragma once

#include "CoreMinimal.h"
#include "CustomSaveGame.h"
#include "MySaveGame.generated.h"

UCLASS()
class MYPROJECT_API UMySaveGame : public UCustomSaveGame
{
	GENERATED_BODY()

public:
	virtual void ClearData() override;
	virtual bool IsEmpty() const override;

private:
	UPROPERTY()
	int32 _Progress = 0;

public:
	void SaveProgress(int32 _value);
	int32 GetProgress() const { return _Progress; }
};
```

```cpp
#include "MySaveGame.h"

void UMySaveGame::ClearData()
{
	Super::ClearData();

	if (CanModify() == false)
		return;

	_Progress = 0;
}

bool UMySaveGame::IsEmpty() const
{
	return Super::IsEmpty() && _Progress == 0;
}

void UMySaveGame::SaveProgress(int32 _value)
{
	if (CanModify() == false)
		return;

	_Progress = _value;
}
```

중요:

- 자체 필드를 수정하는 함수는 `CanModify()`를 확인한다.
- `ClearData()`를 override하면 `Super::ClearData()`도 호출한다.
- `IsEmpty()`를 override하면 base 데이터와 자체 필드를 모두 반영한다.

### 2.2 Project Settings 설정

Project Settings에서 SaveGame 설정을 찾고 다음을 지정한다.

| 설정 | 필수 | 설명 |
| --- | --- | --- |
| SaveGameClass | 필수 | `UCustomSaveGame` 파생 클래스 |
| SaveGameSlotName | 권장 | 저장 슬롯명. 비워두지 않는 것을 권장 |
| AsyncSaveGameWidgetClass | 선택 | async save 중 표시할 `UWidgetBase` 위젯 Blueprint |

`DefaultGame.ini` 예:

```ini
[/Script/SaveGame.SaveGameDeveloperSettings]
_SaveGameClass=/Script/MyProject.MySaveGame
_SaveGameSlotName=SaveGameSlot
_AsyncSaveGameWidgetClass=/Script/UMG.WidgetBlueprintGeneratedClass'/Game/UI/UI_Saving.UI_Saving_C'
```

주의:

- `_SaveGameClass`가 비어 있으면 subsystem 초기화 시 SaveGame 객체를 만들 수 없다.
- async save UI는 `UWidgetBase` 파생 위젯이어야 한다.

---

## 3. 게임 시작 시 로드하기

SaveGameSubsystem은 초기화 시 새 빈 SaveGame 객체를 만든다. 디스크 슬롯을 자동 로드하지 않는다.

따라서 저장된 데이터를 쓰려면 프로젝트 흐름에서 직접 로드해야 한다.

### 3.1 동기 로드

간단한 프로젝트나 로비 진입 직전에 즉시 로드해도 되는 경우:

```cpp
USaveGameSubsystem* SaveSubsystem = GetGameInstance()->GetSubsystem<USaveGameSubsystem>();
if (IsValid(SaveSubsystem))
{
	const bool bLoaded = SaveSubsystem->LoadGame();
	if (bLoaded == false)
	{
		// 슬롯 없음 또는 로드 실패. 기존 메모리 SaveGame은 유지됨.
	}
}
```

### 3.2 비동기 로드

로비 UI에서 버튼 상태를 갱신해야 한다면 async load delegate를 사용한다.

```cpp
void UMyLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	USaveGameSubsystem* SaveSubsystem = GetGameInstance()->GetSubsystem<USaveGameSubsystem>();
	if (IsValid(SaveSubsystem))
	{
		SaveSubsystem->_OnAsyncLoadGameFinished.AddDynamic(this, &UMyLobbyWidget::OnAsyncLoadGameFinished);
		SaveSubsystem->AsyncLoadGame();
	}
}

void UMyLobbyWidget::NativeDestruct()
{
	USaveGameSubsystem* SaveSubsystem = GetGameInstance()->GetSubsystem<USaveGameSubsystem>();
	if (IsValid(SaveSubsystem))
	{
		SaveSubsystem->_OnAsyncLoadGameFinished.RemoveAll(this);
	}

	Super::NativeDestruct();
}
```

```cpp
void UMyLobbyWidget::OnAsyncLoadGameFinished(bool _load_success)
{
	// _load_success가 false여도 기존 메모리 SaveGame은 유지된다.
	// 저장된 진행 상태가 있는지 SaveGame 내용을 직접 확인한다.
}
```

---

## 4. SaveGame 가져오기

### 4.1 C++에서 프로젝트 SaveGame 타입으로 가져오기

```cpp
#include "SaveGameSubsystem.h"
#include "MySaveGame.h"

UMySaveGame* SaveGame = USaveGameHelper::GetSaveGame<UMySaveGame>(this);
if (IsValid(SaveGame))
{
	SaveGame->SaveProgress(3);
}
```

### 4.2 Blueprint에서 가져오기

Blueprint에서는 다음 노드를 사용한다.

- `Get Save Game Editable`
- `Get Save Game Read Only`

반환값을 프로젝트 SaveGame Blueprint/C++ 클래스 타입으로 cast해서 사용한다.

### 4.3 읽기 전용 사용 권장

UI 표시처럼 데이터를 읽기만 하는 곳에서는 read-only helper를 사용하는 것이 의도를 드러내기 좋다.

```cpp
const UCustomSaveGame* SaveGame = USaveGameHelper::GetSaveGame_ReadOnly(this);
```

---

## 5. key-value 데이터 저장하기

`UCustomSaveGame`은 간단한 key-value 저장을 제공한다.

### 5.1 저장

```cpp
UCustomSaveGame* SaveGame = USaveGameHelper::GetSaveGame(this);
if (IsValid(SaveGame))
{
	SaveGame->SaveStringData(TEXT("Nickname"), TEXT("Player"));
	SaveGame->SaveBoolData(TEXT("HasSeenIntro"), true);
	SaveGame->SaveIntData(TEXT("ClearCount"), 1);
	SaveGame->SaveFloatData(TEXT("PlayTime"), 120.5f);
}
```

### 5.2 조회

```cpp
FString Nickname;
if (SaveGame->FindSavedStringData(TEXT("Nickname"), Nickname))
{
	// Nickname 사용
}
```

실패 시 기본값을 유지하고 싶다면 호출 전에 세팅한다.

```cpp
FString Nickname = TEXT("Unknown");
SaveGame->FindSavedStringData(TEXT("Nickname"), Nickname);
```

### 5.3 타입 충돌 주의

한 key는 하나의 타입만 가질 수 있다.

```cpp
SaveGame->SaveStringData(TEXT("Value"), TEXT("Text"));
SaveGame->SaveIntData(TEXT("Value"), 10); // 실패
```

타입을 바꾸고 싶으면 먼저 제거한다.

```cpp
SaveGame->RemoveKey(TEXT("Value"));
SaveGame->SaveIntData(TEXT("Value"), 10);
```

---

## 6. 프로젝트 전용 데이터 저장하기

key-value 데이터로 충분하지 않은 구조체나 참조 값은 `UCustomSaveGame` 파생 클래스에 직접 `UPROPERTY`로 저장한다.

예:

```cpp
UPROPERTY()
FStoryFlowRef _StoryFlowRef;
```

저장 함수:

```cpp
void UMySaveGame::SaveStoryFlowRef(const FStoryFlowRef& _value)
{
	if (CanModify() == false)
		return;

	_StoryFlowRef = _value;
}
```

주의:

- `UPROPERTY()`가 아니면 Unreal SaveGame 직렬화 대상에서 빠질 수 있다.
- async save/load 중에는 `CanModify()`가 false일 수 있다.
- 파생 필드는 base class가 자동으로 지워주지 않으므로 `ClearData()`에서 직접 초기화한다.

---

## 7. 저장하기

### 7.1 동기 저장

```cpp
USaveGameHelper::SaveGame(this);
```

또는:

```cpp
USaveGameSubsystem* SaveSubsystem = GetGameInstance()->GetSubsystem<USaveGameSubsystem>();
if (IsValid(SaveSubsystem))
{
	SaveSubsystem->SaveGame();
}
```

동기 저장은 즉시 결과를 bool로 받을 수 있다.

```cpp
const bool bSaved = SaveSubsystem->SaveGame();
```

### 7.2 비동기 저장

```cpp
SaveSubsystem->AsyncSaveGame();
```

완료 이벤트:

```cpp
SaveSubsystem->_OnAsyncSaveGameFinished.AddDynamic(this, &UMyObject::OnAsyncSaveGameFinished);
```

async save 중에는 다음이 자동으로 처리된다.

1. `_IsAsyncSaving = true`
2. SaveGame 수정 잠금
3. `_OnAsyncSaveGameStarted` broadcast
4. async save UI 표시
5. 저장 완료 callback
6. 수정 잠금 해제
7. async save UI 제거
8. `_OnAsyncSaveGameFinished` broadcast

---

## 8. Reset과 Delete Save Slot 차이

### 8.1 ResetGame

`ResetGame()`은 현재 메모리 SaveGame 데이터만 초기화한다.

```cpp
SaveSubsystem->ResetGame();
```

디스크 슬롯에 반영하려면 저장을 다시 호출한다.

```cpp
SaveSubsystem->ResetGame();
SaveSubsystem->SaveGame();
```

사용 예:

- 새 게임 시작 전 메모리 진행 상태 초기화
- 초기화한 상태를 다시 저장해 기존 슬롯을 덮어쓰기

### 8.2 Delete Save Slot

에디터 툴바의 `Delete Save Slot`은 디스크 슬롯 파일을 삭제한다.

주의:

- 메모리 `_SaveGame`은 그대로 남는다.
- PIE 중 이미 로드된 데이터까지 초기화하지 않는다.
- 런타임에서 슬롯을 삭제하고 싶다면 `UGameplayStatics::DeleteGameInSlot`을 직접 호출한다.

---

## 9. async save UI 만들기

async save 중 표시할 UI가 필요하면 `UWidgetBase` 파생 Widget Blueprint를 만든다.

권장:

1. Widget Blueprint 생성
2. 부모 클래스를 `WidgetBase` 또는 프로젝트의 `UWidgetBase` 파생 클래스로 설정
3. 저장 중 표시할 텍스트/애니메이션 구성
4. Project Settings의 `AsyncSaveGameWidgetClass`에 지정

동작:

- async save 시작 시 최초 1회 생성
- `AddToViewport(999)`로 최상단 표시
- async save 완료 시 `RemoveFromParent`
- subsystem 종료 시 남아 있으면 제거

주의:

- async load에는 별도 UI 표시 로직이 없다.
- 저장 위젯 클래스가 비어 있으면 경고만 출력하고 저장은 계속 진행한다.

---

## 10. 동시 실행 규칙

SaveGame 플러그인은 async 작업 중 다른 저장/로드/리셋을 막는다.

| 진행 중 | 차단되는 작업 |
| --- | --- |
| Async Save | Load, Save, Reset, Async Load, Async Save |
| Async Load | Load, Save, Reset, Async Load, Async Save |

차단 시:

- `TRACE_WARNING` 로그 출력
- 동기 함수는 `false` 반환 또는 return
- async 함수는 완료 delegate에 `false` broadcast

운영 팁:

```cpp
if (SaveSubsystem->CanModifySaveGame())
{
	// 저장 데이터 수정 가능
}
```

---

## 11. 추천 작업 순서

새 프로젝트에 SaveGame 플러그인을 붙일 때는 이 순서가 안전하다.

1. `UCustomSaveGame` 파생 클래스를 만든다.
2. 프로젝트 전용 저장 필드는 `UPROPERTY()`로 선언한다.
3. 저장 필드 수정 함수에는 `CanModify()` 가드를 넣는다.
4. `ClearData()`와 `IsEmpty()`를 override한다.
5. Project Settings에서 `SaveGameClass`와 `SaveGameSlotName`을 지정한다.
6. 로비 또는 GameInstance 흐름에서 `LoadGame()` / `AsyncLoadGame()`을 호출한다.
7. UI는 로드 완료 후 저장된 진행 상태를 확인해 버튼 상태를 갱신한다.
8. 플레이 중 SaveGame 데이터를 수정한다.
9. 중요한 진행 지점에서 `SaveGame()` 또는 `AsyncSaveGame()`을 호출한다.
10. 새 게임 시작 시 `ResetGame()` 후 필요한 초기 진행 상태를 저장한다.
11. 에디터에서 테스트 슬롯을 지울 때는 `Delete Save Slot`을 사용한다.

---

## 12. 운영 팁

- 저장 슬롯이 없을 수 있으므로 load 실패는 정상 흐름으로 처리한다.
- `LoadGame()` 실패 시 메모리 SaveGame은 유지되므로, 실제 진행 가능 여부는 SaveGame 내용으로 판단한다.
- `FindSaved...` 실패 시 out 값은 유지된다. 이전 값이 남으면 안 되는 UI에서는 호출 전에 값을 비운다.
- key 이름은 문자열 literal을 흩뿌리기보다 상수로 관리하는 것이 안전하다.
- 타입을 바꿀 수 있는 key 설계는 피한다. 필요하면 새 key를 만든다.
- async save/load 중에는 SaveGame을 수정하지 않는다.
- 파생 SaveGame 필드는 `CanModify()`를 확인한다.
- `ResetGame()`과 `Delete Save Slot`은 다르다. 메모리 초기화와 디스크 삭제를 구분한다.
- 종료 중 async 완료 이벤트는 보장하지 않는다. 종료 정리는 `Deinitialize()`가 처리한다.
