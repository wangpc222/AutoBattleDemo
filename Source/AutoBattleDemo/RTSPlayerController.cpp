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
#include "Building_Barracks.h" // 引用兵营
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

    SelectedBuilding = nullptr; // 初始化
    SelectedUnit = nullptr;     // 初始化
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
    // 先问问裁判，我有资格造这个兵吗？
    ARTSGameMode* GM = Cast<ARTSGameMode>(GetWorld()->GetAuthGameMode());
    if (GM)
    {
        if (!GM->CheckUnitTechRequirement(UnitType))
        {
            // 如果不满足要求，直接退出，不进入放置模式
            // CheckUnitTechRequirement 内部已经打印了红字提示
            return;
        }
    }

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
        // 重置所有状态标志
        bIsPlacingUnit = false;
        bIsPlacingBuilding = false;
        bIsRemoving = false;

        // 隐藏幽灵
        if (PreviewGhostActor)
        {
            PreviewGhostActor->SetActorHiddenInGame(true);
        }

        // 如果是在移动单位，取消时要还原
        if (UnitBeingMoved)
        {
            // 1. 重新显示
            UnitBeingMoved->SetActorHiddenInGame(false);
            UnitBeingMoved->SetActorEnableCollision(true);

            // 2. 重新锁定格子
            AGridManager* GridManager = Cast<AGridManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass()));
            if (GridManager)
            {
                int32 X, Y;
                if (GridManager->WorldToGrid(UnitBeingMoved->GetActorLocation(), X, Y))
                {
                    GridManager->SetTileBlocked(X, Y, true);
                }
            }

            // 3. 清空引用
            UnitBeingMoved = nullptr;
            if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Orange, TEXT("Move Cancelled - Unit Restored"));
        }

        // 打印提示
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Orange, TEXT("Action Cancelled"));

        return true; // 告诉调用者：我干活了
    }

    return false; // 告诉调用者：我没事干
}

