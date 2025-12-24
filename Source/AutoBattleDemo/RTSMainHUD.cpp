#include "RTSMainHUD.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "RTSPlayerController.h"
#include "RTSGameMode.h"
#include "BaseBuilding.h" 
#include "RTSGameInstance.h"

void URTSMainHUD::NativeConstruct()
{
	Super::NativeConstruct();

	// 绑定单位按钮
	if (Btn_BuyBarbarian)  Btn_BuyBarbarian->OnClicked.AddDynamic(this, &URTSMainHUD::OnClickBuyBarbarian);
	if (Btn_BuyArcher)  Btn_BuyArcher->OnClicked.AddDynamic(this, &URTSMainHUD::OnClickBuyArcher);
	if (Btn_BuyGiant)   Btn_BuyGiant->OnClicked.AddDynamic(this, &URTSMainHUD::OnClickBuyGiant);
	if (Btn_BuyBomber)  Btn_BuyBomber->OnClicked.AddDynamic(this, &URTSMainHUD::OnClickBuyBomber);

	// 绑定建筑按钮
	if (Btn_BuildTower) Btn_BuildTower->OnClicked.AddDynamic(this, &URTSMainHUD::OnClickBuildTower);
	if (Btn_BuildMine)  Btn_BuildMine->OnClicked.AddDynamic(this, &URTSMainHUD::OnClickBuildMine);
	if (Btn_BuildElixir) Btn_BuildElixir->OnClicked.AddDynamic(this, &URTSMainHUD::OnClickBuildElixir);
	if (Btn_BuildWall)	Btn_BuildWall->OnClicked.AddDynamic(this, &URTSMainHUD::OnClickBuildWall);
	if (Btn_BuildBarracks)	Btn_BuildBarracks->OnClicked.AddDynamic(this, &URTSMainHUD::OnClickBuildBarracks);

	// 绑定流程按钮
	if (Btn_StartBattle) Btn_StartBattle->OnClicked.AddDynamic(this, &URTSMainHUD::OnClickStartBattle);
	
	// 绑定移除按钮
	if (Btn_Remove) Btn_Remove->OnClicked.AddDynamic(this, &URTSMainHUD::OnClickRemove);

	// 建筑升级按钮
	if (Btn_Upgrade) Btn_Upgrade->OnClicked.AddDynamic(this, &URTSMainHUD::OnClickUpgrade);
	// 默认隐藏
	if (Btn_Upgrade) Btn_Upgrade->SetVisibility(ESlateVisibility::Hidden);
}

void URTSMainHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());
	if (GI)
	{
		if (Text_GoldInfo)   Text_GoldInfo->SetText(FText::FromString(FString::Printf(TEXT("Gold: %d"), GI->PlayerGold)));
		if (Text_ElixirInfo) Text_ElixirInfo->SetText(FText::FromString(FString::Printf(TEXT("Elixir: %d"), GI->PlayerElixir)));
		
		// 更新人口
		if (Text_PopulationInfo)
		{
			// 格式示例： Pop: 5 / 20
			FString PopStr = FString::Printf(TEXT("Pop: %d / %d"),
				GI->CurrentPopulation,
				GI->MaxPopulation);

			Text_PopulationInfo->SetText(FText::FromString(PopStr));
		}
	}

	// 动态更新升级按钮
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());
	if (PC && Btn_Upgrade && Text_UpgradeCost)
	{
		// 如果有选中的建筑，且该建筑属于玩家
		if (PC->SelectedBuilding && PC->SelectedBuilding->TeamID == ETeam::Player)
		{
			// 显示按钮
			Btn_Upgrade->SetVisibility(ESlateVisibility::Visible);

			// 获取升级价格
			int32 GoldCost, ElixirCost;
			PC->SelectedBuilding->GetUpgradeCost(GoldCost, ElixirCost);

			// 检查是否满级
			if (PC->SelectedBuilding->CanUpgrade())
			{
				Text_UpgradeCost->SetText(FText::FromString(FString::Printf(TEXT("Upgrade (%d G)"), GoldCost)));
				Btn_Upgrade->SetIsEnabled(true);
			}
			else
			{
				Text_UpgradeCost->SetText(FText::FromString(TEXT("MAX LEVEL")));
				Btn_Upgrade->SetIsEnabled(false);
			}
		}
		else
		{
			// 没选中东西，隐藏按钮
			Btn_Upgrade->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (PC)
	{
		// 更新教程文字
		if (Text_Tutorial)
		{
			// 调用 Controller 里的函数获取当前应该显示的文字
			Text_Tutorial->SetText(PC->GetTutorialText());

			// 优化：如果教程完成了，隐藏这个文本框			
			if (PC->GetTutorialStep() == ETutorialStep::Completed)
				Text_Tutorial->SetVisibility(ESlateVisibility::Hidden);
			else
				Text_Tutorial->SetVisibility(ESlateVisibility::Visible);
		}
	}
}

// 辅助函数：处理点击并归还焦点
void URTSMainHUD::OnClickBuyBarbarian()
{
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());
	if (PC) {
		// 拦截检查
		if (!PC->IsActionAllowed("BuyBarbarian"))
		{
			// 屏幕红字提示
			if (GEngine)
			{
				FString Tip = PC->GetTutorialText().ToString();
				GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Tutorial: ") + Tip);
			}
			return;
		}

		PC->OnSelectUnitToPlace(EUnitType::Barbarian);
		PC->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));
	}
}

