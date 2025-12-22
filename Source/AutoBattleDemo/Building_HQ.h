#pragma once
#include "CoreMinimal.h"
#include "BaseBuilding.h"
#include "Building_HQ.generated.h"

UCLASS()
class AUTOBATTLEDEMO_API ABuilding_HQ : public ABaseBuilding
{
    GENERATED_BODY()
public:
    ABuilding_HQ();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};