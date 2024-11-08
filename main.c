#include "raylib.h"
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>



#define MAX_SONGS 100
#define VISIBLE_SONGS 5  // Show 2 songs before and after + current song

typedef struct {
    Music music;
    char *filePath;
    Texture2D albumArt;
    bool hasAlbumArt;
} SongInfo;

SongInfo songs[MAX_SONGS];
int songCount = 0;


static uint32_t getSynchsafeInt(const unsigned char *bytes) {
    return ((bytes[0] & 0x7F) << 21) |
           ((bytes[1] & 0x7F) << 14) |
           ((bytes[2] & 0x7F) << 7) |
           (bytes[3] & 0x7F);
}

Texture2D LoadAlbumArtFromMP3(const char* filePath) {
    Texture2D texture = { 0 };
    printf("\n=== Starting LoadAlbumArtFromMP3 for: %s ===\n", filePath);

    FILE *file = fopen(filePath, "rb");
    if (!file) {
        printf("ERROR: Failed to open file: %s\n", filePath);
        goto use_placeholder;
    }
    printf("Successfully opened file\n");

    // Read ID3 header
    unsigned char header[10];
    if (fread(header, 1, 10, file) != 10 || memcmp(header, "ID3", 3) != 0) {
        printf("ERROR: No valid ID3 tag found\n");
        fclose(file);
        goto use_placeholder;
    }

    uint32_t tagSize = ((header[6] & 0x7F) << 21) |
                       ((header[7] & 0x7F) << 14) |
                       ((header[8] & 0x7F) << 7) |
                       (header[9] & 0x7F);
    uint8_t majorVersion = header[3];

    printf("Found ID3v2.%d tag, size: %u bytes\n", majorVersion, tagSize);

    unsigned char *tagData = malloc(tagSize);
    if (!tagData || fread(tagData, 1, tagSize, file) != tagSize) {
        printf("ERROR: Failed to read tag data\n");
        if (tagData) free(tagData);
        fclose(file);
        goto use_placeholder;
    }

    unsigned char *pos = tagData;
    unsigned char *end = tagData + tagSize;
    unsigned char *imageData = NULL;
    uint32_t imageSize = 0;

    while (pos + 10 <= end) {
        if (memcmp(pos, "APIC", 4) == 0) {
            uint32_t frameSize;
            if (majorVersion == 4) {
                frameSize = ((pos[4] & 0x7F) << 21) |
                           ((pos[5] & 0x7F) << 14) |
                           ((pos[6] & 0x7F) << 7) |
                           (pos[7] & 0x7F);
            } else {
                frameSize = (pos[4] << 24) | (pos[5] << 16) |
                           (pos[6] << 8) | pos[7];
            }

            unsigned char *frameData = pos + 11;
            frameSize -= 1;

            // Skip MIME type
            const char *mimeType = (const char *)frameData;
            size_t mimeLen = strlen(mimeType);
            frameData += mimeLen + 1;
            frameSize -= mimeLen + 1;

            // Skip picture type
            frameData++;
            frameSize--;

            // Skip description
            const char *description = (const char *)frameData;
            size_t descLen = strlen(description);
            frameData += descLen + 1;
            frameSize -= descLen + 1;

            imageData = frameData;
            imageSize = frameSize;
            break;
        }

        uint32_t frameSize;
        if (majorVersion == 4) {
            frameSize = ((pos[4] & 0x7F) << 21) |
                       ((pos[5] & 0x7F) << 14) |
                       ((pos[6] & 0x7F) << 7) |
                       (pos[7] & 0x7F);
        } else {
            frameSize = (pos[4] << 24) | (pos[5] << 16) |
                       (pos[6] << 8) | pos[7];
        }

        pos += 10 + frameSize;
    }

    if (imageData && imageSize > 0) {
        // Create a temporary file with proper extension
        const char *tempFile = "temp_album_art.jpg";
        FILE *outFile = fopen(tempFile, "wb");
        if (!outFile) {
            printf("ERROR: Failed to create temporary file\n");
            free(tagData);
            fclose(file);
            goto use_placeholder;
        }

        // Write image data to temporary file
        size_t written = fwrite(imageData, 1, imageSize, outFile);
        fclose(outFile);

        if (written != imageSize) {
            printf("ERROR: Failed to write image data\n");
            //remove(tempFile);
            free(tagData);
            fclose(file);
            goto use_placeholder;
        }

        // Load the image using raylib
        Image img = LoadImage("./temp.png");
        //remove(tempFile);  // Delete temporary file immediately

        if (img.data) {
            // Resize if necessary
            float aspect = (float)img.width / (float)img.height;
            int newWidth = 200;
            int newHeight = 200;

            if (aspect > 1.0f) {
                newHeight = (int)(200 / aspect);
            } else {
                newWidth = (int)(200 * aspect);
            }

            if (img.width > newWidth || img.height > newHeight) {
                ImageResize(&img, newWidth, newHeight);
            }

            texture = LoadTextureFromImage(img);
            UnloadImage(img);

            if (texture.id != 0) {
                free(tagData);
                fclose(file);
                printf("Successfully loaded album art\n");
                return texture;
            }
        }
    }

    free(tagData);
    fclose(file);

use_placeholder:
    printf("Creating purple placeholder texture\n");
    Image img = GenImageColor(200, 200, PURPLE);
    texture = LoadTextureFromImage(img);
    UnloadImage(img);
    return texture;
}
void AddSong(const char* filePath) {
    if (songCount >= MAX_SONGS) return;

    songs[songCount].filePath = malloc(strlen(filePath) + 1);
    strcpy(songs[songCount].filePath, filePath);
    songs[songCount].music = LoadMusicStream(filePath);
    songs[songCount].albumArt = LoadAlbumArtFromMP3(filePath);
    songs[songCount].hasAlbumArt = true;  // Set to true for now
    songCount++;
}

