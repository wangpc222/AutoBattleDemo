#include "GridManager.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Misc/AssertionMacros.h"
#include "LevelDataAsset.h"
#include "BaseBuilding.h"
#include "Kismet/GameplayStatics.h"

// 构造函数：初始化组件与默认参数
AGridManager::AGridManager()
{
    PrimaryActorTick.bCanEverTick = false;
    bDrawDebug = true;

    // 创建根组件
    USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;
}

// 开始播放：初始化网格（默认值）
void AGridManager::BeginPlay()
{
    Super::BeginPlay();
    // GenerateGrid(20, 20, 100.0f); // 生成网格的权力收回Game Mode
}

// 生成网格数据：初始化所有格子的基础属性
void AGridManager::GenerateGrid(int32 Width, int32 Height, float CellSize)
{
    GridWidthCount = Width;
    GridHeightCount = Height;
    TileSize = CellSize;
    GridNodes.Empty();
    GridNodes.Reserve(Width * Height);

    // 按行列生成格子
    for (int32 Y = 0; Y < Height; Y++)
    {
        for (int32 X = 0; X < Width; X++)
        {
            FGridNode NewNode;
            NewNode.X = X;
            NewNode.Y = Y;
            NewNode.bIsBlocked = false;
            NewNode.Cost = 1.0f;
            NewNode.WorldLocation = GetActorLocation() + FVector(
                X * TileSize + TileSize / 2,  // X方向中心偏移
                Y * TileSize + TileSize / 2,  // Y方向中心偏移
                0.0f                         // Z轴固定（2D平面）
            );
            GridNodes.Add(NewNode);
        }
    }
}

// 绘制网格调试可视化：显示格子状态（正常/阻挡/悬停）
void AGridManager::DrawGridVisuals(int32 HoverX, int32 HoverY)
{
    if (!bDrawDebug || !GetWorld()) return;

    const float LifeTime = GetWorld()->GetDeltaSeconds() * 2.0f;

    for (const FGridNode& Node : GridNodes)
    {
        FColor LineColor = FColor(110, 110, 110);  // 默认灰色
        float LineThickness = 5.0f;

        // 悬停格子高亮
        if (Node.X == HoverX && Node.Y == HoverY)
        {
            LineColor = FColor::Cyan;
            LineThickness = 10.0f;
        }
        // 阻挡格子标红
        else if (Node.bIsBlocked)
        {
            LineColor = FColor::Red;
            LineThickness = 7.5f;
        }

        // 绘制格子边框
        DrawDebugBox(
            GetWorld(),
            Node.WorldLocation,
            FVector(TileSize / 2 * 0.90f, TileSize / 2 * 0.90f, 5.0f),  // 略小于实际尺寸避免重叠
            LineColor,
            false,
            LifeTime,
            0,
            LineThickness
        );
    }
}

// A*算法辅助：从OpenList获取F值最小的节点（模拟优先级队列）
AGridManager::FAStarNode* AGridManager::GetLowestFNode(TArray<FAStarNode*>& Nodes)
{
    if (Nodes.Num() == 0) return nullptr;

    FAStarNode* LowestNode = Nodes[0];
    for (FAStarNode* Node : Nodes)
    {
        if (Node->F() < LowestNode->F())
        {
            LowestNode = Node;
        }
    }
    return LowestNode;
}

