#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RTSMainHUD.generated.h"

class UButton;
class UTextBlock;

UCLASS()
class AUTOBATTLEDEMO_API URTSMainHUD : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	// --- 资源显示 ---
	UPROPERTY(meta = (BindWidget))
		UTextBlock* Text_GoldInfo;

	UPROPERTY(meta = (BindWidget))
		UTextBlock* Text_ElixirInfo;

	// --- 按钮：单位 (消耗圣水) ---
	UPROPERTY(meta = (BindWidget))
		UButton* Btn_BuyBarbarian;

	UPROPERTY(meta = (BindWidget))
		UButton* Btn_BuyArcher;

	// 新增：巨人 & 炸弹人
	UPROPERTY(meta = (BindWidget))
		UButton* Btn_BuyGiant;

	UPROPERTY(meta = (BindWidget))
		UButton* Btn_BuyBomber;

	// --- 按钮：建筑 (消耗金币) ---
	// 新增：塔 & 矿
	UPROPERTY(meta = (BindWidget))
		UButton* Btn_BuildTower;

	UPROPERTY(meta = (BindWidget))
		UButton* Btn_BuildMine;

	// --- 按钮：流程 ---
	UPROPERTY(meta = (BindWidget))
		UButton* Btn_StartBattle;

	// --- 点击事件处理 ---
	UFUNCTION() void OnClickBuyBarbarian();
	UFUNCTION() void OnClickBuyArcher();
	UFUNCTION() void OnClickBuyGiant();
	UFUNCTION() void OnClickBuyBomber();

	UFUNCTION() void OnClickBuildTower();
	UFUNCTION() void OnClickBuildMine();

	UFUNCTION() void OnClickStartBattle();
};