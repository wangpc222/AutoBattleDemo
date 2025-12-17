#include "RTSGameMode.h"
#include "RTSPlayerController.h"
#include "GridManager.h"
#include "BaseUnit.h"
#include "RTSGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Components/CapsuleComponent.h"	// 胶囊体组件

ARTSGameMode::ARTSGameMode()
{
	// 设置默认控制器
	PlayerControllerClass = ARTSPlayerController::StaticClass();
	CurrentState = EGameState::Preparation;
}

void ARTSGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 缓存 GridManager，避免每一帧都去搜索
	GridManager = Cast<AGridManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass()));
}

bool ARTSGameMode::TryBuyUnit(EUnitType Type, int32 Cost, int32 GridX, int32 GridY)
{
    // 1. 规则检查
    if (CurrentState != EGameState::Preparation) return false;

    // 2. 检查金币
    URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());
    int32 CurrentGold = GI ? GI->PlayerGold : 9999;

    if (CurrentGold < Cost)
    {
        UE_LOG(LogTemp, Warning, TEXT("Buy failed: Not enough gold"));
        return false;
    }

    // 3. 选择蓝图类
    TSubclassOf<ABaseUnit> SpawnClass = nullptr;
    if (Type == EUnitType::Soldier) SpawnClass = SoldierClass;
    
    switch (Type)
    {
        case EUnitType::Soldier: SpawnClass = SoldierClass; break;
        case EUnitType::Archer:  SpawnClass = ArcherClass;  break;
        // --- 新增 ---
        case EUnitType::Giant:   SpawnClass = GiantClass;   break;
        case EUnitType::Boomer:  SpawnClass = BoomerClass;  break;
        // ------------
        default: return false;
    }

    if (!SpawnClass || !GridManager) return false;

    // ---------------------------------------------------------
    // 通用高度计算 (兼容 Pawn 和 Character)
    // ---------------------------------------------------------

    float SpawnZOffset = 100.0f; // 默认备用高度

    // 获取默认对象 (CDO)
    ABaseUnit* DefaultUnit = SpawnClass->GetDefaultObject<ABaseUnit>();
    if (DefaultUnit)
    {
        // 尝试找胶囊体组件 (不管它是 Character 还是 Pawn，只要有这个组件就行)
        UCapsuleComponent* UnitCapsule = DefaultUnit->FindComponentByClass<UCapsuleComponent>();

        if (UnitCapsule)
        {
            // 找到了胶囊体，用它的半高
            SpawnZOffset = UnitCapsule->GetScaledCapsuleHalfHeight();
        }
        else
        {
            // 如果没胶囊体(比如是方块)，尝试算整个物体的边界高度
            FVector Origin, BoxExtent;
            DefaultUnit->GetActorBounds(true, Origin, BoxExtent);
            if (BoxExtent.Z > 0)
            {
                SpawnZOffset = BoxExtent.Z + 2.0f;
            }
        }
    }

    // 4. 计算坐标
    FVector SpawnLoc = GridManager->GridToWorld(GridX, GridY);
    SpawnLoc.Z += SpawnZOffset; // 使用算出来的高度

    // 5. 生成
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ABaseUnit* NewUnit = GetWorld()->SpawnActor<ABaseUnit>(SpawnClass, SpawnLoc, FRotator::ZeroRotator, SpawnParams);
    if (NewUnit)
    {
        if (GI) GI->PlayerGold -= Cost;
        NewUnit->TeamID = ETeam::Player;
        GridManager->SetTileBlocked(GridX, GridY, true);
        return true;
    }

    return false;
}

