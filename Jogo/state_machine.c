#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <math.h> 
#include "gameplay.h"

#pragma region Definições e Estruturas

// Identifica quais botões temos no menu principal para facilitar o input
typedef enum {
    BUTTON_START,
    BUTTON_HOW_TO_PLAY,
    BUTTON_EXIT
} ButtonType;

/**
 * Lista de todas as telas possíveis do jogo.
 * Usamos isso para saber o que desenhar e qual lógica rodar a cada frame.
 * 
 */
typedef enum {
    SCREEN_MAIN,
    SCREEN_HOW_TO_PLAY,
    SCREEN_GAMEPLAY,
    SCREEN_LOSE,
    SCREEN_VICTORY
} GameScreen;

// Estados para controlar a transição suave de música (crossfade)
typedef enum {
    MUSIC_PLAYING,
    MUSIC_FADING_OUT,
    MUSIC_FADING_IN
} MusicState;

/**
 * Representa um botão na interface.
 * Guarda tanto a aparência (textura, cor, escala) quanto o estado lógico (mouse em cima, clicado).
 */
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

#pragma endregion

#pragma region Variáveis Globais

// Texturas globais usadas em várias partes
static Texture2D tex_button_atlas = {0};
static Texture2D tex_game_logo = {0};
static Texture2D tex_background = {0};

// Variáveis para o sistema de música e transição de áudio
static Music music_main = {0};
static Music music_win  = {0};
static Music music_lose = {0};

static Music* current_music = NULL;
static Music* next_music = NULL;    

static MusicState music_state = MUSIC_PLAYING;
static float master_volume = 0.5f; 
static float current_volume = 0.0f;
static float audio_fade_speed = 0.02f;    

/** * Controle da transição visual entre telas.
 * transition_offset define o deslocamento horizontal (slide).
 * fade_overlay_alpha define a opacidade do fundo preto.
 */
static float transition_offset = 0.0f;
static float fade_overlay_alpha = 0.0f; 
// Esta flag avisa o sistema para carregar os assets pesados enquanto a tela está escura
static int needs_to_load_gameplay = 0; 
static bool can_interact_with_ui = true;

// Variáveis para animar o logo flutuando na tela inicial
static float mouse_x = 0;
static float mouse_y = 0;
static float logo_x = 0;
static float logo_y = 0;
static float logo_time = 0;
static float logo_angle = 0;

// Estado geral da aplicação
static Button buttons[3] = {0};
static GameScreen current_screen = SCREEN_MAIN;
static GameScreen previous_screen = SCREEN_MAIN; 

static Sound sfx2 = {0};
static Sound sfx3 = {0};

#pragma endregion

#pragma region Gerenciamento de assets básicos

// Carrega a textura de fundo apenas se ela ainda não estiver na memória
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

#pragma endregion

#pragma region Interface e botões

// Diminui um pouco o botão para dar feedback de clique
void press_button(Button* button) {
    button->scale_x = Lerp(button->scale_x, 0.9f, 0.2f);
    button->scale_y = Lerp(button->scale_y, 0.9f, 0.2f);
    button->sourceRec.x = button->sprite.width / 2.0f;

    if (!button->is_pressed) {
        button->is_pressed = 1;
        PlaySound(sfx3);
    }
}

void hover_button(Button* button) {
    if (!button->is_hovered) {
        button->is_hovered = 1;
        button->scale_x = Lerp(button->scale_x, 1.1f, 0.2f);
        button->scale_y = Lerp(button->scale_y, 1.1f, 0.2f);
        button->current_color = button->hover_color;
        // Toca o som de hover apenas uma vez ao entrar no estado hover
        PlaySound(sfx2);
    }
}

// Retorna o botão ao seu estado original
void reset_button_state(Button* button) {
    button->is_hovered = 0;
    button->scale_x = Lerp(button->scale_x, 1.0f, 0.2f);
    button->scale_y = Lerp(button->scale_y, 1.0f, 0.2f);
    button->current_color = button->color;
    button->is_pressed = 0;
    button->sourceRec.x = 0.0f;
}

