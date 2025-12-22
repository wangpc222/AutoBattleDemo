#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RTSCoreTypes.h"
#include "BaseGameEntity.generated.h"

// 所有游戏实体的基类（兵种和建筑的共同父类）
// 修正：继承 AActor 而不是 APawn
UCLASS()
class AUTOBATTLEDEMO_API ABaseGameEntity : public AActor
{
    GENERATED_BODY()

public:
    ABaseGameEntity();

    virtual void BeginPlay() override;

    // --- 核心属性 ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
        float MaxHealth;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
        float CurrentHealth;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
        ETeam TeamID;

    // 标记：是否可以被攻击
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
        bool bIsTargetable = true;

    // --- 接口 ---
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
        class AController* EventInstigator, AActor* DamageCauser) override;

    virtual void Die();

    // 虚函数：子类可以重写死亡逻辑
    UFUNCTION(BlueprintNativeEvent, Category = "Entity")
        void OnDeath();
    virtual void OnDeath_Implementation();

    // 播放受击特效 (C++ 调用，蓝图实现)
    UFUNCTION(BlueprintImplementableEvent, Category = "Visuals")
        void PlayHitVisuals();

    // 播放死亡特效
    UFUNCTION(BlueprintImplementableEvent, Category = "Visuals")
        void PlayDeathVisuals();
};