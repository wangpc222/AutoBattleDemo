#include "RTSProjectile.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "BaseGameEntity.h"

ARTSProjectile::ARTSProjectile()
{
    PrimaryActorTick.bCanEverTick = true;

    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
    RootComponent = MeshComp;
    MeshComp->SetCollisionProfileName(TEXT("NoCollision")); // 子弹本身不负责碰撞，靠逻辑判断

    MovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("MovementComp"));
    MovementComp->InitialSpeed = 1000.0f;
    MovementComp->MaxSpeed = 1000.0f;
    MovementComp->bRotationFollowsVelocity = true;
    MovementComp->bIsHomingProjectile = true; // 关键：追踪导弹
    MovementComp->HomingAccelerationMagnitude = 5000.0f; // 追踪力度
}

void ARTSProjectile::Initialize(AActor* NewTarget, float NewDamage, AActor* NewInstigator)
{
    TargetActor = NewTarget;
    Damage = NewDamage;
    DamageInstigator = NewInstigator;

    // 设置追踪目标
    if (TargetActor && MovementComp)
    {
        MovementComp->HomingTargetComponent = TargetActor->GetRootComponent();
    }

    // 5秒后自毁（防止目标消失后子弹飞到天涯海角）
    SetLifeSpan(5.0f);
}

void ARTSProjectile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (TargetActor)
    {
        // 简单的距离检测：如果飞得够近了，就造成伤害
        float Distance = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
        if (Distance < 50.0f) // 命中阈值
        {
            // 造成伤害
            UGameplayStatics::ApplyDamage(TargetActor, Damage, GetInstigatorController(), DamageInstigator, UDamageType::StaticClass());

            // 这里可以生成爆炸特效 (SpawnEmitterAtLocation)

            // 销毁子弹
            Destroy();
        }
    }
    else
    {
        // 目标都没了，我也没意义了
        Destroy();
    }
}