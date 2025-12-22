#include "RTSGameMode.h"
#include "RTSPlayerController.h"
#include "GridManager.h"
#include "BaseUnit.h"
#include "BaseBuilding.h"
#include "RTSGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Building_Barracks.h"
#include "Components/CapsuleComponent.h"
#include "LevelDataAsset.h" 

ARTSGameMode::ARTSGameMode()
{
    PlayerControllerClass = ARTSPlayerController::StaticClass();
    CurrentState = EGameState::Preparation;
}

void ARTSGameMode::BeginPlay()
{
    Super::BeginPlay();

    // 1. 找到 GridManager
    GridManager = Cast<AGridManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass()));

    // 2. 在这里初始化网格
    // 确保在做任何加载之前，网格已经就绪
    if (GridManager)
    {
        // 收回网格加载
        GridManager->GenerateGrid(20, 20, 100.0f);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("CRITICAL ERROR: GridManager NOT found in level!"));
        return;
    }

    // 3. 判断当前地图
    FString MapName = GetWorld()->GetMapName();

    // --- 战斗关卡 ---
    if (MapName.Contains("BattleField", ESearchCase::IgnoreCase))
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> Detected Battle Map! Spawning Units..."));

        // 只有在战斗关卡，才加载关卡数据 (敌人配置)
        // 如果放在外面的话，可能会导致你的基地里也刷出敌人的塔！
        if (GridManager && CurrentLevelData)
        {
            // 注意：这内部可能会再次 GenerateGrid，但这没关系，反而更安全
            GridManager->LoadLevelFromDataAsset(CurrentLevelData);
        }

        CurrentState = EGameState::Battle;
        LoadAndSpawnUnits(); // 把带来的兵放出来
        StartBattlePhase();  // 激活 AI
    }
    // --- 基地关卡 ---
    else
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> Detected Base Map. Loading Buildings..."));

        CurrentState = EGameState::Preparation;

        URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());
        if (GI)
        {
            // 重置人口上限
            // 因为接下来 LoadAndSpawnBase 会重新生成 HQ 和兵营，
            // 它们会再次执行 BeginPlay 把人口加回来。
            // 如果不归零，就会无限累加。
            GI->MaxPopulation = 0;

            // 顺便校准当前人口
            // 以保存的兵力数组数量为准，防止计数器跑偏
            GI->CurrentPopulation = GI->PlayerArmy.Num();
        }

        // 回到基地，重新把房子盖起来
        LoadAndSpawnBase();

        // 回到基地，带回兵
        LoadAndSpawnUnits();
    }
}

// 造兵逻辑
bool ARTSGameMode::TryBuyUnit(EUnitType Type, int32 Cost, int32 GridX, int32 GridY)
{
    if (CurrentState != EGameState::Preparation) return false;

    URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());
    if (!GI) return false; // 安全检查

    // --- 区域限制 (避免传送到战场后骑在敌人脸上) ---
    // 假设地图高度是 20，我们只允许在 X < 8 的区域造兵 (下半区)
    // 这样 BattleField1 的敌人就可以放在 X > 10 的区域
    int32 MaxPlayerX = 8;

    if (GridX >= MaxPlayerX)
    {
        UE_LOG(LogTemp, Warning, TEXT("Build Failed: Cannot place units in Enemy Territory (X>=%d)!"), MaxPlayerX);
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Can't place here! Enemy Territory!"));
        return false;
    }

    // 检查人口
    if (GI->CurrentPopulation + 1 > GI->MaxPopulation)
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Max Population!"));
        return false;
    }

    // 检查圣水
    // 使用 >= 确保刚好够钱也能买
    if (GI->PlayerElixir < Cost)
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Not Enough Elixir!"));
        return false;
    }

    // 匹配蓝图
    TSubclassOf<ABaseUnit> SpawnClass = nullptr;
    switch (Type)
    {
    case EUnitType::Barbarian:  SpawnClass = BarbarianClass; break;
    case EUnitType::Archer:     SpawnClass = ArcherClass;    break;
    case EUnitType::Giant:      SpawnClass = GiantClass;     break;
    case EUnitType::Bomber:     SpawnClass = BomberClass;    break;
    }

    if (!SpawnClass || !GridManager) return false;

    // 检查格子占用
    if (!GridManager->IsTileWalkable(GridX, GridY))
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Blocked!"));
        return false;
    }

    // 计算高度
    float SpawnZOffset = 0.0f;
    ABaseUnit* DefaultUnit = SpawnClass->GetDefaultObject<ABaseUnit>();
    if (DefaultUnit)
    {
        UCapsuleComponent* Cap = DefaultUnit->FindComponentByClass<UCapsuleComponent>();
        if (Cap)
            SpawnZOffset = Cap->GetScaledCapsuleHalfHeight();
        else
        {
            FVector Origin, BoxExtent;
            DefaultUnit->GetActorBounds(true, Origin, BoxExtent);
            SpawnZOffset = BoxExtent.Z;
        }
    }

    FVector SpawnLoc = GridManager->GridToWorld(GridX, GridY);
    SpawnLoc.Z += SpawnZOffset;

    // 生成
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ABaseUnit* NewUnit = GetWorld()->SpawnActor<ABaseUnit>(SpawnClass, SpawnLoc, FRotator::ZeroRotator, SpawnParams);
    if (NewUnit)
    {        
        GI->PlayerElixir -= Cost; // 扣费
        GI->CurrentPopulation += 1; // 人口

        NewUnit->TeamID = ETeam::Player;
        // GridManager->SetTileBlocked(GridX, GridY, true);
        return true;
    }
    return false;
}

