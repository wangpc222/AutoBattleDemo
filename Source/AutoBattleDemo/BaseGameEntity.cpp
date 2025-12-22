#include "BaseGameEntity.h"
#include "RTSGameMode.h"
#include "Kismet/GameplayStatics.h"

ABaseGameEntity::ABaseGameEntity()
{
    PrimaryActorTick.bCanEverTick = false;

    // 创建根组件
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

    // 默认值
    MaxHealth = 500.0f; // 建筑血量更高
    CurrentHealth = MaxHealth;
    TeamID = ETeam::Enemy;
    bIsTargetable = true;
}

void ABaseGameEntity::BeginPlay()
{
    Super::BeginPlay();

    // 确保血量初始化
    CurrentHealth = MaxHealth;

    UE_LOG(LogTemp, Log, TEXT("[Entity] %s spawned | HP: %f | Team: %d | Targetable: %d"),
        *GetName(), CurrentHealth, (int32)TeamID, bIsTargetable);
}

float ABaseGameEntity::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    if (ActualDamage > 0.0f)
    {
        CurrentHealth -= ActualDamage;

        // 触发蓝图里的受击特效 (音效/粒子)
        PlayHitVisuals();

        if (CurrentHealth <= 0.0f)
        {
            // 触发死亡特效
            PlayDeathVisuals();
            Die();
        }
    }
    return ActualDamage;
}

void ABaseGameEntity::Die()
{
    UE_LOG(LogTemp, Warning, TEXT("[Entity] %s died!"), *GetName());

    // 调用蓝图可重写的死亡事件
    OnDeath();

    // 通知 GameMode
    ARTSGameMode* GM = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(this));
    if (GM)
    {
        GM->OnActorKilled(this, nullptr);
    }

    Destroy();
}

void ABaseGameEntity::OnDeath_Implementation()
{
    // 默认实现为空，子类可以重写添加特效、音效等
}