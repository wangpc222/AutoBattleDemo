#include "BaseBuilding.h"

// 构造函数：负责初始化组件和默认值
ABaseBuilding::ABaseBuilding()
{
    // 允许该 Actor 每帧 Tick（如果是静态建筑，通常可以设为 false 以优化性能）
    PrimaryActorTick.bCanEverTick = true;

    // 1. 初始化模型组件
    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BuildingMesh"));
    RootComponent = MeshComp; // 把模型设为根组件

    // 2. 初始化血量默认值
    MaxHealth = 100.0f;
}

// 游戏开始时执行一次
void ABaseBuilding::BeginPlay()
{
    Super::BeginPlay();

    // 可以在这里打印一条日志，证明代码跑起来了
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("BaseBuilding Created!"));
    }
}

// 每帧执行
void ABaseBuilding::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

}

void ABaseBuilding::NotifyActorOnClicked(FKey ButtonPressed)
{
    Super::NotifyActorOnClicked(ButtonPressed);

    // 1. 扣血逻辑
    MaxHealth -= 50.0f;

    // 2. 打印日志 (C++ 里的 printf)
    if (GEngine)
    {
        FString Msg = FString::Printf(TEXT("Ouch! Health: %f"), MaxHealth);
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, Msg);
    }

    // 3. 死亡逻辑
    if (MaxHealth <= 0.0f)
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Building Destroyed!"));

        // 销毁 Actor
        Destroy();
    }
}