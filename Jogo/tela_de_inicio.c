#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "gameplay.h"

typedef enum {
    BUTTON_START,
    BUTTON_HOW_TO_PLAY,
    BUTTON_EXIT
} ButtonType;

typedef enum {
    SCREEN_MAIN,
    SCREEN_HOW_TO_PLAY,
    SCREEN_GAMEPLAY,
    SCREEN_LOSE,
    SCREEN_VICTORY
} GameScreen;


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

// Verifica a interação com o botão e retorna true se clicado
bool check_button_interaction(Button* button) {
    Vector2 mousePos = GetMousePosition();
    if (CheckCollisionPointRec(mousePos, button->area)) {
        hover_button(button);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
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
// Não precisamos mais da flag 'screen_to_unload'

void initialize_buttons() {
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

// Atualiza a lógica do jogo (máquina de estados)
void update_game_logic() {
    
    // Atualiza a música independentemente da tela
    //UpdateMusicStream(music);
    
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
                            // Limpa os assets da rodada anterior ANTES de carregar os novos
                            unload_gameplay_round(); 
                            
                            // Prepara todos os assets para a próxima rodada
                            
                            load_games(); 
                            load_texture();
                            init_gameplay(); // Prepara o estado do gameplay (shader, camera)
                            
                            current_screen = SCREEN_GAMEPLAY;
                            StopMusicStream(music);
                            music_loaded = 0;
                        } else if (i == BUTTON_HOW_TO_PLAY) {
                            current_screen = SCREEN_HOW_TO_PLAY;
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
                // Atualiza um frame do jogo
                update_gameplay();

                // Verifica se o jogo terminou (por vitória ou ESC)
                if (is_gameplay_finished()) {
                    // Apenas sinaliza a mudança de tela.
                    // O 'unload' acontecerá na próxima vez que 'Play' for clicado.
                    
                    transition_offset = 0;
                    previous_screen = current_screen;
                    
                    // Decide para qual tela ir
                    if (win) {
                        current_screen = SCREEN_VICTORY;
                    } else if (lose) {
                        current_screen = SCREEN_LOSE;
                    } else{
                        current_screen = SCREEN_MAIN;
                    }
                    
                    // Recarrega e toca a música do menu
                    if (!music_loaded) {
                        music = LoadMusicStream("musicas/Project_1.ogg");
                        PlayMusicStream(music);
                        music_loaded = 1;
                    }
                }
            } break;

            // Update da tela de vitória
            case SCREEN_VICTORY: {
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    transition_offset = 0;
                    previous_screen = current_screen;
                    current_screen = SCREEN_MAIN;
                }
            } break;

            case SCREEN_LOSE: {
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    transition_offset = 0;
                    previous_screen = current_screen;
                    current_screen = SCREEN_MAIN;
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

//
// *** FUNÇÃO DE DESENHO MODIFICADA ***
//
// Agora recebe a câmera de transição para aplicar o offset
void draw_screen(GameScreen screen, Camera2D transition_cam) {
    
    switch (screen) {
         case SCREEN_MAIN: {
            BeginMode2D(transition_cam); // Inicia modo 2D com a câmera de transição
                DrawText("GUESS THE GAME", screen_width / 2 - MeasureText("GUESS THE GAME", 40) / 2, 100, 40, WHITE);
                for (int i = 0; i < 3; i++) {
                    draw_button(buttons + i);
                }
            EndMode2D(); // Termina modo 2D
        } break;

        case SCREEN_HOW_TO_PLAY: {
            BeginMode2D(transition_cam);
            // Você começa digitando o nome de qualquer Campeão do LoL.

            // Após cada palpite, o jogo fornece dicas sobre o Campeão correto, comparando as características da sua tentativa (por exemplo: Gênero, Posições, Espécie, Região, etc.) com as do Campeão alvo.
            
            // Indicadores de Dica:
            
            // Verde: A característica está correta.
            
            // Amarelo: A característica está parcialmente correta (ex: uma das posições está correta, mas não a principal, ou é uma das posições possíveis).
            
            // Vermelho: A característica está incorreta.
            
            // Setas: Indicam se o número da Característica correta (ex: ano de lançamento, distância do ataque básico) é maior (↑) ou menor (↓) do que o seu palpite.
                DrawText("Adivinhe o jogo!", 100, 100, 20, WHITE);
                DrawText("Você começa digitando o nome de qualquer jogo.", 100, 130, 20, WHITE);
                DrawText("Após cada palpite, o jogo fornece dicas sobre o jogo correto, ", 100, 160, 20, WHITE);
                DrawText("comparando as características da sua tentativa com as do jogo certo.", 100, 190, 20, WHITE);
                DrawText("Indicadores de Dica:", 100, 240, 20, WHITE);
                DrawText("  A característica está correta.", 100, 270, 20, GREEN);
                DrawText("  A característica está parcialmente correta.", 100, 300, 20, YELLOW);
                DrawText("  A característica está incorreta.", 100, 330, 20, RED);
                DrawText("  Setas: Indicam se o número da Característica correta é maior ou menor do que o seu palpite.", 100, 360, 20, GRAY);
            EndMode2D();
        } break;

        case SCREEN_GAMEPLAY: {
            // Desenho do Gameplay é dividido em duas partes (Mundo e UI)
            
            // 1. DESENHA O MUNDO (com câmera do jogo + câmera de transição)
            Camera2D world_cam = get_gameplay_camera();
            world_cam.offset = Vector2Add(world_cam.offset, transition_cam.offset); 
            
            BeginMode2D(world_cam);
                draw_gameplay_world();
            EndMode2D();
            
            // 2. DESENHA A UI (apenas com câmera de transição)
            BeginMode2D(transition_cam);
                draw_gameplay_ui();
            EndMode2D();
        } break;
        
        // Desenho da tela de vitória
        case SCREEN_VICTORY: {
            BeginMode2D(transition_cam);
                const char* msgVitoria = "VOCE VENCEU!";
                int tamTexto = MeasureText(msgVitoria, 80);
                int posX = (screen_width - tamTexto) / 2;
                int posY = screen_height / 2 - 40;
                DrawText(msgVitoria, posX, posY, 80, GREEN);

                const char* msgRestart = "Pressione ENTER para voltar ao Menu";
                int tamRestart = MeasureText(msgRestart, 20);
                posX = (screen_width - tamRestart) / 2;
                posY = screen_height / 2 + 60;
                DrawText(msgRestart, posX, posY, 20, DARKGRAY);
            EndMode2D();
        } break;

        case SCREEN_LOSE: {
            BeginMode2D(transition_cam);
                const char* msgVitoria = "VOCE PERDEU!";
                int tamTexto = MeasureText(msgVitoria, 80);
                int posX = (screen_width - tamTexto) / 2;
                int posY = screen_height / 2 - 40;
                DrawText(msgVitoria, posX, posY, 80, RED);

                const char* msgRestart = "Pressione ENTER para voltar ao Menu";
                int tamRestart = MeasureText(msgRestart, 20);
                posX = (screen_width - tamRestart) / 2;
                posY = screen_height / 2 + 60;
                DrawText(msgRestart, posX, posY, 20, DARKGRAY);
            EndMode2D();
        } break;
    }
}


int main()
{
    initialize_list();
    update_list();
    
    // srand(time(NULL)) precisa estar aqui
    srand(time(NULL));

    // Inicializa a janela com o tamanho vindo do gameplay.h
    InitWindow(screen_width, screen_height, "Guess The Game");
    InitAudioDevice();

    // Desativa a tecla ESC para fechar o jogo
    SetExitKey(0); 

    // Carrega a lista de jogos UMA VEZ
    load_list();

    SetTargetFPS(60);

    initialize_buttons();

    // Loop principal (O Maestro)
    while (!WindowShouldClose()) {
        
        // 1. ATUALIZAR
        update_game_logic();

        // 2. DESENHAR
        BeginDrawing();
        ClearBackground((Color){30,30,30,255}); // Cor de fundo do jogo
        
        // Câmeras base para a transição
        Camera2D transition_cam_prev = { 0 };
        transition_cam_prev.zoom = 1.0f;
        
        Camera2D transition_cam_curr = { 0 };
        transition_cam_curr.zoom = 1.0f;

        
        if (current_screen != previous_screen)
        {
            // --- 1. ATUALIZA A TRANSIÇÃO (Lerp Puro) ---
            transition_offset = Lerp(transition_offset, (float)screen_width, 0.1f);
            
            // Se a transição terminou (ou está muito perto),
            // reseta o offset e trava a tela em 'previous_screen = current_screen'
            // Isso permite que o 'update_game_logic' volte a rodar
            if (fabs((float)screen_width - transition_offset) < 0.1f) // Mais preciso que 1.0f
            {
                transition_offset = 0.0f; // Reseta para a próxima transição
                previous_screen = current_screen; // Trava na nova tela
            }
            
            // Define os offsets de transição
            transition_cam_prev.offset = (Vector2){ -transition_offset, 0 };
            transition_cam_curr.offset = (Vector2){ screen_width - transition_offset, 0 };
        }
        // Se não estiver em transição, os offsets permanecem (0, 0)
        // o que faz 'transition_cam_curr' ser a câmera padrão.

        // --- 2. DESENHA A TELA ANTERIOR (se estiver em transição) ---
        if (previous_screen != current_screen) 
        {
            draw_screen(previous_screen, transition_cam_prev);
        }

        // --- 3. DESENHA A TELA ATUAL ---
        draw_screen(current_screen, transition_cam_curr);

        EndDrawing();
    }

    // --- LIMPEZA FINAL ---
    unload_gameplay_round(); // Limpa a última rodada
    unload_global_assets(); // Limpa as texturas da lista de jogos
    UnloadMusicStream(music);
    CloseAudioDevice();
    CloseWindow();        
    return 0;
}
