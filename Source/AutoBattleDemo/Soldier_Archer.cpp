#include "Soldier_Archer.h"

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
        FDamageEvent DamageEvent;
        CurrentTarget->TakeDamage(Damage, DamageEvent, nullptr, this);
        LastAttackTime = CurrentTime;

        FVector Direction = (CurrentTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal();
        if (!Direction.IsNearlyZero())
        {
            FRotator NewRotation = Direction.Rotation();
            NewRotation.Pitch = 0;
            NewRotation.Roll = 0;
            SetActorRotation(NewRotation);
        }

        UE_LOG(LogTemp, Log, TEXT("[Archer] %s shot %s from distance %f!"),
            *GetName(), *CurrentTarget->GetName(), Distance);
    }
}