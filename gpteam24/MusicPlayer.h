#pragma once
#include <string>

// 包含 miniaudio 头文件（确保在某个 .cpp 中定义了 MINIAUDIO_IMPLEMENTATION）
#include "miniaudio.h"

class MusicPlayer {
private:
    ma_engine engine;
    ma_sound sound;
    bool loaded;
    bool isPlayingState;  // 记录播放状态

public:
    MusicPlayer() : loaded(false), isPlayingState(false) {
        ma_engine_init(NULL, &engine);
    }

    ~MusicPlayer() {
        stop();
        if (loaded)
            ma_sound_uninit(&sound);
        ma_engine_uninit(&engine);
    }

    bool load(const std::string& filepath) {
        stop(); // 停止之前播放
        if (loaded)
            ma_sound_uninit(&sound);

        // 使用 MA_SOUND_FLAG_DECODE 以便获取准确的播放时间
        if (ma_sound_init_from_file(&engine, filepath.c_str(),
            MA_SOUND_FLAG_DECODE, NULL, NULL, &sound) != MA_SUCCESS) {
            loaded = false;
            return false;
        }

        // 设置循环播放（背景音乐）
        ma_sound_set_looping(&sound, MA_TRUE);

        loaded = true;
        isPlayingState = false;
        return true;
    }

    bool play() {
        if (!loaded) return false;

        if (ma_sound_start(&sound) != MA_SUCCESS) {
            isPlayingState = false;
            return false;
        }

        isPlayingState = true;
        return true;
    }

    void pause() {
        if (loaded && isPlayingState) {
            ma_sound_stop(&sound);
            isPlayingState = false;
        }
    }

    void stop() {
        if (loaded) {
            ma_sound_stop(&sound);
            ma_sound_seek_to_pcm_frame(&sound, 0);
            isPlayingState = false;
        }
    }

    void setVolume(int volume) {
        // volume: 0-1000 映射到 0.0 - 1.0 (保持与原 MCI 版本一致)
        if (loaded) ma_sound_set_volume(&sound, volume / 1000.0f);
    }

    bool getIsPlaying() const {
        return loaded && isPlayingState;
    }

    long getPosition() {
        if (!loaded) return 0;
        return (long)ma_sound_get_time_in_milliseconds(&sound);
    }

    bool isFinished() {
        // 因为设置了循环播放，所以背景音乐永远不会自动结束
        // 只有当用户切换页面（调用stop）时才会停止
        return !isPlayingState;
    }

    // 禁止拷贝（避免多个对象共享同一个音频引擎）
    MusicPlayer(const MusicPlayer&) = delete;
    MusicPlayer& operator=(const MusicPlayer&) = delete;
};
