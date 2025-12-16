#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "RTSCameraPawn.generated.h"

UCLASS()
class AUTOBATTLEDEMO_API ARTSCameraPawn : public APawn
{
    GENERATED_BODY()

public:
    ARTSCameraPawn();
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
    // --- 组件 ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
        class USceneComponent* SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
        class USpringArmComponent* SpringArmComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
        class UCameraComponent* CameraComp;

    // --- 移动与缩放参数 ---
    UPROPERTY(EditAnywhere, Category = "RTS Movement")
        float MoveSpeed;

    UPROPERTY(EditAnywhere, Category = "RTS Movement")
        float ZoomSpeed;

    UPROPERTY(EditAnywhere, Category = "RTS Movement")
        float MinZoom; // 最近距离

    UPROPERTY(EditAnywhere, Category = "RTS Movement")
        float MaxZoom; // 最远距离

        // --- 内部状态 ---
    float TargetZoom; // 目标臂长（用于平滑缩放）

    // --- 输入处理函数 ---
    void MoveForward(float Value);
    void MoveRight(float Value);
    void ZoomIn();
    void ZoomOut();

    // --- 拖拽移动参数 ---
    UPROPERTY(EditAnywhere, Category = "RTS Movement")
        float DragSpeed; // 拖拽灵敏度

    // 记录右键是否按下
    bool bIsRMBDown;

    // --- 函数声明 ---
    void OnRightClickDown();
    void OnRightClickUp();
    void HandleMouseDragX(float Value); // 处理鼠标左右移
    void HandleMouseDragY(float Value); // 处理鼠标上下移
};