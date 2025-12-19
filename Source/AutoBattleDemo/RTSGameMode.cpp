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

    GridManager = Cast<AGridManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass()));

    // 如果配置了关卡数据且 GridManager 存在，加载敌方配置
    if (GridManager && CurrentLevelData)
    {
        GridManager->LoadLevelFromDataAsset(CurrentLevelData);
    }

    // 简单判断：如果是战斗关卡，直接进入战斗状态
    FString MapName = GetWorld()->GetMapName();
    if (MapName.Contains("BattleField")) // 假设战斗关卡名字包含 BattleField
    {
        CurrentState = EGameState::Battle;
        LoadAndSpawnUnits(); // 把带来的兵放出来
        StartBattlePhase();  // 激活 AI
    }
    else
    {
        CurrentState = EGameState::Preparation; // 基地里是备战
    }
}

// 造兵逻辑
bool ARTSGameMode::TryBuyUnit(EUnitType Type, int32 Cost, int32 GridX, int32 GridY)
{
    if (CurrentState != EGameState::Preparation) return false;

    URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());
    // 造兵通常消耗圣水 (Elixir)
    int32 CurrentElixir = GI ? GI->PlayerElixir : 9999;

    if (CurrentElixir < Cost)
    {
        UE_LOG(LogTemp, Warning, TEXT("Buy Unit Failed: Not enough Elixir!"));
        return false;
    }

    // 匹配蓝图
    TSubclassOf<ABaseUnit> SpawnClass = nullptr;
    switch (Type)
    {
        case EUnitType::Barbarian:  SpawnClass = BarbarianClass; break;
        case EUnitType::Archer:     SpawnClass = ArcherClass;    break;
        case EUnitType::Giant:      SpawnClass = GiantClass;     break;
        case EUnitType::Bomber:     SpawnClass = BoomerClass;    break;
    }

    if (!SpawnClass || !GridManager) return false;

    // --- 高度计算 (通用版：支持 Pawn 和 Character) ---
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

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ABaseUnit* NewUnit = GetWorld()->SpawnActor<ABaseUnit>(SpawnClass, SpawnLoc, FRotator::ZeroRotator, SpawnParams);
    if (NewUnit)
    {
        if (GI) GI->PlayerElixir -= Cost; // 扣除圣水
        NewUnit->TeamID = ETeam::Player;
        GridManager->SetTileBlocked(GridX, GridY, true); // 兵也占格子
        return true;
    }
    return false;
}

// 造建筑逻辑
bool ARTSGameMode::TryBuildBuilding(EBuildingType Type, int32 Cost, int32 GridX, int32 GridY)
{
    if (CurrentState != EGameState::Preparation) return false;

    URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());
    // 造建筑通常消耗金币 (Gold)
    int32 CurrentGold = GI ? GI->PlayerGold : 9999;

    if (CurrentGold < Cost)
    {
        UE_LOG(LogTemp, Warning, TEXT("Build Failed: Not enough Gold!"));
        return false;
    }

    TSubclassOf<ABaseBuilding> SpawnClass = nullptr;
    switch (Type)
    {
    case EBuildingType::Defense:      SpawnClass = DefenseTowerClass; break;
    case EBuildingType::Resource:     SpawnClass = GoldMineClass;     break; // 默认金矿
    case EBuildingType::Wall:         SpawnClass = WallClass;         break;
    case EBuildingType::Headquarters: SpawnClass = HQClass;           break;
    default: return false;
    }

    if (!SpawnClass || !GridManager) return false;

    // 建筑高度计算
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
        if (GI) GI->PlayerGold -= Cost; // 扣除金币
        NewBuilding->TeamID = ETeam::Player;

        // 关键：保存网格坐标，因为BaseBuilding有GridX/GridY变量
        NewBuilding->GridX = GridX;
        NewBuilding->GridY = GridY;

        GridManager->SetTileBlocked(GridX, GridY, true);
        return true;
    }
    return false;
}

void ARTSGameMode::SaveAndStartBattle(FName LevelName)
{
    URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());
    if (!GI) return;

    // 1. 清空旧数据
    GI->PlayerArmy.Empty();

    // 2. 遍历场景里所有的玩家单位
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

    // 3. 切换关卡
    UGameplayStatics::OpenLevel(this, LevelName);
}

void ARTSGameMode::LoadAndSpawnUnits()
{
    URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());
    if (!GI || !GridManager) return;

    for (const FUnitSaveData& Data : GI->PlayerArmy)
    {
        TSubclassOf<ABaseUnit> SpawnClass = nullptr;
        switch (Data.UnitType)
        {
        case EUnitType::Soldier:    SpawnClass = BarbarianClass; break;
        case EUnitType::Barbarian:  SpawnClass = BarbarianClass; break;
        case EUnitType::Archer:     SpawnClass = ArcherClass;    break;
        case EUnitType::Giant:      SpawnClass = GiantClass;     break;
        case EUnitType::Bomber:     SpawnClass = BoomerClass;    break;
        }

        if (!SpawnClass) continue;

        // 计算位置
        FVector SpawnLoc = GridManager->GridToWorld(Data.GridX, Data.GridY);
        float SpawnZOffset = 0.0f;
        ABaseUnit* DefaultUnit = SpawnClass->GetDefaultObject<ABaseUnit>();
        if (DefaultUnit)
        {
            UCapsuleComponent* Cap = DefaultUnit->FindComponentByClass<UCapsuleComponent>();
            if (Cap) SpawnZOffset = Cap->GetScaledCapsuleHalfHeight();
        }
        SpawnLoc.Z += SpawnZOffset;

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
    // 这里由你扩展：
    // 检查是否还有 Enemy HQ -> 胜利
    // 检查是否还有 Player Units -> 失败
}