#include "BaseUnit.h"
#include "GridManager.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"

ABaseUnit::ABaseUnit()
{
    PrimaryActorTick.bCanEverTick = true;

    // 默认数值
    MaxHealth = 100.0f;
    AttackRange = 150.0f; // 近战距离
    Damage = 10.0f;
    MoveSpeed = 300.0f;
    AttackInterval = 1.0f;

    CurrentState = EUnitState::Idle;
    LastAttackTime = 0.0f;
    CurrentPathIndex = 0;
    CurrentTarget = nullptr;
    GridManagerRef = nullptr;
    bIsActive = false; // 默认不激活，等待 GameMode 调用

    TeamID = ETeam::Player; // 士兵默认是玩家方
    bIsTargetable = false; // 士兵不是建筑，通常不被防御塔主动攻击（可选）
}

void ABaseUnit::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 如果 AI 未激活（备战阶段），不执行任何逻辑
    if (!bIsActive)
    {
        return;
    }

    // 状态机
    switch (CurrentState)
    {
    case EUnitState::Idle:
        // 如果没目标，找目标
        if (!CurrentTarget)
        {
            CurrentTarget = FindClosestEnemyBuilding(); // 寻找建筑目标
            if (CurrentTarget)
            {
                UE_LOG(LogTemp, Log, TEXT("[Unit] %s found target building: %s"),
                    *GetName(), *CurrentTarget->GetName());

                // 检查目标是否在攻击范围内
                float Distance = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());
                if (Distance <= AttackRange)
                {
                    CurrentState = EUnitState::Attacking;
                    UE_LOG(LogTemp, Log, TEXT("[Unit] %s in attack range, switching to Attack"), *GetName());
                }
                else
                {
                    UE_LOG(LogTemp, Log, TEXT("[Unit] %s requesting path (distance: %f)"), *GetName(), Distance);
                    RequestPathToTarget();
                    if (PathPoints.Num() > 0)
                    {
                        CurrentState = EUnitState::Moving;
                        UE_LOG(LogTemp, Warning, TEXT("[Unit] %s moving with %d waypoints"),
                            *GetName(), PathPoints.Num());
                    }
                    else
                    {
                        UE_LOG(LogTemp, Error, TEXT("[Unit] %s failed to find path!"), *GetName());
                    }
                }
            }
            else
            {
                // 没有找到任何敌方建筑（可能已经全部摧毁）
                UE_LOG(LogTemp, Warning, TEXT("[Unit] %s found NO enemy buildings!"), *GetName());
            }
        }
        else
        {
            // 已经有目标，检查目标是否有效
            ABaseGameEntity* TargetEntity = Cast<ABaseGameEntity>(CurrentTarget);
            if (!TargetEntity || TargetEntity->CurrentHealth <= 0 || CurrentTarget->IsPendingKill())
            {
                UE_LOG(LogTemp, Log, TEXT("[Unit] %s target destroyed, searching for new target"), *GetName());
                CurrentTarget = nullptr; // 目标失效，重新寻找
            }
        }
        break;

    case EUnitState::Moving:
        if (PathPoints.Num() > 0)
        {
            MoveAlongPath(DeltaTime);

            // 移动过程中检查是否进入攻击范围
            if (CurrentTarget)
            {
                float Distance = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());
                if (Distance <= AttackRange)
                {
                    CurrentState = EUnitState::Attacking;
                    PathPoints.Empty();
                    UE_LOG(LogTemp, Warning, TEXT("[Unit] %s reached attack range!"), *GetName());
                }
            }
            else
            {
                // 移动途中目标被摧毁
                CurrentState = EUnitState::Idle;
                PathPoints.Empty();
            }
        }
        else
        {
            CurrentState = EUnitState::Idle;
        }
        break;

    case EUnitState::Attacking:
        PerformAttack();
        break;
    }
}

void ABaseUnit::SetUnitActive(bool bActive)
{
    bIsActive = bActive;

    if (bActive)
    {
        CurrentState = EUnitState::Idle;
        UE_LOG(LogTemp, Warning, TEXT("[Unit] %s AI ACTIVATED!"), *GetName());
    }
    else
    {
        // 停止所有行动（回到备战状态）
        CurrentState = EUnitState::Idle;
        CurrentTarget = nullptr;
        PathPoints.Empty();
        UE_LOG(LogTemp, Warning, TEXT("[Unit] %s AI DEACTIVATED"), *GetName());
    }
}

