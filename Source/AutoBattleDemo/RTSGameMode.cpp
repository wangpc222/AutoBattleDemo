// Fill out your copyright notice in the Description page of Project Settings.



#include "RTSGameMode.h"
#include "RTSPlayerController.h" // 不加这行找不到 RTSPlayerController

ARTSGameMode::ARTSGameMode()
{
    // 指定默认的 PlayerController 为我们写的那个类
    PlayerControllerClass = ARTSPlayerController::StaticClass();
}