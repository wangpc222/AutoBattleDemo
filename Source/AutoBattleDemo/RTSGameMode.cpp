#include "RTSGameMode.h"
#include "RTSPlayerController.h"
#include "GridManager.h"
#include "BaseUnit.h"
#include "BaseBuilding.h"
#include "RTSGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
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

    // [调试] 打印当前地图名字
    FString CurrentMapName = GetWorld()->GetMapName();
    FString CleanMapName = CurrentMapName;
    CleanMapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix); // 去掉 UEDPIE_0_ 前缀

    // 1. 先打个日志证明 GameMode 活了
    UE_LOG(LogTemp, Error, TEXT(">>> RTSGameMode BeginPlay Running! Map: %s"), *GetWorld()->GetMapName());

    GridManager = Cast<AGridManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass()));

    // 如果配置了关卡数据且 GridManager 存在，加载敌方配置
    if (GridManager && CurrentLevelData)
    {
        GridManager->LoadLevelFromDataAsset(CurrentLevelData);
    }

    // 简单判断：如果是战斗关卡，直接进入战斗状态
    FString MapName = GetWorld()->GetMapName();
    if (MapName.Contains("BattleField", ESearchCase::IgnoreCase)) // 战斗关卡名字包含 BattleField
    {
        // 调试
        UE_LOG(LogTemp, Warning, TEXT(">>> Detected Battle Map! Spawning Units...")); // 调试日志

        CurrentState = EGameState::Battle;
        LoadAndSpawnUnits(); // 把带来的兵放出来
        StartBattlePhase();  // 激活 AI
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> Detected Base Map. Loading Buildings...")); // 调试日志

        CurrentState = EGameState::Preparation; // 基地里是备战

        // 回到基地，重新把房子盖起来
        LoadAndSpawnBase();
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
        GridManager->SetTileBlocked(GridX, GridY, true);
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
    if (!GI || !GridManager || !GI->bHasSavedBase) return;

    // 遍历存档
    for (const FBuildingSaveData& Data : GI->SavedBuildings)
    {
        // 1. 找蓝图
        TSubclassOf<ABaseBuilding> SpawnClass = nullptr;
        switch (Data.BuildingType)
        {
            case EBuildingType::Defense:      SpawnClass = DefenseTowerClass; break;
            case EBuildingType::GoldMine:     SpawnClass = GoldMineClass;     break;
            case EBuildingType::ElixirPump:   SpawnClass = ElixirPumpClass;   break;
            case EBuildingType::Wall:         SpawnClass = WallClass;         break;
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

void ARTSGameMode::LoadAndSpawnUnits()
{
    // [调试 1] 检查函数是否被调用
    UE_LOG(LogTemp, Error, TEXT(">>> LoadAndSpawnUnits START! <<<"));

    URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());

    // [调试 2] 检查关键指针
    if (!GI) UE_LOG(LogTemp, Error, TEXT(">>> GI is NULL!"));
    if (!GridManager) UE_LOG(LogTemp, Error, TEXT(">>> GridManager is NULL!"));


    if (!GI || !GridManager) return;

    // [调试 3] 检查存档数量
    UE_LOG(LogTemp, Warning, TEXT(">>> Army Count in SaveData: %d"), GI->PlayerArmy.Num());

    for (const FUnitSaveData& Data : GI->PlayerArmy)
    {
        TSubclassOf<ABaseUnit> SpawnClass = nullptr;
        switch (Data.UnitType)
        {
        case EUnitType::Barbarian:  SpawnClass = BarbarianClass; break;
        case EUnitType::Archer:     SpawnClass = ArcherClass;    break;
        case EUnitType::Giant:      SpawnClass = GiantClass;     break;
        case EUnitType::Bomber:     SpawnClass = BomberClass;    break;
        }

        if (!SpawnClass) continue;

        // 计算位置
        FVector SpawnLoc = GridManager->GridToWorld(Data.GridX, Data.GridY);

        // 使用与 TryBuyUnit 完全一致的高度计算逻辑
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






        // 调试
        UE_LOG(LogTemp, Warning, TEXT("Spawn Unit: %d | GridZ: %f | Offset: %f | FinalZ: %f"),
            (int32)Data.UnitType,
            GridManager->GetActorLocation().Z,
            SpawnZOffset,
            SpawnLoc.Z
        );

        






        

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        ABaseUnit* NewUnit = GetWorld()->SpawnActor<ABaseUnit>(SpawnClass, SpawnLoc, FRotator::ZeroRotator, Params);
        if (NewUnit)
        {
            NewUnit->TeamID = ETeam::Player;
            GridManager->SetTileBlocked(Data.GridX, Data.GridY, true);
        }
    }
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

void ARTSGameMode::OnActorKilled(AActor* Victim, AActor* Killer)
{
    if (!Victim) return;
    UE_LOG(LogTemp, Log, TEXT("Actor Killed: %s"), *Victim->GetName());
    CheckWinCondition();
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
                UGameplayStatics::OpenLevel(this, FName("PlayerBase"));
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
                UGameplayStatics::OpenLevel(this, FName("PlayerBase"));
            }, 3.0f, false);
    }
}

