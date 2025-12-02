#include "GridManager.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "BaseBuilding.h"
#include "BaseSoldier.h"

AGridManager::AGridManager()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AGridManager::BeginPlay()
{
    Super::BeginPlay();
    // 初始化默认网格（可选：也可在蓝图中手动调用InitGrid）
    InitGrid(GridWidth, GridHeight, CellSize, CurrentSceneType);
}

void AGridManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 调试绘制网格
    if (bDrawDebugGrid)
    {
        for (int32 X = 0; X < GridWidth; X++)
        {
            for (int32 Y = 0; Y < GridHeight; Y++)
            {
                FIntPoint GridCoords(X, Y);
                FVector WorldLoc = GetWorldLocationFromGridCoordinates(GridCoords);
                FVector Extent(CellSize / 2, CellSize / 2, 50.0f); // 高度50防止与地面重叠

                // 根据格子状态设置颜色
                FColor DebugColor = FColor::White;
                switch (GetGridCellState(GridCoords))
                {
                case EGridState::Empty:       DebugColor = FColor::Green; break;
                case EGridState::HasBuilding: DebugColor = FColor::Blue;  break;
                case EGridState::HasSoldier:  DebugColor = FColor::Yellow; break;
                case EGridState::Blocked:     DebugColor = FColor::Red;   break;
                }

                DrawDebugBox(GetWorld(), WorldLoc, Extent, DebugColor, false, -1, 0, 2.0f);
            }
        }
    }
}

void AGridManager::InitGrid(int32 InWidth, int32 InHeight, float InCellSize, EGridSceneType InSceneType)
{
    GridWidth = InWidth;
    GridHeight = InHeight;
    CellSize = InCellSize;
    CurrentSceneType = InSceneType;

    // 初始化网格行和单元格
    GridRows.SetNum(GridWidth);
    for (int32 X = 0; X < GridWidth; X++)
    {
        GridRows[X].Cells.SetNum(GridHeight);
        for (int32 Y = 0; Y < GridHeight; Y++)
        {
            GridRows[X].Cells[Y].Coordinates = FIntPoint(X, Y);
            GridRows[X].Cells[Y].State = EGridState::Empty;
            GridRows[X].Cells[Y].OccupyingActor = nullptr;
            GridRows[X].Cells[Y].SoldiersInCell.Empty();
        }
    }
}

FIntPoint AGridManager::GetGridCoordinatesFromWorldLocation(const FVector& WorldLocation) const
{
    // 将世界坐标转换为网格坐标（以GridManager自身位置为原点）
    FVector LocalLocation = WorldLocation - GetActorLocation();
    int32 X = FMath::FloorToInt(LocalLocation.X / CellSize);
    int32 Y = FMath::FloorToInt(LocalLocation.Y / CellSize);
    return FIntPoint(X, Y);
}

FVector AGridManager::GetWorldLocationFromGridCoordinates(const FIntPoint& GridCoordinates) const
{
    // 将网格坐标转换为世界坐标（中心位置）
    FVector WorldLocation;
    WorldLocation.X = (GridCoordinates.X + 0.5f) * CellSize + GetActorLocation().X;
    WorldLocation.Y = (GridCoordinates.Y + 0.5f) * CellSize + GetActorLocation().Y;
    WorldLocation.Z = GetActorLocation().Z; // 与GridManager同高度
    return WorldLocation;
}

EGridState AGridManager::GetGridCellState(const FIntPoint& GridCoordinates) const
{
    if (IsValidGridCoordinates(GridCoordinates))
    {
        return GridRows[GridCoordinates.X].Cells[GridCoordinates.Y].State;
    }
    return EGridState::Blocked; // 无效坐标视为不可通行
}

bool AGridManager::PlaceBuilding(const FIntPoint& GridCoordinates, AActor* BuildingActor)
{
    if (!IsValidGridCoordinates(GridCoordinates) || !BuildingActor || !IsCellBuildable(GridCoordinates))
    {
        return false;
    }

    // 放置建筑并更新状态
    FGridCell& TargetCell = GridRows[GridCoordinates.X].Cells[GridCoordinates.Y];
    TargetCell.OccupyingActor = BuildingActor;
    UpdateGridCellState(GridCoordinates);

    // 移动建筑到格子中心
    BuildingActor->SetActorLocation(GetWorldLocationFromGridCoordinates(GridCoordinates));
    return true;
}

bool AGridManager::RemoveBuilding(const FIntPoint& GridCoordinates)
{
    if (!IsValidGridCoordinates(GridCoordinates))
    {
        return false;
    }

    FGridCell& TargetCell = GridRows[GridCoordinates.X].Cells[GridCoordinates.Y];
    if (TargetCell.State != EGridState::HasBuilding)
    {
        return false;
    }

    // 移除建筑引用并更新状态
    TargetCell.OccupyingActor = nullptr;
    UpdateGridCellState(GridCoordinates);
    return true;
}

