#include "Soldier_Barbarian.h"

ASoldier_Barbarian::ASoldier_Barbarian()
{
    // 设置兵种类型
    UnitType = EUnitType::Barbarian;

    // 野蛮人属性
    MaxHealth = 150.0f;
    AttackRange = 150.0f;
    Damage = 15.0f;
    MoveSpeed = 250.0f;
    AttackInterval = 1.0f;
}

void ASoldier_Barbarian::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("[Barbarian] %s spawned | HP: %f | Damage: %f"),
        *GetName(), MaxHealth, Damage);
}