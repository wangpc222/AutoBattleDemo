#include "RTSPlayerController.h"
#include "RTSGameMode.h"
#include "GridManager.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "RTSMainHUD.h"
#include "DrawDebugHelpers.h"
#include "RTSGameInstance.h"
#include "RTSCoreTypes.h"
#include "BaseUnit.h"
#include "BaseBuilding.h"
#include "Building_Resource.h" // 用于点击收集资源
#include "Components/StaticMeshComponent.h"

ARTSPlayerController::ARTSPlayerController()
{
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
    PrimaryActorTick.bCanEverTick = true;

    bIsPlacingUnit = false;
    bIsPlacingBuilding = false;
    bIsRemoving = false;
    PendingUnitType = EUnitType::Barbarian;
    PendingBuildingType = EBuildingType::None;
}

void ARTSPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (IsLocalPlayerController())
    {
        // 获取当前地图名字
        FString MapName = GetWorld()->GetMapName();
        // 去掉前缀 (UE4 Map名通常带 UEDPIE_0_ 前缀)
        MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);

        UUserWidget* HUDToCreate = nullptr;

        // --- 逻辑判断：在哪张图？ ---
        if (MapName.Contains("BattleField")) // 如果是战斗地图
        {
            if (BattleHUDClass)
            {
                HUDToCreate = CreateWidget<UUserWidget>(this, BattleHUDClass);
            }
        }
        else // 默认是基地地图 (PlayerBase)
        {
            if (MainHUDClass)
            {
                HUDToCreate = CreateWidget<UUserWidget>(this, MainHUDClass);
                // 保存到 MainHUDInstance 方便以后调用（如果有需要）
                MainHUDInstance = Cast<URTSMainHUD>(HUDToCreate);
            }
        }

        // --- 统一显示 ---
        if (HUDToCreate)
        {
            HUDToCreate->AddToViewport();

            FInputModeGameAndUI InputMode;
            InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            SetInputMode(InputMode);
            bShowMouseCursor = true;
        }
    }
}

void ARTSPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 无论是造兵还是造建筑，都更新幽灵和网格
    if (bIsPlacingUnit || bIsPlacingBuilding)
    {
        UpdatePlacementGhost();

        AGridManager* GridManager = Cast<AGridManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass()));
        if (GridManager)
        {
            int32 HoverX = -1;
            int32 HoverY = -1;
            FHitResult Hit;
            GetHitResultUnderCursor(ECC_Visibility, false, Hit);

            if (Hit.bBlockingHit)
            {
                GridManager->WorldToGrid(Hit.Location, HoverX, HoverY);
            }
            GridManager->DrawGridVisuals(HoverX, HoverY);
        }
    }
}

void ARTSPlayerController::OnSelectUnitToPlace(EUnitType UnitType)
{
    PendingUnitType = UnitType;
    bIsPlacingUnit = true;
    bIsPlacingBuilding = false;

    // --- 检查当前幽灵是否匹配 ---

    // 如果当前有幽灵，但它不是“兵种幽灵”的类型 (比如之前是建筑幽灵)
    if (PreviewGhostActor && PreviewGhostActor->GetClass() != PlacementPreviewClass)
    {
        PreviewGhostActor->Destroy(); // 销毁旧的
        PreviewGhostActor = nullptr;
    }

    // 如果没有幽灵，生成一个新的 (兵种版)
    if (!PreviewGhostActor && PlacementPreviewClass)
    {
        PreviewGhostActor = GetWorld()->SpawnActor<AActor>(PlacementPreviewClass, FVector::ZeroVector, FRotator::ZeroRotator);
    }

    if (PreviewGhostActor)
    {
        PreviewGhostActor->SetActorHiddenInGame(false);
        // 在这里重置一下材质，防止残留红色
        UStaticMeshComponent* Mesh = PreviewGhostActor->FindComponentByClass<UStaticMeshComponent>();
        if(Mesh && ValidPlacementMaterial) Mesh->SetMaterial(0, ValidPlacementMaterial);
    }
}

void ARTSPlayerController::OnSelectBuildingToPlace(EBuildingType BuildingType)
{
    PendingBuildingType = BuildingType;
    bIsPlacingBuilding = true;
    bIsPlacingUnit = false;

    // --- [核心逻辑] 检查当前幽灵是否匹配 ---

    // 如果当前有幽灵，但它不是“建筑幽灵”的类型
    if (PreviewGhostActor && PreviewGhostActor->GetClass() != PlacementPreviewBuildingClass)
    {
        PreviewGhostActor->Destroy(); // 销毁旧的
        PreviewGhostActor = nullptr;
    }

    // 如果没有幽灵，生成一个新的 (建筑版)
    if (!PreviewGhostActor && PlacementPreviewBuildingClass)
    {
        PreviewGhostActor = GetWorld()->SpawnActor<AActor>(PlacementPreviewBuildingClass, FVector::ZeroVector, FRotator::ZeroRotator);
    }

    if (PreviewGhostActor)
    {
        PreviewGhostActor->SetActorHiddenInGame(false);
    }
}