AActor* ABaseUnit::FindClosestEnemyBuilding()
{
    AActor* ClosestBuilding = nullptr;
    float ClosestDistance = FLT_MAX;

    // 获取所有 BaseGameEntity（包括建筑和士兵）
    TArray<AActor*> AllEntities;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseGameEntity::StaticClass(), AllEntities);

    UE_LOG(LogTemp, Log, TEXT("[Unit] %s searching among %d entities"),
        *GetName(), AllEntities.Num());

    for (AActor* Actor : AllEntities)
    {
        ABaseGameEntity* Entity = Cast<ABaseGameEntity>(Actor);

        // 筛选条件：
        // 1. 不是自己
        // 2. 敌对阵营
        // 3. 可以被攻击（建筑）
        // 4. 存活
        // 5. 不是士兵（通过检查是否是 ABaseUnit）
        if (Entity && Entity != this &&
            Entity->TeamID != this->TeamID &&
            Entity->bIsTargetable &&
            Entity->CurrentHealth > 0 &&
            !Entity->IsA(ABaseUnit::StaticClass())) // 关键：排除士兵
        {
            float Distance = FVector::Dist(GetActorLocation(), Entity->GetActorLocation());

            // 优先攻击攻击范围内的建筑
            if (Distance <= AttackRange)
            {
                UE_LOG(LogTemp, Log, TEXT("[Unit] %s found building in attack range: %s"),
                    *GetName(), *Entity->GetName());
                return Entity;
            }

            // 否则选择最近的建筑
            if (Distance < ClosestDistance)
            {
                ClosestDistance = Distance;
                ClosestBuilding = Entity;
            }
        }
    }

    if (ClosestBuilding)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Unit] %s closest building: %s (Distance: %f)"),
            *GetName(), *ClosestBuilding->GetName(), ClosestDistance);
    }

    return ClosestBuilding;
}

void ABaseUnit::RequestPathToTarget()
{
    if (!CurrentTarget)
    {
        UE_LOG(LogTemp, Error, TEXT("[Unit] %s RequestPath called with no target!"), *GetName());
        return;
    }

    // 获取 GridManager
    if (!GridManagerRef)
    {
        TArray<AActor*> FoundActors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGridManager::StaticClass(), FoundActors);

        if (FoundActors.Num() > 0)
        {
            GridManagerRef = Cast<AGridManager>(FoundActors[0]);
            UE_LOG(LogTemp, Warning, TEXT("[Unit] %s found GridManager!"), *GetName());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[Unit] %s CANNOT FIND GridManager!"), *GetName());
            return;
        }
    }

    if (GridManagerRef)
    {
        FVector StartPos = GetActorLocation();
        FVector EndPos = CurrentTarget->GetActorLocation();

        UE_LOG(LogTemp, Log, TEXT("[Unit] %s pathfinding: %s -> %s"),
            *GetName(), *StartPos.ToString(), *EndPos.ToString());

        PathPoints = GridManagerRef->FindPath(StartPos, EndPos);

        UE_LOG(LogTemp, Warning, TEXT("[Unit] %s path result: %d waypoints"),
            *GetName(), PathPoints.Num());

        // 调试：显示路径
        if (PathPoints.Num() > 1)
        {
            for (int32 i = 0; i < PathPoints.Num() - 1; i++)
            {
                DrawDebugLine(GetWorld(), PathPoints[i], PathPoints[i + 1],
                    FColor::Cyan, false, 5.0f, 0, 5.0f);
            }
            DrawDebugSphere(GetWorld(), PathPoints[0], 30.0f, 12, FColor::Blue, false, 5.0f);
            DrawDebugSphere(GetWorld(), PathPoints.Last(), 30.0f, 12, FColor::Red, false, 5.0f);
        }

        CurrentPathIndex = 0;

        // 如果起点就是当前位置，跳过第一个点
        if (PathPoints.Num() > 1 && FVector::DistSquared(PathPoints[0], GetActorLocation()) < 100.0f)
        {
            CurrentPathIndex = 1;
        }
    }
}

