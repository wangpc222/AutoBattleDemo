#include "RTSCameraPawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"

ARTSCameraPawn::ARTSCameraPawn()
{
    PrimaryActorTick.bCanEverTick = true;

    // 1. 创建根组件
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    // 2. 创建弹簧臂 (自拍杆)
    SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
    SpringArmComp->SetupAttachment(RootComponent);
    SpringArmComp->SetRelativeRotation(FRotator(-60.0f, 0.0f, 0.0f)); // 默认俯视 60 度
    SpringArmComp->TargetArmLength = 1500.0f; // 默认高度
    SpringArmComp->bDoCollisionTest = false; // RTS相机通常不需要碰撞检测(防止穿墙时镜头乱跳)

    // 3. 创建相机
    CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
    CameraComp->SetupAttachment(SpringArmComp);

    // 4. 初始化参数
    MoveSpeed = 1500.0f;
    ZoomSpeed = 400.0f;
    MinZoom = 300.0f;
    MaxZoom = 2500.0f;
    TargetZoom = 1500.0f;

    // 初始化拖拽灵敏度 (建议值)
    DragSpeed = 20.0f;
    bIsRMBDown = false;
}

void ARTSCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    // 绑定轴映射 (需要在项目设置里配)
    PlayerInputComponent->BindAxis("MoveForward", this, &ARTSCameraPawn::MoveForward);
    PlayerInputComponent->BindAxis("MoveRight", this, &ARTSCameraPawn::MoveRight);

    // 绑定按键 (鼠标滚轮)
    PlayerInputComponent->BindAction("ZoomIn", IE_Pressed, this, &ARTSCameraPawn::ZoomIn);
    PlayerInputComponent->BindAction("ZoomOut", IE_Pressed, this, &ARTSCameraPawn::ZoomOut);

    // --- 右键拖拽绑定 ---
    PlayerInputComponent->BindAction("RightMouseClick", IE_Pressed, this, &ARTSCameraPawn::OnRightClickDown);
    PlayerInputComponent->BindAction("RightMouseClick", IE_Released, this, &ARTSCameraPawn::OnRightClickUp);

    PlayerInputComponent->BindAxis("MouseX", this, &ARTSCameraPawn::HandleMouseDragX);
    PlayerInputComponent->BindAxis("MouseY", this, &ARTSCameraPawn::HandleMouseDragY);
}

void ARTSCameraPawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 平滑插值缩放 (FInterpTo)
    float NewZoom = FMath::FInterpTo(SpringArmComp->TargetArmLength, TargetZoom, DeltaTime, 5.0f);
    SpringArmComp->TargetArmLength = NewZoom;
}

void ARTSCameraPawn::MoveForward(float Value)
{
    if (Value != 0.0f)
    {
        // 沿着世界坐标 X 轴移动 (而不是相机朝向)
        AddActorWorldOffset(FVector(Value * MoveSpeed * GetWorld()->GetDeltaSeconds(), 0.f, 0.f));
    }
}

void ARTSCameraPawn::MoveRight(float Value)
{
    if (Value != 0.0f)
    {
        // 沿着世界坐标 Y 轴移动
        AddActorWorldOffset(FVector(0.f, Value * MoveSpeed * GetWorld()->GetDeltaSeconds(), 0.f));
    }
}

void ARTSCameraPawn::ZoomIn()
{
    TargetZoom = FMath::Clamp(TargetZoom - ZoomSpeed, MinZoom, MaxZoom);
}

void ARTSCameraPawn::ZoomOut()
{
    TargetZoom = FMath::Clamp(TargetZoom + ZoomSpeed, MinZoom, MaxZoom);
}

void ARTSCameraPawn::OnRightClickDown()
{
    bIsRMBDown = true;

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (PC)
    {
        // 1. 隐藏鼠标
        PC->bShowMouseCursor = false;

        // 2. 关键：切换输入模式为 "GameOnly"
        // 这种模式下，鼠标被引擎捕获，不会撞到屏幕边缘，可以无限拖动
        FInputModeGameOnly InputMode;
        PC->SetInputMode(InputMode);
    }
}

void ARTSCameraPawn::OnRightClickUp()
{
    bIsRMBDown = false;

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (PC)
    {
        // 1. 显示鼠标
        PC->bShowMouseCursor = true;

        // 2. 关键：切回 "GameAndUI" 模式
        // 这样你才能点击屏幕上的按钮和单位
        FInputModeGameAndUI InputMode;

        // 设置鼠标不锁定，防止松开后鼠标被困在窗口里出不来
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

        // 确保切回模式后不隐藏光标
        InputMode.SetHideCursorDuringCapture(false);

        PC->SetInputMode(InputMode);
    }
}

void ARTSCameraPawn::HandleMouseDragX(float Value)
{
    // 只有按住右键且鼠标有移动时才执行
    if (bIsRMBDown && Value != 0.0f)
    {
        // 逻辑：鼠标往左移(Value < 0)，相机应该往右走，产生"抓地"效果
        // 所以这里用 -Value
        // 鼠标 X 轴对应世界坐标 Y 轴 (左右平移)
        AddActorWorldOffset(FVector(0.f, -Value * DragSpeed, 0.f));
    }
}

void ARTSCameraPawn::HandleMouseDragY(float Value)
{
    if (bIsRMBDown && Value != 0.0f)
    {
        // 鼠标 Y 轴对应世界坐标 X 轴 (前后平移)
        // 同样取反，鼠标往下拉，相机往上走
        AddActorWorldOffset(FVector(-Value * DragSpeed, 0.f, 0.f));
    }
}