#include "RTSPlayerController.h"
#include "RTSGameMode.h"
#include "GridManager.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "RTSMainHUD.h"
#include "DrawDebugHelpers.h"

ARTSPlayerController::ARTSPlayerController()
{
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
    PrimaryActorTick.bCanEverTick = true;
    bIsPlacingUnit = false;
    PendingUnitType = EUnitType::Soldier;
}

void ARTSPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (IsLocalPlayerController())
    {
        if (MainHUDClass)
        {
            MainHUDInstance = CreateWidget<URTSMainHUD>(this, MainHUDClass);
            if (MainHUDInstance)
            {
                MainHUDInstance->AddToViewport();
                FInputModeGameAndUI InputMode;
                InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
                SetInputMode(InputMode);
                bShowMouseCursor = true;
            }
        }
    }
}

void ARTSPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 只有在造兵模式下，才显示网格
    if (bIsPlacingUnit)
    {
        UpdatePlacementGhost(); // 你的幽灵逻辑

        // --- 新增：画网格线 ---
        AGridManager* GridManager = Cast<AGridManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass()));

        if (GridManager)
        {
            int32 HoverX = -1;
            int32 HoverY = -1;

            // 再次做一次射线检测，获取当前悬停的格子坐标
            // (虽然 UpdatePlacementGhost 里做过一次，但为了逻辑解耦，这里再做一次也无妨，性能损耗极小)
            FHitResult Hit;
            GetHitResultUnderCursor(ECC_Visibility, false, Hit);

            if (Hit.bBlockingHit)
            {
                GridManager->WorldToGrid(Hit.Location, HoverX, HoverY);
            }

            // 呼叫 GridManager 画这一帧的线
            // 如果鼠标没指在格子上，HoverX/Y 就是 -1，就不会有蓝线，只有白/红线
            GridManager->DrawGridVisuals(HoverX, HoverY);
        }
    }
}

void ARTSPlayerController::OnSelectUnitToPlace(EUnitType UnitType)
{
    PendingUnitType = UnitType;
    bIsPlacingUnit = true;

    // 如果还没生成幽灵，就生成一个
    if (!PreviewGhostActor && PlacementPreviewClass)
    {
        FVector SpawnLoc = FVector::ZeroVector;
        FRotator SpawnRot = FRotator::ZeroRotator;
        PreviewGhostActor = GetWorld()->SpawnActor<AActor>(PlacementPreviewClass, SpawnLoc, SpawnRot);
    }

    if (PreviewGhostActor)
    {
        PreviewGhostActor->SetActorHiddenInGame(false);
    }
}

void ARTSPlayerController::HandleLeftClick()
{
    // 1. 基础检测
    FHitResult Hit;
    GetHitResultUnderCursor(ECC_Visibility, false, Hit);

    AGridManager* GridManager = Cast<AGridManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass()));

    if (!Hit.bBlockingHit || !GridManager) return;

    int32 X, Y;
    // 只有点在网格内才继续
    if (GridManager->WorldToGrid(Hit.Location, X, Y))
    {
        // ==========================================
        // 造兵模式 (点击 UI 按钮后)
        // ==========================================
        if (bIsPlacingUnit)
        {
            ARTSGameMode* GM = Cast<ARTSGameMode>(GetWorld()->GetAuthGameMode());
            if (GM)
            {
                // 简单的价格表 (实际项目可以写在配置表里)
                int32 Cost = (PendingUnitType == EUnitType::Soldier) ? 50 : 100;

                // --- 这里就是那个 Success ---
                // 调用裁判：尝试在这里买兵
                bool bSuccess = GM->TryBuyUnit(PendingUnitType, Cost, X, Y);

                if (bSuccess)
                {
                    // 1. 购买成功，隐藏幽灵
                    if (PreviewGhostActor)
                    {
                        PreviewGhostActor->SetActorHiddenInGame(true);
                    }

                    // 2. 退出放置模式
                    bIsPlacingUnit = false;

                    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("Unit Placed!"));
                }
                else
                {
                    // 购买失败 (钱不够，或者格子被占了)
                    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Cannot Place Here (No Money / Blocked)!"));
                }
            }
        }
        //// ==========================================
        //// 分支 B：调试模式 (没点 UI 时，测试寻路)
        //// ==========================================
        //else
        //{
        //    // 这里保留你之前的测试代码，方便成员 A 和 B 调试
        //    FVector StartPos = GridManager->GridToWorld(0, 0);
        //    FVector EndPos = GridManager->GridToWorld(X, Y);
        //    TArray<FVector> Path = GridManager->FindPath(StartPos, EndPos);

        //    if (Path.Num() > 0)
        //    {
        //        FlushPersistentDebugLines(GetWorld());
        //        for (const FVector& Point : Path)
        //        {
        //            DrawDebugSphere(GetWorld(), Point, 15.0f, 12, FColor::Blue, false, 5.0f);
        //        }
        //        for (int32 i = 0; i < Path.Num() - 1; i++)
        //        {
        //            DrawDebugLine(GetWorld(), Path[i], Path[i + 1], FColor::Cyan, false, 5.0f, 0, 3.0f);
        //        }
        //        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, FString::Printf(TEXT("Path Found! Steps: %d"), Path.Num()));
        //    }
        //}
    }
}

// 让幽灵吸附网格
void ARTSPlayerController::UpdatePlacementGhost()
{
    if (!PreviewGhostActor) return;

    FHitResult Hit;
    // 使用 Visibility 通道检测，确保只打到地板 (前提是 Ghost 是 NoCollision)
    GetHitResultUnderCursor(ECC_Visibility, false, Hit);

    // 只有打中东西，且不是 Ghost 自己，才更新
    if (Hit.bBlockingHit && Hit.GetActor() != PreviewGhostActor)
    {
        AGridManager* GridManager = Cast<AGridManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass()));

        if (GridManager)
        {
            int32 X, Y;
            // 算出鼠标指着哪个格子
            if (GridManager->WorldToGrid(Hit.Location, X, Y))
            {
                // 1. 获取网格中心点
                FVector SnapPos = GridManager->GridToWorld(X, Y);

                // 2. [优化] 智能计算 Z 轴高度
                // 获取 Ghost 的边界 (Bounds)
                FVector Origin, BoxExtent;
                PreviewGhostActor->GetActorBounds(false, Origin, BoxExtent);

                // BoxExtent.Z 是半高。
                // 如果 Ghost 原点在中心，我们需要抬高 BoxExtent.Z。
                // 额外加 2.0f 防止 Z-Fighting 闪烁。
                float HoverHeight = BoxExtent.Z + 2.0f;

                SnapPos.Z += HoverHeight;

                // 3. 设置位置
                PreviewGhostActor->SetActorLocation(SnapPos);

                // 4. [可选体验优化] 变色逻辑
                // 如果该格子不可走，把 Ghost 变红；可走变绿
                /*
                bool bCanPlace = GridManager->IsTileWalkable(X, Y);
                UStaticMeshComponent* MeshComp = PreviewGhostActor->FindComponentByClass<UStaticMeshComponent>();
                if(MeshComp)
                {
                    // 需要在材质里设置参数 "Color"，或者直接切换材质
                    // MeshComp->SetVectorParameterValueOnMaterials("Color", bCanPlace ? FVector(0,1,0) : FVector(1,0,0));
                }
                */
            }
        }
    }
}

void ARTSPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindAction("LeftClick", IE_Pressed, this, &ARTSPlayerController::HandleLeftClick);
}

