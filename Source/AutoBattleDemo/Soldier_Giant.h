#pragma once
#include "CoreMinimal.h"
#include "BaseUnit.h"
#include "Soldier_Giant.generated.h"

/**
 * 巨人 - 优先攻击防御塔
 * 特点：超高血量，优先寻找并攻击防御建筑
 */
UCLASS()
class AUTOBATTLEDEMO_API ASoldier_Giant : public ABaseUnit
{
    GENERATED_BODY()

public:
    ASoldier_Giant();

    virtual void BeginPlay() override;

protected:
    // 重写目标寻找：优先攻击防御塔
    virtual AActor* FindClosestEnemyBuilding() override;
};