#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RTSProjectile.generated.h"

UCLASS()
class AUTOBATTLEDEMO_API ARTSProjectile : public AActor
{
    GENERATED_BODY()

public:
    ARTSProjectile();
    virtual void Tick(float DeltaTime) override;

    // 初始化函数：告诉子弹谁射的，射谁，多少伤害
    void Initialize(AActor* NewTarget, float NewDamage, AActor* NewInstigator);

protected:
    UPROPERTY(VisibleAnywhere, Category = "Components")
        class UStaticMeshComponent* MeshComp;

    UPROPERTY(VisibleAnywhere, Category = "Components")
        class UProjectileMovementComponent* MovementComp;

    // 目标引用
    UPROPERTY()
        AActor* TargetActor;

    float Damage;
    AActor* DamageInstigator; // 谁发射的（经验归谁）
};