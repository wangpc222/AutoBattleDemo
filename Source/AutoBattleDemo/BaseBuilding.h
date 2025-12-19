#pragma once

#include "CoreMinimal.h"
#include "BaseGameEntity.h"
#include "RTSCoreTypes.h"
#include "BaseBuilding.generated.h"


UCLASS()
class AUTOBATTLEDEMO_API ABaseBuilding : public ABaseGameEntity
{
    GENERATED_BODY()

public:
    ABaseBuilding();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // --- 建筑属性 ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
        EBuildingType BuildingType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
        int32 BuildingLevel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
        int32 MaxLevel;

    // --- 网格坐标（关键！供炸弹人使用） ---
    // 这个建筑在成员A的GridManager中占据的网格坐标
    // 成员C在生成建筑时需要设置这两个值
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
        int32 GridX;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
        int32 GridY;

    // --- 升级系统 ---
    UFUNCTION(BlueprintCallable, Category = "Building")
        void LevelUp();

    UFUNCTION(BlueprintCallable, Category = "Building")
        void GetUpgradeCost(int32& OutGold, int32& OutElixir);

    UFUNCTION(BlueprintCallable, Category = "Building")
        bool CanUpgrade() const;

    // --- 点击交互 ---
    virtual void NotifyActorOnClicked(FKey ButtonPressed) override;

    // --- 组件 ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
        class UStaticMeshComponent* MeshComp;

protected:
    virtual void ApplyLevelUpBonus();

    UPROPERTY(EditAnywhere, Category = "Building|Upgrade")
        int32 BaseUpgradeGoldCost;


    UPROPERTY(EditAnywhere, Category = "Building|Upgrade")
        int32 BaseUpgradeElixirCost;
};
