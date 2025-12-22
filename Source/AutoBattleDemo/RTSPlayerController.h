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

    // 选择移除模式
    UFUNCTION(BlueprintCallable)
        void OnSelectRemoveMode();

    // 尝试取消当前操作
    // 返回 true 表示成功取消了某个操作
    // 返回 false 表示当前没事可做
    bool CancelCurrentAction();

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