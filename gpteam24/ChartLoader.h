#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>

// 曲谱中的单个音符定义
struct ChartNote {
    double time;  // 出现时间（秒）
    int lane;     // 轨道 (0-5)
};

// 曲谱数据
struct ChartData {
    std::string title = "Untitled";
    double bpm = 120.0;
    double offset = 0.0;  // 音乐偏移（秒）
    std::vector<ChartNote> notes;
    
    bool loadFromFile(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            return false;
        }
        
        notes.clear();
        std::string line;
        
        while (std::getline(file, line)) {
            // 移除前后空格
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);
            
            // 跳过空行
            if (line.empty()) continue;
            
            // 解析注释行（元数据）
            if (line[0] == '#') {
                if (line.find("# Song:") == 0) {
                    title = line.substr(7);
                    // 移除前导空格
                    title.erase(0, title.find_first_not_of(" \t"));
                }
                else if (line.find("# BPM:") == 0) {
                    bpm = std::stod(line.substr(6));
                }
                else if (line.find("# Offset:") == 0) {
                    offset = std::stod(line.substr(9));
                }
                continue;
            }
            
            // 解析音符数据: time, lane
            std::stringstream ss(line);
            std::string timeStr, laneStr;
            
            if (std::getline(ss, timeStr, ',') && std::getline(ss, laneStr, ',')) {
                try {
                    ChartNote note;
                    note.time = std::stod(timeStr);
                    note.lane = std::stoi(laneStr);
                    
                    // 验证轨道有效性
                    if (note.lane >= 0 && note.lane < 6) {
                        notes.push_back(note);
                    }
                } catch (...) {
                    // 忽略解析错误的行
                }
            }
        }
        
        // 按时间排序
        std::sort(notes.begin(), notes.end(), 
            [](const ChartNote& a, const ChartNote& b) {
                return a.time < b.time;
            });
        
        file.close();
        return !notes.empty();
    }
    
    bool saveToFile(const std::string& filepath) const {
        std::ofstream file(filepath);
        if (!file.is_open()) {
            return false;
        }
        
        file << "# Song: " << title << "\n";
        file << "# BPM: " << bpm << "\n";
        file << "# Offset: " << offset << "\n";
        file << "\n";
        file << "# Format: time(seconds), lane(0-5)\n";
        
        for (const auto& note : notes) {
            file << note.time << ", " << note.lane << "\n";
        }
        
        file.close();
        return true;
    }
};
