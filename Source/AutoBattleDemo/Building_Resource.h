#pragma once
#include "CoreMinimal.h"
#include "BaseBuilding.h"
#include "Building_Resource.generated.h"

/**
 * 资源建筑（矿场）
 * 功能：每秒产出资源
 */
UCLASS()
class AUTOBATTLEDEMO_API ABuilding_Resource : public ABaseBuilding
{
    GENERATED_BODY()

public:
    ABuilding_Resource();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // --- 资源生产属性 ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
        float ProductionRate; // 每秒产量

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
        float MaxStorage; // 最大存储量

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Resource")
        float CurrentStorage; // 当前存储量

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
        bool bProducesGold; // true=金币, false=圣水

        // 收集资源（供玩家点击调用）
    UFUNCTION(BlueprintCallable, Category = "Resource")
        float CollectResource();

protected:
    virtual void ApplyLevelUpBonus() override;

private:
    // 资源生产计时
    float ProductionTimer;
};