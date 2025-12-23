#include "Building_Barracks.h"
#include "RTSGameInstance.h"
#include "BaseUnit.h"
#include "GridManager.h"
#include "RTSGameMode.h"
#include "Kismet/GameplayStatics.h"

ABuilding_Barracks::ABuilding_Barracks()
{
    BuildingType = EBuildingType::Barracks; // 确保 CoreTypes 里有这个枚举
    MaxHealth = 800.0f;
    TeamID = ETeam::Player;
    BuildingLevel = 1;
    // 默认每个兵营提供 5 人口
    // PopulationBonus = 5;
}

// [核心公式] 初始5，2,3每级+2，4,5每级+3
int32 ABuilding_Barracks::GetCurrentCapacity(int Level) const
{
    // Level 1: 5 + 0 = 5
    // Level 2: 5 + 2 = 7
    // Level 3: 5 + 4 = 9
    // Level 4: 5 + 4 + 3 = 12
    // Level 5: 5 + 4 + 6 = 15
    if (Level <= 3) return 5 + (Level - 1) * 2;
    else return 9 + (Level - 3) * 3;
}

void ABuilding_Barracks::BeginPlay()
{
    Super::BeginPlay();

    if (TeamID == ETeam::Player)
    {
        URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());
        if (GI)
        {
            GI->MaxPopulation += GetCurrentCapacity(BuildingLevel);
        }
    }
}

void ABuilding_Barracks::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    if (EndPlayReason == EEndPlayReason::Destroyed && TeamID == ETeam::Player)
    {
        URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());
        if (GI)
        {
            GI->MaxPopulation = FMath::Max(0, GI->MaxPopulation - GetCurrentCapacity(BuildingLevel));
        }
    }
}

// 存兵逻辑
void ABuilding_Barracks::StoreUnit(ABaseUnit* UnitToStore)
{
    if (!UnitToStore) return;

    // 容量检查
    if (StoredUnits.Num() >= GetCurrentCapacity(BuildingLevel))
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("Barracks Full!"));
        return;
    }

    // 保存数据
    FUnitSaveData Data;
    Data.UnitType = UnitToStore->UnitType;
    StoredUnits.Add(Data);

    // 解锁它原来占用的格子
    AGridManager* GM = Cast<AGridManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass()));
    if (GM)
    {
        int32 X, Y;
        if (GM->WorldToGrid(UnitToStore->GetActorLocation(), X, Y))
        {
            GM->SetTileBlocked(X, Y, false);
        }
    }

    // 销毁 Actor
    UnitToStore->Destroy();

    UE_LOG(LogTemp, Warning, TEXT("Unit Stored in Barracks! Total: %d"), StoredUnits.Num());
}

// 释放逻辑
void ABuilding_Barracks::ReleaseAllUnits()
{
    if (StoredUnits.Num() == 0) return;

    ARTSGameMode* GameMode = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(this));
    AGridManager* GM = Cast<AGridManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass()));

    if (!GameMode || !GM) return;

    // 遍历库存
    for (int32 i = StoredUnits.Num() - 1; i >= 0; i--)
    {
        FUnitSaveData UnitData = StoredUnits[i];

        // --- 寻找空位 (从兵营位置开始向外扩散) ---
        bool bFoundSpot = false;
        int32 TargetX = GridX;
        int32 TargetY = GridY;

        // 半径从 1 开始扩 (找周围一圈，再两圈...)
        for (int32 Radius = 1; Radius <= 5; Radius++)
        {
            for (int32 x = GridX - Radius; x <= GridX + Radius; x++)
            {
                for (int32 y = GridY - Radius; y <= GridY + Radius; y++)
                {
                    if (GM->IsTileWalkable(x, y))
                    {
                        TargetX = x;
                        TargetY = y;
                        bFoundSpot = true;
                        goto SpotFound; // 跳出循环
                    }
                }
            }
        }
        SpotFound:;

        // 如果找到空位，生成！
        if (bFoundSpot)
        {
            if (GameMode->SpawnUnitAt(UnitData.UnitType, TargetX, TargetY))
            {
                // 生成成功，从仓库移除
                StoredUnits.RemoveAt(i);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Barracks full! No space to release unit."));
            break; // 周围堵死了，别放了
        }
    }
}

// 升级逻辑
void ABuilding_Barracks::LevelUp()
{
    // 1. 检查是否能升级
    if (!CanUpgrade()) return;

    URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());

    int32 OldCapacity = GetCurrentCapacity(BuildingLevel); // 获取 Lv1 的容量 (5)

    // 2. [关键] 在升级前，先扣除当前等级提供的人口
    if (GI && TeamID == ETeam::Player)
    {
        
        GI->MaxPopulation -= OldCapacity;

        // 防止中间出现负数（虽然理论上不会）
        GI->MaxPopulation = FMath::Max(0, GI->MaxPopulation);
    }

    // 3. 执行等级提升 (调用父类逻辑，父类会做 BuildingLevel++ 和扣血回血)
    // 注意：这里调用的是父类的 LevelUp，不是 ApplyLevelUpBonus
    Super::LevelUp();

    // 4. [关键] 升级后，加上新等级的人口
    if (GI && TeamID == ETeam::Player)
    {
        // 此时 BuildingLevel 已经是 Lv2 了
        int32 NewCapacity = GetCurrentCapacity(BuildingLevel); // 获取 Lv2 的容量 (7)
        GI->MaxPopulation += NewCapacity;

        UE_LOG(LogTemp, Warning, TEXT("Barracks Upgraded Lv%d->Lv%d. Pop: -%d, +%d. Net: +%d"),
            BuildingLevel - 1, BuildingLevel, OldCapacity, NewCapacity, NewCapacity - OldCapacity);
    }
}


// 这个函数现在只负责视觉效果，不负责数值，避免逻辑混乱
void ABuilding_Barracks::ApplyLevelUpBonus()
{
    Super::ApplyLevelUpBonus();
    // 这里可以加特效，比如变大、播放声音
}

TArray<EUnitType> ABuilding_Barracks::GetStoredUnitTypes() const
{
    TArray<EUnitType> Types;
    for (const FUnitSaveData& UnitData : StoredUnits)
    {
        Types.Add(UnitData.UnitType);
    }
    return Types;
}

void ABuilding_Barracks::RestoreStoredUnits(const TArray<EUnitType>& InUnits)
{
    StoredUnits.Empty();
    for (const EUnitType& Type : InUnits)
    {
        FUnitSaveData Data;
        Data.UnitType = Type;
        // GridX/Y 不重要，因为是在仓库里
        StoredUnits.Add(Data);
    }
}