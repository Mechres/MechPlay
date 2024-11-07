#include "raylib.h"
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_SONGS 100
Music musicArray[MAX_SONGS];
char *songFiles[MAX_SONGS];
int songCount = 0;


void LoadMusicFiles(const char *directoryPath) {
    DIR *dir;
    struct dirent *entry;

    dir = opendir(directoryPath);
    if (dir == NULL) {
        printf("Error opening directory %s\n", directoryPath);
        return;
    }
    while ((entry = readdir(dir)) != NULL && songCount < MAX_SONGS) {
        if(strstr(entry->d_name, ".mp3") != NULL) {

            char *filePath = malloc(strlen(entry->d_name) + strlen(directoryPath) + 2);
            sprintf(filePath, "%s%s", directoryPath, entry->d_name);

            // Store file path and load music
            songFiles[songCount] = filePath;
            musicArray[songCount] = LoadMusicStream(filePath);
            songCount++;
        }
    }
    closedir(dir);
}


int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------

    const int screenWidth = 800;
    const int screenHeight = 450;


    InitWindow(screenWidth, screenHeight, "MechPlay");

    InitAudioDevice();              // Initialize audio device

    LoadMusicFiles("C:/Users/myagi/Desktop/MP3/"); // MP3 Directory
    int currentSongIndex = 0;
    PlayMusicStream(musicArray[currentSongIndex]);

    // TODO: Add Load and bulk load
    //Music music = LoadMusicStream("Ashes.mp3");


    //PlayMusicStream(music);

    float timePlayed = 1.0f;
    bool pause = false;             // Music playing paused

    SetTargetFPS(60);

    float volume = 2.0f;
    SetMusicVolume(musicArray[currentSongIndex], volume);

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        UpdateMusicStream(musicArray[currentSongIndex]);   // Update music buffer with new stream data

        // Next song
        if (IsKeyDown(KEY_RIGHT)) {
            StopMusicStream(musicArray[currentSongIndex]);
            currentSongIndex = (currentSongIndex + 1) % songCount;
            PlayMusicStream(musicArray[currentSongIndex]);
            _sleep(100);
        }

        // Previous Song
        if (IsKeyDown(KEY_LEFT)) {
            if (currentSongIndex == 0) {
                printf("First song! Can't go back baby!");
            } else {
                StopMusicStream(musicArray[currentSongIndex]);
                currentSongIndex = (currentSongIndex - 1) % songCount;
                PlayMusicStream(musicArray[currentSongIndex]);
                _sleep(100);
            }
        }

        // Restart music playing (stop and play)
        if (IsKeyPressed(KEY_R))
        {
            StopMusicStream(musicArray[currentSongIndex]);
            PlayMusicStream(musicArray[currentSongIndex]);
        }

        // Pause/Resume music playing
        if (IsKeyPressed(KEY_SPACE))
        {
            pause = !pause;

            if (pause) PauseMusicStream(musicArray[currentSongIndex]);
            else ResumeMusicStream(musicArray[currentSongIndex]);
        }

        if (IsKeyPressed(KEY_UP)) {
            if (volume >= -0.1) {
                volume += 0.2;
                SetMusicVolume(musicArray[currentSongIndex], volume);
                printf("Volume level: %.2f",volume);
            }
        }

        if (IsKeyPressed(KEY_DOWN)) {
            if (volume > 0) {
                volume -= 0.2;
                SetMusicVolume(musicArray[currentSongIndex], volume);
                printf("Volume level: %.2f",volume);
            }
        }

        // Get normalized time played for current music stream
        timePlayed = GetMusicTimePlayed(musicArray[currentSongIndex])/GetMusicTimeLength(musicArray[currentSongIndex]);

        if (timePlayed >= 0.999f) {
            if (currentSongIndex == songCount - 1) {
                currentSongIndex = 0;
            } else {
                currentSongIndex++;
            }
            StopMusicStream(musicArray[currentSongIndex - 1]);
            PlayMusicStream(musicArray[currentSongIndex]);
        }

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();
            // TODO: ADD LIST AND DRAG & DROP
            ClearBackground(BLACK);


            // For Showing music name without full path
            const char *musicName = strrchr(songFiles[currentSongIndex], '//');
            if (musicName == NULL) musicName = songFiles[currentSongIndex]; // No backslashes found
            else musicName++; // Move past the backslash
            DrawText(musicName, 180, 150, 20, DARKPURPLE);

            DrawRectangle(200, 200, 400, 12, LIGHTGRAY);
            DrawRectangle(200, 200, (int)(timePlayed*400.0f), 12, MAROON);
            DrawRectangleLines(200, 200, 400, 12, GRAY);


            // TODO: ADD KEYBOARD MUSIC CONTROLS
            DrawText("R TO RESTART MUSIC", 300, 250, 15, PURPLE);
            DrawText("SPACE TO PAUSE/RESUME MUSIC", 300, 250+15, 15, PURPLE);
            DrawText("UP&DOWN TO CONTROL VOLUME", 300, 250+30, 15, PURPLE);
            // DRAW VOLUME
            char vol[64];
            sprintf(vol, "%.1f", volume*100);
            DrawText(vol, 95,70,12, DARKPURPLE);
            DrawRectangle(100,100,12,320, DARKPURPLE);
            DrawRectangle(100, 100, 12, 300-volume*20.0f, DARKBLUE);
            DrawRectangleLines(100,100,12,320, DARKPURPLE);
        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadMusicStream(musicArray[currentSongIndex]);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}