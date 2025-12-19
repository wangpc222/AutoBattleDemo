#pragma once
#include "CoreMinimal.h"
#include "BaseUnit.h"
#include "Soldier_Bomber.generated.h"

/**
 * 炸弹人 - 自爆单位
 * 特点：优先找墙，走到旁边自爆，造成高额范围伤害
 */
UCLASS()
class AUTOBATTLEDEMO_API ASoldier_Bomber : public ABaseUnit
{
    GENERATED_BODY()

public:
    ASoldier_Bomber();

    virtual void BeginPlay() override;

    // 自爆范围伤害
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bomb")
        float ExplosionRadius;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bomb")
        float ExplosionDamage;

protected:
    // 重写目标寻找：优先找墙
    virtual AActor* FindClosestEnemyBuilding() override;

    // 重写攻击：自爆逻辑
    virtual void PerformAttack() override;

private:
    // 执行自爆
    void SuicideAttack();
};