void AGridManager::AddSoldierToCell(const FIntPoint& GridCoordinates, AActor* SoldierActor)
{
    if (!IsValidGridCoordinates(GridCoordinates) || !SoldierActor)
    {
        return;
    }

    FGridCell& TargetCell = GridRows[GridCoordinates.X].Cells[GridCoordinates.Y];
    if (!TargetCell.SoldiersInCell.Contains(SoldierActor))
    {
        TargetCell.SoldiersInCell.Add(SoldierActor);
        UpdateGridCellState(GridCoordinates);
    }
}

void AGridManager::RemoveSoldierFromCell(const FIntPoint& GridCoordinates, AActor* SoldierActor)
{
    if (!IsValidGridCoordinates(GridCoordinates) || !SoldierActor)
    {
        return;
    }

    FGridCell& TargetCell = GridRows[GridCoordinates.X].Cells[GridCoordinates.Y];
    if (TargetCell.SoldiersInCell.Remove(SoldierActor) > 0)
    {
        UpdateGridCellState(GridCoordinates);
    }
}

bool AGridManager::PlaceObstacle(const FIntPoint& GridCoordinates, AActor* ObstacleActor)
{
    if (!IsValidGridCoordinates(GridCoordinates) || !ObstacleActor || !IsCellPassable(GridCoordinates))
    {
        return false;
    }

    FGridCell& TargetCell = GridRows[GridCoordinates.X].Cells[GridCoordinates.Y];
    TargetCell.OccupyingActor = ObstacleActor;
    TargetCell.State = EGridState::Blocked; // 阻碍物强制设为Blocked
    ObstacleActor->SetActorLocation(GetWorldLocationFromGridCoordinates(GridCoordinates));
    return true;
}

bool AGridManager::RemoveObstacle(const FIntPoint& GridCoordinates, int32& ResourceCost)
{
    if (!IsValidGridCoordinates(GridCoordinates))
    {
        ResourceCost = 0;
        return false;
    }

    FGridCell& TargetCell = GridRows[GridCoordinates.X].Cells[GridCoordinates.Y];
    if (TargetCell.State != EGridState::Blocked)
    {
        ResourceCost = 0;
        return false;
    }

    // 消耗资源并移除阻碍物
    ResourceCost = ObstacleRemovalCost;
    TargetCell.OccupyingActor = nullptr;
    UpdateGridCellState(GridCoordinates);
    return true;
}

bool AGridManager::IsCellBuildable(const FIntPoint& GridCoordinates) const
{
    if (!IsValidGridCoordinates(GridCoordinates))
    {
        return false;
    }

    const EGridState State = GetGridCellState(GridCoordinates);
    return (State == EGridState::Empty); // 仅空格子可建造
}

bool AGridManager::IsCellPassable(const FIntPoint& GridCoordinates) const
{
    if (!IsValidGridCoordinates(GridCoordinates))
    {
        return false;
    }

    const EGridState State = GetGridCellState(GridCoordinates);
    return (State != EGridState::Blocked && State != EGridState::HasBuilding); // 阻碍物和建筑不可通行
}

void AGridManager::SetSceneType(EGridSceneType NewSceneType)
{
    CurrentSceneType = NewSceneType;
    // 可根据场景类型切换网格规则（如敌方基地不可建造）
}

bool AGridManager::IsValidGridCoordinates(const FIntPoint& GridCoordinates) const
{
    return (GridCoordinates.X >= 0 && GridCoordinates.X < GridWidth&&
        GridCoordinates.Y >= 0 && GridCoordinates.Y < GridHeight);
}

void AGridManager::UpdateGridCellState(const FIntPoint& GridCoordinates)
{
    if (!IsValidGridCoordinates(GridCoordinates))
    {
        return;
    }

    FGridCell& TargetCell = GridRows[GridCoordinates.X].Cells[GridCoordinates.Y];

    // 状态优先级：阻碍物 > 建筑 > 士兵 > 空
    if (TargetCell.OccupyingActor && TargetCell.State == EGridState::Blocked)
    {
        TargetCell.State = EGridState::Blocked;
    }
    else if (TargetCell.OccupyingActor)
    {
        TargetCell.State = EGridState::HasBuilding;
    }
    else if (TargetCell.SoldiersInCell.Num() > 0)
    {
        TargetCell.State = EGridState::HasSoldier;
    }
    else
    {
        TargetCell.State = EGridState::Empty;
    }
}