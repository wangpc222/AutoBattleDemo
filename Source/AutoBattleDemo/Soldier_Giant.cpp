#include "Soldier_Giant.h"
#include "Building_Defense.h"
#include "BaseBuilding.h"
#include "Kismet/GameplayStatics.h"

ASoldier_Giant::ASoldier_Giant()
{
    // 设置兵种类型
    UnitType = EUnitType::Giant;

    // 巨人属性
    MaxHealth = 500.0f;
    AttackRange = 150.0f;
    Damage = 30.0f;
    MoveSpeed = 150.0f;
    AttackInterval = 1.5f;
}

void ASoldier_Giant::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("[Giant] %s spawned | HP: %f | Damage: %f"),
        *GetName(), MaxHealth, Damage);
}

AActor* ASoldier_Giant::FindClosestEnemyBuilding()
{
    // 第一优先级：防御塔
    TArray<AActor*> AllDefenses;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABuilding_Defense::StaticClass(), AllDefenses);

    AActor* ClosestDefense = nullptr;
    float ClosestDefenseDistance = FLT_MAX;

    for (AActor* Actor : AllDefenses)
    {
        ABuilding_Defense* Defense = Cast<ABuilding_Defense>(Actor);

        if (Defense &&
            Defense->TeamID != this->TeamID &&
            Defense->CurrentHealth > 0)
        {
            float Distance = FVector::Dist(GetActorLocation(), Defense->GetActorLocation());

            if (Distance <= AttackRange)
            {
                UE_LOG(LogTemp, Warning, TEXT("[Giant] %s targeting defense tower: %s (in range)"),
                    *GetName(), *Defense->GetName());
                return Defense;
            }

            if (Distance < ClosestDefenseDistance)
            {
                ClosestDefenseDistance = Distance;
                ClosestDefense = Defense;
            }
        }
    }

    if (ClosestDefense)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Giant] %s targeting defense tower: %s (distance: %f)"),
            *GetName(), *ClosestDefense->GetName(), ClosestDefenseDistance);
        return ClosestDefense;
    }

    // 第二优先级：普通建筑
    TArray<AActor*> AllBuildings;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseBuilding::StaticClass(), AllBuildings);

    AActor* ClosestBuilding = nullptr;
    float ClosestDistance = FLT_MAX;

    for (AActor* Actor : AllBuildings)
    {
        ABaseBuilding* Building = Cast<ABaseBuilding>(Actor);

        if (Building &&
            !Building->IsA(ABuilding_Defense::StaticClass()) &&
            Building->TeamID != this->TeamID &&
            Building->bIsTargetable &&
            Building->CurrentHealth > 0)
        {
            float Distance = FVector::Dist(GetActorLocation(), Building->GetActorLocation());

            if (Distance <= AttackRange)
            {
                return Building;
            }

            if (Distance < ClosestDistance)
            {
                ClosestDistance = Distance;
                ClosestBuilding = Building;
            }
        }
    }

    if (ClosestBuilding)
    {
        UE_LOG(LogTemp, Log, TEXT("[Giant] %s targeting normal building: %s"),
            *GetName(), *ClosestBuilding->GetName());
    }

    return ClosestBuilding;
}