#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RTSMainHUD.generated.h"

// 前置声明
class UButton;
class UTextBlock;

UCLASS()
class AUTOBATTLEDEMO_API URTSMainHUD : public UUserWidget
{
    GENERATED_BODY()

public:
    // 初始化函数 (类似 BeginPlay)
    virtual void NativeConstruct() override;

    // 每帧更新 (类似 Tick)
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
    // --- 核心技巧：BindWidget ---
    // 只要你在蓝图里把按钮命名为 "Btn_BuySoldier"，C++ 就会自动绑定它！

    UPROPERTY(meta = (BindWidget))
        UButton* Btn_BuySoldier;

    UPROPERTY(meta = (BindWidget))
        UButton* Btn_BuyArcher;

    UPROPERTY(meta = (BindWidget))
        UButton* Btn_StartBattle;

    // --- 按钮点击处理函数 ---
    UFUNCTION()
        void OnClickBuySoldier();

    UFUNCTION()
        void OnClickBuyArcher();

    UFUNCTION()
        void OnClickStartBattle();

    // 显示金币
    UPROPERTY(meta = (BindWidget))
        UTextBlock* Text_GoldInfo;

    // 显示圣水
    UPROPERTY(meta = (BindWidget))
        UTextBlock* Text_ElixirInfo;
};