bool ARTSGameMode::TryBuildBuilding(EBuildingType Type, int32 Cost, int32 GridX, int32 GridY)
{
    // 1. 状态检查 (备战期才能造)
    if (CurrentState != EGameState::Preparation) return false;

    // 2. 检查金币
    URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());
    int32 CurrentGold = GI ? GI->PlayerGold : 9999;
    if (CurrentGold < Cost) return false;

    // 3. 选择建筑蓝图
    TSubclassOf<ABaseBuilding> SpawnClass = nullptr;
    switch (Type)
    {
    case EBuildingType::DefenseTower: SpawnClass = DefenseTowerClass; break;
    case EBuildingType::GoldMine:     SpawnClass = GoldMineClass; break;
    case EBuildingType::HQ:           SpawnClass = HQClass; break;
    default: return false;
    }

    if (!SpawnClass || !GridManager) return false;

    // 4. 计算高度 (建筑版)
    float SpawnZOffset = 0.0f;
    ABaseBuilding* DefaultBuilding = SpawnClass->GetDefaultObject<ABaseBuilding>();
    if (DefaultBuilding)
    {
        // 建筑通常没有胶囊体，而是通过 GetActorBounds 算 Mesh 的高度
        FVector Origin, BoxExtent;
        DefaultBuilding->GetActorBounds(true, Origin, BoxExtent);

        // 如果原点在底部，Offset设为0；如果原点在中心，Offset设为半高
        // 假设大家做的建筑模型原点都在中心
        SpawnZOffset = BoxExtent.Z;
    }

    // 计算位置
    FVector SpawnLoc = GridManager->GridToWorld(GridX, GridY);
    SpawnLoc.Z += SpawnZOffset; // 建筑贴地

    // 5. 生成
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ABaseBuilding* NewBuilding = GetWorld()->SpawnActor<ABaseBuilding>(SpawnClass, SpawnLoc, FRotator::ZeroRotator, SpawnParams);

    if (NewBuilding)
    {
        // 扣钱
        if (GI) GI->PlayerGold -= Cost;

        // 设置阵营
        NewBuilding->TeamID = ETeam::Player;

        // 锁定格子
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
            // 还需要知道它在哪个格子里！
            // 这里假设 GridManager 提供了 WorldToGrid，或者我们在 BaseUnit 里存了 GridX/Y
            // 为了简单，我们反向计算一下：
            if (GridManager)
            {
                int32 X, Y;
                if (GridManager->WorldToGrid(Unit->GetActorLocation(), X, Y))
                {
                    FUnitSaveData Data;
                    // 这里假设 BaseUnit 里有个变量存 Type，或者根据 Class 判断
                    // 简单起见，假设所有 BaseUnit 都是 Soldier，你可以后续优化区分
                    Data.UnitType = EUnitType::Soldier;
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

    // 遍历保存的数据
    for (const FUnitSaveData& Data : GI->PlayerArmy)
    {
        // 复用之前的 TryBuyUnit 逻辑，但是把 Cost 设为 0 (因为已经买过了)
        // 或者直接调用 SpawnActor (推荐直接 Spawn，因为 TryBuyUnit 会扣钱且限制阶段)
        TSubclassOf<ABaseUnit> SpawnClass = nullptr;

        switch (Data.UnitType)
        {
            case EUnitType::Soldier: SpawnClass = SoldierClass; break;
            case EUnitType::Archer:  SpawnClass = ArcherClass;  break;
            case EUnitType::Giant:   SpawnClass = GiantClass;   break;
            case EUnitType::Boomer:  SpawnClass = BoomerClass;  break;
        }

        // 如果没找到对应的类，跳过
        if (!SpawnClass) continue;
        // 计算位置 (复制之前的逻辑)
        FVector SpawnLoc = GridManager->GridToWorld(Data.GridX, Data.GridY);

        // 重新计算高度 (使用之前的 CDO 方法)
        float Height = 100.0f;
        if (SpawnClass)
        {
            ABaseUnit* CDO = SpawnClass->GetDefaultObject<ABaseUnit>();
            UCapsuleComponent* Cap = CDO->FindComponentByClass<UCapsuleComponent>();
            if (Cap) Height = Cap->GetScaledCapsuleHalfHeight();
        }
        SpawnLoc.Z += Height;

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

	// 遍历所有单位，激活 AI
	for (TActorIterator<ABaseUnit> It(GetWorld()); It; ++It)
	{
		if (*It) (*It)->SetUnitActive(true);
	}

	UE_LOG(LogTemp, Log, TEXT("Battle Phase Started!"));
}

void ARTSGameMode::RestartLevel()
{
	// 重新加载当前关卡
	UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetName()), false);
}

void ARTSGameMode::OnActorKilled(AActor* Victim, AActor* Killer)
{
	if (!Victim) return;

	UE_LOG(LogTemp, Log, TEXT("Actor Killed: %s"), *Victim->GetName());

	// 检查胜负条件
	CheckWinCondition();
}

void ARTSGameMode::CheckWinCondition()
{
	// 示例逻辑：
	// 1. 统计场上还剩多少敌人
	// 2. 统计场上还剩多少玩家单位
	// 3. 如果某一方数量为 0，调用 FinishGame()
}