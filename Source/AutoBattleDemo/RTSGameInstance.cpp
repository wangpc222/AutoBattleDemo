#include "RTSGameInstance.h"
#include "RTSCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "RTSSaveGame.h"

void URTSGameInstance::StartNewGame()
{
    // 1. 重置所有数据
    ResetData();

    // 2. 强制重置教程状态
    bTutorialFinished = false; // 还没做完教程

    // 3. 初始资源 (给一点钱造矿，不给水逼迫按教程走)
    PlayerGold = 300;
    PlayerElixir = 0;

    // 4. 删除旧存档 (可选，或者覆盖)
    // UGameplayStatics::DeleteGameInSlot("SaveSlot1", 0); 

    // 5. 保存初始状态
    SaveGameToDisk();
}

bool URTSGameInstance::ContinueGame()
{
    if (UGameplayStatics::DoesSaveGameExist("SaveSlot1", 0))
    {
        LoadGameFromDisk();
        return true;
    }
    return false;
}

bool URTSGameInstance::HasSaveGame()
{
    // 这里的 SlotName 必须和你保存时用的一模一样 ("SaveSlot1")
    return UGameplayStatics::DoesSaveGameExist(TEXT("SaveSlot1"), 0);
}