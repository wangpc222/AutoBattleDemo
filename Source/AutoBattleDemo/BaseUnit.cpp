#include "BaseUnit.h"
#include "BaseBuilding.h"
#include "GridManager.h"
#include "Kismet/GameplayStatics.h"

#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"

ABaseUnit::ABaseUnit()
{
    PrimaryActorTick.bCanEverTick = true;

    // 1. 创建胶囊体 (根组件)
    CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
    CapsuleComp->SetupAttachment(RootComponent);
    CapsuleComp->InitCapsuleSize(40.0f, 90.0f);
    CapsuleComp->SetCollisionProfileName(TEXT("Pawn"));

    // 2. 创建模型
    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
    MeshComp->SetupAttachment(CapsuleComp);
    // 保持为 0，具体偏移在蓝图里调
    MeshComp->SetRelativeLocation(FVector(0.f, 0.f, 0.f));

    // 榛樿灞炴��
    MaxHealth = 100.0f;
    AttackRange = 150.0f;
    Damage = 10.0f;
    MoveSpeed = 300.0f;
    AttackInterval = 1.0f;

    UnitType = EUnitType::Barbarian; // 榛樿閲庤洰浜�
    CurrentState = EUnitState::Idle;
    LastAttackTime = 0.0f;
    CurrentPathIndex = 0;
    CurrentTarget = nullptr;
    GridManagerRef = nullptr;
    bIsActive = false;

    TeamID = ETeam::Player;
    bIsTargetable = true; // [修改] 兵当然可以被攻击
}

void ABaseUnit::BeginPlay()
{
    Super::BeginPlay();

    // 1. 获取 GridManager
    for (TActorIterator<AGridManager> It(GetWorld()); It; ++It)
    {
        GridManagerRef = *It;
        break;
    }

    if (!GridManagerRef)
    {
        // 只是警告
        // UE_LOG(LogTemp, Error, TEXT("[Unit] %s cannot find GridManager!"), *GetName());
    }

    // 2. [新增] 自动激活逻辑 (针对手动放置在战场的敌人)
    // 如果我是敌人，且地图是战场，那我就不需要 GameMode 来激活我，我自己激活自己
    FString MapName = GetWorld()->GetMapName();
    if (MapName.Contains("BattleField") && TeamID == ETeam::Enemy)
    {
        SetUnitActive(true);
    }
}

