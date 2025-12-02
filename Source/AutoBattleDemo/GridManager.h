#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridManager.generated.h"

// 格子状态枚举
UENUM(BlueprintType)
enum class EGridState : uint8
{
    Empty,       // 空
    HasBuilding, // 有建筑
    HasSoldier,  // 有士兵（允许多个）
    Blocked      // 不可通行（岩石/树林等）
};

// 格子数据结构
USTRUCT(BlueprintType)
struct FGridCell
{
    GENERATED_BODY()

        // 格子坐标
        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
        FIntPoint Coordinates;

    // 当前状态
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
        EGridState State = EGridState::Empty;

    // 包含的Actor（建筑/阻碍物）
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
        AActor* OccupyingActor = nullptr;

    // 包含的士兵列表（允许多个）
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
        TArray<AActor*> SoldiersInCell;
};

// 封装单行网格单元格，解决嵌套容器问题
USTRUCT(BlueprintType)
struct FGridRow
{
    GENERATED_BODY()

        // 单行中的单元格数组（原二维数组的内层）
        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid Data")
        TArray<FGridCell> Cells;
};

// 场景类型枚举
UENUM(BlueprintType)
enum class EGridSceneType : uint8
{
    PlayerBase,   // 我方基地
    EnemyBase     // 敌人基地
};

UCLASS()
class AUTOBATTLEDEMO_API AGridManager : public AActor
{
    GENERATED_BODY()

public:
    AGridManager();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // 初始化网格
    UFUNCTION(BlueprintCallable, Category = "Grid System")
        void InitGrid(int32 InWidth, int32 InHeight, float InCellSize, EGridSceneType InSceneType);

    // 获取指定世界坐标对应的格子坐标
    UFUNCTION(BlueprintPure, Category = "Grid System")
        FIntPoint GetGridCoordinatesFromWorldLocation(const FVector& WorldLocation) const;

    // 获取指定格子坐标对应的世界位置
    UFUNCTION(BlueprintPure, Category = "Grid System")
        FVector GetWorldLocationFromGridCoordinates(const FIntPoint& GridCoordinates) const;

    // 获取格子状态
    UFUNCTION(BlueprintPure, Category = "Grid System")
        EGridState GetGridCellState(const FIntPoint& GridCoordinates) const;

    // 更新格子状态（放置建筑）
    UFUNCTION(BlueprintCallable, Category = "Grid System")
        bool PlaceBuilding(const FIntPoint& GridCoordinates, AActor* BuildingActor);

    // 移除建筑
    UFUNCTION(BlueprintCallable, Category = "Grid System")
        bool RemoveBuilding(const FIntPoint& GridCoordinates);

    // 添加士兵到格子
    UFUNCTION(BlueprintCallable, Category = "Grid System")
        void AddSoldierToCell(const FIntPoint& GridCoordinates, AActor* SoldierActor);

    // 从格子移除士兵
    UFUNCTION(BlueprintCallable, Category = "Grid System")
        void RemoveSoldierFromCell(const FIntPoint& GridCoordinates, AActor* SoldierActor);

    // 放置阻碍物（岩石/树林）
    UFUNCTION(BlueprintCallable, Category = "Grid System")
        bool PlaceObstacle(const FIntPoint& GridCoordinates, AActor* ObstacleActor);

    // 移除阻碍物（消耗资源）
    UFUNCTION(BlueprintCallable, Category = "Grid System")
        bool RemoveObstacle(const FIntPoint& GridCoordinates, int32& ResourceCost);

    // 检查格子是否可建造
    UFUNCTION(BlueprintPure, Category = "Grid System")
        bool IsCellBuildable(const FIntPoint& GridCoordinates) const;

    // 检查格子是否可通行
    UFUNCTION(BlueprintPure, Category = "Grid System")
        bool IsCellPassable(const FIntPoint& GridCoordinates) const;

    // 切换场景类型
    UFUNCTION(BlueprintCallable, Category = "Grid System")
        void SetSceneType(EGridSceneType NewSceneType);

private:
    // 检查坐标是否有效
    bool IsValidGridCoordinates(const FIntPoint& GridCoordinates) const;

    // 更新格子状态
    void UpdateGridCellState(const FIntPoint& GridCoordinates);

protected:
    // 网格宽度
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid Settings")
        int32 GridWidth = 20;

    // 网格高度
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid Settings")
        int32 GridHeight = 20;

    // 格子大小（厘米）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid Settings")
        float CellSize = 100.0f;

    // 当前场景类型
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid Settings")
        EGridSceneType CurrentSceneType = EGridSceneType::PlayerBase;

    // 移除阻碍物的资源成本
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid Settings")
        int32 ObstacleRemovalCost = 50;

    // 网格数据（修正：使用FGridRow解决嵌套容器问题）
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid Data")
        TArray<FGridRow> GridRows;

    // 调试用网格显示
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
        bool bDrawDebugGrid = true;
};