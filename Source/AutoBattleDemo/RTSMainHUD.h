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

	// --- 人口显示 ---
	UPROPERTY(meta = (BindWidget))
		UTextBlock* Text_PopulationInfo;

	// --- 按钮：单位 (消耗圣水) ---
	UPROPERTY(meta = (BindWidget))
		UButton* Btn_BuyBarbarian;

	UPROPERTY(meta = (BindWidget))
		UButton* Btn_BuyArcher;

	// 巨人 & 炸弹人
	UPROPERTY(meta = (BindWidget))
		UButton* Btn_BuyGiant;

	UPROPERTY(meta = (BindWidget))
		UButton* Btn_BuyBomber;

	// 建筑 (消耗金币) ---
	// 塔 & 矿
	UPROPERTY(meta = (BindWidget))
		UButton* Btn_BuildTower;

	UPROPERTY(meta = (BindWidget))
		UButton* Btn_BuildMine;

	// 圣水收集器
	UPROPERTY(meta = (BindWidget))
		UButton* Btn_BuildElixir;

	// 造墙按钮
	UPROPERTY(meta = (BindWidget))
		UButton* Btn_BuildWall;

	// 造兵营按钮
	UPROPERTY(meta = (BindWidget))
		UButton* Btn_BuildBarracks;



	// --- 按钮：流程 ---
	UPROPERTY(meta = (BindWidget))
		UButton* Btn_StartBattle;

	// --- 按钮：移除 ---
	UPROPERTY(meta = (BindWidget))
		UButton* Btn_Remove;

	// 升级按钮
	UPROPERTY(meta = (BindWidget))
		UButton* Btn_Upgrade;

	UPROPERTY(meta = (BindWidget))
		UTextBlock* Text_UpgradeCost; // 显示 "Upgrade (500 G)"

	 // 教程提示文本
	UPROPERTY(meta = (BindWidget))
		UTextBlock* Text_Tutorial;

	


	// --- 点击事件处理 ---
	UFUNCTION() void OnClickBuyBarbarian();
	UFUNCTION() void OnClickBuyArcher();
	UFUNCTION() void OnClickBuyGiant();
	UFUNCTION() void OnClickBuyBomber();

	UFUNCTION() void OnClickBuildTower();
	UFUNCTION() void OnClickBuildMine();
	UFUNCTION()	void OnClickBuildElixir();
	UFUNCTION()	void OnClickBuildWall();
	UFUNCTION()	void OnClickBuildBarracks();

	UFUNCTION()	void OnClickUpgrade();

	UFUNCTION() void OnClickStartBattle();
	UFUNCTION() void OnClickRemove();

	
};