#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RTSCoreTypes.h"
#include "RTSGameMode.generated.h"

// 前置声明
class ABaseUnit;
class ABaseBuilding;
class ULevelDataAsset;
class AGridManager;

UCLASS()
class AUTOBATTLEDEMO_API ARTSGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ARTSGameMode();
    virtual void BeginPlay() override;

    // --- 1. 造兵逻辑 ---
    UFUNCTION(BlueprintCallable, Category = "GameFlow")
        bool TryBuyUnit(EUnitType Type, int32 Cost, int32 GridX, int32 GridY);

    // --- 2. 造建筑逻辑 (新增) ---
    UFUNCTION(BlueprintCallable, Category = "GameFlow")
        bool TryBuildBuilding(EBuildingType Type, int32 Cost, int32 GridX, int32 GridY);

    // --- 3. 战斗流程 ---
    UFUNCTION(BlueprintCallable, Category = "GameFlow")
        void StartBattlePhase();

    // 保存当前布阵并前往战斗关卡
    UFUNCTION(BlueprintCallable, Category = "GameFlow")
        void SaveAndStartBattle(FName LevelName);

    // 在战斗关卡开始时调用：生成带来的兵
    UFUNCTION(BlueprintCallable, Category = "GameFlow")
        void LoadAndSpawnUnits();

    // 重新开始本关
    UFUNCTION(BlueprintCallable, Category = "GameFlow")
        void RestartLevel();

    // --- 裁判逻辑 ---
    void OnActorKilled(AActor* Victim, AActor* Killer);
    void CheckWinCondition();

    // 保存基地建筑数据
    void SaveBaseLayout();

    // 加载并生成基地建筑
    void LoadAndSpawnBase();

    // 尝试升级指定建筑
    bool TryUpgradeBuilding(class ABaseBuilding* BuildingToUpgrade);

    // 结算并回城 (只保存活着的兵，并切换地图)
    UFUNCTION(BlueprintCallable, Category = "GameFlow")
        void ReturnToBase();

    // 在指定位置强制生成单位 (不扣资源，供兵营释放使用)
    bool SpawnUnitAt(EUnitType Type, int32 GridX, int32 GridY);

    // 获取当前玩家拥有的最高兵营等级 (如果没有兵营返回 0)
    UFUNCTION(BlueprintCallable, Category = "GameFlow")
        int32 GetCurrentTechLevel();

    // 检查是否满足造兵的科技要求
    bool CheckUnitTechRequirement(EUnitType Type);

    UFUNCTION(BlueprintPure)
        EGameState GetCurrentState() const { return CurrentState; }

protected:
    UPROPERTY(BlueprintReadOnly, Category = "GameFlow")
        EGameState CurrentState;

    UPROPERTY()
        class AGridManager* GridManager;

    // --- 兵种蓝图配置 (Classes|Units) ---
    UPROPERTY(EditDefaultsOnly, Category = "Classes|Units")
        TSubclassOf<class ABaseUnit> BarbarianClass; // 对应 Soldier/Barbarian

    UPROPERTY(EditDefaultsOnly, Category = "Classes|Units")
        TSubclassOf<class ABaseUnit> ArcherClass;

    UPROPERTY(EditDefaultsOnly, Category = "Classes|Units")
        TSubclassOf<class ABaseUnit> GiantClass;     // 新增

    UPROPERTY(EditDefaultsOnly, Category = "Classes|Units")
        TSubclassOf<class ABaseUnit> BomberClass;    // 新增

        // --- 建筑蓝图配置 (Classes|Buildings) ---
    UPROPERTY(EditDefaultsOnly, Category = "Classes|Buildings")
        TSubclassOf<class ABaseBuilding> DefenseTowerClass;

    UPROPERTY(EditDefaultsOnly, Category = "Classes|Buildings")
        TSubclassOf<class ABaseBuilding> GoldMineClass;

    UPROPERTY(EditDefaultsOnly, Category = "Classes|Buildings")
        TSubclassOf<class ABaseBuilding> ElixirPumpClass; // 新增

    UPROPERTY(EditDefaultsOnly, Category = "Classes|Buildings")
        TSubclassOf<class ABaseBuilding> WallClass;       // 新增

    UPROPERTY(EditDefaultsOnly, Category = "Classes|Buildings")
        TSubclassOf<class ABaseBuilding> HQClass;         // 大本营

    // --- 关卡数据 (可选，用于加载敌人配置) ---
    UPROPERTY(EditDefaultsOnly, Category = "Level Setup")
        ULevelDataAsset* CurrentLevelData;

    // 兵营蓝图
    UPROPERTY(EditDefaultsOnly, Category = "Classes|Buildings")
        TSubclassOf<class ABaseBuilding> BarracksClass;
};