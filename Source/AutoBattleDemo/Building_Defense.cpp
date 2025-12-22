#include "Building_Defense.h"
#include "BaseUnit.h"
#include "Kismet/GameplayStatics.h"
#include "RTSProjectile.h" 
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

    // 1. 计算朝向 (塔身旋转面向敌人)
    FVector Direction = (TargetUnit->GetActorLocation() - GetActorLocation()).GetSafeNormal();
    if (!Direction.IsNearlyZero())
    {
        FRotator NewRotation = Direction.Rotation();
        NewRotation.Pitch = 0; // 塔身只转水平方向，不抬头
        NewRotation.Roll = 0;
        SetActorRotation(NewRotation);
    }

    // 2. 攻击逻辑分流
    if (ProjectileClass)
    {
        // --- 方案 A: 发射实体子弹 (视觉表现好) ---

        // 计算发射点：从塔顶发射，而不是塔底
        // 假设塔高约 200，我们从 150 的高度发射 (或者你可以加一个 ArrowComponent 作为枪口)
        FVector SpawnLocation = GetActorLocation() + FVector(0.f, 0.f, 150.0f);

        // 子弹的朝向指向敌人
        FRotator SpawnRotation = (TargetUnit->GetActorLocation() - SpawnLocation).Rotation();

        // 生成子弹
        ARTSProjectile* NewProjectile = GetWorld()->SpawnActor<ARTSProjectile>(ProjectileClass, SpawnLocation, SpawnRotation);

        if (NewProjectile)
        {
            // 初始化子弹 (告诉它打谁、伤害多少、谁发射的)
            NewProjectile->Initialize(TargetUnit, Damage, this);

            UE_LOG(LogTemp, Log, TEXT("[Defense] %s Fired Projectile!"), *GetName());
        }
    }
    else
    {
        // --- 方案 B: 直接伤害 (保底逻辑，防止没配子弹时没伤害) ---

        FDamageEvent DamageEvent;
        TargetUnit->TakeDamage(Damage, DamageEvent, nullptr, this);

        UE_LOG(LogTemp, Log, TEXT("[Defense] %s Instant Hit %s!"), *GetName(), *TargetUnit->GetName());
    }
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