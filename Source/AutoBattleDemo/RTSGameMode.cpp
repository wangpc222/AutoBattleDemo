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
    else if (Type == EUnitType::Archer) SpawnClass = ArcherClass;

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