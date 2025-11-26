#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <math.h> // Necessário para sinf()
#include "gameplay.h"

// --- DEFINIÇÕES E ENUMS ---

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

// Enum para o Fade de Áudio
typedef enum {
    MUSIC_PLAYING,
    MUSIC_FADING_OUT,
    MUSIC_FADING_IN
} MusicState;

typedef struct {
    Texture2D sprite;
    Rectangle sourceRec;
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

// --- VARIÁVEIS GLOBAIS ---

// Texturas
static Texture2D tex_button_atlas = {0};
static Texture2D tex_game_logo = {0};

// --- SISTEMA DE MÚSICA COM FADE ---
static Music music_main = {0};
static Music music_win  = {0};
static Music music_lose = {0};

static Music* current_music = NULL;
static Music* next_music = NULL;    

static MusicState music_state = MUSIC_PLAYING;
static float master_volume = 0.5f; 
static float current_volume = 0.0f;
static float audio_fade_speed = 0.02f;   

// --- SISTEMA DE TRANSIÇÃO (SLIDE + FADE) ---
static float transition_offset = 0.0f;
static float fade_overlay_alpha = 0.0f; // Opacidade do retângulo preto
static int needs_to_load_gameplay = 0;  // Flag para carregar no meio da transição

// Animação do Logo
static float mouse_x = 0;
static float mouse_y = 0;
static float logo_x = 0;
static float logo_y = 0;
static float logo_time = 0;
static float logo_angle = 0;

// Estado do Jogo
static Button buttons[3] = {0};
static GameScreen current_screen = SCREEN_MAIN;
static GameScreen previous_screen = SCREEN_MAIN; // Restaurado para o slide

// Background texture
static Texture2D tex_background = {0};

void load_background_texture(void) {
    if (tex_background.id == 0) {
        tex_background = LoadTexture("sources/background.png");
    }
}

void unload_background_texture(void) {
    if (tex_background.id != 0) {
        UnloadTexture(tex_background);
        tex_background = (Texture2D){0};
    }
}

// --- FUNÇÕES DE BOTÃO ---

void press_button(Button* button) {
    button->scale_x = Lerp(button->scale_x, 0.9f, 0.2f);
    button->scale_y = Lerp(button->scale_y, 0.9f, 0.2f);
    button->sourceRec.x = button->sprite.width / 2.0f;
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
    button->sourceRec.x = 0.0f;
}

void draw_button(Button* button) {
    float _width = button->area.width * button->scale_x;
    float _height = button->area.height * button->scale_y;
    float _x = button->area.x - (_width - button->area.width) / 2.0f + mouse_x;
    float _y = button->area.y - (_height - button->area.height) / 2.0f + mouse_y;

    Rectangle destRec = { _x, _y, _width, _height };
    DrawTexturePro(button->sprite, button->sourceRec, destRec, (Vector2){0,0}, 0.0f, button->current_color);

    float spacing = 2.0f;
    Vector2 textSize = MeasureTextEx(GetFontDefault(), button->text, 20, spacing);
    float textX = _x + (_width - textSize.x) * 0.5f;
    float textY = _y + (_height - textSize.y) * 0.5f;
    DrawTextEx(GetFontDefault(), button->text, (Vector2){(int)textX, (int)textY}, 20, spacing, BLACK);
}

bool check_button_interaction(Button* button) {
    // Se estiver em transição (slide), bloqueia clique
    if (current_screen != previous_screen) return false;

    Vector2 mousePos = GetMousePosition();
    if (CheckCollisionPointRec(mousePos, button->area)) {
        hover_button(button); 
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            press_button(button); 
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            return true;
        }
    }
    return 0;
}

