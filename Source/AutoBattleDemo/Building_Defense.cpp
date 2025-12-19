#include "Building_Defense.h"
#include "BaseUnit.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

ABuilding_Defense::ABuilding_Defense()
{
    PrimaryActorTick.bCanEverTick = true;

    BuildingType = EBuildingType::Defense;

    // 防御塔默认属性
    MaxHealth = 800.0f;
    AttackRange = 600.0f;
    Damage = 20.0f;
    FireRate = 1.0f; // 每秒1发

    CurrentTarget = nullptr;
    LastFireTime = 0.0f;
}

void ABuilding_Defense::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("[Defense] %s ready | Range: %f | Damage: %f | FireRate: %f/s"),
        *GetName(), AttackRange, Damage, FireRate);
}
 
void ABuilding_Defense::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 绘制攻击范围（调试用）
    DrawAttackRange();

    // 如果没有目标或目标失效，寻找新目标
    if (!CurrentTarget || CurrentTarget->IsPendingKill())
    {
        CurrentTarget = FindTargetInRange();
    }

    // 如果有目标，执行攻击
    if (CurrentTarget)
    {
        // 检查目标是否还在范围内
        float Distance = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());
        if (Distance > AttackRange)
        {
            CurrentTarget = nullptr; // 目标超出范围
            return;
        }

        // 攻击冷却检查
        float CurrentTime = GetWorld()->GetTimeSeconds();
        if (CurrentTime - LastFireTime >= (1.0f / FireRate))
        {
            PerformAttack();
            LastFireTime = CurrentTime;
        }
    }
}

AActor* ABuilding_Defense::FindTargetInRange()
{
    TArray<AActor*> AllUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseUnit::StaticClass(), AllUnits);

    AActor* ClosestEnemy = nullptr;
    float ClosestDistance = FLT_MAX;

    for (AActor* Actor : AllUnits)
    {
        ABaseUnit* Unit = Cast<ABaseUnit>(Actor);
        if (!Unit) continue;

        // 筛选：敌对阵营 + 存活
        if (Unit->TeamID != this->TeamID && Unit->CurrentHealth > 0)
        {
            float Distance = FVector::Dist(GetActorLocation(), Unit->GetActorLocation());

            if (Distance <= AttackRange && Distance < ClosestDistance)
            {
                ClosestDistance = Distance;
                ClosestEnemy = Unit;
            }
        }
    }

    if (ClosestEnemy)
    {
        UE_LOG(LogTemp, Log, TEXT("[Defense] %s locked target: %s (Distance: %f)"),
            *GetName(), *ClosestEnemy->GetName(), ClosestDistance);
    }

    return ClosestEnemy;
}

void ABuilding_Defense::PerformAttack()
{
    if (!CurrentTarget) return;

    ABaseUnit* TargetUnit = Cast<ABaseUnit>(CurrentTarget);
    if (!TargetUnit || TargetUnit->CurrentHealth <= 0)
    {
        CurrentTarget = nullptr;
        return;
    }

    // 直接伤害（简化版，不生成投射物）
    FDamageEvent DamageEvent;
    CurrentTarget->TakeDamage(Damage, DamageEvent, nullptr, this);

    // 面向目标
    FVector Direction = (CurrentTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal();
    if (!Direction.IsNearlyZero())
    {
        FRotator NewRotation = Direction.Rotation();
        NewRotation.Pitch = 0;
        NewRotation.Roll = 0;
        SetActorRotation(NewRotation);
    }

    UE_LOG(LogTemp, Log, TEXT("[Defense] %s fired at %s for %f damage!"),
        *GetName(), *CurrentTarget->GetName(), Damage);

    // TODO: 如果有投射物类，在这里生成投射物
    // if (ProjectileClass)
    // {
    //     FVector SpawnLoc = GetActorLocation() + Direction * 50.0f;
    //     GetWorld()->SpawnActor<AProjectile>(ProjectileClass, SpawnLoc, Direction.Rotation());
    // }
}

void ABuilding_Defense::ApplyLevelUpBonus()
{
    Super::ApplyLevelUpBonus();

    // 每级提升 15% 攻击力和 10% 攻击范围
    Damage *= 1.15f;
    AttackRange *= 1.1f;

    UE_LOG(LogTemp, Warning, TEXT("[Defense] %s upgraded | Damage: %f | Range: %f"),
        *GetName(), Damage, AttackRange);
}

void ABuilding_Defense::DrawAttackRange()
{
    // 每0.5秒绘制一次范围圈（调试用）
    static float LastDrawTime = 0.0f;
    float CurrentTime = GetWorld()->GetTimeSeconds();

    if (CurrentTime - LastDrawTime > 0.5f)
    {
        DrawDebugCircle(GetWorld(), GetActorLocation(), AttackRange,
            32, FColor::Red, false, 0.6f, 0, 5.0f,
            FVector(0, 1, 0), FVector(1, 0, 0), false);
        LastDrawTime = CurrentTime;
    }
}