// 造建筑逻辑
bool ARTSGameMode::TryBuildBuilding(EBuildingType Type, int32 Cost, int32 GridX, int32 GridY)
{
    if (CurrentState != EGameState::Preparation) return false;

    URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());
    if (!GI) return false;

    // 检查金币
    if (GI->PlayerGold < Cost)
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Not Enough Gold!"));
        return false;
    }

    // 建筑可以放在限制区域
    int32 MaxPlayerX = 8;

    if (GridX >= MaxPlayerX)
    {
        UE_LOG(LogTemp, Warning, TEXT("Build Failed: Cannot build buildings in Enemy Territory (X>=%d)!"), MaxPlayerX);
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Can't place here! Enemy Territory!"));
        return false;
    }

    TSubclassOf<ABaseBuilding> SpawnClass = nullptr;
    switch (Type)
    {
        case EBuildingType::Defense:      SpawnClass = DefenseTowerClass; break;
        case EBuildingType::GoldMine:     SpawnClass = GoldMineClass;     break;
        case EBuildingType::ElixirPump:   SpawnClass = ElixirPumpClass;   break;
        case EBuildingType::Wall:         SpawnClass = WallClass;         break;
        case EBuildingType::Headquarters: SpawnClass = HQClass;           break;
        case EBuildingType::Barracks:     SpawnClass = BarracksClass;     break;
    default: return false;
    }

    if (!SpawnClass || !GridManager) return false;

    if (!GridManager->IsTileWalkable(GridX, GridY))
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Blocked!"));
        return false;
    }

    // 高度计算
    float SpawnZOffset = 0.0f;
    ABaseBuilding* DefaultBuilding = SpawnClass->GetDefaultObject<ABaseBuilding>();
    if (DefaultBuilding)
    {
        FVector Origin, BoxExtent;
        DefaultBuilding->GetActorBounds(true, Origin, BoxExtent);
        SpawnZOffset = BoxExtent.Z;
    }

    FVector SpawnLoc = GridManager->GridToWorld(GridX, GridY);
    SpawnLoc.Z += SpawnZOffset;

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ABaseBuilding* NewBuilding = GetWorld()->SpawnActor<ABaseBuilding>(SpawnClass, SpawnLoc, FRotator::ZeroRotator, SpawnParams);
    if (NewBuilding)
    {        
        GI->PlayerGold -= Cost;

        NewBuilding->TeamID = ETeam::Player;
        NewBuilding->GridX = GridX;
        NewBuilding->GridY = GridY;

        GridManager->SetTileBlocked(GridX, GridY, true);
        return true;
    }
    return false;
}

