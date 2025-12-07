#include "BaseGameEntity.h"
#include "RTSGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/CollisionProfile.h"

ABaseGameEntity::ABaseGameEntity()
{
    PrimaryActorTick.bCanEverTick = true;

    // 创建根组件（场景组件）
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

    // 创建静态网格组件（视觉表现）
    StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
    StaticMeshComponent->SetupAttachment(RootComponent);

    // 设置碰撞
    StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    StaticMeshComponent->SetCollisionObjectType(ECC_Pawn);
    StaticMeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
    StaticMeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
    StaticMeshComponent->SetGenerateOverlapEvents(true);


    // 默认值
    MaxHealth = 100.0f;
    CurrentHealth = MaxHealth;
    TeamID = ETeam::Enemy; // 默认为敌人，子类可修改
}

float ABaseGameEntity::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    // 调用父类TakeDamage，获取实际伤害值
    float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    // 如果有实际伤害，处理受伤逻辑
    if (ActualDamage > 0.0f)
    {
        CurrentHealth -= ActualDamage;

        // 显示受伤效果（可选）
        // UE_LOG(LogTemp, Warning, TEXT("%s took %f damage. Current HP: %f"), *GetName(), ActualDamage, CurrentHealth);

        // 检查是否死亡
        if (CurrentHealth <= 0.0f)
        {
            Die();
        }
    }

    return ActualDamage;
}

void ABaseGameEntity::Die()
{
    // 通知 GameMode (我是受害者，谁杀了我这里暂时传空)
    ARTSGameMode* GM = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(this));
    if (GM)
    {
        GM->OnActorKilled(this, nullptr);
    }

    // 销毁自己
    Destroy();
}