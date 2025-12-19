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

	// 绑定单位按钮
	if (Btn_BuyBarbarian)  Btn_BuyBarbarian->OnClicked.AddDynamic(this, &URTSMainHUD::OnClickBuyBarbarian);
	if (Btn_BuyArcher)  Btn_BuyArcher->OnClicked.AddDynamic(this, &URTSMainHUD::OnClickBuyArcher);
	if (Btn_BuyGiant)   Btn_BuyGiant->OnClicked.AddDynamic(this, &URTSMainHUD::OnClickBuyGiant);
	if (Btn_BuyBomber)  Btn_BuyBomber->OnClicked.AddDynamic(this, &URTSMainHUD::OnClickBuyBomber);

	// 绑定建筑按钮
	if (Btn_BuildTower) Btn_BuildTower->OnClicked.AddDynamic(this, &URTSMainHUD::OnClickBuildTower);
	if (Btn_BuildMine)  Btn_BuildMine->OnClicked.AddDynamic(this, &URTSMainHUD::OnClickBuildMine);

	// 绑定流程按钮
	if (Btn_StartBattle) Btn_StartBattle->OnClicked.AddDynamic(this, &URTSMainHUD::OnClickStartBattle);
}

void URTSMainHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());
	if (GI)
	{
		if (Text_GoldInfo)   Text_GoldInfo->SetText(FText::FromString(FString::Printf(TEXT("Gold: %d"), GI->PlayerGold)));
		if (Text_ElixirInfo) Text_ElixirInfo->SetText(FText::FromString(FString::Printf(TEXT("Elixir: %d"), GI->PlayerElixir)));
	}
}

// 辅助函数：处理点击并归还焦点
// 为了减少重复代码，你可以写个私有函数，不过这里为了清晰直接展开写
void URTSMainHUD::OnClickBuyBarbarian()
{
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());
	if (PC) {
		PC->OnSelectUnitToPlace(EUnitType::Soldier);
		PC->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));
	}
}

void URTSMainHUD::OnClickBuyArcher()
{
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());
	if (PC) {
		PC->OnSelectUnitToPlace(EUnitType::Archer);
		PC->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));
	}
}

void URTSMainHUD::OnClickBuyGiant()
{
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());
	if (PC) {
		PC->OnSelectUnitToPlace(EUnitType::Giant);
		PC->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));
	}
}

void URTSMainHUD::OnClickBuyBomber()
{
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());
	if (PC) {
		PC->OnSelectUnitToPlace(EUnitType::Bomber);
		PC->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));
	}
}

void URTSMainHUD::OnClickBuildTower()
{
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());
	if (PC) {
		PC->OnSelectBuildingToPlace(EBuildingType::Defense); // 注意：对应 GameMode switch 里的类型
		PC->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));
	}
}

void URTSMainHUD::OnClickBuildMine()
{
	ARTSPlayerController* PC = Cast<ARTSPlayerController>(GetOwningPlayer());
	if (PC) {
		PC->OnSelectBuildingToPlace(EBuildingType::Resource);
		PC->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));
	}
}

void URTSMainHUD::OnClickStartBattle()
{
	ARTSGameMode* GM = Cast<ARTSGameMode>(UGameplayStatics::GetGameMode(this));
	if (GM)
	{
		// 假设战斗地图叫 "BattleField1"，请确保你有这个地图文件
		GM->SaveAndStartBattle(FName("BattleField1"));
	}
}