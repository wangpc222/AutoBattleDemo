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

    // 鍒涘缓鑳跺泭浣�
    CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
    CapsuleComp->SetupAttachment(RootComponent);
    CapsuleComp->InitCapsuleSize(40.0f, 90.0f);
    CapsuleComp->SetCollisionProfileName(TEXT("Pawn"));

    // 鍒涘缓妯″瀷
    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
    MeshComp->SetupAttachment(CapsuleComp);

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
    bIsTargetable = false;
}

void ABaseUnit::BeginPlay()
{
    Super::BeginPlay();

    // 鑾峰彇 GridManager
    for (TActorIterator<AGridManager> It(GetWorld()); It; ++It)
    {
        GridManagerRef = *It;
        break;
    }

    if (!GridManagerRef)
    {
        UE_LOG(LogTemp, Error, TEXT("[Unit] %s cannot find GridManager!"), *GetName());
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
            CurrentTarget = FindClosestEnemyBuilding();
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

AActor* ABaseUnit::FindClosestEnemyBuilding()
{
    AActor* ClosestBuilding = nullptr;
    float ClosestDistance = FLT_MAX;

    TArray<AActor*> AllBuildings;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseBuilding::StaticClass(), AllBuildings);

    for (AActor* Actor : AllBuildings)
    {
        ABaseBuilding* Building = Cast<ABaseBuilding>(Actor);

        if (Building &&
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

    return ClosestBuilding;
}

void ABaseUnit::RequestPathToTarget()
{
    if (!CurrentTarget || !GridManagerRef)
    {
        UE_LOG(LogTemp, Error, TEXT("[Unit] %s cannot request path!"), *GetName());
        return;
    }

    FVector StartPos = GetActorLocation();
    FVector EndPos = CurrentTarget->GetActorLocation();

    PathPoints = GridManagerRef->FindPath(StartPos, EndPos);
    CurrentPathIndex = 0;

    UE_LOG(LogTemp, Log, TEXT("[Unit] %s path: %d waypoints"), *GetName(), PathPoints.Num());

    if (PathPoints.Num() > 1)
    {
        for (int32 i = 0; i < PathPoints.Num() - 1; i++)
        {
            DrawDebugLine(GetWorld(), PathPoints[i], PathPoints[i + 1],
                FColor::Cyan, false, 3.0f, 0, 3.0f);
        }
    }

    if (PathPoints.Num() > 1 && FVector::DistSquared(PathPoints[0], GetActorLocation()) < 100.0f)
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