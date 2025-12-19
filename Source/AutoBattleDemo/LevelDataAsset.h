// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Sound/SoundBase.h"
#include "LevelDataAsset.generated.h"

// 资源类型枚举（金币/圣水等）
UENUM(BlueprintType)
enum class EResourceType : uint8
{
    RT_Gold UMETA(DisplayName = "金币"),
    RT_Elixir UMETA(DisplayName = "圣水"),
    //RT_DarkElixir UMETA(DisplayName = "暗黑重油"),
    RT_None UMETA(DisplayName = "无资源")
};

USTRUCT(BlueprintType)
struct FLevelGridConfig
{
    GENERATED_BODY()

        // 格子在网格中的X坐标
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Config")
        int32 GridX;

    // 格子在网格中的Y坐标
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Config")
        int32 GridY;

    // 该格子是否为阻挡状态（建筑/障碍物）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Config")
        bool bIsBlocked;

    // 该格子上生成的建筑类（如防御塔、基地等）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Config")
        TSubclassOf<class ABaseBuilding> BuildingClass;

    // --- 新增：建筑等级配置 ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building Settings", meta = (ClampMin = 1))
        int32 BuildingLevel = 1; // 默认为1级

        // --- 新增：资源点配置 ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Settings")
        EResourceType ResourceType = EResourceType::RT_None; // 默认为无资源

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource Settings", meta = (ClampMin = 0, EditCondition = "ResourceType != EResourceType::RT_None"))
        int32 ResourceAmount = 0; // 资源数量（仅当有资源时生效）

    FLevelGridConfig() : GridX(0), GridY(0), bIsBlocked(true) {}
};

/**
 * 关卡数据资产类，用于存储单关的网格配置和敌人建筑布局
 */
UCLASS()
class AUTOBATTLEDEMO_API ULevelDataAsset : public UDataAsset
{
    GENERATED_BODY()
public:
    // 网格宽度（X方向格子数量）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Settings", meta = (MinValue = 10))
        int32 GridWidth;

    // 网格高度（Y方向格子数量）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Settings", meta = (MinValue = 10))
        int32 GridHeight;

    // 单个格子的尺寸（虚幻单位，需与GridManager的TileSize保持一致）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Settings", meta = (MinValue = 50.0f))
        float CellSize;

    // 敌人建筑布局配置列表
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Buildings")
        TArray<FLevelGridConfig> EnemyBuildingConfigs;

    // 玩家基地的网格坐标
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Locations")
        FIntPoint PlayerBaseLocation;

    // 敌人基地的网格坐标（用于胜利判定）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Locations")
        FIntPoint EnemyBaseLocation;

    // --- 新增：背景音乐/音效 ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
        USoundBase* BackgroundMusic; // 关卡背景音乐

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
        USoundBase* VictorySound; // 胜利音效

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
        USoundBase* DefeatSound; // 失败音效

        // --- 新增：玩家初始资源 ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Start Resources", meta = (ClampMin = 0))
        int32 InitialGold = 500; // 初始金币

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Start Resources", meta = (ClampMin = 0))
        int32 InitialElixir = 300; // 初始圣水

    //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Start Resources", meta = (ClampMin = 0))
        //int32 InitialDarkElixir = 0; // 初始暗黑重油

    ULevelDataAsset() : GridWidth(20), GridHeight(20), CellSize(100.0f) {}
};