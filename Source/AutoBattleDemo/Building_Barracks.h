#pragma once
#include "CoreMinimal.h"
#include "BaseBuilding.h"
#include "RTSCoreTypes.h" // 为了用 FUnitSaveData
#include "Building_Barracks.generated.h"

UCLASS()
class AUTOBATTLEDEMO_API ABuilding_Barracks : public ABaseBuilding
{
    GENERATED_BODY()

public:
    ABuilding_Barracks();

    virtual void BeginPlay() override;

    // 关键：当物体被销毁（被移除或被打爆）时调用
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintCallable, Category = "Barracks")
        void StoreUnit(class ABaseUnit* UnitToStore);

    UFUNCTION(BlueprintCallable, Category = "Barracks")
        void ReleaseAllUnits();

    // 获取所有库存兵种类型 (用于存档)
    TArray<EUnitType> GetStoredUnitTypes() const;

    // 从存档恢复库存 (用于读档)
    void RestoreStoredUnits(const TArray<EUnitType>& InUnits);

    // 获取当前等级提供的人口/容量
    int32 GetCurrentCapacity(int Level) const;

    // 覆盖父类的 LevelUp，完全接管升级逻辑
    virtual void LevelUp() override;

protected:
    // 提供的额外人口上限
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
        int32 PopulationBonus;

    // 仓库：存放在里面的兵
    // 复用 FUnitSaveData 结构体，只存类型即可，位置无所谓
    UPROPERTY(VisibleAnywhere, Category = "Storage")
        TArray<FUnitSaveData> StoredUnits;

    // 覆盖父类的 ApplyLevelUpBonus
    virtual void ApplyLevelUpBonus() override;

};