// ---------------------------------------------------------
// 保存基地 (离开基地前调用)
// ---------------------------------------------------------
void ARTSGameMode::SaveBaseLayout()
{
    URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());
    if (!GI) return;

    // 1. 清空旧存档
    GI->SavedBuildings.Empty();
    GI->bHasSavedBase = true; // 标记我们已经存过档了

    // 2. 遍历场景里所有的建筑
    for (TActorIterator<ABaseBuilding> It(GetWorld()); It; ++It)
    {
        ABaseBuilding* Building = *It;
        // 只保存玩家的建筑，且活着的
        if (Building && Building->TeamID == ETeam::Player && Building->CurrentHealth > 0)
        {
            FBuildingSaveData Data;
            Data.BuildingType = Building->BuildingType;
            Data.GridX = Building->GridX;
            Data.GridY = Building->GridY;
            Data.Level = Building->BuildingLevel;

            // 如果是兵营，保存里面的兵
            if (Building->BuildingType == EBuildingType::Barracks)
            {
                // Cast 一下才能调子类函数
                if (ABuilding_Barracks* Barracks = Cast<ABuilding_Barracks>(Building))
                {
                    Data.StoredUnitTypes = Barracks->GetStoredUnitTypes();
                }
            }

            GI->SavedBuildings.Add(Data);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Base Saved! Total Buildings: %d"), GI->SavedBuildings.Num());
}

// ---------------------------------------------------------
// 加载基地 (回到基地时调用)
// ---------------------------------------------------------
void ARTSGameMode::LoadAndSpawnBase()
{
    URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());
    // 如果没有存过档(第一次玩)，或者没有 GridManager，就啥也不做
    if (!GI || !GridManager) return;

    // 检查是否有存档，如果没有，或者是初次游玩，也要保证有 HQ
    bool bHasHQ = false;

    if (GI->bHasSavedBase)
    {
        for (const FBuildingSaveData& Data : GI->SavedBuildings)
        {
            if (Data.BuildingType == EBuildingType::Headquarters) bHasHQ = true;

            TSubclassOf<ABaseBuilding> SpawnClass = nullptr;
            switch (Data.BuildingType)
            {
                case EBuildingType::Defense:      SpawnClass = DefenseTowerClass; break;
                case EBuildingType::GoldMine:     SpawnClass = GoldMineClass;     break;
                case EBuildingType::ElixirPump:   SpawnClass = ElixirPumpClass;   break;
                case EBuildingType::Wall:         SpawnClass = WallClass;         break;
                case EBuildingType::Barracks:     SpawnClass = BarracksClass;     break;
                case EBuildingType::Headquarters: SpawnClass = HQClass;           break;
            }


            if (!SpawnClass) continue;

            // 2. 算高度 (用之前的通用逻辑)
            float SpawnZOffset = 0.0f;
            ABaseBuilding* DefaultBuilding = SpawnClass->GetDefaultObject<ABaseBuilding>();
            if (DefaultBuilding)
            {
                FVector Origin, BoxExtent;
                DefaultBuilding->GetActorBounds(true, Origin, BoxExtent);
                SpawnZOffset = BoxExtent.Z;
            }

            FVector SpawnLoc = GridManager->GridToWorld(Data.GridX, Data.GridY);
            SpawnLoc.Z += SpawnZOffset;

            // 3. 生成
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

            ABaseBuilding* NewBuilding = GetWorld()->SpawnActor<ABaseBuilding>(SpawnClass, SpawnLoc, FRotator::ZeroRotator, Params);
            if (NewBuilding)
            {
                NewBuilding->TeamID = ETeam::Player;
                NewBuilding->GridX = Data.GridX;
                NewBuilding->GridY = Data.GridY;
                NewBuilding->BuildingLevel = Data.Level; // 恢复等级

                // 恢复阻挡
                GridManager->SetTileBlocked(Data.GridX, Data.GridY, true);

                // 如果是兵营，恢复库存
                if (Data.BuildingType == EBuildingType::Barracks)
                {
                    if (ABuilding_Barracks* Barracks = Cast<ABuilding_Barracks>(NewBuilding))
                    {
                        Barracks->RestoreStoredUnits(Data.StoredUnitTypes);
                    }
                }
            }
        }
    }

    // 保底逻辑：如果存档里没 HQ (或者第一次玩)，强制在 (0,0) 生一个 ---
    if (!bHasHQ)
    {
        UE_LOG(LogTemp, Warning, TEXT("No HQ found in save data. Spawning default HQ at (0,0)."));

        // 确保 (0,0) 没被占 (虽然 HQ 权力最大，但还是检查一下好)
        // 实际上 HQ 应该最先生成。
        if (HQClass)
        {
            TryBuildBuilding(EBuildingType::Headquarters, 0, 0, 0); // Cost设为0免费造
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Base Loaded!"));
}

void ARTSGameMode::SaveAndStartBattle(FName LevelName)
{
    // 先保存基地建筑
    SaveBaseLayout();


    // 保存兵力
    URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());
    if (!GI) return;

    // 清空旧数据
    GI->PlayerArmy.Empty();

    // 遍历场景里所有的玩家单位
    for (TActorIterator<ABaseUnit> It(GetWorld()); It; ++It)
    {
        ABaseUnit* Unit = *It;
        // 确保只保存玩家的兵，且活着的
        if (Unit && Unit->TeamID == ETeam::Player && Unit->CurrentHealth > 0)
        {
            if (GridManager)
            {
                int32 X, Y;
                // 使用成员B写在GridManager里的WorldToGrid
                if (GridManager->WorldToGrid(Unit->GetActorLocation(), X, Y))
                {
                    FUnitSaveData Data;
                    Data.UnitType = Unit->UnitType; // 使用 BaseUnit 里的 UnitType 变量
                    Data.GridX = X;
                    Data.GridY = Y;
                    GI->PlayerArmy.Add(Data);
                }
            }
        }
    }

    // 切换关卡
    UGameplayStatics::OpenLevel(this, LevelName);
}

void ARTSGameMode::StartBattlePhase()
{
    CurrentState = EGameState::Battle;
    for (TActorIterator<ABaseUnit> It(GetWorld()); It; ++It)
    {
        if (*It) (*It)->SetUnitActive(true);
    }
    UE_LOG(LogTemp, Log, TEXT("Battle Phase Started!"));
}

void ARTSGameMode::RestartLevel()
{
    UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetName()), false);
}

