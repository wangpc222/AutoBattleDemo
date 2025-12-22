#include "Building_HQ.h"
#include "RTSGameInstance.h"
#include "Kismet/GameplayStatics.h"

ABuilding_HQ::ABuilding_HQ()
{
    BuildingType = EBuildingType::Headquarters;
    MaxHealth = 5000.0f; // 大本营血厚
    TeamID = ETeam::Player;
}

void ABuilding_HQ::BeginPlay()
{
    Super::BeginPlay();

    // 只有玩家的 HQ 才加人口
    if (TeamID == ETeam::Player)
    {
        URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());
        if (GI)
        {
            GI->MaxPopulation += 20; // 基础人口
            UE_LOG(LogTemp, Log, TEXT("HQ Built! MaxPop +20. Current: %d"), GI->MaxPopulation);
        }
    }
}

void ABuilding_HQ::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    if (EndPlayReason == EEndPlayReason::Destroyed && TeamID == ETeam::Player)
    {
        URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());
        if (GI)
        {
            GI->MaxPopulation = FMath::Max(0, GI->MaxPopulation - 20);
        }
    }
}