#pragma once
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "RTSCoreTypes.h"
#include "RTSGameInstance.generated.h"

UCLASS()
class AUTOBATTLEDEMO_API URTSGameInstance : public UGameInstance
{
    GENERATED_BODY()
public:

    // 当前关卡索引 (0, 1, 2)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Data")
        int32 CurrentLevelIndex;

    // 玩家持有的金币 (跨关卡保存)，金币 (造防御塔)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Data")
        int32 PlayerGold = 5000;

    // 圣水 (造兵)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Data")
        int32 PlayerElixir = 5000;

    // 人口
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Data")
        int32 CurrentPopulation = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Data")
        int32 MaxPopulation = 0; // 初始为0，全靠 HQ 和 兵营 提供

    // 玩家的军队阵容
    UPROPERTY(BlueprintReadWrite, Category = "SaveData")
        TArray<FUnitSaveData> PlayerArmy;

    // 保存基地建筑布局
    UPROPERTY(BlueprintReadWrite, Category = "SaveData")
        TArray<FBuildingSaveData> SavedBuildings;

    // 标记：是否已经初始化过基地？(防止第一次运行把预设的删了)
    bool bHasSavedBase = false;

    // 重置所有玩家数据 (新游戏时调用)
    UFUNCTION(BlueprintCallable)
        void ResetData()
    {
        PlayerGold = 5000;   // 恢复初始钱
        PlayerElixir = 5000;
        CurrentPopulation = 0;
        MaxPopulation = 0;
        PlayerArmy.Empty();      // 清空兵
        SavedBuildings.Empty();  // 清空建筑
        bHasSavedBase = false;
    }
};