// 结算逻辑：把活着的兵存下来，然后回城
void ARTSGameMode::ReturnToBase()
{
    URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());
    if (!GI) return;

    // 1. 清空旧数据 (把出发前的阵容删掉，防止死兵复活)
    GI->PlayerArmy.Empty();

    // 2. 遍历战场上所有的兵
    for (TActorIterator<ABaseUnit> It(GetWorld()); It; ++It)
    {
        ABaseUnit* Unit = *It;

        // 筛选：必须是玩家的兵 + 必须活着 + 没有被标记销毁
        if (Unit &&
            Unit->TeamID == ETeam::Player &&
            Unit->CurrentHealth > 0 &&
            !Unit->IsPendingKill())
        {
            FUnitSaveData Data;
            Data.UnitType = Unit->UnitType;

            // 计算它当前在战场上的哪个格子 (作为回城后的参考位置)
            if (GridManager)
            {
                // 如果兵在移动中，可能不在格子上，但这没关系，WorldToGrid 会算最近的
                GridManager->WorldToGrid(Unit->GetActorLocation(), Data.GridX, Data.GridY);
            }

            GI->PlayerArmy.Add(Data);
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Returning to Base with %d survivors."), GI->PlayerArmy.Num());

    // 3. 回家
    UGameplayStatics::OpenLevel(this, FName("PlayerBase"));
}

// 加载逻辑：带防重叠功能的生成
void ARTSGameMode::LoadAndSpawnUnits()
{
    URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());
    if (!GI || !GridManager) return;

    FString MapName = GetWorld()->GetMapName();
    bool bIsHomeBase = !MapName.Contains("BattleField", ESearchCase::IgnoreCase);

    // 临时记录本轮生成的兵占用的格子 (防止自己人踩自己人)
    TSet<FIntPoint> OccupiedByUnits;

    for (const FUnitSaveData& Data : GI->PlayerArmy)
    {
        // 1. 匹配蓝图
        TSubclassOf<ABaseUnit> SpawnClass = nullptr;
        switch (Data.UnitType)
        {
            case EUnitType::Barbarian:  SpawnClass = BarbarianClass; break;
            case EUnitType::Archer:     SpawnClass = ArcherClass;    break;
            case EUnitType::Giant:      SpawnClass = GiantClass;     break;
            case EUnitType::Bomber:     SpawnClass = BomberClass;    break;
        }

        if (!SpawnClass) continue;

        // 确定中心点
        int32 CenterX, CenterY;
        if (bIsHomeBase)
        {
            CenterX = 0; CenterY = 0; // 基地：围绕 HQ(0,0)
        }
        else
        {
            CenterX = Data.GridX; CenterY = Data.GridY; // 战场：原位
        }

        // --- 螺旋搜索空位 ---
        bool bFoundSpot = false;
        int32 FinalX = CenterX;
        int32 FinalY = CenterY;

        // 搜索半径
        for (int32 Radius = 0; Radius <= 15; ++Radius) // 从 0 开始，先查中心
        {
            // 简单的矩形遍历算法
            for (int32 x = CenterX - Radius; x <= CenterX + Radius; ++x)
            {
                for (int32 y = CenterY - Radius; y <= CenterY + Radius; ++y)
                {
                    // 1. 检查物理阻挡 (建筑/墙)
                    if (!GridManager->IsTileWalkable(x, y)) continue;

                    // 2. 检查本轮生成的兵是否占了这个坑
                    if (OccupiedByUnits.Contains(FIntPoint(x, y))) continue;

                    // 找到空位了！
                    FinalX = x;
                    FinalY = y;
                    bFoundSpot = true;
                    goto SpotFound;
                }
            }
        }
        SpotFound:;

        // 如果实在没地方了，也只能重叠了(或者不生成)，这里选择跳过不生成以防卡死
        if (!bFoundSpot)
        {
            UE_LOG(LogTemp, Error, TEXT("No space to spawn unit!"));
            continue;
        }

        // 记录占用
        OccupiedByUnits.Add(FIntPoint(FinalX, FinalY));

        // 3. 计算世界坐标
        FVector SpawnLoc = GridManager->GridToWorld(FinalX, FinalY);

        // 4. 计算高度 (保持之前的完美贴地逻辑)
        float SpawnZOffset = 0.0f;
        ABaseUnit* DefaultUnit = SpawnClass->GetDefaultObject<ABaseUnit>();
        if (DefaultUnit)
        {
            UCapsuleComponent* Cap = DefaultUnit->FindComponentByClass<UCapsuleComponent>();
            if (Cap)
                SpawnZOffset = Cap->GetScaledCapsuleHalfHeight();
            else
            {
                FVector Origin, BoxExtent;
                DefaultUnit->GetActorBounds(true, Origin, BoxExtent);
                SpawnZOffset = BoxExtent.Z;
            }
        }
        SpawnLoc.Z += SpawnZOffset;

        // 5. 生成
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        ABaseUnit* NewUnit = GetWorld()->SpawnActor<ABaseUnit>(SpawnClass, SpawnLoc, FRotator::ZeroRotator, Params);
        if (NewUnit)
        {
            NewUnit->TeamID = ETeam::Player;
            // 锁定新的格子
            // GridManager->SetTileBlocked(FinalX, FinalY, true);
        }
    }
}