void ABaseUnit::MoveAlongPath(float DeltaTime)
{
    if (PathPoints.Num() == 0 || CurrentPathIndex >= PathPoints.Num())
    {
        CurrentState = EUnitState::Idle;
        return;
    }

    if (!CurrentTarget || CurrentTarget->IsPendingKill())
    {
        CurrentState = EUnitState::Idle;
        CurrentTarget = nullptr;
        PathPoints.Empty();
        return;
    }

    FVector TargetPoint = PathPoints[CurrentPathIndex];
    FVector CurrentLocation = GetActorLocation();
    FVector Direction = (TargetPoint - CurrentLocation).GetSafeNormal();

    // 移动
    FVector NewLocation = CurrentLocation + Direction * MoveSpeed * DeltaTime;
    SetActorLocation(NewLocation);

    // 面向移动方向
    if (!Direction.IsNearlyZero())
    {
        FRotator NewRotation = Direction.Rotation();
        NewRotation.Pitch = 0;
        NewRotation.Roll = 0;
        SetActorRotation(NewRotation);
    }

    // 检查是否到达当前路径点
    float DistanceToPoint = FVector::DistSquared(NewLocation, TargetPoint);
    if (DistanceToPoint < 100.0f) // 10cm 容差
    {
        CurrentPathIndex++;

        if (CurrentPathIndex >= PathPoints.Num())
        {
            // 到达路径终点
            if (CurrentTarget)
            {
                float Distance = FVector::Dist(NewLocation, CurrentTarget->GetActorLocation());
                if (Distance <= AttackRange)
                {
                    CurrentState = EUnitState::Attacking;
                    UE_LOG(LogTemp, Warning, TEXT("[Unit] %s path complete, starting attack!"), *GetName());
                }
                else
                {
                    // 目标可能移动了（建筑不会动，但以防万一）
                    UE_LOG(LogTemp, Warning, TEXT("[Unit] %s reached end but target still far, re-pathing"), *GetName());
                    RequestPathToTarget();
                    if (PathPoints.Num() == 0)
                    {
                        CurrentState = EUnitState::Idle;
                    }
                }
            }
            else
            {
                CurrentState = EUnitState::Idle;
            }
        }
    }
}

void ABaseUnit::PerformAttack()
{
    if (!CurrentTarget)
    {
        CurrentState = EUnitState::Idle;
        return;
    }

    ABaseGameEntity* TargetEntity = Cast<ABaseGameEntity>(CurrentTarget);
    if (!TargetEntity || TargetEntity->CurrentHealth <= 0)
    {
        UE_LOG(LogTemp, Log, TEXT("[Unit] %s target destroyed during attack"), *GetName());
        CurrentTarget = nullptr;
        CurrentState = EUnitState::Idle;
        return;
    }

    // 检查目标是否在攻击范围内
    float Distance = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());
    if (Distance > AttackRange)
    {
        // 目标超出攻击范围，重新寻路
        UE_LOG(LogTemp, Warning, TEXT("[Unit] %s target out of range, re-pathing"), *GetName());
        RequestPathToTarget();
        if (PathPoints.Num() > 0)
        {
            CurrentState = EUnitState::Moving;
        }
        return;
    }

    // 攻击冷却检查
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastAttackTime >= AttackInterval)
    {
        // 应用伤害
        FDamageEvent DamageEvent;
        CurrentTarget->TakeDamage(Damage, DamageEvent, nullptr, this);

        LastAttackTime = CurrentTime;

        // 面向目标
        FVector Direction = (CurrentTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal();
        if (!Direction.IsNearlyZero())
        {
            FRotator NewRotation = Direction.Rotation();
            NewRotation.Pitch = 0;
            NewRotation.Roll = 0;
            SetActorRotation(NewRotation);
        }

        UE_LOG(LogTemp, Log, TEXT("[Unit] %s attacked %s for %f damage!"),
            *GetName(), *CurrentTarget->GetName(), Damage);
    }
}