#pragma once
#include "MusicPlayer.h"

class MusicManager {
private:
    static MusicPlayer player;
    static bool initialized;

public:
    // 初始化音频系统
    static void initialize() {
        if (!initialized) {
            // MusicPlayer 构造函数会自动初始化
            initialized = true;
        }
    }

    // 获取 MusicPlayer 引用
    static MusicPlayer& getPlayer() {
        return player;
    }

    // 播放背景音乐（自动停止当前音乐）
    static bool playBackgroundMusic(const std::string& filepath) {
        player.stop();
        if (player.load(filepath)) {
            return player.play();
        }
        return false;
    }

    // 停止音乐
    static void stop() {
        player.stop();
    }

    // 暂停音乐
    static void pause() {
        player.pause();
    }

    // 恢复播放
    static bool resume() {
        return player.play();
    }

    // 设置音量
    static void setVolume(int volume) {
        player.setVolume(volume);
    }

    // 清理资源
    static void cleanup() {
        player.stop();
    }

    // 检查是否在播放
    static bool isPlaying() {
        return player.getIsPlaying();
    }

    // 检查是否播放完成
    static bool isFinished() {
        return player.isFinished();
    }
};
