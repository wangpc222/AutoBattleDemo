#include "BaseGameEntity.h"
#include "RTSGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"

ABaseGameEntity::ABaseGameEntity()
{
    PrimaryActorTick.bCanEverTick = false; // 建筑不需要 Tick

    // 创建根组件
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

    // 创建静态网格组件
    StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
    StaticMeshComponent->SetupAttachment(RootComponent);

    // 设置碰撞
    StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    StaticMeshComponent->SetCollisionObjectType(ECC_Pawn);
    StaticMeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
    StaticMeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
    StaticMeshComponent->SetGenerateOverlapEvents(true);

    // 默认值
    MaxHealth = 500.0f; // 建筑血量更高
    CurrentHealth = MaxHealth;
    TeamID = ETeam::Enemy; // 默认为敌方建筑
    bIsTargetable = true; // 可以被攻击
}

void ABaseGameEntity::BeginPlay()
{
    Super::BeginPlay();

    // 确保血量初始化
    CurrentHealth = MaxHealth;

    // 调试输出
    UE_LOG(LogTemp, Warning, TEXT("[Building] %s spawned | HP: %f | Team: %d | Targetable: %d"),
        *GetName(), CurrentHealth, (int32)TeamID, bIsTargetable);
}

float ABaseGameEntity::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    if (ActualDamage > 0.0f)
    {
        CurrentHealth -= ActualDamage;

        // 调试输出
        UE_LOG(LogTemp, Warning, TEXT("[Building] %s took %f damage | HP: %f/%f"),
            *GetName(), ActualDamage, CurrentHealth, MaxHealth);

        if (CurrentHealth <= 0.0f)
        {
            Die();
        }
    }

    return ActualDamage;
}

void ABaseGameEntity::Die()
{
    UE_LOG(LogTemp, Error, TEXT("[Building] %s destroyed!"), *GetName());

    ARTSGameMode* GM = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(this));
    if (GM)
    {
        GM->OnActorKilled(this, nullptr);
    }

    // 可以在这里播放爆炸特效

    Destroy();
}