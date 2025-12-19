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
    Soldier UMETA(DisplayName = "Soldier"),
    Barbarian  UMETA(DisplayName = "Barbarian"),  // 野蛮人
    Archer     UMETA(DisplayName = "Archer"),     // 弓箭手
    Giant      UMETA(DisplayName = "Giant"),      // 巨人
    Bomber     UMETA(DisplayName = "Bomber")      // 炸弹人
};

// 建筑类型枚举
UENUM(BlueprintType)
enum class EBuildingType : uint8
{
    Resource      UMETA(DisplayName = "Resource"),      // 资源建筑（矿场）
    Defense       UMETA(DisplayName = "Defense"),       // 防御建筑（炮塔）
    Headquarters  UMETA(DisplayName = "Headquarters"),  // 大本营
    Wall          UMETA(DisplayName = "Wall"),          // 墙
    Other         UMETA(DisplayName = "Other")          // 其他
};