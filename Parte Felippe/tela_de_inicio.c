#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

// Enum for button identifiers
typedef enum {
    BUTTON_START,
    BUTTON_HOW_TO_PLAY,
    BUTTON_EXIT
} ButtonType;

// Enum for game screens
typedef enum {
    SCREEN_MAIN,
    SCREEN_HOW_TO_PLAY,
    SCREEN_GAMEPLAY
} GameScreen;

// Struct for a UI Button
typedef struct {
    Texture2D sprite;
    Rectangle area;
    Vector2 position;
    float scale_x;
    float scale_y;

    Color color;
    Color current_color;
    Color hover_color;
    int is_hovered;
    int is_pressed;

    const char* text;
} Button;

void press_button(Button* button) {
    button->is_pressed = 1;
    button->scale_x = 0.9f;
    button->scale_y = 0.9f;
}

void hover_button(Button* button) {
    if (!button->is_hovered) {
        button->is_hovered = 1;
        button->scale_x = Lerp(button->scale_x, 1.1f, 0.2f);
        button->scale_y = Lerp(button->scale_y, 1.1f, 0.2f);
        button->current_color = button->hover_color;
    }
}

void reset_button_state(Button* button) {
    button->is_hovered = 0;
    button->scale_x = Lerp(button->scale_x, 1.0f, 0.2f);
    button->scale_y = Lerp(button->scale_y, 1.0f, 0.2f);
    button->current_color = button->color;
    button->is_pressed = 0;
}

void draw_button(Button* button) {
    float _width = button->area.width * button->scale_x;
    float _height = button->area.height * button->scale_y;
    float _x = button->area.x - (_width - button->area.width) / 2.0f;
    float _y = button->area.y - (_height - button->area.height) / 2.0f;
    DrawRectangle(_x, _y, _width, _height, button->current_color);

    float spacing = 2.0f;
    Vector2 textSize = MeasureTextEx(GetFontDefault(), button->text, 20, spacing);
    float textX = _x + (_width - textSize.x) * 0.5f;
    float textY = _y + (_height - textSize.y) * 0.5f;
    DrawTextEx(GetFontDefault(), button->text, (Vector2){(int)textX, (int)textY}, 20, spacing, BLACK);
}

// Checks button interaction and returns true if clicked
bool check_button_interaction(Button* button)
{
    Vector2 mousePos = GetMousePosition();

    if (CheckCollisionPointRec(mousePos, button->area))
    {
        hover_button(button);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            press_button(button);
            return 1;
        }
    }

    return 0;
}

// --- Global Static Variables ---
static Music music = {0};
static int music_loaded = 0;

static float transition_offset = 0.0f;
static Button buttons[3] = {0};
static GameScreen current_screen = SCREEN_MAIN;
static GameScreen previous_screen = SCREEN_MAIN;

void initialize_buttons() {
    for (int i = 0; i < 3; i++) {
        buttons[i].sprite = (Texture2D){0};
        buttons[i].area = (Rectangle){
            120.0f,
            80 + i * 100.0f,
            180.0f,
            64.0f
        };

        buttons[i].position = (Vector2){
            buttons[i].area.x + buttons[i].area.width / 2.0f,
            buttons[i].area.y + buttons[i].area.height / 2.0f
        };

        buttons[i].scale_x = 1.0f;
        buttons[i].scale_y = 1.0f;
        buttons[i].color = Fade(SKYBLUE, 0.95f);
        buttons[i].hover_color = Fade(LIGHTGRAY, 0.95f);
        buttons[i].current_color = buttons[i].color;
        buttons[i].is_hovered = 0;
        buttons[i].is_pressed = 0;
    }

    // Finalize initialization
    buttons[BUTTON_START].text = "Play";
    buttons[BUTTON_HOW_TO_PLAY].text = "How to Play";
    buttons[BUTTON_EXIT].text = "Exit";
}

