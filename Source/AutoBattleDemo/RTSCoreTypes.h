// RTSCoreTypes.h
#pragma once
#include "CoreMinimal.h"
#include "RTSCoreTypes.generated.h"

// 游戏阶段状态
UENUM(BlueprintType)
enum class EGameState : uint8
{
    Preparation, // 备战阶段：玩家买兵、摆阵
    Battle,      // 战斗阶段：自动寻路、攻击
    Victory,     // 结算：胜利
    Defeat       // 结算：失败
};

// 队伍阵营
UENUM(BlueprintType)
enum class ETeam : uint8
{
    Player,
    Enemy
};

// 兵种类型枚举
UENUM(BlueprintType)
enum class EUnitType : uint8
{
    //Soldier    UMETA(DisplayName = "Soldier"),
    Barbarian  UMETA(DisplayName = "Barbarian"),  // 野蛮人
    Archer     UMETA(DisplayName = "Archer"),     // 弓箭手
    Giant      UMETA(DisplayName = "Giant"),      // 巨人
    Bomber     UMETA(DisplayName = "Bomber")      // 炸弹人
};

// 建筑类型枚举
UENUM(BlueprintType)
enum class EBuildingType : uint8
{
    None          UMETA(DisplayName = "None"),
    //Resource      UMETA(DisplayName = "Resource"),      // 资源建筑（矿场）
    GoldMine      UMETA(DisplayName = "Gold Mine"),     // 金矿
    ElixirPump    UMETA(DisplayName = "Elixir Pump"),   // 圣水收集器
    Defense       UMETA(DisplayName = "Defense"),       // 防御建筑（炮塔）
    Headquarters  UMETA(DisplayName = "Headquarters"),  // 大本营
    Wall          UMETA(DisplayName = "Wall"),          // 墙
    Barracks      UMETA(DisplayName = "Barracks"),      // 兵营
    Other         UMETA(DisplayName = "Other")          // 其他
};

// 用于保存士兵数据的结构体
USTRUCT(BlueprintType)
struct FUnitSaveData
{
    GENERATED_BODY()

        UPROPERTY(BlueprintReadWrite)
        EUnitType UnitType;

    UPROPERTY(BlueprintReadWrite)
        int32 GridX;

    UPROPERTY(BlueprintReadWrite)
        int32 GridY;
};


// 资源类型 (用于通用扣费逻辑)
UENUM(BlueprintType)
enum class EResourceType : uint8
{
    None,
    Gold,
    Elixir,
    Gem // 预留
};

// 建筑存档数据 (类似于 FUnitSaveData)
// 用于保存玩家基地的布局，哪怕关卡重启也能读回来
USTRUCT(BlueprintType)
struct FBuildingSaveData
{
    GENERATED_BODY()

        UPROPERTY(BlueprintReadWrite)
        EBuildingType BuildingType;

    UPROPERTY(BlueprintReadWrite)
        int32 GridX;

    UPROPERTY(BlueprintReadWrite)
        int32 GridY;

    UPROPERTY(BlueprintReadWrite)
        int32 Level; // 建筑等级

    // 兵营里的库存
    UPROPERTY(BlueprintReadWrite)
        TArray<EUnitType> StoredUnitTypes;
};

UENUM(BlueprintType)
enum class ETutorialStep : uint8
{
    None,
    Welcome,          // 欢迎：试着移动一下镜头
    BuildGoldMine,    // 经济：造金矿 (体验赚钱)
    BuildBarracks,    // 科技：造兵营 (体验解锁人口)
    UpgradeBarracks,  // 进阶：升级兵营 (体验升级系统)
    TrainBarbarian,   // 兵力：造野蛮人 (体验花钱)
    MoveUnit,         // 操作：移动士兵 (体验操作)
    ClickStart,       // 战斗：出征
    Completed
};