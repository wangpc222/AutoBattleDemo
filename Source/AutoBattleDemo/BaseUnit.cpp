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

    // 2. 自动激活逻辑 (针对手动放置在战场的敌人)
    // 如果我是敌人，且地图是战场，那我就不需要 GameMode 来激活我，我自己激活自己
    FString MapName = GetWorld()->GetMapName();
    if (MapName.Contains("BattleField") && TeamID == ETeam::Enemy)
    {
        SetUnitActive(true);
    }

    // 记录模型的初始位置（攻击冲撞动画用）
    if (MeshComp)
    {
        OriginalMeshOffset = MeshComp->GetRelativeLocation();
    }
    bIsLunging = false;
}

void ABaseUnit::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bIsActive) return;

    switch (CurrentState)
    {
    case EUnitState::Idle:
        // 倒计时
        if (RetargetTimer > 0.0f)
        {
            RetargetTimer -= DeltaTime;
        }
        else
        {
            // 只有计时器归零才思考
            RetargetTimer = 1.0f; // 每 1 秒思考一次，防止卡顿

            if (!CurrentTarget)
            {
                CurrentTarget = FindClosestTarget();
            }

            if (CurrentTarget)
            {
                float Dist = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());
                if (Dist <= AttackRange)
                {
                    CurrentState = EUnitState::Attacking;
                }
                else
                {
                    RequestPathToTarget();
                    // 如果寻路成功，切换状态；如果失败，继续保持 Idle，等下一秒再试
                    if (PathPoints.Num() > 0)
                    {
                        CurrentState = EUnitState::Moving;
                    }
                }
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

    // 防重叠与侧滑逻辑
    if (CurrentState != EUnitState::Idle)
    {
        TArray<FOverlapResult> Overlaps;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(this);

        // 稍微加大检测范围
        bool bHit = GetWorld()->OverlapMultiByChannel(
            Overlaps,
            GetActorLocation(),
            FQuat::Identity,
            ECC_Pawn,
            FCollisionShape::MakeSphere(60.0f),
            Params
        );

        if (bHit)
        {
            FVector SeparationForce = FVector::ZeroVector;
            int32 NeighborCount = 0;

            for (const FOverlapResult& Res : Overlaps)
            {
                ABaseUnit* OtherUnit = Cast<ABaseUnit>(Res.GetActor());
                if (OtherUnit && OtherUnit->CurrentHealth > 0)
                {
                    FVector Dir = GetActorLocation() - OtherUnit->GetActorLocation();
                    Dir.Z = 0;
                    float Dist = Dir.Size();

                    // 距离越近，推力指数级增大
                    if (Dist > 0.1f)
                    {
                        SeparationForce += Dir.GetSafeNormal() * (500.0f / Dist);
                        NeighborCount++;
                    }
                }
            }

            if (NeighborCount > 0)
            {
                // 推力系数
                FVector FinalPush = SeparationForce.GetClampedToMaxSize(2.0f);
                AddActorWorldOffset(FinalPush * 200.0f * DeltaTime);
            }
        }
    }

    // --- 处理攻击动画 ---
    if (bIsLunging && MeshComp)
    {
        LungeTimer += DeltaTime * LungeSpeed;

        // 使用 Sin 函数模拟：0 -> 1 -> 0 (出去再回来)
        // PI 是半个圆周，正好是从 0 到 峰值 再回 0
        float Alpha = FMath::Sin(LungeTimer);

        if (LungeTimer >= PI) // 动画结束 (Sin(PI) = 0)
        {
            bIsLunging = false;
            LungeTimer = 0.0f;
            MeshComp->SetRelativeLocation(OriginalMeshOffset); // 归位
        }
        else
        {
            // 向前移动 (X轴是前方)
            FVector ForwardOffset = FVector(Alpha * LungeDistance, 0.f, 0.f);
            MeshComp->SetRelativeLocation(OriginalMeshOffset + ForwardOffset);
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
    if (PathPoints.Num() == 0 || CurrentPathIndex >= PathPoints.Num()) { CurrentState = EUnitState::Idle; return; }
    if (!CurrentTarget || CurrentTarget->IsPendingKill()) { CurrentState = EUnitState::Idle; CurrentTarget = nullptr; PathPoints.Empty(); return; }

    FVector TargetPoint = PathPoints[CurrentPathIndex];
    FVector CurrentLocation = GetActorLocation();

    // 1. 抹平 Z 轴
    TargetPoint.Z = CurrentLocation.Z;

    // 2. 计算方向
    FVector Direction = (TargetPoint - CurrentLocation).GetSafeNormal();

    // [关键] 强制 Z 为 0，绝对防止钻地
    Direction.Z = 0.0f;

    // 3. 物理移动
    FHitResult Hit;
    AddActorWorldOffset(Direction * MoveSpeed * DeltaTime, true, &Hit);

    // 4. 撞墙处理
    if (Hit.bBlockingHit)
    {
        AActor* HitActor = Hit.GetActor();
        ABaseUnit* HitUnit = Cast<ABaseUnit>(HitActor);
        // 如果撞到敌人，且我是普通兵，打它
        if (HitUnit && HitUnit->TeamID != this->TeamID && HitUnit->CurrentHealth > 0)
        {
            if (UnitType != EUnitType::Giant && UnitType != EUnitType::Bomber)
            {
                CurrentTarget = HitUnit;
                CurrentState = EUnitState::Attacking;
                PathPoints.Empty();
                return;
            }
        }
    }

    // 5. 旋转
    if (!Direction.IsNearlyZero())
    {
        FRotator NewRot = FMath::RInterpTo(GetActorRotation(), Direction.Rotation(), DeltaTime, 10.0f);
        SetActorRotation(NewRot);
    }

    // 6. [修复] 到达判定
    float DistanceToPoint = FVector::DistSquared2D(CurrentLocation, TargetPoint);

    // 如果是脱困模式，必须走得准一点 (10cm)，防止还没走出包围圈就停了
    // 正常模式可以宽一点 (30cm)
    float AcceptanceRadiusSq = bIsUnstucking ? 100.0f : 900.0f;

    if (DistanceToPoint < AcceptanceRadiusSq)
    {
        CurrentPathIndex++;

        // 路径走完了，判断是否能打到目标
        if (CurrentPathIndex >= PathPoints.Num())
        {
            if (CurrentTarget)
            {
                float Distance = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());
                if (Distance <= AttackRange + 100.0f) // 稍微宽容一点+100
                {
                    CurrentState = EUnitState::Attacking;
                    UE_LOG(LogTemp, Warning, TEXT("[Unit] %s path complete, starting attack!"), *GetName());
                }
                else
                {
                    // 还没走到，重新寻路
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

            // 触发冲撞动画
            bIsLunging = true;
            LungeTimer = 0.0f;
        }

        UE_LOG(LogTemp, Log, TEXT("[Unit] %s attacked %s for %f damage!"),
            *GetName(), *CurrentTarget->GetName(), Damage);
    }
}