void ARTSPlayerController::HandlePlacementMode(const FHitResult& Hit, AGridManager* GridManager)
{
    // 特殊检查：在放置模式下，如果点到了兵营，且正在移动兵
    AActor* HitActor = Hit.GetActor();
    if (UnitBeingMoved && bIsPlacingUnit)
    {
        ABuilding_Barracks* Barracks = Cast<ABuilding_Barracks>(HitActor);
        if (Barracks && Barracks->TeamID == ETeam::Player)
        {
            // 直接存入兵营
            Barracks->StoreUnit(UnitBeingMoved);

            // 清理状态
            UnitBeingMoved = nullptr; // 已经交给兵营销毁了，指针置空
            bIsPlacingUnit = false;
            if (PreviewGhostActor) PreviewGhostActor->SetActorHiddenInGame(true);

            if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("Moved into Barracks!"));
            return; // 结束逻辑
        }
    }

    int32 X, Y;
    // 检查是否点在有效网格内
    if (!GridManager->WorldToGrid(Hit.Location, X, Y)) return;

    ARTSGameMode* GM = Cast<ARTSGameMode>(GetWorld()->GetAuthGameMode());
    if (!GM) return;

    bool bSuccess = false;

    // 分流：造兵 vs 造建筑
    if (bIsPlacingUnit)
    {
        // 1. 区域限制 (无论是买还是移动，都不能去敌区)
        if (X >= 8)
        {
            if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Must place in Deployment Zone (X < 8)!"));
            return;
        }

        // 2. 判断是【移动旧兵】还是【购买新兵】
        if (UnitBeingMoved)
        {
            // 移动模式 (Move)
            // 检查格子是否可走
            if (GridManager->IsTileWalkable(X, Y))
            {
                // 使用 SpawnUnitAt (只生成，不扣钱，但内部会自动 +1 人口)
                bSuccess = GM->SpawnUnitAt(PendingUnitType, X, Y);

                if (bSuccess)
                {
                    // 移动不应该增加总人口
                    // SpawnUnitAt 加了 1，所以我们要手动减 1，保持总量不变
                    /*URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());
                    if (GI)
                    {
                        GI->CurrentPopulation = FMath::Max(0, GI->CurrentPopulation - 1);
                    }*/

                    // 销毁旧的兵
                    UnitBeingMoved->Destroy();
                    UnitBeingMoved = nullptr; // 指针置空
                }
            }
        }
        else
        {
            // 购买模式 (Buy)
            int32 Cost = 50;
            if (PendingUnitType == EUnitType::Archer) Cost = 100;
            else if (PendingUnitType == EUnitType::Giant) Cost = 300;
            else if (PendingUnitType == EUnitType::Bomber) Cost = 150;

            // 使用 TryBuyUnit (扣钱，加人口)
            bSuccess = GM->TryBuyUnit(PendingUnitType, Cost, X, Y);
        }

        // 无论哪种情况，成功后退出放置模式
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
        else if (PendingBuildingType == EBuildingType::Barracks) Cost = 300;

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
        // [新增] 兵营移除限制检查
        ABuilding_Barracks* Barracks = Cast<ABuilding_Barracks>(HitActor);
        if (Barracks)
        {
            URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());
            if (GI)
            {
                // 1. 获取这个兵营提供的人口
                int32 PopBonus = Barracks->GetCurrentCapacity(Barracks->BuildingLevel);

                // 2. 计算移除后的人口上限
                int32 FutureMaxPop = GI->MaxPopulation - PopBonus;

                // 3. 如果移除后，上限 < 当前人口，禁止移除！
                if (FutureMaxPop < GI->CurrentPopulation)
                {
                    UE_LOG(LogTemp, Warning, TEXT("Cannot remove Barracks! Population would overflow (%d > %d)"),
                        GI->CurrentPopulation, FutureMaxPop);

                    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Cannot Remove! Population limit too low!"));

                    return; // 直接返回，不执行后面的销毁逻辑
                }
            }
        }

        // 检查是否是 HQ
        ABaseBuilding* Building = Cast<ABaseBuilding>(HitActor);
        if (Building)
        {
            if (Building->BuildingType == EBuildingType::Headquarters)
            {
                if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Cannot Remove HQ!"));
                return; // 直接返回，不执行销毁
            }
        }

        // 解锁网格：建筑
        if (Building && GridManager)
        {
            GridManager->SetTileBlocked(Building->GridX, Building->GridY, false);
        }

        // 士兵
        ABaseUnit* Unit = Cast<ABaseUnit>(HitActor);
        if (Unit && GridManager)
        {
            // 解锁网格：士兵
            /*int32 UX, UY;
            if (GridManager->WorldToGrid(Unit->GetActorLocation(), UX, UY))
            {
                GridManager->SetTileBlocked(UX, UY, false);
            }*/

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

void ARTSPlayerController::StartRepositioningSelectedUnit()
{
    if (!SelectedUnit) return;
    if (SelectedUnit->IsPendingKill()) return;

    // 1. 记录正在移动的兵
    UnitBeingMoved = SelectedUnit;

    // 2. 隐藏它 (假装拿起来了)
    UnitBeingMoved->SetActorHiddenInGame(true);
    UnitBeingMoved->SetActorEnableCollision(false); // 关闭碰撞，防止射线打到它自己

    // 3. 解锁它原来的格子
    /*AGridManager* GridManager = Cast<AGridManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridManager::StaticClass()));
    if (GridManager)
    {
        int32 X, Y;
        if (GridManager->WorldToGrid(UnitBeingMoved->GetActorLocation(), X, Y))
        {
            GridManager->SetTileBlocked(X, Y, false);
        }
    }*/

    // 4. 进入放置模式 (复用造兵逻辑)
    // 注意：这里不用扣钱，因为我们只是移动
    OnSelectUnitToPlace(UnitBeingMoved->UnitType);

    // 清空选中状态
    SelectedUnit = nullptr;

    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("Repositioning Unit..."));
}

