#pragma once
#include "CoreMinimal.h"
#include "BaseBuilding.h"
#include "Building_Defense.generated.h"

UCLASS()
class AUTOBATTLEDEMO_API ABuilding_Defense : public ABaseBuilding
{
    GENERATED_BODY()

public:
    ABuilding_Defense();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // --- 防御塔属性 ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense")
        float AttackRange;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense")
        float Damage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense")
        float FireRate; // 每秒开火次数

    

protected:
    // 升级时提升攻击力
    virtual void ApplyLevelUpBonus() override;

private:
    // 寻找范围内最近的敌人
    AActor* FindTargetInRange();

    // 执行攻击
    void PerformAttack();

    // 当前锁定的目标
    UPROPERTY()
        AActor* CurrentTarget;

    // 攻击计时器
    float LastFireTime;

    // 可视化调试
    void DrawAttackRange();
};
