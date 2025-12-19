#include "BaseBuilding.h"
#include "Components/StaticMeshComponent.h"

ABaseBuilding::ABaseBuilding()
{
    PrimaryActorTick.bCanEverTick = true;

    // 创建根组件
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

    // 创建模型组件
    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BuildingMesh"));
    MeshComp->SetupAttachment(RootComponent);
    MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    MeshComp->SetCollisionObjectType(ECC_WorldStatic);
    MeshComp->SetCollisionResponseToAllChannels(ECR_Block);
    MeshComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

    // 默认属性
    MaxHealth = 500.0f;
    TeamID = ETeam::Enemy;
    bIsTargetable = true;

    BuildingType = EBuildingType::Other;
    BuildingLevel = 1;
    MaxLevel = 5;

    BaseUpgradeGoldCost = 100;
    BaseUpgradeElixirCost = 50;

    // 网格坐标初始化为 -1（未设置）
    GridX = -1;
    GridY = -1;
}

void ABaseBuilding::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Log, TEXT("[Building] %s | Type: %d | Level: %d | GridPos: (%d, %d)"),
        *GetName(), (int32)BuildingType, BuildingLevel, GridX, GridY);
}

void ABaseBuilding::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ABaseBuilding::LevelUp()
{
    if (!CanUpgrade())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Building] %s cannot upgrade (Max level reached)"), *GetName());
        return;
    }

    BuildingLevel++;

    ApplyLevelUpBonus();

    UE_LOG(LogTemp, Warning, TEXT("[Building] %s upgraded to Level %d | HP: %f"),
        *GetName(), BuildingLevel, MaxHealth);

    CurrentHealth = MaxHealth;
}

void ABaseBuilding::GetUpgradeCost(int32& OutGold, int32& OutElixir)
{
    OutGold = BaseUpgradeGoldCost * BuildingLevel;
    OutElixir = BaseUpgradeElixirCost * BuildingLevel;
}

bool ABaseBuilding::CanUpgrade() const
{
    return BuildingLevel < MaxLevel;
}

void ABaseBuilding::ApplyLevelUpBonus()
{
    MaxHealth *= 1.2f;
    CurrentHealth = MaxHealth;
}

void ABaseBuilding::NotifyActorOnClicked(FKey ButtonPressed)
{
    Super::NotifyActorOnClicked(ButtonPressed);

    UE_LOG(LogTemp, Warning, TEXT("[Building] Clicked: %s | Level: %d | HP: %f/%f | Grid: (%d, %d)"),
        *GetName(), BuildingLevel, CurrentHealth, MaxHealth, GridX, GridY);

    if (GEngine)
    {
        FString Msg = FString::Printf(TEXT("%s | Lv.%d | HP: %.0f/%.0f"),
            *GetName(), BuildingLevel, CurrentHealth, MaxHealth);
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, Msg);
    }
}