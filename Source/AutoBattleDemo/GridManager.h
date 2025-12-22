#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseBuilding.h"
#include "GridManager.generated.h"
// 前向声明
class ULevelDataAsset;
class ABaseBuilding;

USTRUCT(BlueprintType)
struct FGridNode
{
    GENERATED_BODY()

    UPROPERTY()
        int32 X;                  // 网格X坐标
    UPROPERTY()
        int32 Y;                  // 网格Y坐标
    UPROPERTY()
        bool bIsBlocked;          // 是否阻挡
    UPROPERTY()
        FVector WorldLocation;    // 世界坐标中心点
    UPROPERTY()
        float Cost;               // 寻路成本（如地形权重）
};

// 格子阻挡状态变化委托（供单位重新寻路）
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnTileBlockedChanged, int32 /*GridX*/, int32 /*GridY*/);

UCLASS()
class AUTOBATTLEDEMO_API AGridManager : public AActor
{
    GENERATED_BODY()

protected:
    virtual void BeginPlay() override;

public:
    AGridManager();

    // 关卡数据加载：从LevelDataAsset初始化网格和建筑
    UFUNCTION(BlueprintCallable, Category = "Level")
        void LoadLevelFromDataAsset(ULevelDataAsset* LevelData);

    // 网格核心功能
    UFUNCTION(BlueprintCallable, Category = "Grid")
        bool IsTileWalkable(int32 X, int32 Y);                      // 检查格子是否可通行
    UFUNCTION(BlueprintCallable, Category = "Grid")
        void GenerateGrid(int32 Width, int32 Height, float CellSize); // 生成网格数据
    UFUNCTION(BlueprintCallable, Category = "Grid")
        TArray<FVector> FindPath(const FVector& StartWorldLoc, const FVector& EndWorldLoc); // 寻路算法
    UFUNCTION(BlueprintCallable, Category = "Grid")
        void SetTileBlocked(int32 GridX, int32 GridY, bool bBlocked); // 设置格子阻挡状态（名称不变）
    UFUNCTION(BlueprintCallable, Category = "Grid")
        FVector GridToWorld(int32 GridX, int32 GridY) const;          // 网格坐标转世界坐标
    UFUNCTION(BlueprintCallable, Category = "Grid")
        bool WorldToGrid(const FVector& WorldLoc, int32& OutGridX, int32& OutGridY) const; // 世界坐标转网格坐标
    UFUNCTION(BlueprintCallable, Category = "Grid")
        void DrawGridVisuals(int32 HoverX, int32 HoverY);             // 绘制网格调试 visuals
    // 新增：玩家大本营建筑类（在蓝图中指定具体类型）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Setup")
        TSubclassOf<ABaseBuilding> PlayerBaseClass;
    // 新增：敌方大本营建筑类（在蓝图中指定具体类型）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Setup")
        TSubclassOf<ABaseBuilding> EnemyBaseClass;

        // 阻挡状态变化通知（供外部绑定，如单位重新寻路）
    FOnTileBlockedChanged OnTileBlockedChanged;

private:
    // A*算法内部节点（纯逻辑用，不暴露给蓝图）
    struct FAStarNode
    {
        int32 X;
        int32 Y;
        float G; // 起点到当前节点的实际成本
        float H; // 预估到终点的 heuristic 成本
        FAStarNode* Parent;

        float F() const { return G + H; } // 总成本
        FAStarNode(int32 InX, int32 InY) : X(InX), Y(InY), G(0), H(0), Parent(nullptr) {}
    };

    // A*辅助函数
    FAStarNode* GetLowestFNode(TArray<FAStarNode*>& Nodes); // 获取F值最小的节点
    bool IsTileValid(int32 GridX, int32 GridY) const;       // 检查坐标是否在网格范围内
    float GetHeuristicCost(int32 X1, int32 Y1, int32 X2, int32 Y2) const; // 计算启发式成本
    TArray<FIntPoint> GetNeighborNodes(int32 X, int32 Y) const; // 获取邻居节点
    void OptimizePath(TArray<FIntPoint>& RawPath);           // 优化路径点（减少冗余节点）

    // 网格数据存储
    UPROPERTY()
        TArray<FGridNode> GridNodes;       // 扁平化存储的网格节点数组
    UPROPERTY()
        int32 GridWidthCount;              // 网格宽度（X方向格子数）
    UPROPERTY()
        int32 GridHeightCount;             // 网格高度（Y方向格子数）
    UPROPERTY()
        float TileSize;                    // 单个格子的尺寸（世界单位）
    UPROPERTY(EditAnywhere, Category = "Debug")
        bool bDrawDebug;                   // 是否绘制调试信息
};