const char* GetFileNameFromPath(const char* path) {
    const char* fileName = strrchr(path, '\\');
    if (fileName == NULL) {
        fileName = strrchr(path, '/');
        if (fileName == NULL) {
            return path;
        }
    }
    return fileName + 1;
}

void DrawPlaylist(int currentSongIndex, int screenWidth) {
    int start = currentSongIndex - 2;
    int end = currentSongIndex + 2;

    // Adjust bounds
    if (start < 0) {
        end -= start;  // Extend end to show more songs after
        start = 0;
    }
    if (end >= songCount) {
        start -= (end - songCount + 1);  // Extend start to show more songs before
        end = songCount - 1;
    }
    // Final bounds check
    if (start < 0) start = 0;
    if (end >= songCount) end = songCount - 1;

    int playlistY = 280;  // Adjusted Y position to make room for album art
    for (int i = start; i <= end; i++) {
        const char* songName = GetFileNameFromPath(songs[i].filePath);
        if (i == currentSongIndex) {
            DrawRectangle(20, playlistY, screenWidth - 40, 20, DARKPURPLE);
            DrawText(songName, 25, playlistY + 2, 10, LIGHTGRAY);
        } else {
            DrawText(songName, 25, playlistY + 2, 10, GRAY);
        }
        playlistY += 25;
    }
}

