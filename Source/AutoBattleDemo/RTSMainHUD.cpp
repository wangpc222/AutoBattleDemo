#include "RTSMainHUD.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "RTSPlayerController.h"
#include "RTSGameMode.h"
#include "RTSGameInstance.h"

void URTSMainHUD::NativeConstruct()
{
    Super::NativeConstruct();

    // 绑定点击事件
    if (Btn_BuySoldier)
    {
        Btn_BuySoldier->OnClicked.AddDynamic(this, &URTSMainHUD::OnClickBuySoldier);
    }
    if (Btn_BuyArcher)
    {
        Btn_BuyArcher->OnClicked.AddDynamic(this, &URTSMainHUD::OnClickBuyArcher);
    }
    if (Btn_StartBattle)
    {
        Btn_StartBattle->OnClicked.AddDynamic(this, &URTSMainHUD::OnClickStartBattle);
    }
}

// RTSMainHUD.cpp

void URTSMainHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());
    if (GI)
    {
        // 更新金币
        if (Text_GoldInfo)
        {
            Text_GoldInfo->SetText(FText::FromString(FString::Printf(TEXT("Gold: %d"), GI->PlayerGold)));
        }

        // 更新圣水 (Elixir)
        if (Text_ElixirInfo)
        {
            Text_ElixirInfo->SetText(FText::FromString(FString::Printf(TEXT("Elixir: %d"), GI->PlayerElixir)));
        }
    }
}

void URTSMainHUD::OnClickBuySoldier()
{
    // 获取 Controller，告诉它我要买步兵
    ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());
    if (PC)
    {
        PC->OnSelectUnitToPlace(EUnitType::Soldier);

        // 把焦点还给游戏，否则 Tick 里的鼠标检测可能会卡住
        FInputModeGameAndUI InputMode;
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        InputMode.SetHideCursorDuringCapture(false);
        // 关键：不要让 Widget 继续持有焦点
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = true;
    }
}

void URTSMainHUD::OnClickBuyArcher()
{
    // 告诉 Controller 我要买弓箭手
    ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());
    if (PC)
    {
        PC->OnSelectUnitToPlace(EUnitType::Archer);
    }
}

void URTSMainHUD::OnClickStartBattle()
{
    // 获取 GameMode，开始战斗
    ARTSGameMode* GM = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(this));
    if (GM)
    {
        GM->StartBattlePhase();

        // 隐藏开始按钮，防止重复点击
        if (Btn_StartBattle)
        {
            Btn_StartBattle->SetVisibility(ESlateVisibility::Hidden);
        }
    }
}