void ARTSPlayerController::OnSelectRemoveMode()
{
    bIsRemoving = true;
    bIsPlacingUnit = false;
    bIsPlacingBuilding = false;

    if (PreviewGhostActor) PreviewGhostActor->SetActorHiddenInGame(true);

    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Orange, TEXT("Remove Mode Active!"));
}

bool ARTSPlayerController::CancelCurrentAction()
{
    // 检查是否有正在进行的状态
    if (bIsPlacingUnit || bIsPlacingBuilding || bIsRemoving)
    {
        // 1. 重置所有状态标志
        bIsPlacingUnit = false;
        bIsPlacingBuilding = false;
        bIsRemoving = false;

        // 2. 隐藏幽灵
        if (PreviewGhostActor)
        {
            PreviewGhostActor->SetActorHiddenInGame(true);
        }

        // 3. 打印提示
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Orange, TEXT("Action Cancelled"));

        return true; // 告诉调用者：我干活了
    }

    return false; // 告诉调用者：我没事干
}

void ARTSPlayerController::HandlePlacementMode(const FHitResult& Hit, AGridManager* GridManager)
{
    int32 X, Y;
    // 检查是否点在有效网格内
    if (!GridManager->WorldToGrid(Hit.Location, X, Y)) return;

    ARTSGameMode* GM = Cast<ARTSGameMode>(GetWorld()->GetAuthGameMode());
    if (!GM) return;

    bool bSuccess = false;

    // 分流：造兵 vs 造建筑
    if (bIsPlacingUnit)
    {
        // 玩家只能在半区 (X < 8) 放兵
        if (X >= 8)
        {
            if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Must place in Deployment Zone (X < 8)!"));
            return; // 直接返回，不执行购买
        }

        // 价格表 (建议后续移至 DataTable)
        int32 Cost = 50;
        if (PendingUnitType == EUnitType::Archer) Cost = 100;
        else if (PendingUnitType == EUnitType::Giant) Cost = 300;
        else if (PendingUnitType == EUnitType::Bomber) Cost = 150;

        bSuccess = GM->TryBuyUnit(PendingUnitType, Cost, X, Y);

        // 成功后退出状态
        if (bSuccess) bIsPlacingUnit = false;
    }
    else if (bIsPlacingBuilding)
    {
        // 玩家只能在半区 (X < 8) 放
        if (X >= 8)
        {
            if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Must place in Deployment Zone (X < 8)!"));
            return; // 直接返回，不执行购买
        }

        int32 Cost = 200;
        if (PendingBuildingType == EBuildingType::GoldMine) Cost = 150;
        else if (PendingBuildingType == EBuildingType::ElixirPump) Cost = 150;
        else if (PendingBuildingType == EBuildingType::Wall) Cost = 50;

        bSuccess = GM->TryBuildBuilding(PendingBuildingType, Cost, X, Y);

        if (bSuccess) bIsPlacingBuilding = false;
    }

    // 统一处理反馈
    if (bSuccess)
    {
        if (PreviewGhostActor) PreviewGhostActor->SetActorHiddenInGame(true);
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("Placed Successfully!"));
    }
    else
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Cannot Place Here!"));
    }
}

void ARTSPlayerController::HandleRemoveMode(AActor* HitActor, AGridManager* GridManager)
{
    // 尝试转换为游戏实体
    ABaseGameEntity* Entity = Cast<ABaseGameEntity>(HitActor);

    // 只能删玩家自己的东西
    if (Entity && Entity->TeamID == ETeam::Player)
    {
        // 解锁网格：建筑
        ABaseBuilding* Building = Cast<ABaseBuilding>(HitActor);
        if (Building && GridManager)
        {
            GridManager->SetTileBlocked(Building->GridX, Building->GridY, false);
        }

        // 士兵
        ABaseUnit* Unit = Cast<ABaseUnit>(HitActor);
        if (Unit && GridManager)
        {
            // 解锁网格：士兵
            int32 UX, UY;
            if (GridManager->WorldToGrid(Unit->GetActorLocation(), UX, UY))
            {
                GridManager->SetTileBlocked(UX, UY, false);
            }

            // 返还人口 (固定 -1)
            URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());
            if (GI)
            {
                // 只要删了一个兵，人口就减 1，最小减到 0
                GI->CurrentPopulation = FMath::Max(0, GI->CurrentPopulation - 1);

                UE_LOG(LogTemp, Log, TEXT("Unit Removed. Population -1. Current: %d/%d"),
                    GI->CurrentPopulation, GI->MaxPopulation);
            }
        }

        // 销毁
        Entity->Destroy();

        // 退出移除模式
        bIsRemoving = false;

        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Entity Removed"));
    }
}

