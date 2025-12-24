#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RTSCoreTypes.h"
#include "RTSPlayerController.generated.h"

class AGridManager;
class URTSMainHUD;

UCLASS()
class AUTOBATTLEDEMO_API ARTSPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    ARTSPlayerController();
    virtual void Tick(float DeltaTime) override;
    virtual void SetupInputComponent() override;
    virtual void BeginPlay() override;

    // --- 交互逻辑 ---

    // 玩家点击了UI上的“购买单位”
    UFUNCTION(BlueprintCallable)
        void OnSelectUnitToPlace(EUnitType UnitType);

    // 玩家点击了UI上的“建造建筑”
    UFUNCTION(BlueprintCallable)
        void OnSelectBuildingToPlace(EBuildingType BuildingType);

    // 鼠标点击左键
    UFUNCTION(BlueprintCallable)
        void HandleLeftClick();

    // 供 HUD 调用：执行升级
    UFUNCTION(BlueprintCallable)
        void RequestUpgradeSelectedBuilding();

    // 供 HUD 获取：当前选中的建筑 (用于显示信息)
    UPROPERTY(BlueprintReadOnly, Category = "Selection")
        class ABaseBuilding* SelectedBuilding;

    // 选择移除模式
    UFUNCTION(BlueprintCallable)
        void OnSelectRemoveMode();

    // 当前选中的我方单位
    UPROPERTY(BlueprintReadOnly, Category = "Selection")
        class ABaseUnit* SelectedUnit;

    // 处理 Esc 按键
    void OnPressEsc();

    // 开始移动选中的单位
    UFUNCTION(BlueprintCallable)
        void StartRepositioningSelectedUnit();

    // 尝试取消当前操作
    // 返回 true 表示成功取消了某个操作
    // 返回 false 表示当前没事可做
    bool CancelCurrentAction();

    // 当前教程步骤
    UPROPERTY(BlueprintReadOnly, Category = "Tutorial")
        ETutorialStep CurrentTutorialStep;

    // 推进教程到下一步
    void AdvanceTutorial();

    // 检查某个操作是否被教程允许
    bool IsActionAllowed(FString ActionName);

    // 供 HUD 调用：显示当前教程提示 (绑定给 UI Text)
    UFUNCTION(BlueprintCallable, Category = "Tutorial")
        FText GetTutorialText() const;

    UFUNCTION(BlueprintCallable, Category = "Tutorial")
        ETutorialStep GetTutorialStep() const { return CurrentTutorialStep; }

protected:
    // UI 配置
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
        TSubclassOf<URTSMainHUD> MainHUDClass;

    // 战斗 UI 配置
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
        TSubclassOf<UUserWidget> BattleHUDClass; // 战斗用的 UI 模板

    UPROPERTY()
        URTSMainHUD* MainHUDInstance;

    // 幽灵 Actor
    UPROPERTY()
        AActor* PreviewGhostActor;


    // 兵种的幽灵
    UPROPERTY(EditDefaultsOnly, Category = "UI")
        TSubclassOf<AActor> PlacementPreviewClass;

    // 建筑的幽灵
    UPROPERTY(EditDefaultsOnly, Category = "UI")
        TSubclassOf<AActor> PlacementPreviewBuildingClass;

    // 辅助函数：更新幽灵位置
    void UpdatePlacementGhost();

    // 用于幽灵材质切换
    UPROPERTY(EditDefaultsOnly, Category = "UI|Placement")
        class UMaterialInterface* ValidPlacementMaterial; // 合法放置时的材质

    UPROPERTY(EditDefaultsOnly, Category = "UI|Placement")
        UMaterialInterface* InvalidPlacementMaterial; // 非法放置时的材质

    // 正在被移动的单位
    UPROPERTY()
        class ABaseUnit* UnitBeingMoved;

    
    // 初始化教程
    void InitTutorial();


private:
    // 当前状态数据
    EUnitType PendingUnitType;
    EBuildingType PendingBuildingType;

    bool bIsPlacingUnit;
    bool bIsPlacingBuilding;

    // 是否处于移除模式
    bool bIsRemoving;

    // 逻辑拆分
    void HandlePlacementMode(const FHitResult& Hit, AGridManager* GridManager);
    void HandleRemoveMode(AActor* HitActor, AGridManager* GridManager);
    void HandleNormalMode(AActor* HitActor);

    
};