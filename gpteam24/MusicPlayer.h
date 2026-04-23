#pragma once
#include <string>

// Include miniaudio header (MINIAUDIO_IMPLEMENTATION must be defined in one .cpp file).
#include "miniaudio.h"

class MusicPlayer {
private:
    ma_engine engine;
    ma_sound sound;
    bool loaded;
    bool isPlayingState;  // Tracks current playing state.

public:
    // What it does: Constructs the player and initializes the miniaudio engine.
    // Inputs: None.
    // Outputs: Constructs a ready-to-use MusicPlayer instance.
    MusicPlayer() : loaded(false), isPlayingState(false) {
        ma_engine_init(NULL, &engine);
    }

    // What it does: Stops playback and releases audio resources.
    // Inputs: None.
    // Outputs: Destroys the instance and uninitializes miniaudio objects.
    ~MusicPlayer() {
        stop();
        if (loaded)
            ma_sound_uninit(&sound);
        ma_engine_uninit(&engine);
    }

    // What it does: Loads an audio file into the internal sound object.
    // Inputs: filepath is the audio file path.
    // Outputs: Returns true if loading succeeds, otherwise false.
    bool load(const std::string& filepath) {
        stop(); // Stop previously playing audio.
        if (loaded)
            ma_sound_uninit(&sound);

        // Use decode flag so playback time can be read accurately.
        if (ma_sound_init_from_file(&engine, filepath.c_str(),
            MA_SOUND_FLAG_DECODE, NULL, NULL, &sound) != MA_SUCCESS) {
            loaded = false;
            return false;
        }

        // Enable looping for background music behavior.
        ma_sound_set_looping(&sound, MA_TRUE);

        loaded = true;
        isPlayingState = false;
        return true;
    }

    // What it does: Starts playback of the loaded audio.
    // Inputs: None.
    // Outputs: Returns true on successful start, otherwise false.
    bool play() {
        if (!loaded) return false;

        if (ma_sound_start(&sound) != MA_SUCCESS) {
            isPlayingState = false;
            return false;
        }

        isPlayingState = true;
        return true;
    }

    // What it does: Pauses current playback without unloading audio.
    // Inputs: None.
    // Outputs: None.
    void pause() {
        if (loaded && isPlayingState) {
            ma_sound_stop(&sound);
            isPlayingState = false;
        }
    }

    // What it does: Stops playback and rewinds to the beginning.
    // Inputs: None.
    // Outputs: None.
    void stop() {
        if (loaded) {
            ma_sound_stop(&sound);
            ma_sound_seek_to_pcm_frame(&sound, 0);
            isPlayingState = false;
        }
    }

    // What it does: Sets playback volume.
    // Inputs: volume in the range 0-1000.
    // Outputs: None.
    void setVolume(int volume) {
        // volume: 0-1000 mapped to 0.0-1.0.
        if (loaded) ma_sound_set_volume(&sound, volume / 1000.0f);
    }

    // What it does: Reports whether audio is currently playing.
    // Inputs: None.
    // Outputs: Returns true if loaded and playing, otherwise false.
    bool getIsPlaying() const {
        return loaded && isPlayingState;
    }

    // What it does: Gets the current playback position in milliseconds.
    // Inputs: None.
    // Outputs: Returns playback position in milliseconds, or 0 if no audio is loaded.
    long getPosition() {
        if (!loaded) return 0;
        return (long)ma_sound_get_time_in_milliseconds(&sound);
    }

    // What it does: Reports whether playback is finished according to this project's logic.
    // Inputs: None.
    // Outputs: Returns true when not playing, otherwise false.
    bool isFinished() {
        // Looping is enabled for background music, so it does not end automatically.
        // Playback stops only when stop() or pause() is called.
        return !isPlayingState;
    }

    // Disable copy to avoid sharing one audio engine between multiple objects.
    MusicPlayer(const MusicPlayer&) = delete;
    MusicPlayer& operator=(const MusicPlayer&) = delete;
};

