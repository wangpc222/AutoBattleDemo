#pragma once
#include "CoreMinimal.h"
#include "BaseUnit.h"
#include "Soldier_Archer.generated.h"

/**
 * 弓箭手 - 远程单位
 * 特点：远程攻击，不需要走到目标脸上
 */
UCLASS()
class AUTOBATTLEDEMO_API ASoldier_Archer : public ABaseUnit
{
    GENERATED_BODY()

public:
    ASoldier_Archer();

    virtual void BeginPlay() override;

protected:
    // 重写攻击逻辑：远程攻击不需要移动到近战距离
    virtual void PerformAttack() override;
};