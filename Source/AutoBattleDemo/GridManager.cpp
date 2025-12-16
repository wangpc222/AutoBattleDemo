#include "GridManager.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"

AGridManager::AGridManager()
{
    PrimaryActorTick.bCanEverTick = false;  // 不需要每帧更新
    bDrawDebug = true;                      // 默认开启调试绘制（开发模式）

    // 网格尺寸初始化（来自 main_地图格子 分支，业务核心值）
    GridWidth = 20;
    GridHeight = 20;
    CellSize = 100.0f;

    // 创建一个根组件，否则它在场景里没有坐标（Location 全是 0）
    USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;
}

void AGridManager::BeginPlay()
{
    Super::BeginPlay();

    GenerateGrid(20, 20, 100.0f);




}

void AGridManager::InitializeGridFromLevelData(ULevelDataAsset* LevelData)
{
    if (LevelData)
    {
        // 保留你的分支：加载关卡数据（业务必需）
        LoadLevelData(LevelData);
    }
    else
    {
        // 保留你的分支：LevelData 判空报错（异常处理）
        UE_LOG(LogTemp, Error, TEXT("LevelData is null!"));
    }

    // 保留主分支：循环生成网格节点（核心逻辑）
    for (int32 X = 0; X < Width; X++)
    {
        FGridNode NewNode;
        NewNode.X = X;
        NewNode.Y = Y;
        NewNode.bIsBlocked = false;       // 默认所有格子可通行
        NewNode.Cost = 1.0f;              // 默认地形成本为1.0
        // 计算格子中心点世界坐标（基于管理器自身位置）
        NewNode.WorldLocation = GetActorLocation() + FVector(
            X * TileSize + TileSize / 2,  // X方向偏移（加一半尺寸居中）
            Y * TileSize + TileSize / 2,  // Y方向偏移（加一半尺寸居中）
            0.0f                         // Z轴默认为0（忽略高度）
        );
        GridNodes.Add(NewNode);

        //// 调试绘制格子边框（仅在非编辑器世界且开启调试时）
        //if (bDrawDebug && GetWorld()->GetName() != TEXT("EditorWorld"))
        //{
        //    DrawDebugBox(
        //        GetWorld(),
        //        NewNode.WorldLocation,
        //        FVector(TileSize / 2 * 0.9f, TileSize / 2 * 0.9f, 1.0f),  // 稍微缩小一点避免边框重叠
        //        FColor::White,
        //        true,                           // 持续显示
        //        -1.0f,                          // 永久存在
        //        0,
        //        4.0f                            // 线宽
        //    );
        //}
    }
}

// 保留主分支：蓝图可调用的网格可视化绘制方法（团队核心功能）
void AGridManager::DrawGridVisuals(int32 HoverX, int32 HoverY)
{
    float LifeTime = GetWorld()->GetDeltaSeconds() * 2.0f;

    for (const FGridNode& Node : GridNodes)
    {
        // 默认状态 (模拟半透明)
        // 使用灰色代替白色
        FColor LineColor = FColor(110, 110, 110);

        // 默认线宽变细，让它退居背景
        float LineThickness = 5.0f;

        // 1. 选中状态 (高亮 + 加粗)
        if (Node.X == HoverX && Node.Y == HoverY)
        {
            LineColor = FColor::Cyan; // 青色比蓝色更显眼
            LineThickness = 10.0f;     // 选中时很粗
        }
        // 2. 阻挡状态 (红色 + 中粗)
        else if (Node.bIsBlocked)
        {
            LineColor = FColor::Red;
            LineThickness = 7.5f;
        }

        // 绘制
        DrawDebugBox(
            GetWorld(),
            Node.WorldLocation,
            // 普通格子稍微小一点(0.9f)，给线之间留缝隙，看着更舒服
            // 选中格子可以稍微大一点吗？这里统一点比较好
            FVector(TileSize / 2 * 0.90f, TileSize / 2 * 0.90f, 5.0f),
            LineColor,
            false,
            LifeTime,
            0,
            LineThickness
        );
    }
}

void AGridManager::SetTileBlocked(int32 X, int32 Y, bool bBlocked, AActor* OccupyingActor)
{
    if (!IsValidCoordinate(X, Y)) return;

    int32 Index = Y * GridWidth + X;
    if (Index >= 0 && Index < GridNodes.Num())
    {
        GridNodes[Index].bIsBlocked = bBlocked;
        GridNodes[Index].OccupyingActor = OccupyingActor;

        // 通知所有单位该格子状态已更新，需要重新寻路
        OnGridUpdated.Broadcast(X, Y);

        // 调试输出
        UE_LOG(LogTemp, Log, TEXT("Grid (%d,%d) updated - Blocked: %s"), X, Y, bBlocked ? TEXT("True") : TEXT("False"));
    }
}

