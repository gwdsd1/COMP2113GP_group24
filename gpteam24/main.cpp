#include "game.h"
#include "MusicManager.h"

int main() {
    // 初始化音频系统
    MusicManager::initialize();

    // 显示主菜单
    showMainMenu();

    // 清理音频资源
    MusicManager::cleanup();

    return 0;
}