void initialize_buttons() {
    if (tex_button_atlas.id == 0) {
        tex_button_atlas = LoadTexture("sources/botao.png");
    }

    for (int i = 0; i < 3; i++) {
        buttons[i].sprite = tex_button_atlas;
        buttons[i].sourceRec = (Rectangle){ 0.0f, 0.0f, (float)tex_button_atlas.width / 2.0f, (float)tex_button_atlas.height };
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
        buttons[i].color = LIGHTGRAY;
        buttons[i].hover_color = WHITE;
        buttons[i].current_color = buttons[i].color;
        buttons[i].is_hovered = 0;
        buttons[i].is_pressed = 0;
    }

    buttons[BUTTON_START].text = "Play";
    buttons[BUTTON_HOW_TO_PLAY].text = "How to Play";
    buttons[BUTTON_EXIT].text = "Exit";
}

// --- FUNÇÕES DE MÚSICA COM FADE ---

void switch_music_track(Music* requested_track) {
    if (current_music == requested_track && music_state != MUSIC_FADING_OUT) return;
    if (next_music == requested_track) return;

    next_music = requested_track;

    if (current_music != NULL) {
        music_state = MUSIC_FADING_OUT;
    } else {
        current_music = next_music;
        next_music = NULL;
        if (current_music != NULL) PlayMusicStream(*current_music);
        current_volume = 0.0f;
        music_state = MUSIC_FADING_IN;
    }
}

void update_music_system() {
    if (music_state == MUSIC_FADING_OUT) {
        current_volume -= audio_fade_speed;
        
        if (current_volume <= 0.0f) {
            current_volume = 0.0f;
            if (current_music != NULL) StopMusicStream(*current_music);
            
            current_music = next_music;
            next_music = NULL;
            
            if (current_music != NULL) {
                PlayMusicStream(*current_music);
                music_state = MUSIC_FADING_IN;
            } else {
                music_state = MUSIC_PLAYING; 
            }
        }
    }
    else if (music_state == MUSIC_FADING_IN) {
        current_volume += audio_fade_speed;
        
        if (current_volume >= master_volume) {
            current_volume = master_volume;
            music_state = MUSIC_PLAYING;
        }
    }

    if (current_music != NULL) {
        SetMusicVolume(*current_music, current_volume);
        UpdateMusicStream(*current_music);
    }
}

// --- FUNÇÃO AUXILIAR DE INÍCIO DE TRANSIÇÃO ---
void start_transition_to(GameScreen target_screen) {
    previous_screen = current_screen;
    current_screen = target_screen;
    transition_offset = 0.0f; // Reinicia o slide
    fade_overlay_alpha = 0.0f; // Reinicia o fade
}


// --- LÓGICA DO JOGO ---

void update_game_logic() {
    mouse_x = (screen_width / 2 - GetMouseX()) / 30;
    mouse_y = (screen_height / 2 - GetMouseY()) / 30;
    
    logo_time += 0.05f;
    logo_x = cos(logo_time/2) * 15 + mouse_x * 1.5f;
    logo_y = sin(logo_time) * 10 + mouse_y * 1.5f;
    logo_angle = cos(logo_time/2) * 2.5;
    
    // Atualiza sistemas de Fade de Áudio
    update_music_system();
    
    // A lógica principal do jogo só roda se NÃO estivermos em transição
    // (Para evitar interações enquanto a tela desliza)
    if (current_screen == previous_screen) {
        switch (current_screen) {
            case SCREEN_MAIN: {
                switch_music_track(&music_main);

                for (int i = 0; i < 3; i++) {
                    reset_button_state(&buttons[i]);
                    if (check_button_interaction(&buttons[i])) {
                        
                        if (i == BUTTON_START) {
                            // Marca para carregar os assets no meio do slide
                            needs_to_load_gameplay = 1; 
                            start_transition_to(SCREEN_GAMEPLAY);
                            
                        } else if (i == BUTTON_HOW_TO_PLAY) {
                            start_transition_to(SCREEN_HOW_TO_PLAY);

                        } else if (i == BUTTON_EXIT) {
                            CloseWindow(); 
                        }
                    }
                }
            } break;

            case SCREEN_HOW_TO_PLAY: {
                if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    start_transition_to(SCREEN_MAIN);
                }
            } break;

            case SCREEN_GAMEPLAY: {
                update_gameplay();

                if (is_gameplay_finished()) {
                    if (win) {
                        switch_music_track(&music_win);
                        start_transition_to(SCREEN_VICTORY);
                    } else if (lose) {
                        switch_music_track(&music_lose); 
                        start_transition_to(SCREEN_LOSE);
                    } else {
                        switch_music_track(&music_main);
                        start_transition_to(SCREEN_MAIN);
                    }
                }
            } break;

            case SCREEN_VICTORY: {
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    start_transition_to(SCREEN_MAIN);
                }
            } break;

            case SCREEN_LOSE: {
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    start_transition_to(SCREEN_MAIN);
                }
            } break;
        }
    }
}