// 核心寻路算法：A*路径查找
TArray<FVector> AGridManager::FindPath(const FVector& StartWorldLoc, const FVector& EndWorldLoc)
{
    TArray<FVector> Path;
    int32 StartX, StartY, EndX, EndY;

    // 转换起点和终点到网格坐标
    if (!WorldToGrid(StartWorldLoc, StartX, StartY) || !WorldToGrid(EndWorldLoc, EndX, EndY))
    {
        UE_LOG(LogTemp, Warning, TEXT("Start/End out of grid bounds"));
        return Path;
    }

    // 终点不可行走则返回空路径
    if (!IsTileWalkable(EndX, EndY))
    {
        UE_LOG(LogTemp, Warning, TEXT("End tile is blocked"));
        return Path;
    }

    // 初始化A*算法容器
    TArray<FAStarNode*> OpenList;       // 待检查节点
    TSet<FIntPoint> ClosedList;         // 已检查节点
    TMap<FIntPoint, FAStarNode*> NodeMap; // 节点缓存（避免重复创建）

    // 创建起点节点
    FAStarNode* StartNode = new FAStarNode(StartX, StartY);
    StartNode->H = GetHeuristicCost(StartX, StartY, EndX, EndY);
    OpenList.Add(StartNode);
    NodeMap.Add(FIntPoint(StartX, StartY), StartNode);

    // 主循环：处理OpenList中的节点
    while (OpenList.Num() > 0)  // 修复原代码逻辑错误（!OpenList.Num() == 0 改为 OpenList.Num() > 0）
    {
        // 获取当前F值最小的节点
        FAStarNode* CurrentNode = GetLowestFNode(OpenList);
        OpenList.Remove(CurrentNode);
        FIntPoint CurrentPos(CurrentNode->X, CurrentNode->Y);

        // 跳过已处理节点
        if (ClosedList.Contains(CurrentPos))
        {
            delete CurrentNode;
            continue;
        }
        ClosedList.Add(CurrentPos);

        // 到达终点：回溯路径
        if (CurrentNode->X == EndX && CurrentNode->Y == EndY)
        {
            TArray<FIntPoint> RawPath;
            FAStarNode* TempNode = CurrentNode;
            while (TempNode != nullptr)
            {
                RawPath.Insert(FIntPoint(TempNode->X, TempNode->Y), 0);
                TempNode = TempNode->Parent;
            }

            // 优化路径并转换为世界坐标
            OptimizePath(RawPath);
            for (const FIntPoint& Point : RawPath)
            {
                Path.Add(GridToWorld(Point.X, Point.Y));
            }

            // 清理内存
            for (auto& Pair : NodeMap)
                delete Pair.Value;
            return Path;
        }

        // 处理邻居节点
        TArray<FIntPoint> Neighbors = GetNeighborNodes(CurrentNode->X, CurrentNode->Y);
        for (const FIntPoint& NeighborPos : Neighbors)
        {
            if (ClosedList.Contains(NeighborPos))
                continue;

            // 计算移动成本（基础距离 * 格子权重）
            const float MoveCost = FVector::Dist(
                GridToWorld(CurrentNode->X, CurrentNode->Y),
                GridToWorld(NeighborPos.X, NeighborPos.Y)
            ) * GridNodes[NeighborPos.Y * GridWidthCount + NeighborPos.X].Cost;

            const float NewGCost = CurrentNode->G + MoveCost;
            FAStarNode* NeighborNode = nullptr;

            // 复用已有节点或创建新节点
            if (NodeMap.Contains(NeighborPos))
            {
                NeighborNode = NodeMap[NeighborPos];
                if (NewGCost >= NeighborNode->G)
                    continue;  // 新路径成本更高，跳过
            }
            else
            {
                NeighborNode = new FAStarNode(NeighborPos.X, NeighborPos.Y);
                NodeMap.Add(NeighborPos, NeighborNode);
            }

            // 更新节点信息
            NeighborNode->G = NewGCost;
            NeighborNode->H = GetHeuristicCost(NeighborPos.X, NeighborPos.Y, EndX, EndY);
            NeighborNode->Parent = CurrentNode;

            // 添加到OpenList（避免重复添加）
            if (!OpenList.Contains(NeighborNode))
            {
                OpenList.Add(NeighborNode);
            }
        }
    }

    // 未找到路径：清理内存
    for (auto& Pair : NodeMap)
        delete Pair.Value;
    UE_LOG(LogTemp, Warning, TEXT("No path found"));
    return Path;
}

// 设置格子阻挡状态：并触发状态变化通知
void AGridManager::SetTileBlocked(int32 GridX, int32 GridY, bool bBlocked)
{
    if (!IsTileValid(GridX, GridY))
        return;

    const int32 Index = GridY * GridWidthCount + GridX;
    if (!GridNodes.IsValidIndex(Index))
        return;

    // 仅在状态变化时更新（避免无效操作）
    if (GridNodes[Index].bIsBlocked != bBlocked)
    {
        GridNodes[Index].bIsBlocked = bBlocked;
        OnTileBlockedChanged.Broadcast(GridX, GridY);  // 通知外部（如单位重新寻路）
    }

    //// 调试绘制：显示阻挡状态变化
    //if (bDrawDebug && GetWorld())
    //{
    //    DrawDebugBox(
    //        GetWorld(),
    //        GridNodes[Index].WorldLocation,
    //        FVector(TileSize / 2 * 0.9f, TileSize / 2 * 0.9f, 2.0f),
    //        bBlocked ? FColor::Red : FColor::White,
    //        true,
    //        30.0f,
    //        0,
    //        3.0f
    //    );
    //}
}

// 网格坐标转世界坐标：获取格子中心点
FVector AGridManager::GridToWorld(int32 GridX, int32 GridY) const
{
    if (!IsTileValid(GridX, GridY))
        return FVector::ZeroVector;

    const int32 Index = GridY * GridWidthCount + GridX;
    return GridNodes.IsValidIndex(Index) ? GridNodes[Index].WorldLocation : FVector::ZeroVector;
}

// 世界坐标转网格坐标：计算对应格子索引
bool AGridManager::WorldToGrid(const FVector& WorldLoc, int32& OutGridX, int32& OutGridY) const
{
    const FVector LocalLoc = WorldLoc - GetActorLocation();
    OutGridX = FMath::FloorToInt(LocalLoc.X / TileSize);
    OutGridY = FMath::FloorToInt(LocalLoc.Y / TileSize);
    return IsTileValid(OutGridX, OutGridY);
}

// 检查格子是否在网格范围内
bool AGridManager::IsTileValid(int32 GridX, int32 GridY) const
{
    return GridX >= 0 && GridX < GridWidthCount&& GridY >= 0 && GridY < GridHeightCount;
}

