#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>

// Represents a single note entry in a chart file.
struct ChartNote {
    double time;  // Spawn time in seconds.
    int lane;     // Lane index (0-5).
};

// Stores chart metadata and note sequence.
struct ChartData {
    std::string title = "Untitled";
    double bpm = 120.0;
    double offset = 0.0;  // Audio offset in seconds.
    std::vector<ChartNote> notes;

    // What it does: Loads chart metadata and notes from a text chart file.
    // Inputs: filepath is the chart file path.
    // Outputs: Returns true if loading succeeds and at least one valid note is found; otherwise false.
    bool loadFromFile(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            return false;
        }

        notes.clear();
        std::string line;

        while (std::getline(file, line)) {
            // Trim leading and trailing whitespace.
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);

            // Skip empty lines.
            if (line.empty()) continue;

            // Parse metadata comment lines.
            if (line[0] == '#') {
                if (line.find("# Song:") == 0) {
                    title = line.substr(7);
                    // Remove leading whitespace in title.
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

            // Parse note row: time, lane.
            std::stringstream ss(line);
            std::string timeStr, laneStr;

            if (std::getline(ss, timeStr, ',') && std::getline(ss, laneStr, ',')) {
                try {
                    ChartNote note;
                    note.time = std::stod(timeStr);
                    note.lane = std::stoi(laneStr);

                    // Keep only valid lane values.
                    if (note.lane >= 0 && note.lane < 6) {
                        notes.push_back(note);
                    }
                } catch (...) {
                    // Ignore malformed rows.
                }
            }
        }

        // Sort notes by time.
        std::sort(notes.begin(), notes.end(),
            [](const ChartNote& a, const ChartNote& b) {
                return a.time < b.time;
            });

        file.close();
        return !notes.empty();
    }

    // What it does: Writes chart metadata and note rows to a chart file.
    // Inputs: filepath is the destination file path.
    // Outputs: Returns true if writing succeeds; otherwise false.
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

