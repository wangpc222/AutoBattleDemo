#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "RTSCoreTypes.h"
#include "BaseGameEntity.generated.h"

UCLASS()
class AUTOBATTLEDEMO_API ABaseGameEntity : public APawn
{
    GENERATED_BODY()

public:
    ABaseGameEntity();

    virtual void BeginPlay() override;

    // --- 属性 ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
        float MaxHealth;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
        float CurrentHealth;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
        ETeam TeamID;

    // 标记：这是否是可以被攻击的建筑目标
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
        bool bIsTargetable = true;

    // --- 接口 ---
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
    virtual void Die();

    // --- 组件 ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
        class UStaticMeshComponent* StaticMeshComponent;
};