// 检查格子是否可通行（有效且未被阻挡）
bool AGridManager::IsTileWalkable(int32 X, int32 Y)
{
    if (!IsTileValid(X, Y))
        return false;

    const int32 Index = Y * GridWidthCount + X;
    return GridNodes.IsValidIndex(Index) && !GridNodes[Index].bIsBlocked;
}

// 计算启发式成本（曼哈顿距离，适合四方向移动）
float AGridManager::GetHeuristicCost(int32 X1, int32 Y1, int32 X2, int32 Y2) const
{
    // �����پ��루�ʺ��ķ����ƶ���
    return FMath::Abs(X1 - X2) + FMath::Abs(Y1 - Y2);
}

// 获取邻居节点：四方向（上下左右）
TArray<FIntPoint> AGridManager::GetNeighborNodes(int32 X, int32 Y) const
{
    TArray<FIntPoint> Neighbors;
    const int32 Directions[4][2] = { {1,0}, {-1,0}, {0,1}, {0,-1} };  // 四方向

    for (const auto& Dir : Directions)
    {
        const int32 NewX = X + Dir[0];
        const int32 NewY = Y + Dir[1];
        // 仅添加有效且未被阻挡的邻居
        if (IsTileValid(NewX, NewY) && !GridNodes[NewY * GridWidthCount + NewX].bIsBlocked)
        {
            Neighbors.Add(FIntPoint(NewX, NewY));
        }
    }
    return Neighbors;
}

// 路径优化：移除直线上的冗余节点
void AGridManager::OptimizePath(TArray<FIntPoint>& RawPath)
{
    if (RawPath.Num() <= 2)
        return;

    TArray<FIntPoint> Optimized;
    Optimized.Add(RawPath[0]);
    FIntPoint PrevDir = RawPath[1] - RawPath[0];

    // 保留方向变化的节点
    for (int32 i = 2; i < RawPath.Num(); i++)
    {
        const FIntPoint CurrentDir = RawPath[i] - RawPath[i - 1];
        if (CurrentDir != PrevDir)
        {
            Optimized.Add(RawPath[i - 1]);
            PrevDir = CurrentDir;
        }
    }
    Optimized.Add(RawPath.Last());  // 保留终点
    RawPath = Optimized;
}

// 从数据资产加载关卡：初始化网格与建筑
void AGridManager::LoadLevelFromDataAsset(ULevelDataAsset* LevelData)
{
    if (!LevelData)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid LevelDataAsset!"));
        return;
    }

    // 1. 基于数据资产重新生成网格
    GenerateGrid(LevelData->GridWidth, LevelData->GridHeight, LevelData->CellSize);

    // 2. 生成敌方建筑并设置阻挡
    for (const FLevelGridConfig& Config : LevelData->EnemyBuildingConfigs)
    {
        SetTileBlocked(Config.GridX, Config.GridY, Config.bIsBlocked);

        // 生成建筑实例（若配置了建筑类）
        if (Config.BuildingClass)
        {
            const FVector SpawnLocation = GridToWorld(Config.GridX, Config.GridY);
            ABaseBuilding* NewBuilding = GetWorld()->SpawnActor<ABaseBuilding>(
                Config.BuildingClass,
                SpawnLocation,
                FRotator::ZeroRotator
                );
            if (NewBuilding)
            {
                NewBuilding->GridX = Config.GridX;
                NewBuilding->GridY = Config.GridY;
                NewBuilding->TeamID = ETeam::Enemy;
            }
        }
    }

    // 3. 生成玩家大本营
    if (IsTileValid(LevelData->PlayerBaseLocation.X, LevelData->PlayerBaseLocation.Y))
    {
        const int32 PX = LevelData->PlayerBaseLocation.X;
        const int32 PY = LevelData->PlayerBaseLocation.Y;
        SetTileBlocked(PX, PY, true);

        if (PlayerBaseClass)
        {
            ABaseBuilding* PlayerBase = GetWorld()->SpawnActor<ABaseBuilding>(
                PlayerBaseClass,
                GridToWorld(PX, PY),
                FRotator::ZeroRotator
                );
            if (PlayerBase)
            {
                PlayerBase->GridX = PX;
                PlayerBase->GridY = PY;
                PlayerBase->TeamID = ETeam::Player;
            }
        }
    }

    // 4. 生成敌方大本营
    if (IsTileValid(LevelData->EnemyBaseLocation.X, LevelData->EnemyBaseLocation.Y))
    {
        const int32 EX = LevelData->EnemyBaseLocation.X;
        const int32 EY = LevelData->EnemyBaseLocation.Y;
        SetTileBlocked(EX, EY, true);

        if (EnemyBaseClass)
        {
            ABaseBuilding* EnemyBase = GetWorld()->SpawnActor<ABaseBuilding>(
                EnemyBaseClass,
                GridToWorld(EX, EY),
                FRotator::ZeroRotator
                );
            if (EnemyBase)
            {
                EnemyBase->GridX = EX;
                EnemyBase->GridY = EY;
                EnemyBase->TeamID = ETeam::Enemy;
            }
        }
    }
}