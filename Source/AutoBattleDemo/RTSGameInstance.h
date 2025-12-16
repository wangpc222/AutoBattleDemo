#pragma once
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
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
        int32 MaxPopulation = 20;
};