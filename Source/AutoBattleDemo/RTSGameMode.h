// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h" // 必须包含这个
#include "RTSGameMode.generated.h"

UCLASS()
class AUTOBATTLEDEMO_API ARTSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	// 声明构造函数
	ARTSGameMode();
};