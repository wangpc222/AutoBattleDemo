#include "BaseBuilding.h"
#include "Components/StaticMeshComponent.h"

ABaseBuilding::ABaseBuilding()
{
    // 1. 初始化组件
    // 注意：BaseGameEntity 继承自 Pawn，可能自带了 RootComponent。
    // 这里我们强制把 Mesh 设为根，或者挂在父类的根下面。
    // 为了保险，我们新建一个 Mesh 并设为根。
    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BuildingMesh"));
    RootComponent = MeshComp;

    // 2. 初始化属性
    // MaxHealth = 1000.0f; // 可以设置父类的属性
    // CurrentHealth = MaxHealth;

    Level = 1;
    MaxLevel = 3;
}

void ABaseBuilding::BeginPlay()
{
    Super::BeginPlay();
    // 可以在这里根据 Level 设置初始材质颜色
}

void ABaseBuilding::NotifyActorOnClicked(FKey ButtonPressed)
{
    // 这里不要再写“点击扣血”了，那是之前的测试代码。
    // 现在的逻辑应该是：点击显示“升级”UI
    UE_LOG(LogTemp, Log, TEXT("Building Clicked! Level: %d"), Level);
}

void ABaseBuilding::Upgrade()
{
    if (Level >= MaxLevel) return;

    Level++;

    // 1. 提升属性 (数值随便定)
    MaxHealth += 500.0f;
    CurrentHealth = MaxHealth; // 升级补满血

    // 2. 视觉变化 (示例：变大一点，或者换材质)
    SetActorScale3D(GetActorScale3D() * 1.2f);

    UE_LOG(LogTemp, Warning, TEXT("Building Upgraded to Level %d! HP is now %f"), Level, MaxHealth);
}