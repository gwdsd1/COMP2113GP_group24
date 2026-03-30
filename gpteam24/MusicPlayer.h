#pragma once
#include <string>
#include <algorithm>

#if defined(_WIN32)
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

class MusicPlayer {
private:
    std::string currentAlias;
    bool isPlaying = false;
    
    std::string getFileExtension(const std::string& filepath) {
        size_t pos = filepath.find_last_of('.');
        if (pos == std::string::npos) return "";
        std::string ext = filepath.substr(pos + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext;
    }
    
public:
    bool load(const std::string& filepath) {
        // 停止当前播放
        stop();
        
        // 使用时间戳作为唯一别名
        currentAlias = "music_" + std::to_string(GetTickCount64());
        
        // 根据文件扩展名选择合适的 type 参数
        std::string ext = getFileExtension(filepath);
        std::string typeParam;
        
        if (ext == "wav") {
            typeParam = " type waveaudio";
        } else if (ext == "mp3") {
            typeParam = " type mpegvideo";
        } else if (ext == "mid" || ext == "midi") {
            typeParam = " type sequencer";
        } else {
            // 无法识别的格式，尝试不指定 type 让系统自动检测
            typeParam = "";
        }
        
        // 打开音频文件
        std::string cmd = "open \"" + filepath + "\"" + typeParam + " alias " + currentAlias;
        MCIERROR err = mciSendStringA(cmd.c_str(), NULL, 0, NULL);
        
        if (err != 0) {
            currentAlias.clear();
            return false;
        }
        
        return true;
    }
    
    bool play() {
        if (currentAlias.empty()) return false;
        
        std::string cmd = "play " + currentAlias + " from 0";
        MCIERROR err = mciSendStringA(cmd.c_str(), NULL, 0, NULL);
        isPlaying = (err == 0);
        return isPlaying;
    }
    
    void pause() {
        if (currentAlias.empty()) return;
        mciSendStringA(("pause " + currentAlias).c_str(), NULL, 0, NULL);
        isPlaying = false;
    }
    
    void stop() {
        if (currentAlias.empty()) return;
        mciSendStringA(("stop " + currentAlias).c_str(), NULL, 0, NULL);
        mciSendStringA(("close " + currentAlias).c_str(), NULL, 0, NULL);
        currentAlias.clear();
        isPlaying = false;
    }
    
    void setVolume(int volume) {
        // volume: 0-1000
        if (currentAlias.empty()) return;
        std::string cmd = "setaudio " + currentAlias + " volume to " + std::to_string(volume);
        mciSendStringA(cmd.c_str(), NULL, 0, NULL);
    }
    
    bool getIsPlaying() const {
        return isPlaying;
    }
    
    long getPosition() {
        if (currentAlias.empty()) return 0;

        char buffer[128];
        std::string cmd = "status " + currentAlias + " position";
        mciSendStringA(cmd.c_str(), buffer, sizeof(buffer), NULL);

        return atol(buffer);
    }

    bool isFinished() {
        if (currentAlias.empty()) return true;

        char buffer[128] = {};
        std::string cmd = "status " + currentAlias + " mode";
        mciSendStringA(cmd.c_str(), buffer, sizeof(buffer), NULL);

        // MCI returns "stopped" when playback reaches the end
        return (std::string(buffer) == "stopped");
    }

    ~MusicPlayer() {
        stop();
    }
};

#else
class MusicPlayer {
public:
    bool load(const std::string& filepath) { return false; }
    bool play() { return false; }
    void pause() {}
    void stop() {}
    void setVolume(int volume) {}
    bool getIsPlaying() const { return false; }
    long getPosition() { return 0; }
    bool isFinished() { return true; }
};
#endif
