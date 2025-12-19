#include "Building_Resource.h"

ABuilding_Resource::ABuilding_Resource()
{
    PrimaryActorTick.bCanEverTick = true;

    BuildingType = EBuildingType::Resource;

    // 资源建筑属性
    MaxHealth = 600.0f;
    ProductionRate = 10.0f;    // 每秒10金币
    MaxStorage = 1000.0f;
    CurrentStorage = 0.0f;
    bProducesGold = true;      // 默认产金币

    ProductionTimer = 0.0f;

    // 资源建筑通常属于玩家
    TeamID = ETeam::Player;
    bIsTargetable = true;
}
 
void ABuilding_Resource::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Log, TEXT("[Resource] %s | Rate: %f/s | MaxStorage: %f | Type: %s"),
        *GetName(), ProductionRate, MaxStorage, bProducesGold ? TEXT("Gold") : TEXT("Elixir"));
}

void ABuilding_Resource::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 资源生产逻辑
    if (CurrentStorage < MaxStorage)
    {
        ProductionTimer += DeltaTime;

        // 每秒检查一次
        if (ProductionTimer >= 1.0f)
        {
            float ProducedAmount = ProductionRate * ProductionTimer;
            CurrentStorage = FMath::Min(CurrentStorage + ProducedAmount, MaxStorage);

            ProductionTimer = 0.0f;

            // 调试输出
            UE_LOG(LogTemp, Log, TEXT("[Resource] %s produced %.1f | Storage: %.0f/%.0f"),
                *GetName(), ProducedAmount, CurrentStorage, MaxStorage);
        }
    }
}

float ABuilding_Resource::CollectResource()
{
    float CollectedAmount = CurrentStorage;
    CurrentStorage = 0.0f;

    UE_LOG(LogTemp, Warning, TEXT("[Resource] %s collected %.0f %s!"),
        *GetName(), CollectedAmount, bProducesGold ? TEXT("Gold") : TEXT("Elixir"));

    if (GEngine)
    {
        FString ResourceType = bProducesGold ? TEXT("Gold") : TEXT("Elixir");
        FString Msg = FString::Printf(TEXT("Collected %.0f %s"), CollectedAmount, *ResourceType);
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, Msg);
    }

    return CollectedAmount;
}

void ABuilding_Resource::ApplyLevelUpBonus()
{
    Super::ApplyLevelUpBonus();

    // 每级提升 20% 产量和 30% 存储上限
    ProductionRate *= 1.2f;
    MaxStorage *= 1.3f;

    UE_LOG(LogTemp, Warning, TEXT("[Resource] %s upgraded | Rate: %f/s | MaxStorage: %f"),
        *GetName(), ProductionRate, MaxStorage);
}