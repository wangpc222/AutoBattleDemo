#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h" // 用 Pawn 比 Actor 好，方便扩展移动
#include "RTSCoreTypes.h"
#include "BaseGameEntity.generated.h"

UCLASS()
class AUTOBATTLEDEMO_API ABaseGameEntity : public APawn
{
    GENERATED_BODY()

public:
    ABaseGameEntity();

    // --- 属性 ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
        float MaxHealth;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
        float CurrentHealth;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
        ETeam TeamID; // 区分玩家还是敌人

        // --- 接口 ---
        // 受伤逻辑
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

    // 死亡逻辑（供 GameMode 监听）
    virtual void Die();

    // 甚至可以加一个委托，当死亡时通知 GameMode 检查胜利条件
    // FOnEntityDiedSignature OnDeath; 
    
    // --- 组件 ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
        class UStaticMeshComponent* StaticMeshComponent;
};