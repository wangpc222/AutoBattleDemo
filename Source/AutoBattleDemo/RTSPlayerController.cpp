// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSPlayerController.h"

ARTSPlayerController::ARTSPlayerController()
{
    // 1. 显示鼠标光标
    bShowMouseCursor = true;

    // 2. 开启点击事件
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
}