bool AGridManager::WorldToGrid(FVector WorldPos, int32& OutX, int32& OutY)
{
    OutX = FMath::FloorToInt((WorldPos.X) / CellSize);
    OutY = FMath::FloorToInt((WorldPos.Y) / CellSize);

    return IsValidCoordinate(OutX, OutY);
}

FVector AGridManager::GridToWorld(int32 X, int32 Y)
{
    if (!IsValidCoordinate(X, Y)) return FVector::ZeroVector;

    return FVector(
        X * CellSize + CellSize / 2.0f,
        Y * CellSize + CellSize / 2.0f,
        0.0f
    );
}

bool AGridManager::IsTileWalkable(int32 X, int32 Y)
{
    if (!IsValidCoordinate(X, Y)) return false;

    int32 Index = Y * GridWidth + X;
    return !GridNodes[Index].bIsBlocked;
}

TArray<FVector> AGridManager::FindPath(FVector StartPos, FVector EndPos)
{
    TArray<FVector> Path;

    // 转换起点和终点到网格坐标
    int32 StartX, StartY, EndX, EndY;
    if (!WorldToGrid(StartPos, StartX, StartY) || !WorldToGrid(EndPos, EndX, EndY))
    {
        UE_LOG(LogTemp, Warning, TEXT("Start or end position is out of grid bounds!"));
        return Path;
    }

    // 检查终点是否可走
    if (!IsTileWalkable(EndX, EndY))
    {
        UE_LOG(LogTemp, Warning, TEXT("End position is blocked!"));
        return Path;
    }

    // 重置寻路数据
    ResetPathfindingData();

    // 找到起点和终点节点
    FGridNode* StartNode = &GridNodes[StartY * GridWidth + StartX];
    FGridNode* EndNode = &GridNodes[EndY * GridWidth + EndX];

    // ========== 修复：替换 TPriorityQueue 为 TArray + 排序 ==========
    // 开放列表（存储待处理节点）
    TArray<FGridNode*> OpenList;
    // 关闭列表（存储已处理节点）
    TSet<FGridNode*> ClosedList;

    // 初始化起点节点
    StartNode->GCost = 0;
    StartNode->HCost = CalculateHCost(StartNode->X, StartNode->Y, EndX, EndY);
    OpenList.Add(StartNode);

    while (OpenList.Num() > 0)
    {
        // 1. 从开放列表中找到 FCost 最小的节点（替代优先队列）
        FGridNode* CurrentNode = nullptr;
        float MinFCost = TNumericLimits<float>::Max();
        for (FGridNode* Node : OpenList)
        {
            if (Node->GetFCost() < MinFCost)
            {
                MinFCost = Node->GetFCost();
                CurrentNode = Node;
            }
        }

        if (!CurrentNode) break; // 无可用节点，退出

        // 2. 将当前节点移出开放列表，加入关闭列表
        OpenList.Remove(CurrentNode);
        ClosedList.Add(CurrentNode);

        // 3. 到达终点，回溯生成路径
        if (CurrentNode == EndNode)
        {
            FGridNode* PathNode = EndNode;
            while (PathNode != nullptr)
            {
                Path.Insert(PathNode->WorldLocation, 0);
                PathNode = PathNode->ParentNode;
            }
            return Path;
        }

        // 4. 处理邻居节点
        TArray<FGridNode*> Neighbors = GetNeighborNodes(CurrentNode);
        for (FGridNode* Neighbor : Neighbors)
        {
            // 跳过不可走或已处理的节点
            if (Neighbor->bIsBlocked || ClosedList.Contains(Neighbor))
                continue;

            // 计算从当前节点到邻居的成本
            float TentativeGCost = CurrentNode->GCost +
                FVector::Dist(CurrentNode->WorldLocation, Neighbor->WorldLocation) * Neighbor->Cost;

            // 如果是更优路径，更新邻居节点
            if (TentativeGCost < Neighbor->GCost || !OpenList.Contains(Neighbor))
            {
                Neighbor->GCost = TentativeGCost;
                Neighbor->HCost = CalculateHCost(Neighbor->X, Neighbor->Y, EndX, EndY);
                Neighbor->ParentNode = CurrentNode;

                // 未在开放列表则添加
                if (!OpenList.Contains(Neighbor))
                {
                    OpenList.Add(Neighbor);
                }
            }
        }
    }

    // 找不到路径
    UE_LOG(LogTemp, Warning, TEXT("No path found!"));
    return Path;
}

