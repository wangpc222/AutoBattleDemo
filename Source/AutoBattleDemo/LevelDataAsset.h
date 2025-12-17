// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LevelDataAsset.generated.h"

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

    ULevelDataAsset() : GridWidth(20), GridHeight(20), CellSize(100.0f) {}
};
