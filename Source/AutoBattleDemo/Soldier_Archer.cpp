#include "Soldier_Archer.h"
#include "RTSProjectile.h" 

ASoldier_Archer::ASoldier_Archer()
{
    // 设置兵种类型
    UnitType = EUnitType::Archer;

    // 弓箭手属性
    MaxHealth = 80.0f;
    AttackRange = 500.0f;
    Damage = 12.0f;
    MoveSpeed = 200.0f;
    AttackInterval = 1.2f;
}

void ASoldier_Archer::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("[Archer] %s spawned | HP: %f | Range: %f"),
        *GetName(), MaxHealth, AttackRange);
}

void ASoldier_Archer::PerformAttack()
{
    if (!CurrentTarget)
    {
        CurrentState = EUnitState::Idle;
        return;
    }

    ABaseGameEntity* TargetEntity = Cast<ABaseGameEntity>(CurrentTarget);
    if (!TargetEntity || TargetEntity->CurrentHealth <= 0)
    {
        CurrentTarget = nullptr;
        CurrentState = EUnitState::Idle;
        return;
    }

    float Distance = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());

    if (Distance > AttackRange)
    {
        RequestPathToTarget();
        if (PathPoints.Num() > 0)
        {
            CurrentState = EUnitState::Moving;
        }
        return;
    }

    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastAttackTime >= AttackInterval)
    {
        LastAttackTime = CurrentTime;

        // --- 修改：生成投射物，而不是直接 TakeDamage ---
        if (ProjectileClass)
        {
            FVector SpawnLoc = GetActorLocation() + FVector(0, 0, 50); // 从胸口发射
            FRotator SpawnRot = (CurrentTarget->GetActorLocation() - SpawnLoc).Rotation();

            ARTSProjectile* Arrow = GetWorld()->SpawnActor<ARTSProjectile>(ProjectileClass, SpawnLoc, SpawnRot);
            if (Arrow)
            {
                Arrow->Initialize(CurrentTarget, Damage, this);
            }
        }
        else
        {
            // 如果没配子弹，保底还是直接扣血
            FDamageEvent DamageEvent;
            CurrentTarget->TakeDamage(Damage, DamageEvent, nullptr, this);
        }

        UE_LOG(LogTemp, Log, TEXT("Archer Fired Arrow!"));
    }
}