// Manages screen logic and transitions
void update_game_logic() {
    switch (current_screen) {
        case SCREEN_MAIN: {
            for (int i = 0; i < 3; i++) {
                reset_button_state(&buttons[i]);
                if (check_button_interaction(&buttons[i])) {
                    transition_offset = 0;
                    previous_screen = current_screen;

                    if (i == BUTTON_START) {
                        current_screen = SCREEN_GAMEPLAY;
                        StopMusicStream(music);
                        //UnloadMusicStream(music);
                        music_loaded = 0;
                    } else if (i == BUTTON_HOW_TO_PLAY) {
                        current_screen = SCREEN_HOW_TO_PLAY;
                        StopMusicStream(music);
                        //UnloadMusicStream(music);
                        music_loaded = 0;
                    } else if (i == BUTTON_EXIT) {
                        CloseWindow();
                        exit(1);
                    }
                }
            }
        } break;

        case SCREEN_HOW_TO_PLAY: {
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                transition_offset = 0;
                previous_screen = current_screen;
                current_screen = SCREEN_MAIN;

                
                if (!music_loaded) {
                    // Adjust the path to the audio file (.ogg/.wav) in your resources folder
                    music = LoadMusicStream("musicas/Project_1.ogg");
                    PlayMusicStream(music);
                    music_loaded = 1;
                }
            }
        } break;

        case SCREEN_GAMEPLAY: {
            
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                transition_offset = 0;
                previous_screen = current_screen;
                current_screen = SCREEN_MAIN;
                
                if (!music_loaded) {
                    // Adjust the path to the audio file (.ogg/.wav) in your resources folder
                    music = LoadMusicStream("musicas/Project_1.ogg");
                    PlayMusicStream(music);
                    music_loaded = 1;
                }
            }

        } break;
    }
}

void draw_screen(GameScreen screen) {
    switch (screen) {
         case SCREEN_MAIN: {
            for (int i = 0; i < 3; i++) {
                draw_button(buttons + i);
            }
        } break;

        case SCREEN_HOW_TO_PLAY: {
            DrawText("guess the game and then get it right and then see and then try\n again and then until you get it right :)", 100, 100, 20, WHITE);
        } break;

        case SCREEN_GAMEPLAY: {
            DrawText("GAMEPLAY by lemes", 100, 100, 20, WHITE);
        } break;
    }
}


int main(void)
{
    const int screen_width = 800;
    const int screen_height = 450;

    
    InitWindow(screen_width, screen_height, "Hello World Raylib 5.5");
    InitAudioDevice();
    SetTargetFPS(60);

    initialize_buttons();

    if (!music_loaded) {
        // Adjust the path to the audio file (.ogg/.wav) in your resources folder
        music = LoadMusicStream("musicas/Project_1.ogg");
        PlayMusicStream(music);
        music_loaded = 1;
    }

    while (!WindowShouldClose()) {
        UpdateMusicStream(music);

        update_game_logic();

        BeginDrawing();
        ClearBackground(BLACK);
        
        Camera2D camera = { 0 };
        camera.zoom = 1.0f; // Normal zoom

        
        if (current_screen != previous_screen)
        {
            if (music_loaded) {
                
                music_loaded = 0;
            }
            
            transition_offset = Lerp(transition_offset, screen_width, 0.1f);
            
            // --- 1. DRAW TRANSITION (Two screens) ---
            
            // A. Draw OLD screen (sliding left)
            camera.offset = (Vector2){ -transition_offset, 0 };
            
            BeginMode2D(camera);
            
            draw_screen(previous_screen);
                
            EndMode2D(); 
            
            
            // B. Draw NEW screen (sliding in from right)
            camera.offset = (Vector2){ screen_width - transition_offset, 0 };

            BeginMode2D(camera); 
            
            draw_screen(current_screen);
                
            EndMode2D();
        }
        else
        {
            // --- 2. DRAW NORMAL (One screen) ---
            
            camera.offset = (Vector2){ 0, 0 };
            
            BeginMode2D(camera); 
            
            draw_screen(current_screen);
                
            EndMode2D();
        }

        EndDrawing();
    }

    CloseWindow();        
    return 0;
}