#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataAsset.h"
#include "GridManager.generated.h"

// 单个格子的配置数据
USTRUCT(BlueprintType)
struct FGridConfig
{
    GENERATED_BODY()

        UPROPERTY(EditAnywhere, BlueprintReadWrite)
        int32 X;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        int32 Y;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        bool bIsBlocked;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
        TSubclassOf<AActor> BuildingClass; // 该格子上的建筑类型
};

// 关卡地图数据资产
UCLASS()
class AUTOBATTLEDEMO_API ULevelDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Config")
        int32 GridWidth;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Config")
        int32 GridHeight;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Config")
        float CellSize;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Config")
        TArray<FGridConfig> GridConfigurations; // 格子配置列表

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Config")
        FIntPoint PlayerBaseLocation; // 玩家基地位置

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Config")
        FIntPoint EnemyBaseLocation; // 敌人基地位置
};

// 格子节点结构体
USTRUCT(BlueprintType)
struct FGridNode
{
    GENERATED_BODY()

        int32 X;
    int32 Y;
    FVector WorldLocation;
    bool bIsBlocked;
    float Cost;
    AActor* OccupyingActor; // 当前占据的演员（建筑/单位）

    // 寻路相关
    float GCost; // 从起点到当前节点的成本
    float HCost; // 到终点的预估成本
    FGridNode* ParentNode;

    FGridNode() : X(0), Y(0), bIsBlocked(false), Cost(1.0f),
        OccupyingActor(nullptr), GCost(0), HCost(0), ParentNode(nullptr) {}

    float GetFCost() const { return GCost + HCost; }
};

// 用于通知单位重新寻路的委托
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnGridUpdated, int32, int32); // 保持不变，语义仍匹配

UCLASS()
class AUTOBATTLEDEMO_API AGridManager : public AActor
{
    GENERATED_BODY()

protected:
    // --- 声明游戏开始函数 ---
    virtual void BeginPlay() override;

public:
    AGridManager();

    // 初始化网格
    UFUNCTION(BlueprintCallable, Category = "Grid")
        void InitializeGridFromLevelData(ULevelDataAsset* LevelData);

    // 更新格子状态（动态阻挡）
    UFUNCTION(BlueprintCallable, Category = "Grid")
        void SetTileBlocked(int32 X, int32 Y, bool bBlocked, AActor* OccupyingActor = nullptr);

    // 世界坐标转格子坐标
    UFUNCTION(BlueprintCallable, Category = "Grid")
        bool WorldToGrid(FVector WorldPos, int32& OutX, int32& OutY);

    // 格子坐标转世界坐标
    UFUNCTION(BlueprintCallable, Category = "Grid")
        FVector GridToWorld(int32 X, int32 Y);

    // 检查格子是否可走
    UFUNCTION(BlueprintCallable, Category = "Grid")
        bool IsTileWalkable(int32 X, int32 Y);

    // 寻路算法（A*）
    UFUNCTION(BlueprintCallable, Category = "Pathfinding")
        TArray<FVector> FindPath(FVector StartPos, FVector EndPos);

    // 获取当前关卡数据
    UFUNCTION(BlueprintCallable, Category = "Level")
        ULevelDataAsset* GetCurrentLevelData() const { return CurrentLevelData; }

    // 加载新关卡
    UFUNCTION(BlueprintCallable, Category = "Level")
        void LoadLevelData(ULevelDataAsset* NewLevelData);

    // 获取网格更新委托
    FOnGridUpdated& GetGridUpdatedDelegate() { return OnGridUpdated; }

protected:
    // 保留你的分支：BeginPlay生命周期（主分支无，且是UE核心生命周期，必须保留）
    virtual void BeginPlay() override;

public:
    // 保留主分支：蓝图可调用的绘制方法（团队新增的蓝图交互能力）
    UFUNCTION(BlueprintCallable, Category = "Grid")
        void DrawGridVisuals(int32 HoverX, int32 HoverY);

private:
    // 保留主分支：A*算法节点结构体（团队重构的寻路核心，必须保留）
    struct FAStarNode
    {
        int32 X;               // 节点X坐标
        int32 Y;               // 节点Y坐标
        float G;               // 起点到当前节点的实际成本
        float H;               // 当前节点到终点的预估成本（启发式）
        TWeakPtr<FAStarNode> Parent;  // 父节点（用于回溯路径）

        // 计算总成本（F = G + H）
        float F() const { return G + H; }
        // 构造函数
        FAStarNode(int32 InX, int32 InY) : X(InX), Y(InY), G(0), H(0) {}
    };

    // ========== 你的分支核心方法（保留，需和主分支方法区分/兼容） ==========
    // 检查坐标是否有效（你的原始方法，可保留或替换为主分支的IsTileValid，需确认逻辑）
    bool IsValidCoordinate(int32 X, int32 Y) const;

    // 获取邻居节点（你的原始方法，主分支已重构，建议后续替换为GetNeighborNodes(int32 X, int32 Y)）
    TArray<FGridNode*> GetNeighborNodes(FGridNode* CurrentNode);

    // 计算启发式成本（你的原始方法，可替换为主分支的GetHeuristicCost）
    float CalculateHCost(int32 X, int32 Y, int32 TargetX, int32 TargetY);

    // 重置寻路数据（你的业务方法，主分支无，必须保留）
    void ResetPathfindingData();

    // 从配置生成网格（你的业务方法，主分支无，必须保留）
    void GenerateGridFromConfig(ULevelDataAsset* LevelData);

    // 调试用：绘制网格（你的原始调试方法）
    void DrawDebugGrid();

    // ========== 主分支重构方法（保留，团队规范） ==========
    /**
     * 检查格子是否有效（在网格范围内且未被阻挡）
     * @param GridX 格子X坐标
     * @param GridY 格子Y坐标
     * @return 是否有效
     */
    bool IsTileValid(int32 GridX, int32 GridY) const;

    /**
     * 计算启发式成本（曼哈顿距离）
     * @param X1 起点X
     * @param Y1 起点Y
     * @param X2 终点X
     * @param Y2 终点Y
     * @return 启发式成本值
     */
    float GetHeuristicCost(int32 X1, int32 Y1, int32 X2, int32 Y2) const;

    /**
     * 获取指定格子的所有有效邻居节点（四方向）
     * @param X 格子X坐标
     * @param Y 格子Y坐标
     * @return 邻居节点坐标列表
     */
    TArray<FIntPoint> GetNeighborNodes(int32 X, int32 Y) const;

    /**
     * 优化路径（移除冗余节点，使路径更平滑）
     * @param RawPath 原始路径
     */
    void OptimizePath(TArray<FIntPoint>& RawPath);

    // ========== 变量合并（主分支命名规范 + 你的业务变量） ==========
    // 保留主分支：加UPROPERTY标记的网格节点数组（团队规范）
    UPROPERTY()
        TArray<FGridNode> GridNodes;

    // 保留你的分支：当前关卡数据（业务必需）
    UPROPERTY(Transient)
        ULevelDataAsset* CurrentLevelData;

    // 主分支重构的网格属性（语义化命名 + UPROPERTY，替代你的旧变量）
    UPROPERTY()
        int32 GridWidthCount;    // 替代你的GridWidth
    UPROPERTY()
        int32 GridHeightCount;   // 替代你的GridHeight
    UPROPERTY()
        float TileSize;          // 替代你的CellSize

    // 保留你的分支：网格更新委托（业务必需）
    FOnGridUpdated OnGridUpdated;

    // 保留主分支：调试绘制开关（团队新增）
    UPROPERTY(EditAnywhere, Category = "Debug")
        bool bDrawDebug;
};
