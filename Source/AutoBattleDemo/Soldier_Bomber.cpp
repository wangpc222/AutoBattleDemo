#include "Soldier_Bomber.h"
#include "BaseBuilding.h"
#include "GridManager.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "BaseUnit.h"
#include "Particles/ParticleSystem.h"

ASoldier_Bomber::ASoldier_Bomber()
{
    // 设置兵种类型
    UnitType = EUnitType::Bomber;

    // 炸弹人属性
    MaxHealth = 50.0f;
    AttackRange = 100.0f;
    Damage = 0.0f;
    MoveSpeed = 350.0f;
    AttackInterval = 0.0f;

    ExplosionRadius = 300.0f;
    ExplosionDamage = 200.0f;
}

void ASoldier_Bomber::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("[Bomber] %s spawned | ExplosionDamage: %f | Radius: %f"),
        *GetName(), ExplosionDamage, ExplosionRadius);
}

AActor* ASoldier_Bomber::FindClosestTarget()
{
    TArray<AActor*> AllBuildings;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseBuilding::StaticClass(), AllBuildings);

    AActor* ClosestWall = nullptr;
    float ClosestWallDistance = FLT_MAX;

    AActor* ClosestOther = nullptr;
    float ClosestOtherDistance = FLT_MAX;

    for (AActor* Actor : AllBuildings)
    {
        ABaseBuilding* Building = Cast<ABaseBuilding>(Actor);

        if (Building &&
            Building->TeamID != this->TeamID &&
            Building->CurrentHealth > 0)
        {
            float Distance = FVector::Dist(GetActorLocation(), Building->GetActorLocation());

            // 优先选择墙
            if (Building->BuildingType == EBuildingType::Wall)
            {
                if (Distance <= AttackRange)
                {
                    UE_LOG(LogTemp, Warning, TEXT("[Bomber] %s found wall in range: %s"),
                        *GetName(), *Building->GetName());
                    return Building;
                }

                if (Distance < ClosestWallDistance)
                {
                    ClosestWallDistance = Distance;
                    ClosestWall = Building;
                }
            }
            else
            {
                if (Distance < ClosestOtherDistance)
                {
                    ClosestOtherDistance = Distance;
                    ClosestOther = Building;
                }
            }
        }
    }

    if (ClosestWall)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Bomber] %s targeting wall: %s"),
            *GetName(), *ClosestWall->GetName());
        return ClosestWall;
    }

    if (ClosestOther)
    {
        UE_LOG(LogTemp, Log, TEXT("[Bomber] %s no walls found, targeting: %s"),
            *GetName(), *ClosestOther->GetName());
    }

    return ClosestOther;
}

void ASoldier_Bomber::PerformAttack()
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

    UE_LOG(LogTemp, Error, TEXT("[Bomber] %s EXPLODING!!!"), *GetName());
    SuicideAttack();
}

void ASoldier_Bomber::SuicideAttack()
{
    FVector ExplosionCenter = GetActorLocation();

    // 播放视觉特效
    if (ExplosionVFX)
    {
        // 在当前位置生成粒子
        // FVector(3.0f) 是缩放比例，让爆炸看起来大一点
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionVFX, ExplosionCenter, FRotator::ZeroRotator, FVector(1.0f));
    }

    // 2. 播放声音
    /*if (ExplosionSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, ExplosionCenter);
    }*/

    // 绘制爆炸范围
    /*DrawDebugSphere(GetWorld(), ExplosionCenter, ExplosionRadius,
        16, FColor::Orange, false, 3.0f, 0, 5.0f);*/

    // 对范围内所有建筑造成伤害
    TArray<AActor*> AllBuildings;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseBuilding::StaticClass(), AllBuildings);

    int32 HitCount = 0;
    TArray<ABaseBuilding*> DestroyedWalls; // 记录被炸毁的墙

    for (AActor* Actor : AllBuildings)
    {
        ABaseBuilding* Building = Cast<ABaseBuilding>(Actor);

        if (Building &&
            Building->TeamID != this->TeamID &&
            Building->CurrentHealth > 0)
        {
            float Distance = FVector::Dist(ExplosionCenter, Building->GetActorLocation());

            if (Distance <= ExplosionRadius)
            {
                // 记录爆炸前血量
                float HealthBefore = Building->CurrentHealth;

                // 应用伤害
                FDamageEvent DamageEvent;
                Building->TakeDamage(ExplosionDamage, DamageEvent, nullptr, this);
                HitCount++;

                UE_LOG(LogTemp, Warning, TEXT("[Bomber] %s hit %s for %f damage!"),
                    *GetName(), *Building->GetName(), ExplosionDamage);

                // 检查是否炸毁了墙
                if (Building->BuildingType == EBuildingType::Wall &&
                    HealthBefore > 0 && Building->CurrentHealth <= 0)
                {
                    DestroyedWalls.Add(Building);
                }
            }
        }
    }

    UE_LOG(LogTemp, Error, TEXT("[Bomber] %s explosion hit %d buildings!"), *GetName(), HitCount);

    // 【关键】通知 GridManager 更新网格
    if (DestroyedWalls.Num() > 0 && GridManagerRef)
    {
        for (ABaseBuilding* Wall : DestroyedWalls)
        {
            // 检查墙是否有有效的网格坐标
            if (Wall->GridX >= 0 && Wall->GridY >= 0)
            {
                // 调用成员A的接口，将这个格子设为可通行
                GridManagerRef->SetTileBlocked(Wall->GridX, Wall->GridY, false);

                UE_LOG(LogTemp, Error, TEXT("[Bomber] Wall destroyed at Grid (%d, %d) - Grid updated!"),
                    Wall->GridX, Wall->GridY);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[Bomber] Wall %s has no valid grid position! (GridX=%d, GridY=%d)"),
                    *Wall->GetName(), Wall->GridX, Wall->GridY);
            }
        }
    }

    // 自爆后销毁
    Destroy();
}