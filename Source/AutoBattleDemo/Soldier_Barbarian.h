// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseUnit.h"
#include "Soldier_Barbarian.generated.h"

/**
 * 野蛮人 - 默认近战单位
 * 特点：高血量，近战攻击，找最近的建筑打
 */
UCLASS()
class AUTOBATTLEDEMO_API ASoldier_Barbarian : public ABaseUnit
{
	GENERATED_BODY()
	
public:
	ASoldier_Barbarian();

	virtual void BeginPlay() override;
};