void AGridManager::LoadLevelData(ULevelDataAsset* NewLevelData)
{
    if (!NewLevelData) return;

    CurrentLevelData = NewLevelData;
    GenerateGridFromConfig(NewLevelData);

       // 保留你的分支：调用自定义调试绘制方法（若后续弃用可注释）
DrawDebugGrid();

    // 保留主分支：标准化调试绘制（阻挡格子红色边框，可控开关）
  if (bDrawDebug)
  {
      DrawDebugBox(
          GetWorld(),
          GridNodes[Index].WorldLocation,
          FVector(TileSize / 2 * 0.9f, TileSize / 2 * 0.9f, 2.0f),
          bBlocked ? FColor::Red : FColor::White,  // 阻挡为红色，否则白色
          true,
          30.0f,  // 持续30秒（便于观察）
          0,
          3.0f    // 线宽加粗
      );
  }

    // 保留你的分支：日志打印（替换变量名适配主分支规范）
UE_LOG(LogTemp, Log, TEXT("Loaded level data: Grid %dx%d, Cell size: %f"),
      GridWidthCount, GridHeightCount, TileSize);
}

bool AGridManager::IsValidCoordinate(int32 X, int32 Y) const
{
    return X >= 0 && X < GridWidth&& Y >= 0 && Y < GridHeight;
}

TArray<FGridNode*> AGridManager::GetNeighborNodes(FGridNode* CurrentNode)
{
    TArray<FGridNode*> Neighbors;

    // 检查8个方向的邻居（如果需要4方向寻路可以注释掉对角线方向）
    for (int32 YOffset = -1; YOffset <= 1; YOffset++)
    {
        for (int32 XOffset = -1; XOffset <= 1; XOffset++)
        {
            // 跳过当前节点
            if (XOffset == 0 && YOffset == 0)
                continue;

            int32 CheckX = CurrentNode->X + XOffset;
            int32 CheckY = CurrentNode->Y + YOffset;

            if (IsValidCoordinate(CheckX, CheckY))
            {
                Neighbors.Add(&GridNodes[CheckY * GridWidth + CheckX]);
            }
        }
    }

    return Neighbors;
}

float AGridManager::CalculateHCost(int32 X, int32 Y, int32 TargetX, int32 TargetY)
{
    // 使用曼哈顿距离作为启发式函数
    int32 DeltaX = FMath::Abs(X - TargetX);
    int32 DeltaY = FMath::Abs(Y - TargetY);
    return (DeltaX + DeltaY) * 10.0f; // 乘以格子大小相关的系数
}

void AGridManager::ResetPathfindingData()
{
    for (auto& Node : GridNodes)
    {
        Node.GCost = 0;
        Node.HCost = 0;
        Node.ParentNode = nullptr;
    }
}

void AGridManager::GenerateGridFromConfig(ULevelDataAsset* LevelData)
{
    GridWidth = LevelData->GridWidth;
    GridHeight = LevelData->GridHeight;
    CellSize = LevelData->CellSize;
    GridNodes.Empty(GridWidth * GridHeight);

    // 初始化网格
    for (int32 Y = 0; Y < GridHeight; Y++)
    {
        for (int32 X = 0; X < GridWidth; X++)
        {
            FGridNode Node;
            Node.X = X;
            Node.Y = Y;
            Node.WorldLocation = GridToWorld(X, Y);
            Node.bIsBlocked = false;
            Node.OccupyingActor = nullptr;
            Node.Cost = 1.0f;

            GridNodes.Add(Node);
        }
    }

    // 应用关卡配置
    for (const auto& GridConfig : LevelData->GridConfigurations)
    {
        SetTileBlocked(GridConfig.X, GridConfig.Y, GridConfig.bIsBlocked);

        // 如果配置了建筑类型，生成建筑
        if (GridConfig.BuildingClass)
        {
            FVector SpawnLocation = GridToWorld(GridConfig.X, GridConfig.Y);
            SpawnLocation.Z += 50.0f; // 稍微抬高避免碰撞
            AActor* NewBuilding = GetWorld()->SpawnActor<AActor>(
                GridConfig.BuildingClass,
                SpawnLocation,
                FRotator::ZeroRotator
                );

            if (NewBuilding)
            {
                // 更新格子的占据者
                int32 Index = GridConfig.Y * GridWidth + GridConfig.X;
                if (Index >= 0 && Index < GridNodes.Num())
                {
                    GridNodes[Index].OccupyingActor = NewBuilding;
                }
            }
        }
    }
}

void AGridManager::DrawDebugGrid()
{
#if WITH_EDITOR
    for (int32 Y = 0; Y < GridHeight; Y++)
    {
        for (int32 X = 0; X < GridWidth; X++)
        {
            FVector WorldLoc = GridToWorld(X, Y);
            FVector BoxExtent(CellSize / 2.0f - 1.0f, CellSize / 2.0f - 1.0f, 50.0f);

            // 根据是否被阻挡设置不同颜色
            FColor Color = IsTileWalkable(X, Y) ? FColor::Green : FColor::Red;

            DrawDebugBox(GetWorld(), WorldLoc, BoxExtent, Color, false, 30.0f, 0, 2.0f);
        }
    }
#endif
}