#ifndef GAMEPLAY_H
#define GAMEPLAY_H

#include "raylib.h" 

// --- Estrutura de Dados Compartilhada ---

typedef struct
{
    char name[50];
    int year;
    char origin[50];
    char genre[3][50];
    char theme[50];
    char gamemode[3][50];
    char platform[3][50];
    char phrase[200];
    Texture2D flag_texture;
    Texture2D logo_texture;
} GAME;


// --- Variáveis Globais Públicas ---

extern int screen_width;
extern int screen_height;

// --- Funções Públicas ---

// Funções de inicialização
void load_list(void);        // Chamada 1x no início do programa
void load_games(void);       // Chamada toda vez que um novo jogo começa
void load_texture(void);     // Chamada toda vez que um novo jogo começa

// Funções de loop (Controladas pelo menu)
void init_gameplay(void);    // Prepara os assets da rodada (shader, etc)
void update_gameplay(void);  // Atualiza um frame da lógica do jogo
bool is_gameplay_finished(void); // Verifica se o jogo acabou (win ou ESC)

// Funções de desenho (divididas)
Camera2D get_gameplay_camera(void); // Permite ao main pegar a câmera (com scroll/shake)
void draw_gameplay_world(void);     // Desenha o MUNDO do jogo (tentativas, hints)
void draw_gameplay_ui(void);        // Desenha a UI do jogo (corações, busca)

// Funções de limpeza
void unload_gameplay_round(void); // Limpa os assets da rodada
void unload_global_assets(void);  // Limpa os assets da load_list

#endif // GAMEPLAY_H