int main(void)
{
    Image img;
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "MechPlay");
    InitAudioDevice();

    int currentSongIndex = 0;
    float timePlayed = 1.0f;
    bool pause = false;
    float volume = 1.0f;

    SetTargetFPS(60);

    // Main game loop
    while (!WindowShouldClose())
    {
        // Only try to update music if we have songs loaded
        if (songCount > 0) {
            UpdateMusicStream(songs[currentSongIndex].music);

            if (IsKeyPressed(KEY_RIGHT) && songCount > 0) {
                StopMusicStream(songs[currentSongIndex].music);
                currentSongIndex = (currentSongIndex + 1) % songCount;
                PlayMusicStream(songs[currentSongIndex].music);
                SetMusicVolume(songs[currentSongIndex].music, volume);
                Sleep(100);
            }

            if (IsKeyPressed(KEY_LEFT) && songCount > 0) {
                if (currentSongIndex > 0) {
                    StopMusicStream(songs[currentSongIndex].music);
                    currentSongIndex--;
                    PlayMusicStream(songs[currentSongIndex].music);
                    SetMusicVolume(songs[currentSongIndex].music, volume);
                    Sleep(100);
                }
            }

            if (IsKeyPressed(KEY_R) && songCount > 0) {
                StopMusicStream(songs[currentSongIndex].music);
                PlayMusicStream(songs[currentSongIndex].music);
                Sleep(100);
            }

            if (IsKeyPressed(KEY_SPACE) && songCount > 0) {
                pause = !pause;
                if (pause) PauseMusicStream(songs[currentSongIndex].music);
                else ResumeMusicStream(songs[currentSongIndex].music);
            }

            // Volume controls
            if (IsKeyPressed(KEY_UP)) {
                volume = fmin(volume + 0.1f, 1.0f);
                if (songCount > 0) SetMusicVolume(songs[currentSongIndex].music, volume);
            }

            if (IsKeyPressed(KEY_DOWN)) {
                volume = fmax(volume - 0.1f, 0.0f);
                if (songCount > 0) SetMusicVolume(songs[currentSongIndex].music, volume);
            }

            // Auto-advance to next song
            if (songCount > 0) {
                timePlayed = GetMusicTimePlayed(songs[currentSongIndex].music)/GetMusicTimeLength(songs[currentSongIndex].music);
                if (timePlayed >= 0.999f) {
                    StopMusicStream(songs[currentSongIndex].music);
                    currentSongIndex = (currentSongIndex + 1) % songCount;
                    PlayMusicStream(songs[currentSongIndex].music);
                    SetMusicVolume(songs[currentSongIndex].music, volume);
                }
            }
        }

        // Draw
        BeginDrawing();
        ClearBackground(BLACK);

        if (songCount > 0) {
            // Draw album art
            if (songs[currentSongIndex].hasAlbumArt) {
                DrawTexture(songs[currentSongIndex].albumArt, screenWidth/2 - 100, 20, WHITE);
            }

            const char *musicName = GetFileNameFromPath(songs[currentSongIndex].filePath);
            DrawText(musicName, 180, 230, 20, DARKPURPLE);

            // Progress bar
            DrawRectangle(200, 260, 400, 12, LIGHTGRAY);
            DrawRectangle(200, 260, (int)(timePlayed*400.0f), 12, MAROON);
            DrawRectangleLines(200, 260, 400, 12, GRAY);

            // Draw playlist
            DrawPlaylist(currentSongIndex, screenWidth);
        } else {
            DrawText("Drop MP3 files here to play!", screenWidth/2.2 - 100, screenHeight/2, 20, LIGHTGRAY);
        }

        // Controls info
        DrawText("R - Restart | SPACE - Pause/Resume | UP/DOWN - Volume | RIGHT/LEFT - Song Selection", 20, screenHeight - 30, 15, PURPLE);

        // Volume indicator
        DrawRectangle(20, 20, 10, 100, LIGHTGRAY);
        DrawRectangle(20, 20 + (100 - volume*100), 10, volume*100, MAROON);

        // Handle drag and drop
        if (IsFileDropped()) {
            FilePathList droppedFiles = LoadDroppedFiles();
            for (int i = 0; i < droppedFiles.count; i++) {
                if (strstr(droppedFiles.paths[i], ".mp3") != NULL) {
                    AddSong(droppedFiles.paths[i]);
                    if (songCount == 1) {  // First song added
                        PlayMusicStream(songs[0].music);
                        SetMusicVolume(songs[0].music, volume);
                    }
                }
            }
            UnloadDroppedFiles(droppedFiles);
        }

        EndDrawing();
    }

    // Cleanup
    for (int i = 0; i < songCount; i++) {
        UnloadMusicStream(songs[i].music);
        UnloadTexture(songs[i].albumArt);
        free(songs[i].filePath);
    }

    CloseAudioDevice();
    CloseWindow();
    return 0;
}