#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "gameplay.h" // Inclui nosso header do jogo

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

// --- Funções de Botão (Sem alterações) ---
void press_button(Button* button) {
// ... (código da função press_button sem alterações) ...
    button->is_pressed = 1;
    button->scale_x = 0.9f;
    button->scale_y = 0.9f;
}

void hover_button(Button* button) {
// ... (código da função hover_button sem alterações) ...
    if (!button->is_hovered) {
        button->is_hovered = 1;
        button->scale_x = Lerp(button->scale_x, 1.1f, 0.2f);
        button->scale_y = Lerp(button->scale_y, 1.1f, 0.2f);
        button->current_color = button->hover_color;
    }
}

void reset_button_state(Button* button) {
// ... (código da função reset_button_state sem alterações) ...
    button->is_hovered = 0;
    button->scale_x = Lerp(button->scale_x, 1.0f, 0.2f);
    button->scale_y = Lerp(button->scale_y, 1.0f, 0.2f);
    button->current_color = button->color;
    button->is_pressed = 0;
}

void draw_button(Button* button) {
// ... (código da função draw_button sem alterações) ...
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

bool check_button_interaction(Button* button)
{
// ... (código da função check_button_interaction sem alterações) ...
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

// --- Variáveis Globais do Menu ---
static Music music = {0};
static int music_loaded = 0;

static float transition_offset = 0.0f;
static Button buttons[3] = {0};
static GameScreen current_screen = SCREEN_MAIN;
static GameScreen previous_screen = SCREEN_MAIN;

void initialize_buttons() {
// ... (código da função initialize_buttons sem alterações, mas usando screen_width/height) ...
    for (int i = 0; i < 3; i++) {
        buttons[i].sprite = (Texture2D){0};
        buttons[i].area = (Rectangle){
            (screen_width / 2) - 90.0f,
            (screen_height / 2) - 100.0f + i * 100.0f,
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
    buttons[BUTTON_START].text = "Play";
    buttons[BUTTON_HOW_TO_PLAY].text = "How to Play";
    buttons[BUTTON_EXIT].text = "Exit";
}

// *** LÓGICA DE UPDATE PRINCIPAL (MODIFICADA) ***
void update_game_logic() {
    
    // Atualiza a música independentemente da tela
    UpdateMusicStream(music);
    
    // A lógica de update só roda se NÃO estivermos em transição
    if (current_screen == previous_screen) {
        switch (current_screen) {
            case SCREEN_MAIN: {
                for (int i = 0; i < 3; i++) {
                    reset_button_state(&buttons[i]);
                    if (check_button_interaction(&buttons[i])) {
                        transition_offset = 0; // Prepara para a transição
                        previous_screen = current_screen;

                        if (i == BUTTON_START) {
                            // Prepara todos os assets para a próxima rodada
                            load_games(); 
                            load_texture();
                            init_gameplay(); // Prepara o estado do gameplay (shader, camera)
                            
                            current_screen = SCREEN_GAMEPLAY;
                            StopMusicStream(music);
                            music_loaded = 0;
                        } else if (i == BUTTON_HOW_TO_PLAY) {
                            current_screen = SCREEN_HOW_TO_PLAY;
                            // (Opcional) Poderia parar a música aqui também
                        } else if (i == BUTTON_EXIT) {
                            CloseWindow(); // O loop principal no main() vai parar
                        }
                    }
                }
            } break;

            case SCREEN_HOW_TO_PLAY: {
                if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    transition_offset = 0;
                    previous_screen = current_screen;
                    current_screen = SCREEN_MAIN;
                }
            } break;

            case SCREEN_GAMEPLAY: {
                // Agora, em vez de um loop 'sequestrado',
                // nós apenas atualizamos um frame do jogo.
                update_gameplay();

                // Verificamos se o jogo terminou (por vitória ou ESC)
                if (is_gameplay_finished()) {
                    unload_gameplay_round(); // Limpa os assets da rodada
                    
                    transition_offset = 0;
                    previous_screen = current_screen;
                    current_screen = SCREEN_MAIN;
                    
                    // Recarrega e toca a música do menu
                    if (!music_loaded) {
                        music = LoadMusicStream("musicas/Project_1.ogg");
                        PlayMusicStream(music);
                        music_loaded = 1;
                    }
                }
            } break;
        }
    }

    // Gerencia a música do menu
    if (current_screen == SCREEN_MAIN && !music_loaded) {
        music = LoadMusicStream("musicas/Project_1.ogg");
        PlayMusicStream(music);
        music_loaded = 1;
    }
}

// *** LÓGICA DE DRAW PRINCIPAL (MODIFICADA) ***
// Esta função agora só desenha as telas simples.
// O gameplay é desenhado diretamente no loop main.
void draw_screen(GameScreen screen) {
    // A tela é limpa no loop 'main'
    
    switch (screen) {
         case SCREEN_MAIN: {
            DrawText("GUESS THE GAME", screen_width / 2 - MeasureText("GUESS THE GAME", 40) / 2, 100, 40, WHITE);
            for (int i = 0; i < 3; i++) {
                draw_button(buttons + i);
            }
        } break;

        case SCREEN_HOW_TO_PLAY: {
            DrawText("Adivinhe o jogo!", 100, 100, 20, WHITE);
            DrawText("Use as setas para navegar na busca e Enter para tentar.", 100, 130, 20, WHITE);
            DrawText("Aperte ESC para voltar ao menu a qualquer momento.", 100, 160, 20, WHITE);
        } break;

        case SCREEN_GAMEPLAY: {
            // Esta função não faz mais nada aqui.
            // O gameplay é desenhado no loop principal.
        } break;
    }
}


int main(void)
{
    // srand(time(NULL)) precisa estar aqui, já que este é o main()
    srand(time(NULL));

    // Inicializa a janela com o tamanho vindo do gameplay.h
    InitWindow(screen_width, screen_height, "Guess The Game");
    InitAudioDevice();

    // CORREÇÃO: Desativa a tecla ESC para fechar o jogo
    // Agora só o botão 'X' da janela fecha, ou o CloseWindow()
    SetExitKey(0); 

    // Carrega a lista de jogos UMA VEZ
    load_list();

    SetTargetFPS(60);

    initialize_buttons();

    // Loop principal (Agora 100% no controle)
    while (!WindowShouldClose()) {
        
        // 1. ATUALIZAR
        update_game_logic();

        // 2. DESENHAR
        BeginDrawing();
        ClearBackground((Color){30,30,30,255});
        
        // Câmeras base para a transição
        Camera2D transition_cam_prev = { 0 };
        transition_cam_prev.zoom = 1.0f;
        
        Camera2D transition_cam_curr = { 0 };
        transition_cam_curr.zoom = 1.0f;

        
        if (current_screen != previous_screen)
        {
            // --- 1. ATUALIZA A TRANSIÇÃO ---
            transition_offset = Lerp(transition_offset, screen_width, 0.1f);
            
            if ((screen_width - transition_offset) < 1.0f) {
                transition_offset = screen_width;
                previous_screen = current_screen; // Trava a transição
            }
            
            // Define os offsets de transição
            transition_cam_prev.offset = (Vector2){ -transition_offset, 0 };
            transition_cam_curr.offset = (Vector2){ screen_width - transition_offset, 0 };
        }

        // --- 2. DESENHA A TELA ANTERIOR (se estiver em transição) ---
        if (transition_offset < screen_width) 
        {
            if (previous_screen == SCREEN_GAMEPLAY) {
                // Pega a câmera do jogo (com shake/scroll)
                Camera2D world_cam = get_gameplay_camera();
                // Adiciona o offset da transição
                world_cam.offset = Vector2Add(world_cam.offset, transition_cam_prev.offset); 
                
                // Desenha o MUNDO com a câmera combinada
                BeginMode2D(world_cam);
                    draw_gameplay_world();
                EndMode2D();
                
                // Desenha a UI apenas com a câmera de transição
                BeginMode2D(transition_cam_prev);
                    draw_gameplay_ui();
                EndMode2D();
            } else {
                // Desenha o menu/how-to-play
                BeginMode2D(transition_cam_prev);
                    draw_screen(previous_screen);
                EndMode2D();
            }
        }

        // --- 3. DESENHA A TELA ATUAL ---
        if (current_screen == SCREEN_GAMEPLAY) {
            // Pega a câmera do jogo (com shake/scroll)
            Camera2D world_cam = get_gameplay_camera();
            // Adiciona o offset da transição
            world_cam.offset = Vector2Add(world_cam.offset, transition_cam_curr.offset);
            
            // Desenha o MUNDO com a câmera combinada
            BeginMode2D(world_cam);
                draw_gameplay_world();
            EndMode2D();
            
            // Desenha a UI apenas com a câmera de transição
            BeginMode2D(transition_cam_curr);
                draw_gameplay_ui();
            EndMode2D();
        } else {
            // Desenha o menu/how-to-play
            BeginMode2D(transition_cam_curr);
                draw_screen(current_screen);
            EndMode2D();
        }

        EndDrawing();
    }

    // --- LIMPEZA FINAL ---
    unload_global_assets(); // Limpa as texturas da lista de jogos
    UnloadMusicStream(music);
    CloseAudioDevice();
    CloseWindow();        
    return 0;
}