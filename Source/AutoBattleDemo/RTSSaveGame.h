#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "RTSCoreTypes.h"
#include "RTSSaveGame.generated.h"

UCLASS()
class AUTOBATTLEDEMO_API URTSSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, Category = "SaveData")
        int32 PlayerGold;

    UPROPERTY(VisibleAnywhere, Category = "SaveData")
        int32 PlayerElixir;

    UPROPERTY(VisibleAnywhere, Category = "SaveData")
        TArray<FUnitSaveData> PlayerArmy;

    UPROPERTY(VisibleAnywhere, Category = "SaveData")
        TArray<FBuildingSaveData> SavedBuildings;

    // 教程进度也要存，防止下次进游戏又从头教一遍
    UPROPERTY(VisibleAnywhere, Category = "SaveData")
        bool bTutorialFinished;

    // 如果你想存教程具体到了哪一步
    UPROPERTY(VisibleAnywhere, Category = "SaveData")
        ETutorialStep CurrentTutorialStep;

    // 存档槽名 (默认 "Slot1")
    UPROPERTY(VisibleAnywhere, Category = "Basic")
        FString SaveSlotName;

    UPROPERTY(VisibleAnywhere, Category = "Basic")
        int32 UserIndex;

    URTSSaveGame()
    {
        SaveSlotName = TEXT("SaveSlot1");
        UserIndex = 0;
        bTutorialFinished = false;
        CurrentTutorialStep = ETutorialStep::None;
    }
};