void ABaseUnit::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bIsActive) return;

    switch (CurrentState)
    {
    case EUnitState::Idle:
        if (!CurrentTarget)
        {
            CurrentTarget = FindClosestTarget();
            if (CurrentTarget)
            {
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
            // 检查目标是否失效
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

            // 移动中也要时刻检查是否已经进入攻击范围 (防止走到脸贴脸才停)
            if (CurrentTarget)
            {
                float Distance = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());
                if (Distance <= AttackRange)
                {
                    CurrentState = EUnitState::Attacking;
                    PathPoints.Empty();
                }
            }
            else
            {
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

    // 防重叠逻辑 (简单的群聚分离力)
    if (CurrentState != EUnitState::Idle)
    {
        // 找附近的队友
        TArray<FOverlapResult> Overlaps;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(this);

        // 检测周围 50cm 内有没有人
        bool bHit = GetWorld()->OverlapMultiByChannel(
            Overlaps,
            GetActorLocation(),
            FQuat::Identity,
            ECC_Pawn,
            FCollisionShape::MakeSphere(50.0f),
            Params
        );

        if (bHit)
        {
            FVector SeparationForce = FVector::ZeroVector;
            for (const FOverlapResult& Res : Overlaps)
            {
                ABaseUnit* OtherUnit = Cast<ABaseUnit>(Res.GetActor());
                // 只推开活着的、同阵营的单位 (或者是敌人也推开防止穿模)
                if (OtherUnit && OtherUnit->CurrentHealth > 0)
                {
                    // 计算推开的方向
                    FVector Dir = GetActorLocation() - OtherUnit->GetActorLocation();
                    Dir.Z = 0; // 不要在垂直方向推
                    SeparationForce += Dir.GetSafeNormal();
                }
            }

            // 施加推力 (轻微偏移)
            if (!SeparationForce.IsNearlyZero())
            {
                AddActorWorldOffset(SeparationForce * 10.0f * DeltaTime);
            }
        }
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
        CurrentState = EUnitState::Idle;
        CurrentTarget = nullptr;
        PathPoints.Empty();
        UE_LOG(LogTemp, Warning, TEXT("[Unit] %s AI DEACTIVATED"), *GetName());
    }
}

AActor* ABaseUnit::FindClosestTarget()
{
    AActor* ClosestActor = nullptr;
    float ClosestDistance = FLT_MAX;

    // 1. 获取所有实体 (包括兵和建筑)
    TArray<AActor*> AllEntities;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseGameEntity::StaticClass(), AllEntities);

    for (AActor* Actor : AllEntities)
    {
        ABaseGameEntity* Entity = Cast<ABaseGameEntity>(Actor);

        // 过滤条件：
        // 1. 必须存在
        // 2. 必须是敌人 (TeamID 不同)
        // 3. 必须活着
        // 4. 必须可被攻击 (bIsTargetable)
        if (Entity &&
            Entity->TeamID != this->TeamID &&
            Entity->CurrentHealth > 0 &&
            Entity->bIsTargetable)
        {
            float Distance = FVector::Dist(GetActorLocation(), Entity->GetActorLocation());

            // 如果已经在攻击范围内，直接锁定，不用找更近的了
            if (Distance <= AttackRange)
            {
                return Entity;
            }

            // 找最近的
            if (Distance < ClosestDistance)
            {
                ClosestDistance = Distance;
                ClosestActor = Entity;
            }
        }
    }

    return ClosestActor;
}

void ABaseUnit::RequestPathToTarget()
{
    if (!GridManagerRef)
    {
        GridManagerRef = Cast<AGridManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass()));
    }

    if (!CurrentTarget || !GridManagerRef)
    {
        // UE_LOG(LogTemp, Error, TEXT("[Unit] %s cannot request path!"), *GetName());
        return;
    }

    FVector StartPos = GetActorLocation();
    FVector EndPos = CurrentTarget->GetActorLocation();

    PathPoints = GridManagerRef->FindPath(StartPos, EndPos);
    CurrentPathIndex = 0;

    UE_LOG(LogTemp, Log, TEXT("[Unit] %s path: %d waypoints"), *GetName(), PathPoints.Num());

    /*调试画线
    if (PathPoints.Num() > 1)
    {
        for (int32 i = 0; i < PathPoints.Num() - 1; i++)
        {
            DrawDebugLine(GetWorld(), PathPoints[i], PathPoints[i + 1],
                FColor::Cyan, false, 3.0f, 0, 3.0f);
        }
    }
    */

    // 简单的路径平滑：如果第一个点就在脚下，直接跳过
    if (PathPoints.Num() > 1 && FVector::DistSquared(PathPoints[0], GetActorLocation()) < 2500.0f)
    {
        CurrentPathIndex = 1;
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

    TargetPoint.Z = CurrentLocation.Z;

    FVector Direction = (TargetPoint - CurrentLocation).GetSafeNormal();

    FVector NewLocation = CurrentLocation + Direction * MoveSpeed * DeltaTime;
    SetActorLocation(NewLocation);

    if (!Direction.IsNearlyZero())
    {
        FRotator NewRotation = Direction.Rotation();
        NewRotation.Pitch = 0;
        NewRotation.Roll = 0;
        SetActorRotation(NewRotation);
    }

    float DistanceToPoint = FVector::DistSquared(NewLocation, TargetPoint);
    if (DistanceToPoint < 100.0f)
    {
        CurrentPathIndex++;

        if (CurrentPathIndex >= PathPoints.Num())
        {
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

        UE_LOG(LogTemp, Log, TEXT("[Unit] %s attacked %s for %f damage!"),
            *GetName(), *CurrentTarget->GetName(), Damage);
    }
}