// Desenha o botão na tela, aplicando um leve efeito de paralaxe com o mouse
void draw_button(Button* button) {
    float _width = button->area.width * button->scale_x;
    float _height = button->area.height * button->scale_y;
    
    // Calcula posição com leve deslocamento baseado no mouse (efeito paralaxe)
    float _x = button->area.x - (_width - button->area.width) / 2.0f + mouse_x;
    float _y = button->area.y - (_height - button->area.height) / 2.0f + mouse_y;

    Rectangle destRec = { _x, _y, _width, _height };
    DrawTexturePro(button->sprite, button->sourceRec, destRec, (Vector2){0,0}, 0.0f, button->current_color);

    // Centraliza o texto no botão
    float spacing = 2.0f;
    Vector2 textSize = MeasureTextEx(GetFontDefault(), button->text, 20, spacing);
    float textX = _x + (_width - textSize.x) * 0.5f;
    float textY = _y + (_height - textSize.y) * 0.5f;
    DrawTextEx(GetFontDefault(), button->text, (Vector2){(int)textX, (int)textY}, 20, spacing, BLACK);
}

// Verifica se o usuário interagiu com o botão, mas ignora se estivermos trocando de tela
bool check_button_interaction(Button* button) {
    Vector2 mousePos = GetMousePosition();
    if (CheckCollisionPointRec(mousePos, button->area)) {
        hover_button(button);
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            press_button(button); 
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            return true;
        }
    } else {
        reset_button_state(button);
    }
    return 0;
}