void URTSMainHUD::OnClickBuyArcher()
{
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());
	if (PC) {
		// 拦截检查
		if (!PC->IsActionAllowed("BuyArcher"))
		{
			// 屏幕红字提示
			if (GEngine)
			{
				FString Tip = PC->GetTutorialText().ToString();
				GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Tutorial: ") + Tip);
			}
			return;
		}

		PC->OnSelectUnitToPlace(EUnitType::Archer);
		PC->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));
	}
}

void URTSMainHUD::OnClickBuyGiant()
{
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());
	if (PC) {
		// 拦截检查
		if (!PC->IsActionAllowed("BuyGiant"))
		{
			// 屏幕红字提示
			if (GEngine)
			{
				FString Tip = PC->GetTutorialText().ToString();
				GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Tutorial: ") + Tip);
			}
			return;
		}

		PC->OnSelectUnitToPlace(EUnitType::Giant);
		PC->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));
	}
}

void URTSMainHUD::OnClickBuyBomber()
{
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());
	if (PC) {
		// 拦截检查
		if (!PC->IsActionAllowed("BuyBomber"))
		{
			// 屏幕红字提示
			if (GEngine)
			{
				FString Tip = PC->GetTutorialText().ToString();
				GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Tutorial: ") + Tip);
			}
			return;
		}

		PC->OnSelectUnitToPlace(EUnitType::Bomber);
		PC->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));
	}
}

void URTSMainHUD::OnClickBuildTower()
{
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());
	if (PC) {
		if (!PC->IsActionAllowed("BuildTower")) {
			// 屏幕红字提示
			if (GEngine)
			{
				FString Tip = PC->GetTutorialText().ToString();
				GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Tutorial: ") + Tip);
			}
			return; // <--- 拦截
		}
		PC->OnSelectBuildingToPlace(EBuildingType::Defense); // 注意：对应 GameMode switch 里的类型
		PC->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));
	}
}

void URTSMainHUD::OnClickBuildMine()
{
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());
	if (PC) {
		if (!PC->IsActionAllowed("BuildGoldMine")) {
			// 屏幕红字提示
			if (GEngine)
			{
				FString Tip = PC->GetTutorialText().ToString();
				GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Tutorial: ") + Tip);
			}
			return; // <--- 拦截
		}

		PC->OnSelectBuildingToPlace(EBuildingType::GoldMine);
		PC->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));
	}
}

void URTSMainHUD::OnClickBuildElixir()
{
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());
	if (PC) {
		if (!PC->IsActionAllowed("BuildElixir")) {
			// 屏幕红字提示
			if (GEngine)
			{
				FString Tip = PC->GetTutorialText().ToString();
				GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Tutorial: ") + Tip);
			}
			return; // <--- 拦截
		}

		// 告诉 Controller 我要造圣水收集器
		PC->OnSelectBuildingToPlace(EBuildingType::ElixirPump);

		// 归还鼠标焦点
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
	}
}

// 实现造墙逻辑
void URTSMainHUD::OnClickBuildWall()
{
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());
	if (PC)
	{
		if (!PC->IsActionAllowed("BuildWall")) {
			// 屏幕红字提示
			if (GEngine)
			{
				FString Tip = PC->GetTutorialText().ToString();
				GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Tutorial: ") + Tip);
			}
			return; // <--- 拦截
		}

		// 告诉 Controller：我要造墙
		PC->OnSelectBuildingToPlace(EBuildingType::Wall);

		// 归还鼠标焦点 (老规矩，防止点完按钮后没法点地板)
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
	}
}

// 实现造兵营逻辑
void URTSMainHUD::OnClickBuildBarracks()
{
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());
	if (PC)
	{
		if (!PC->IsActionAllowed("BuildBarracks")) {
			// 屏幕红字提示
			if (GEngine)
			{
				FString Tip = PC->GetTutorialText().ToString();
				GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Tutorial: ") + Tip);
			}
			return; // <--- 拦截
		}

		PC->OnSelectBuildingToPlace(EBuildingType::Barracks);

		// 归还鼠标焦点
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
	}
}

void URTSMainHUD::OnClickStartBattle()
{
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());

	// 检查 Controller 是否允许
	if (PC && !PC->IsActionAllowed("StartBattle")) {
		// 屏幕红字提示
		if (GEngine)
		{
			FString Tip = PC->GetTutorialText().ToString();
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Tutorial: ") + Tip);
		}
		return; // <--- 拦截
	}

	ARTSGameMode* GM = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(this));
	if (GM)
	{

		// 如果是教程最后一步，这里会触发完成
		if (PC) PC->AdvanceTutorial();

		// 假设战斗地图叫 "BattleField1"
		GM->SaveAndStartBattle(FName("BattleField1"));
	}
}

void URTSMainHUD::OnClickRemove()
{
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());
	if (PC)
	{
		if (!PC->IsActionAllowed("Remove"))
		{
			if (GEngine)
			{
				FString Tip = PC->GetTutorialText().ToString();
				GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Tutorial: ") + Tip);
			}
			return; // 被拦截，不进入移除模式
		}

		PC->OnSelectRemoveMode();
		// 归还焦点
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
	}
}

void URTSMainHUD::OnClickUpgrade()
{
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());
	if (PC)
	{
		PC->RequestUpgradeSelectedBuilding();
	}
}

