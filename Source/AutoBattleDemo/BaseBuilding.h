// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseBuilding.generated.h"

UCLASS()
class AUTOBATTLEDEMO_API ABaseBuilding : public AActor
{
	GENERATED_BODY()

public:
	// 构造函数
	ABaseBuilding();

	// 覆盖引擎自带的点击事件处理函数
	virtual void NotifyActorOnClicked(FKey ButtonPressed = EKeys::LeftMouseButton) override;

protected:
	// 游戏开始时调用（你之前报错是因为缺了这一行声明）
	virtual void BeginPlay() override;

public:
	// 每帧调用
	virtual void Tick(float DeltaTime) override;

	// --- 核心变量声明 ---

	// 可视化组件（模型）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		class UStaticMeshComponent* MeshComp;

	// 血量
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
		float MaxHealth;
};
