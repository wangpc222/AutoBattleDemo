#include "BaseSoldier.h"

ABaseSoldier::ABaseSoldier()
{
	PrimaryActorTick.bCanEverTick = true;

	// 默认数值（可以在子类蓝图里改）
	Damage = 10.0f;
	AttackRange = 150.0f; // 近战默认距离
	AttackInterval = 1.0f;

	// 初始化血量
	MaxHealth = 100.0f; 
}

void ABaseSoldier::BeginPlay()
{
	Super::BeginPlay();
}

void ABaseSoldier::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// 这里以后写寻路逻辑
}

// 父类的默认攻击逻辑（比如近战挥拳）
void ABaseSoldier::Attack(AActor* Target)
{
	if (Target)
	{
		// C++ 日志：打印我在打谁
		UE_LOG(LogTemp, Warning, TEXT("BaseSoldier Attacking %s for %f damage!"), *Target->GetName(), Damage);

		// 这里以后可以写：Target->TakeDamage(...)
	}
}

// 点击扣血逻辑（跟 BaseBuilding 很像）
void ABaseSoldier::NotifyActorOnClicked(FKey ButtonPressed)
{
    Super::NotifyActorOnClicked(ButtonPressed);

    // 扣血
    MaxHealth -= 20.0f; // 每次扣20

    // 打印日志
    if (GEngine)
    {
        FString Msg = FString::Printf(TEXT("Soldier Hit! Health: %f"), MaxHealth);
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, Msg);
    }

    // 死亡逻辑
    if (MaxHealth <= 0.0f)
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Soldier Died!"));

        // 销毁自己
        Destroy();
    }
}