void ARTSGameMode::OnActorKilled(AActor* Victim, AActor* Killer)
{
    if (!Victim) return;
    UE_LOG(LogTemp, Log, TEXT("Actor Killed: %s"), *Victim->GetName());

    // 士兵死亡返还人口
    // 1. 尝试把受害者转成士兵
    ABaseUnit* DeadUnit = Cast<ABaseUnit>(Victim);

    // 2. 只有当它是士兵，且属于玩家阵营时，才返还
    if (DeadUnit && DeadUnit->TeamID == ETeam::Player)
    {
        URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());
        if (GI)
        {
            // 人口 -1，但最小不能小于 0
            GI->CurrentPopulation = FMath::Max(0, GI->CurrentPopulation - 1);

            UE_LOG(LogTemp, Warning, TEXT("Unit Died! Population returned. Current: %d/%d"),
                GI->CurrentPopulation, GI->MaxPopulation);
        }
    }

    CheckWinCondition();
}

bool ARTSGameMode::SpawnUnitAt(EUnitType Type, int32 GridX, int32 GridY)
{
    if (!GridManager) return false;

    // 1. 匹配蓝图
    TSubclassOf<ABaseUnit> SpawnClass = nullptr;
    switch (Type)
    {
    case EUnitType::Barbarian:  SpawnClass = BarbarianClass; break;
    case EUnitType::Archer:     SpawnClass = ArcherClass;    break;
    case EUnitType::Giant:      SpawnClass = GiantClass;     break;
    case EUnitType::Bomber:     SpawnClass = BomberClass;    break;
    }
    if (!SpawnClass) return false;

    // 2. 高度计算 (复制之前的逻辑)
    FVector SpawnLoc = GridManager->GridToWorld(GridX, GridY);
    float SpawnZOffset = 0.0f;
    ABaseUnit* DefaultUnit = SpawnClass->GetDefaultObject<ABaseUnit>();
    if (DefaultUnit)
    {
        UCapsuleComponent* Cap = DefaultUnit->FindComponentByClass<UCapsuleComponent>();
        if (Cap) SpawnZOffset = Cap->GetScaledCapsuleHalfHeight();
        else
        {
            FVector Origin, BoxExtent;
            DefaultUnit->GetActorBounds(true, Origin, BoxExtent);
            SpawnZOffset = BoxExtent.Z;
        }
    }
    SpawnLoc.Z += SpawnZOffset;

    // 3. 生成
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ABaseUnit* NewUnit = GetWorld()->SpawnActor<ABaseUnit>(SpawnClass, SpawnLoc, FRotator::ZeroRotator, SpawnParams);
    if (NewUnit)
    {
        NewUnit->TeamID = ETeam::Player;
        GridManager->SetTileBlocked(GridX, GridY, true);
        return true;
    }
    return false;
}