void ARTSPlayerController::HandleNormalMode(AActor* HitActor)
{
    // 1. 尝试点击资源建筑 (收集资源) - 保持原样
    ABuilding_Resource* ResBuilding = Cast<ABuilding_Resource>(HitActor);
    if (ResBuilding)
    {
        float Amount = ResBuilding->CollectResource();
        if (Amount > 0)
        {
            URTSGameInstance* GI = Cast<URTSGameInstance>(GetGameInstance());
            if (GI)
            {
                if (ResBuilding->bProducesGold) GI->PlayerGold += Amount;
                else GI->PlayerElixir += Amount;
            }
            // return; // 如果收集了资源，就不算选中操作了(可选)
        }
    }

    // 2. 选中逻辑
    // 如果点击的是一个建筑 (且属于玩家)
    ABaseBuilding* ClickedBuilding = Cast<ABaseBuilding>(HitActor);
    if (ClickedBuilding && ClickedBuilding->TeamID == ETeam::Player)
    {
        SelectedBuilding = ClickedBuilding;

        // 打印日志
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan,
            FString::Printf(TEXT("Selected: %s (Lv.%d)"), *ClickedBuilding->GetName(), ClickedBuilding->BuildingLevel));
    }
    else
    {
        // 点击了地板或敌人 -> 取消选中
        SelectedBuilding = nullptr;
    }

    // 尝试点击兵营
    ABuilding_Barracks* Barracks = Cast<ABuilding_Barracks>(HitActor);
    if (Barracks && Barracks->TeamID == ETeam::Player)
    {
        // 场景 A：手里选了个兵 -> 存进去
        if (SelectedUnit)
        {
            // 确保兵活着且没死
            if (SelectedUnit->IsValidLowLevel() && !SelectedUnit->IsPendingKill())
            {
                Barracks->StoreUnit(SelectedUnit);
                SelectedUnit = nullptr; // 存完了，清空选中
                if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Unit Stored!"));
                return;
            }
        }
        // 场景 B：手里没兵 -> 把兵营里的兵放出来
        else
        {
            Barracks->ReleaseAllUnits();
            if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Units Released!"));
            return;
        }
    }

    // 尝试点击士兵 (选中)
    ABaseUnit* ClickedUnit = Cast<ABaseUnit>(HitActor);
    if (ClickedUnit && ClickedUnit->TeamID == ETeam::Player)
    {
        // 1. 选中它 (任何时候都可以选中查看属性)
        SelectedUnit = ClickedUnit;

        // 2. 获取 GameMode 检查状态
        ARTSGameMode* GM = Cast<ARTSGameMode>(GetWorld()->GetAuthGameMode());

        if (GM && GM->GetCurrentState() == EGameState::Preparation)
        {
            // 只有在【备战阶段】(基地)，才能拿起并移动
            StartRepositioningSelectedUnit();
            if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("Unit Picked Up!"));
        }
        else
        {
            // 在【战斗阶段】，不能移动，只能看着
            if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("Cannot move units during battle!"));
        }

        return;
    }

    // 3. 点击空地 -> 取消选中
    SelectedUnit = nullptr;
}

// 请求升级
void ARTSPlayerController::RequestUpgradeSelectedBuilding()
{
    if (SelectedBuilding)
    {
        ARTSGameMode* GM = Cast<ARTSGameMode>(GetWorld()->GetAuthGameMode());
        if (GM)
        {
            GM->TryUpgradeBuilding(SelectedBuilding);
        }
    }
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

                // 2. 区域检查：是否在 X < 8 区域
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

    // 绑定 Esc
    InputComponent->BindAction("PauseMenu", IE_Pressed, this, &ARTSPlayerController::OnPressEsc);
}

void ARTSPlayerController::OnPressEsc()
{
    // 1. 优先尝试取消当前操作 (造兵/移除/选中)
    // 如果 CancelCurrentAction 返回 true，说明刚才处于放置模式，现在取消了，逻辑结束
    if (CancelCurrentAction())
    {
        return;
    }

    // 2. 还有选中单位吗？如果有，取消选中
    if (SelectedUnit || SelectedBuilding)
    {
        SelectedUnit = nullptr;
        SelectedBuilding = nullptr;
        // 顺便通知 UI 隐藏升级按钮 (Tick 里会自动处理，或者你可以在这里发广播)
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, TEXT("Selection Cleared"));
        return;
    }

    // 3. 根据地图决定去向
    FString MapName = GetWorld()->GetMapName();
    ARTSGameMode* GM = Cast<ARTSGameMode>(GetWorld()->GetAuthGameMode());

    // 如果在战场 -> 执行撤退 (这会保存幸存者，然后回基地)
    if (MapName.Contains("BattleField") && GM)
    {
        GM->ReturnToBase();
    }
    // 如果在基地 -> 回主菜单 (记得保存基地布局)
    else
    {
        if (GM) GM->SaveBaseLayout(); // 顺手保存一下家
        UGameplayStatics::OpenLevel(this, FName("MainMenu"));
    }
}