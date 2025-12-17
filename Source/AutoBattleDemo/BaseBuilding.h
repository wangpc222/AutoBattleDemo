#pragma once

#include "CoreMinimal.h"
#include "BaseGameEntity.h"
#include "BaseBuilding.generated.h"

UCLASS()
class AUTOBATTLEDEMO_API ABaseBuilding : public ABaseGameEntity
{
    GENERATED_BODY()

public:
    ABaseBuilding();

    virtual void BeginPlay() override;

    // 建筑特有的交互：被点击时可能触发升级菜单 (暂时保留)
    virtual void NotifyActorOnClicked(FKey ButtonPressed = EKeys::LeftMouseButton) override;

    // --- 核心变量 ---

    // 可视化组件
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
        class UStaticMeshComponent* MeshComp;

    // ? 删除 MaxHealth！因为 BaseGameEntity 里已经有了！
    // ? 删除 TeamID！因为 BaseGameEntity 里也有了！

    // --- 新增：等级系统 ---

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level")
        int32 Level;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level")
        int32 MaxLevel;

    // 升级函数：提升属性，扣钱(在外部扣)，改变外观
    UFUNCTION(BlueprintCallable, Category = "Level")
        void Upgrade();
};