void ARTSGameMode::CheckWinCondition()
{
    // 统计场上敌人的大本营
    TArray<AActor*> EnemyHQs;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseBuilding::StaticClass(), EnemyHQs);

    int32 EnemyHQCount = 0;
    for (AActor* Actor : EnemyHQs)
    {
        ABaseBuilding* Building = Cast<ABaseBuilding>(Actor);
        // 必须是敌人，必须是大本营，必须活着
        if (Building && Building->TeamID == ETeam::Enemy &&
            Building->BuildingType == EBuildingType::Headquarters &&
            Building->CurrentHealth > 0)
        {
            EnemyHQCount++;
        }
    }

    // 胜利条件
    if (EnemyHQCount == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("VICTORY! Enemy HQ Destroyed!"));
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("VICTORY!"));

        // [奖励逻辑] 在这里给 GameInstance 加钱
        URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());
        if (GI)
        {
            GI->PlayerGold += 1000;
            GI->PlayerElixir += 1000;
        }

        // 3秒后回城 (使用 Timer)
        FTimerHandle TimerHandle;
        GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
            {
                ReturnToBase();
            }, 3.0f, false);

        return; // 结束检查
    }

    // 2. 统计场上还剩多少【玩家的兵】
    TArray<AActor*> PlayerUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseUnit::StaticClass(), PlayerUnits);

    int32 PlayerUnitCount = 0;
    for (AActor* Actor : PlayerUnits)
    {
        ABaseUnit* Unit = Cast<ABaseUnit>(Actor);
        if (Unit && Unit->TeamID == ETeam::Player && Unit->CurrentHealth > 0)
        {
            PlayerUnitCount++;
        }
    }

    // --- 失败条件 ---
    // 如果玩家没兵了，且没赢
    if (PlayerUnitCount == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("DEFEAT! All units lost!"));
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("DEFEAT!"));

        // 3秒后回城
        FTimerHandle TimerHandle;
        GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
            {
                ReturnToBase();
            }, 3.0f, false);
    }
}

bool ARTSGameMode::TryUpgradeBuilding(ABaseBuilding* BuildingToUpgrade)
{
    if (!BuildingToUpgrade) return false;
    if (CurrentState != EGameState::Preparation) return false; // 只有备战期能升级

    // 1. 检查是否满级
    if (!BuildingToUpgrade->CanUpgrade())
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Max Level Reached!"));
        return false;
    }

    // 2. 获取费用
    int32 GoldCost = 0;
    int32 ElixirCost = 0;
    BuildingToUpgrade->GetUpgradeCost(GoldCost, ElixirCost);

    URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());
    if (!GI) return false;

    // 3. 检查资源
    if (GI->PlayerGold < GoldCost)
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Not Enough Gold to Upgrade!"));
        return false;
    }

    // 4. 执行升级
    GI->PlayerGold -= GoldCost;

    BuildingToUpgrade->LevelUp(); // 调用建筑自己的升级逻辑(加血/变大)

    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("Upgrade Successful!"));
    return true;
}