#pragma once
#include "MusicPlayer.h"

class MusicManager {
private:
    static MusicPlayer player;
    static bool initialized;

public:
    // What it does: Initializes the shared audio manager state.
    // Inputs: None.
    // Outputs: None.
    static void initialize() {
        if (!initialized) {
            // MusicPlayer constructor initializes the underlying engine.
            initialized = true;
        }
    }

    // What it does: Returns access to the shared MusicPlayer instance.
    // Inputs: None.
    // Outputs: Returns a mutable reference to the global MusicPlayer.
    static MusicPlayer& getPlayer() {
        return player;
    }

    // What it does: Stops current playback, loads a new background track, and starts playback.
    // Inputs: filepath is the audio file path to load.
    // Outputs: Returns true if loading and playback both succeed, otherwise false.
    static bool playBackgroundMusic(const std::string& filepath) {
        player.stop();
        if (player.load(filepath)) {
            return player.play();
        }
        return false;
    }

    // What it does: Stops the current track and resets playback position.
    // Inputs: None.
    // Outputs: None.
    static void stop() {
        player.stop();
    }

    // What it does: Pauses current playback.
    // Inputs: None.
    // Outputs: None.
    static void pause() {
        player.pause();
    }

    // What it does: Resumes playback of the currently loaded track.
    // Inputs: None.
    // Outputs: Returns true if playback starts successfully, otherwise false.
    static bool resume() {
        return player.play();
    }

    // What it does: Sets playback volume.
    // Inputs: volume is expected in the project range used by MusicPlayer.
    // Outputs: None.
    static void setVolume(int volume) {
        player.setVolume(volume);
    }

    // What it does: Stops playback as part of audio cleanup flow.
    // Inputs: None.
    // Outputs: None.
    static void cleanup() {
        player.stop();
    }

    // What it does: Reports whether audio is currently playing.
    // Inputs: None.
    // Outputs: Returns true if playing, otherwise false.
    static bool isPlaying() {
        return player.getIsPlaying();
    }

    // What it does: Reports whether playback is considered finished.
    // Inputs: None.
    // Outputs: Returns true if finished, otherwise false.
    static bool isFinished() {
        return player.isFinished();
    }
};