// --- DESENHO DA TELA ---
// Agora recebe a câmera de transição para manter o efeito de slide
void draw_screen(GameScreen screen, Camera2D transition_cam) {
    
    // Desenha o background sem afetar pela câmera (opcional, ou pode por dentro do Mode2D)
    DrawTexture(tex_background, mouse_x / 2, mouse_y / 2, WHITE);
    
    switch (screen) {
         case SCREEN_MAIN: {
            BeginMode2D(transition_cam);
                {
                    float logoW = (float)tex_game_logo.width;
                    float logoH = (float)tex_game_logo.height;
                    float posX = screen_width/2.0f + logo_x;
                    float posY = 100.0f + logo_y;
                    Rectangle src = { 0.0f, 0.0f, logoW, logoH };
                    Rectangle dest = { posX, posY, logoW, logoH };
                    Vector2 origin = { logoW/2.0f, logoH/2.0f };
                    DrawTexturePro(tex_game_logo, src, dest, origin, logo_angle, WHITE);
                }
                for (int i = 0; i < 3; i++) {
                    draw_button(buttons + i);
                }
            EndMode2D();
        } break;

        case SCREEN_HOW_TO_PLAY: {
            BeginMode2D(transition_cam);
                DrawText("Adivinhe o jogo!", 100 + mouse_x, 100 + mouse_y, 20, WHITE);
                DrawText("Você começa digitando o nome de qualquer jogo.", 100 + mouse_x, 130 + mouse_y, 20, WHITE);
                DrawText("Após cada palpite, o jogo fornece dicas sobre o jogo correto, ", 100 + mouse_x, 160 + mouse_y, 20, WHITE);
                DrawText("comparando as características da sua tentativa com as do jogo certo.", 100 + mouse_x, 190 + mouse_y, 20, WHITE);
                DrawText("Indicadores de Dica:", 100 + mouse_x, 240 + mouse_y, 20, WHITE);
                DrawText("  A característica está correta.", 100 + mouse_x, 270 + mouse_y, 20, GREEN);
                DrawText("  A característica está parcialmente correta.", 100 + mouse_x, 300 + mouse_y, 20, YELLOW);
                DrawText("  A característica está incorreta.", 100 + mouse_x, 330 + mouse_y, 20, RED);
                DrawText("  Setas: Indicam se o número da Característica correta é maior ou menor do que o seu palpite.", 100 + mouse_x, 360 + mouse_y, 20, GRAY);
            EndMode2D();
        } break;

        case SCREEN_GAMEPLAY: {
            // 1. Mundo do Jogo (Soma a câmera do gameplay com a transição)
            Camera2D world_cam = get_gameplay_camera();
            world_cam.offset = Vector2Add(world_cam.offset, transition_cam.offset); 
            
            BeginMode2D(world_cam);
                draw_gameplay_world();
            EndMode2D();
            
            // 2. UI do Jogo (Apenas transição)
            BeginMode2D(transition_cam);
                draw_gameplay_ui();
            EndMode2D();
        } break;
        
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

// --- MAIN ---

int main()
{
    initialize_list();
    update_list();
    
    srand(time(NULL));

    InitWindow(screen_width, screen_height, "Guess The Game");
    InitAudioDevice(); 

    // --- CARREGAMENTO DAS MÚSICAS ---
    music_main = LoadMusicStream("musicas/main_theme_2.ogg");
    music_win  = LoadMusicStream("musicas/win_theme.ogg");
    music_lose = LoadMusicStream("musicas/lose_theme.ogg");

    current_volume = 0.0f;
    master_volume = 0.5f; 

    PlayMusicStream(music_main);
    current_music = &music_main;
    music_state = MUSIC_FADING_IN; 

    SetExitKey(0); 
    load_list();
    SetTargetFPS(60);
    initialize_buttons();

    if (tex_game_logo.id == 0) {
        tex_game_logo = LoadTexture("sources/logojogo.png");
    }

    load_background_texture();
    
    // Loop principal
    while (!WindowShouldClose()) {
        update_game_logic();

        BeginDrawing();
        ClearBackground((Color){30,30,30,255}); 
        
        // --- LÓGICA DE TRANSIÇÃO (SLIDE + FADE) ---
        Camera2D transition_cam_prev = { 0 };
        transition_cam_prev.zoom = 1.0f;
        
        Camera2D transition_cam_curr = { 0 };
        transition_cam_curr.zoom = 1.0f;
        
        if (current_screen != previous_screen)
        {
            // 1. Move a câmera (SLIDE)
            transition_offset = Lerp(transition_offset, (float)screen_width, 0.05f); // 0.05f é a velocidade do slide
            
            // 2. Calcula o Alpha do Fade baseado na posição do Slide (0 -> 1 -> 0)
            float slide_progress = transition_offset / (float)screen_width;
            // Usa seno para criar arco: inicia 0, meio 1, fim 0
            fade_overlay_alpha = sinf(slide_progress * PI); 

            // 3. CARREGAMENTO DE ASSETS (O Pulo do Gato)
            // Se estamos perto do meio da transição (tela quase preta) e precisamos carregar o jogo
            if (needs_to_load_gameplay && slide_progress > 0.4f) {
                
                // Força desenhar um frame totalmente preto antes de travar para carregar
                DrawRectangle(0, 0, screen_width, screen_height, BLACK);
                EndDrawing(); // Força update da tela
                
                // --- AREA DE CARREGAMENTO PESADO ---
                unload_gameplay_round(); 
                load_games(); 
                load_texture();
                init_gameplay();
                // -----------------------------------
                
                needs_to_load_gameplay = 0; // Já carregou
                
                BeginDrawing(); // Retoma o drawing normal
            }

            // Se terminou o slide
            if (fabs((float)screen_width - transition_offset) < 1.0f) 
            {
                transition_offset = 0.0f; 
                previous_screen = current_screen; 
                fade_overlay_alpha = 0.0f; // Garante que o fade some
            }
            
            // Define offsets das câmeras
            transition_cam_prev.offset = (Vector2){ -transition_offset, 0 };
            transition_cam_curr.offset = (Vector2){ screen_width - transition_offset, 0 };
        }

        // Desenha a tela anterior (saindo)
        if (previous_screen != current_screen) {
            draw_screen(previous_screen, transition_cam_prev);
        }

        // Desenha a tela atual (entrando)
        draw_screen(current_screen, transition_cam_curr);
        
        // DESENHA O RETÂNGULO DE FADE (TELA PRETA)
        if (fade_overlay_alpha > 0.01f) {
            // Usa Clamp para garantir que alpha fique entre 0 e 1
            float safe_alpha = Clamp(fade_overlay_alpha, 0.0f, 1.0f);
            DrawRectangle(0, 0, screen_width, screen_height, Fade(BLACK, safe_alpha));
        }

        EndDrawing();
    }

    // --- LIMPEZA ---
    UnloadTexture(tex_button_atlas);
    unload_gameplay_round(); 
    unload_global_assets(); 
    unload_background_texture();

    UnloadMusicStream(music_main);
    UnloadMusicStream(music_win);
    UnloadMusicStream(music_lose);

    CloseAudioDevice();
    CloseWindow();        
    return 0;
}