void ARTSPlayerController::HandleNormalMode(AActor* HitActor)
{
    // 尝试点击资源建筑
    ABuilding_Resource* ResBuilding = Cast<ABuilding_Resource>(HitActor);
    if (ResBuilding)
    {
        float Amount = ResBuilding->CollectResource();
        if (Amount > 0)
        {
            URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());
            if (GI)
            {
                if (ResBuilding->bProducesGold)
                    GI->PlayerGold += Amount;
                else
                    GI->PlayerElixir += Amount;
            }
        }
    }

    // (未来可以在这里扩展：点击士兵显示血条信息等)
}

void ARTSPlayerController::HandleLeftClick()
{
    // 1. 获取射线检测结果
    FHitResult Hit;
    GetHitResultUnderCursor(ECC_Visibility, false, Hit);

    // 2. 获取常用引用
    AGridManager* GridManager = Cast<AGridManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass()));
    AActor* HitActor = Hit.GetActor();

    // 3. 模式分发
    if (bIsPlacingUnit || bIsPlacingBuilding)
    {
        // 放置模式：必须点到网格上才处理
        if (Hit.bBlockingHit && GridManager)
        {
            HandlePlacementMode(Hit, GridManager);
        }
    }
    else if (bIsRemoving)
    {
        // 移除模式：必须点到物体才处理
        if (HitActor)
        {
            HandleRemoveMode(HitActor, GridManager);
        }
    }
    else
    {
        // 普通模式：收集资源或选中
        if (HitActor)
        {
            HandleNormalMode(HitActor);
        }
    }
}

void ARTSPlayerController::UpdatePlacementGhost()
{
    if (!PreviewGhostActor) return;

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(PreviewGhostActor);
    QueryParams.AddIgnoredActor(this);

    FHitResult Hit;
    FVector WorldLoc, WorldDir;
    DeprojectMousePositionToWorld(WorldLoc, WorldDir);
    GetWorld()->LineTraceSingleByChannel(Hit, WorldLoc, WorldLoc + WorldDir * 10000.0f, ECC_Visibility, QueryParams);

    if (Hit.bBlockingHit)
    {
        AGridManager* GridManager = Cast<AGridManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass()));
        if (GridManager)
        {
            int32 X, Y;
            if (GridManager->WorldToGrid(Hit.Location, X, Y))
            {
                FVector SnapPos = GridManager->GridToWorld(X, Y);

                // 智能计算 Z 轴高度
                FVector Origin, BoxExtent;
                PreviewGhostActor->GetActorBounds(false, Origin, BoxExtent);
                float HoverHeight = BoxExtent.Z + 2.0f;
                SnapPos.Z += HoverHeight;

                PreviewGhostActor->SetActorLocation(SnapPos);


                // 材质切换逻辑

                // 1. 基础检查：格子是否被阻挡
                bool bTileWalkable = GridManager->IsTileWalkable(X, Y);

                // 2. 区域检查：是否在 X < 8 区域 (只针对造兵)
                bool bInValidZone = true;
                if (bIsPlacingUnit || bIsPlacingBuilding)
                {
                    if (X >= 8)
                    {
                        bInValidZone = false;
                    }
                }

                // 3. 综合判断：必须既可走，又在有效区域内，才是“绿色”
                bool bCanPlace = bTileWalkable && bInValidZone;

                // 4. 应用材质
                UStaticMeshComponent* MeshComp = PreviewGhostActor->FindComponentByClass<UStaticMeshComponent>();
                if (MeshComp)
                {
                    if (bCanPlace)
                    {
                        // 绿色材质
                        if (ValidPlacementMaterial)
                        {
                            MeshComp->SetMaterial(0, ValidPlacementMaterial);
                        }
                    }
                    else
                    {
                        // 红色材质
                        if (InvalidPlacementMaterial)
                        {
                            MeshComp->SetMaterial(0, InvalidPlacementMaterial);
                        }
                    }
                }
            }
        }
    }
}

void ARTSPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindAction("LeftClick", IE_Pressed, this, &ARTSPlayerController::HandleLeftClick);
}