void initialize_buttons() {
    if (tex_button_atlas.id == 0) {
        tex_button_atlas = LoadTexture("sources/botao.png");
    }

    for (int i = 0; i < 3; i++) {
        buttons[i].sprite = tex_button_atlas;
        // Divide a textura ao meio horizontalmente (estado normal vs pressionado/alternativo)
        buttons[i].sourceRec = (Rectangle){ 0.0f, 0.0f, (float)tex_button_atlas.width / 2.0f, (float)tex_button_atlas.height };
        
        // Organiza os botões em uma coluna vertical
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

#pragma endregion

#pragma region Sistema de audio

/**
 * Troca a música atual para uma nova, gerenciando o fade out/fade in.
 * Se já estiver tocando a música pedida, não faz nada.
 */
void switch_music_track(Music* requested_track) {
    if (current_music == requested_track && music_state != MUSIC_FADING_OUT) return;
    if (next_music == requested_track) return;

    next_music = requested_track;

    if (current_music != NULL) {
        // Se já tem música tocando, inicia o fade-out
        music_state = MUSIC_FADING_OUT;
    } else {
        // Se não tem música, toca a próxima imediatamente (com fade-in)
        current_music = next_music;
        next_music = NULL;
        if (current_music != NULL) PlayMusicStream(*current_music);
        current_volume = 0.0f;
        music_state = MUSIC_FADING_IN;
    }
}

// Função auxiliar para pegar o volume base de cada música
float get_track_multiplier(Music* track) {
    if (track == NULL) return 0.0f;
    if (track == &music_win)  return 0.5f;
    if (track == &music_lose) return 0.8f;
    if (track == &music_main) return 1.0f;

    return 1.0f;
}

// Atualiza o volume frame a frame para criar o efeito suave de transição
void update_music_system() {
    float track_mult = get_track_multiplier(current_music);
    float target_volume = master_volume * track_mult;

    if (music_state == MUSIC_FADING_OUT) {
        current_volume -= audio_fade_speed;
        
        if (current_volume <= 0.0f) {
            current_volume = 0.0f;
            if (current_music != NULL) StopMusicStream(*current_music);
            
            // Troca a música
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

        if (current_volume >= target_volume) {
            current_volume = target_volume;
            music_state = MUSIC_PLAYING;
        }
    } else if (music_state == MUSIC_PLAYING) {
        if (current_volume != target_volume) {
            current_volume = target_volume; 
        }
    }

    if (current_music != NULL) {
        SetMusicVolume(*current_music, current_volume);
        UpdateMusicStream(*current_music);
    }
}

#pragma endregion

#pragma region Lógica principal

// Prepara as variáveis para iniciar o deslizamento de tela
void start_transition_to(GameScreen target_screen) {
    previous_screen = current_screen;
    current_screen = target_screen;
    transition_offset = 0.0f; // Reinicia o contador do slide
    fade_overlay_alpha = 0.0f; 
}

/**
 * Atualiza toda a lógica do jogo (inputs, animações, regras).
 * Evita processar cliques se estivermos no meio de uma transição de tela.
 */
void update_game_logic() {
    // Calcula a posição do mouse relativa ao centro para o efeito paralaxe
    mouse_x = (screen_width / 2 - GetMouseX()) / 30;
    mouse_y = (screen_height / 2 - GetMouseY()) / 30;
    
    // Animação senoidal para o logo flutuar
    logo_time += 0.05f;
    logo_x = cos(logo_time/2) * 15 + mouse_x * 1.5f;
    logo_y = sin(logo_time) * 10 + mouse_y * 1.5f;
    logo_angle = cos(logo_time/2) * 2.5;
    
    update_music_system();
    
    // Só permite interação se a tela já terminou de deslizar
    if (can_interact_with_ui) {
        switch (current_screen) {
            case SCREEN_MAIN: {
                master_volume += (IsKeyPressed(KEY_EQUAL) - IsKeyPressed(KEY_MINUS)) * 0.05f;
                master_volume = fmaxf(0.0f, master_volume);

                switch_music_track(&music_main);
                for (int i = 0; i < 3; i++) {
                    if (check_button_interaction(&buttons[i])) {
                        if (i == BUTTON_START) {
                            // Sinalizamos que precisaremos carregar o gameplay pesado durante a transição
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
                // Chama a função de atualização do módulo gameplay.h
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

#pragma endregion

#pragma region Renderização

/**
 * Desenha a tela solicitada.
 * A transition_cam é usada para o efeito de deslizar (slide) lateral.
 */
void draw_screen(GameScreen screen, Camera2D transition_cam) {
    
    // O fundo desenhamos estático para não causar náusea com o movimento rápido
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
                int start_x = 100;
                int start_y = 100;
                int line_spacing = 30;      // Distância entre cada linha
                int section_gap = 20;       // Espaço extra antes da lista de indicadores
                int font_size = 30;
                int shadow_off = 2;         // Distância da sombra
                
                int cursor_y = start_y;
                int draw_x = start_x + (int)mouse_x;
                int shadow_x = draw_x + shadow_off;

                // Título
                DrawText("Adivinhe o jogo!", shadow_x, cursor_y + (int)mouse_y + shadow_off, font_size, BLACK);
                DrawText("Adivinhe o jogo!", draw_x, cursor_y + (int)mouse_y, font_size, WHITE);
                cursor_y += line_spacing;

                // Linha 1
                DrawText("Você começa digitando o nome de qualquer jogo.", shadow_x, cursor_y + (int)mouse_y + shadow_off, font_size, BLACK);
                DrawText("Você começa digitando o nome de qualquer jogo.", draw_x, cursor_y + (int)mouse_y, font_size, WHITE);
                cursor_y += line_spacing;

                // Linha 2
                DrawText("Após cada palpite, o jogo fornece dicas sobre o jogo correto, ", shadow_x, cursor_y + (int)mouse_y + shadow_off, font_size, BLACK);
                DrawText("Após cada palpite, o jogo fornece dicas sobre o jogo correto, ", draw_x, cursor_y + (int)mouse_y, font_size, WHITE);
                cursor_y += line_spacing;

                // Linha 3
                DrawText("comparando as características da sua tentativa com as do jogo certo.", shadow_x, cursor_y + (int)mouse_y + shadow_off, font_size, BLACK);
                DrawText("comparando as características da sua tentativa com as do jogo certo.", draw_x, cursor_y + (int)mouse_y, font_size, WHITE);
                
                // Adiciona um espaço extra antes da próxima seção
                cursor_y += (line_spacing + section_gap);

                // Título da seção
                DrawText("Indicadores de Dica:", shadow_x, cursor_y + (int)mouse_y + shadow_off, font_size, BLACK);
                DrawText("Indicadores de Dica:", draw_x, cursor_y + (int)mouse_y, font_size, WHITE);
                cursor_y += line_spacing;

                // Verde
                DrawText("  A característica está correta.", shadow_x, cursor_y + (int)mouse_y + shadow_off, font_size, BLACK);
                DrawText("  A característica está correta.", draw_x, cursor_y + (int)mouse_y, font_size, GREEN);
                cursor_y += line_spacing;

                // Amarelo
                DrawText("  A característica está parcialmente correta.", shadow_x, cursor_y + (int)mouse_y + shadow_off, font_size, BLACK);
                DrawText("  A característica está parcialmente correta.", draw_x, cursor_y + (int)mouse_y, font_size, YELLOW);
                cursor_y += line_spacing;

                // Vermelho
                DrawText("  A característica está incorreta.", shadow_x, cursor_y + (int)mouse_y + shadow_off, font_size, BLACK);
                DrawText("  A característica está incorreta.", draw_x, cursor_y + (int)mouse_y, font_size, RED);
                cursor_y += line_spacing;

                // Cinza (Setas)
                DrawText("  Setas: Indicam se o número da característica correta\n  é maior ou menor do que o seu palpite.", shadow_x, cursor_y + (int)mouse_y + shadow_off, font_size, BLACK);
                DrawText("  Setas: Indicam se o número da característica correta\n  é maior ou menor do que o seu palpite.", draw_x, cursor_y + (int)mouse_y, font_size, GRAY);
            EndMode2D();
        } break;

        case SCREEN_GAMEPLAY: {
            // Combinamos a câmera do gameplay (que faz scroll vertical) com a de transição (que faz horizontal)
            Camera2D world_cam = get_gameplay_camera();
            world_cam.offset = Vector2Add(world_cam.offset, transition_cam.offset); 
            
            BeginMode2D(world_cam);
                draw_gameplay_world();
            EndMode2D();
            
            // A UI do gameplay não deve fazer scroll vertical, apenas acompanhar o slide
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
                DrawText(msgVitoria, posX + 2, posY + 2, 80, BLACK);
                DrawText(msgVitoria, posX, posY, 80, GREEN);

                const char* msgRestart = "Pressione ENTER para voltar ao Menu";
                int tamRestart = MeasureText(msgRestart, 30);
                posX = (screen_width - tamRestart) / 2;
                posY = screen_height / 2 + 60;
                DrawText(msgRestart, posX + 2, posY + 2, 30, BLACK);
                DrawText(msgRestart, posX, posY, 30, GRAY);
            EndMode2D();
        } break;

        case SCREEN_LOSE: {
            BeginMode2D(transition_cam);
                const char* msgDerrota = "VOCE PERDEU!";
                int tamTexto = MeasureText(msgDerrota, 80);
                int posX = (screen_width - tamTexto) / 2;
                int posY = screen_height / 2 - 40;
                DrawText(msgDerrota, posX + 2, posY + 2, 80, BLACK);
                DrawText(msgDerrota, posX, posY, 80, RED);

                const char* msgRestart = "Pressione ENTER para voltar ao Menu";
                int tamRestart = MeasureText(msgRestart, 30);
                posX = (screen_width - tamRestart) / 2;
                posY = screen_height / 2 + 60;
                DrawText(msgRestart, posX + 2, posY + 2, 30, BLACK);
                DrawText(msgRestart, posX, posY, 30, GRAY);
            EndMode2D();
        } break;
    }
}

#pragma endregion

#pragma region Main

int main()
{
    // Inicialização de dados do banco de jogos
    initialize_list();
    update_list();
    
    srand(time(NULL));

    InitWindow(screen_width, screen_height, "Gamedle");
    InitAudioDevice(); 

    // Carregamento inicial de áudio
    music_main = LoadMusicStream("musicas/main_theme_2.ogg");
    music_win  = LoadMusicStream("musicas/win_theme.ogg");
    music_lose = LoadMusicStream("musicas/lose_theme.ogg");
    sfx2 = LoadSound("musicas/sfx2.ogg");
    sfx3 = LoadSound("musicas/sfx3.ogg");
    

    current_volume = 0.0f;
    master_volume = 1.0f;

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
    
    // Loop principal da janela
    while (!WindowShouldClose()) {
        update_game_logic();

        BeginDrawing();
        ClearBackground((Color){30,30,30,255}); 
        
        // Configuração das câmeras para o efeito de slide
        Camera2D transition_cam_prev = { 0 };
        transition_cam_prev.zoom = 1.0f;
        
        Camera2D transition_cam_curr = { 0 };
        transition_cam_curr.zoom = 1.0f;
        
        // Verifica se precisamos fazer a transição visual
        if (current_screen != previous_screen)
        {
            // Interpolação Linear para mover a câmera (Slide horizontal)
            transition_offset = Lerp(transition_offset, (float)screen_width, 0.05f); 
            
            // Calcula a opacidade do fundo preto baseado no progresso do slide
            // Cria um arco senoidal
            float slide_progress = transition_offset / (float)screen_width;
            fade_overlay_alpha = sinf(slide_progress * PI); 

            // Estratégia de carregamento: usamos o momento em que a tela está escura (> 40% do fade) 
            // para carregar os dados pesados do jogo, escondendo qualquer travamento.
            if (needs_to_load_gameplay && slide_progress > 0.4f) {
                
                // Forçamos um frame preto para garantir que o usuário não veja o jogo travado
                DrawRectangle(0, 0, screen_width, screen_height, BLACK);
                EndDrawing(); 
                
                // Agora podemos carregar e resetar tudo com calma
                unload_gameplay_round(); 
                load_games(); 
                load_texture();
                init_gameplay();
                for (int i = 0; i < 3; i++) {
                    reset_button_state(&buttons[i]);
                }
                
                needs_to_load_gameplay = 0;
                
                BeginDrawing(); // Voltamos a desenhar normalmente
            }

            float transition_difference = fabs((float)screen_width - transition_offset);

            if (transition_difference < 50.0f) {
                can_interact_with_ui = true;
            } else {
                can_interact_with_ui = false;
            }

            if (transition_difference < 1.0f) 
            {
                // A transição acabou
                // Resetar tudo e garantir que a câmera esteja zerada
                transition_offset = 0.0f;
                previous_screen = current_screen;
                fade_overlay_alpha = 0.0f;

                // Fprça a câmera para o centro neste frame final
                transition_cam_prev.offset = (Vector2){ 0, 0 };
                transition_cam_curr.offset = (Vector2){ 0, 0 };
            }
            else 
            {
                // A transição ainda está acontecendo
                // Calcular os offsets normalmente
                transition_cam_prev.offset = (Vector2){ -transition_offset, 0 };
                transition_cam_curr.offset = (Vector2){ screen_width - transition_offset, 0 };

                // Se houver transição (e não tiver acabado neste frame exato), desenha a tela antiga
                draw_screen(previous_screen, transition_cam_prev);
            }
        }

        // Desenha a tela atual entrando (ou parada se não houver transição)
        draw_screen(current_screen, transition_cam_curr);
        
        // Aplica o fade preto por cima de tudo se necessário
        if (fade_overlay_alpha > 0.01f) {
            float safe_alpha = Clamp(fade_overlay_alpha, 0.0f, 1.0f);
            DrawRectangle(0, 0, screen_width, screen_height, Fade(BLACK, safe_alpha));
        }

        EndDrawing();
    }

    // Limpa a memória antes de fechar o jogo
    UnloadTexture(tex_button_atlas);
    unload_gameplay_round(); 
    unload_global_assets(); 
    unload_background_texture();

    UnloadMusicStream(music_main);
    UnloadMusicStream(music_win);
    UnloadMusicStream(music_lose);
    UnloadSound(sfx2);
    UnloadSound(sfx3);

    CloseAudioDevice();
    CloseWindow();        
    return 0;
}

#pragma endregion