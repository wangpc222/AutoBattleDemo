#pragma once
#include "CoreMinimal.h"
#include "BaseGameEntity.h"
#include "BaseUnit.generated.h"

// 士兵状态机
UENUM()
enum class EUnitState : uint8
{
    Idle,       // 站桩（备战阶段或无目标）
    Moving,     // 正在沿路径移动
    Attacking   // 攻击中
};

UCLASS()
class AUTOBATTLEDEMO_API ABaseUnit : public ABaseGameEntity
{
    GENERATED_BODY()
public:
    ABaseUnit();
    virtual void Tick(float DeltaTime) override;

    // --- 供 GameMode 调用 ---
    // 游戏开始，激活战斗 AI
    UFUNCTION(BlueprintCallable)
        void SetUnitActive(bool bActive);

    // --- 属性 ---
    UPROPERTY(EditAnywhere, Category = "Combat")
        float AttackRange;

    UPROPERTY(EditAnywhere, Category = "Combat")
        float Damage;

    UPROPERTY(EditAnywhere, Category = "Movement")
        float MoveSpeed;

    UPROPERTY(EditAnywhere, Category = "Combat")
        float AttackInterval;

    // 添加胶囊体组件声明
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
        class UCapsuleComponent* CapsuleComp;

    // 添加网格组件声明 (如果你之前是在 BaseBuilding 里写的，这里也要写，或者统一写在 BaseGameEntity 里)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
        class UStaticMeshComponent* MeshComp;

protected:
    // --- 核心逻辑 ---
    // 1. 寻找最近的敌人 (遍历所有 ABaseGameEntity)
    AActor* FindClosestEnemy();

    // 2. 请求路径 (调用 Member A 的 GridManager)
    void RequestPathToTarget();

    // 3. 沿路径移动 (Tick 中执行)
    void MoveAlongPath(float DeltaTime);

    // 4. 执行攻击
    void PerformAttack();

private:
    EUnitState CurrentState;

    // 缓存当前的路径点列表
    TArray<FVector> PathPoints;
    int32 CurrentPathIndex;

    // 当前锁定的目标
    UPROPERTY()
        AActor* CurrentTarget;

    // 攻击计时器
    float LastAttackTime;

    // 引用 GridManager (BeginPlay 获取)
    class AGridManager* GridManagerRef;
};