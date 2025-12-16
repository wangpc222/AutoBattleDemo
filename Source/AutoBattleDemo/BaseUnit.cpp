#include "BaseUnit.h"
#include "GridManager.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h" // 用于遍历 Actor
#include "Components/CapsuleComponent.h" 

ABaseUnit::ABaseUnit()
{
    PrimaryActorTick.bCanEverTick = true;

    // 1. 创建胶囊体作为根组件
    CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
    RootComponent = CapsuleComp;

    // 给个常用的默认值即可，具体的数值去 BP_Soldier 蓝图里设
    CapsuleComp->InitCapsuleSize(40.0f, 90.0f);
    CapsuleComp->SetCollisionProfileName(TEXT("Pawn"));
    

    // 2. 创建模型组件，挂在胶囊体下面
    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
    MeshComp->SetupAttachment(RootComponent);

 
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
}

void ABaseUnit::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 只在战斗状态下执行AI逻辑
    // 注意：这里假设ETeam::Player是玩家控制的单位，ETeam::Enemy是AI控制的
    if (TeamID == ETeam::Enemy && CurrentState == EUnitState::Idle)
    {
        // 敌人单位在Idle状态下会自动寻找并攻击
        return;
    }

    // 状态机
    switch (CurrentState)
    {
    case EUnitState::Idle:
        // 如果没目标，找目标
        if (!CurrentTarget)
        {
            CurrentTarget = FindClosestEnemy();
            if (CurrentTarget)
            {
                // 检查目标是否在攻击范围内
                float Distance = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());
                if (Distance <= AttackRange)
                {
                    CurrentState = EUnitState::Attacking;
                }
                else
                {
                    RequestPathToTarget();
                    if (PathPoints.Num() > 0)
                    {
                        CurrentState = EUnitState::Moving;
                    }
                }
            }
        }
        else
        {
            // 已经有目标，检查目标是否有效
            if (CurrentTarget->IsPendingKill() ||
                Cast<ABaseGameEntity>(CurrentTarget)->CurrentHealth <= 0)
            {
                CurrentTarget = nullptr;
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
                    PathPoints.Empty(); // 清除路径
                }
            }
        }
        else
        {
            // 没有路径，回到Idle状态
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
    if (bActive)
    {
        CurrentState = EUnitState::Idle;
        // 可以在这里加个特效或动作
    }
    else
    {
        // 停止所有行动
        CurrentState = EUnitState::Idle;
        CurrentTarget = nullptr;
        PathPoints.Empty();
    }
}

AActor* ABaseUnit::FindClosestEnemy()
{
    AActor* ClosestEnemy = nullptr;
    float ClosestDistance = FLT_MAX;

    // 获取当前关卡的所有BaseGameEntity
    TArray<AActor*> AllEntities;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseGameEntity::StaticClass(), AllEntities);

    for (AActor* Actor : AllEntities)
    {
        ABaseGameEntity* Entity = Cast<ABaseGameEntity>(Actor);
        if (Entity && Entity != this && Entity->TeamID != this->TeamID)
        {
            // 检查目标是否存活
            if (Entity->CurrentHealth > 0)
            {
                // 计算距离
                float Distance = FVector::Dist(GetActorLocation(), Entity->GetActorLocation());

                // 如果当前目标是攻击范围内的，优先选择
                if (Distance <= AttackRange)
                {
                    return Entity; // 立即返回攻击范围内的敌人
                }

                // 否则选择最近的敌人
                if (Distance < ClosestDistance)
                {
                    ClosestDistance = Distance;
                    ClosestEnemy = Entity;
                }
            }
        }
    }

    return ClosestEnemy;
}

void ABaseUnit::RequestPathToTarget()
{
    if (!CurrentTarget)
    {
        return;
    }

    // 获取GridManager
    if (!GridManagerRef)
    {
        // 查找场景中的GridManager
        TArray<AActor*> FoundActors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGridManager::StaticClass(), FoundActors);

        if (FoundActors.Num() > 0)
        {
            GridManagerRef = Cast<AGridManager>(FoundActors[0]);
        }
    }

    if (GridManagerRef)
    {
        // 调用寻路函数
        PathPoints = GridManagerRef->FindPath(GetActorLocation(), CurrentTarget->GetActorLocation());

        // 调试：显示路径
#if WITH_EDITOR
        if (PathPoints.Num() > 1)
        {
            for (int32 i = 0; i < PathPoints.Num() - 1; i++)
            {
                DrawDebugLine(GetWorld(), PathPoints[i], PathPoints[i + 1],
                    FColor::Green, false, 2.0f, 0, 2.0f);
            }
        }
#endif

        CurrentPathIndex = 0;

        if (PathPoints.Num() > 0)
        {
            // 确保起点正确（去掉第一个点如果是当前位置）
            if (PathPoints.Num() > 1 && FVector::DistSquared(PathPoints[0], GetActorLocation()) < 100.0f)
            {
                CurrentPathIndex = 1;
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("找不到GridManager！"));
    }
}

void ABaseUnit::MoveAlongPath(float DeltaTime)
{
    // 检查是否还有路径
    if (PathPoints.Num() == 0 || CurrentPathIndex >= PathPoints.Num())
    {
        CurrentState = EUnitState::Idle;
        return;
    }

    // 检查目标是否还存在
    if (!CurrentTarget || CurrentTarget->IsPendingKill())
    {
        CurrentState = EUnitState::Idle;
        CurrentTarget = nullptr;
        PathPoints.Empty();
        return;
    }

    // 获取当前目标点
    FVector TargetPoint = PathPoints[CurrentPathIndex];

    // 计算移动方向
    FVector CurrentLocation = GetActorLocation();
    FVector Direction = (TargetPoint - CurrentLocation).GetSafeNormal();

    // 移动
    FVector NewLocation = CurrentLocation + Direction * MoveSpeed * DeltaTime;
    SetActorLocation(NewLocation);

    // 面向移动方向
    if (!Direction.IsNearlyZero())
    {
        FRotator NewRotation = Direction.Rotation();
        NewRotation.Pitch = 0; // 保持水平
        NewRotation.Roll = 0;
        SetActorRotation(NewRotation);
    }

    // 检查是否到达当前路径点
    float DistanceToPoint = FVector::DistSquared(NewLocation, TargetPoint);
    if (DistanceToPoint < 100.0f) // 10cm 容差
    {
        CurrentPathIndex++;

        // 如果到达最后一个点，转为Idle状态
        if (CurrentPathIndex >= PathPoints.Num())
        {
            // 检查是否在攻击范围内
            if (CurrentTarget)
            {
                float Distance = FVector::Dist(NewLocation, CurrentTarget->GetActorLocation());
                if (Distance <= AttackRange)
                {
                    CurrentState = EUnitState::Attacking;
                }
                else
                {
                    // 重新寻路
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

    // 检查目标是否死亡
    ABaseGameEntity* TargetEntity = Cast<ABaseGameEntity>(CurrentTarget);
    if (!TargetEntity || TargetEntity->CurrentHealth <= 0)
    {
        CurrentTarget = nullptr;
        CurrentState = EUnitState::Idle;
        return;
    }

    // 检查目标是否在攻击范围内
    float Distance = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());
    if (Distance > AttackRange)
    {
        // 目标跑出攻击范围，重新寻路
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

        // 更新攻击时间
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

        // 播放攻击动画/音效（如果有的话）
        // UE_LOG(LogTemp, Log, TEXT("%s attacked %s!"), *GetName(), *CurrentTarget->GetName());
    }
}