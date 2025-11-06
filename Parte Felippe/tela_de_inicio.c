/**
 * @file hello.c
 * @author Muriel Godoi (muriel@utfpr.edu.br)
 * @brief Hello World in Raylib 5.5
 * @version 0.1
 * @date 2024-11-27
 * 
 * @copyright Copyright (c) 2024S
 * 
 */

#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>


/*
- Sprites ao deixar o cursor em cima ou não
- Escala variável
*/

typedef enum {
    BOTAO_INICIAR,
    BOTAO_COMO_JOGAR,
    BOTAO_SAIR
} Botoes;

typedef enum {
    TELA_INICIAL,
    TELA_COMO_JOGAR,
} Telas;

typedef struct {
    Texture2D sprite;
    Rectangle area;
    Vector2 posicao;
    float escalaX;
    float escalaY;

    Color cor;
    Color corAtual;
    Color hoverCor;
    int hover;
    int pressionado;

    const char* texto;
} Botao;

void botaoPress(Botao* botao) {
    botao->pressionado = 1;
    botao->escalaX = 0.9f;
    botao->escalaY = 0.9f;
}

void botaoHover(Botao* botao) {
    if (!botao->hover) {
        botao->hover = 1;
        botao->escalaX = Lerp(botao->escalaX, 1.1f, 0.2f);
        botao->escalaY = Lerp(botao->escalaY, 1.1f, 0.2f);
        botao->corAtual = botao->hoverCor;
    }
}

void resetBotaoState(Botao* botao) {
    botao->hover = 0;
    botao->escalaX = Lerp(botao->escalaX, 1.0f, 0.2f);
    botao->escalaY = Lerp(botao->escalaY, 1.0f, 0.2f);
    botao->corAtual = botao->cor;
    botao->pressionado = 0;
}

void desenharBotao(Botao* botao) {
    float _width = botao->area.width * botao->escalaX;
    float _height = botao->area.height * botao->escalaY;
    float _x = botao->area.x - (_width - botao->area.width) / 2.0f;
    float _y = botao->area.y - (_height - botao->area.height) / 2.0f;
    DrawRectangle(_x, _y, _width, _height, botao->corAtual);

    float spacing = 2.0f;
    Vector2 textSize = MeasureTextEx(GetFontDefault(), botao->texto, 20, spacing);
    float textX = _x + (_width - textSize.x) * 0.5f;
    float textY = _y + (_height - textSize.y) * 0.5f;
    DrawTextEx(GetFontDefault(), botao->texto, (Vector2){(int)textX, (int)textY}, 20, spacing, BLACK);
}

bool checarBotao(Botao* botao)
{
    Vector2 mousePos = GetMousePosition();

    if (CheckCollisionPointRec(mousePos, botao->area))
    {
        botaoHover(botao);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            botaoPress(botao);

            return 1;
        }
    }

    return 0;
}

static Botao botoes[3] = {0};
static Telas telaAtual = TELA_INICIAL;
void inicializarBotoes() {
    for (int i = 0; i < 3; i++) {
        botoes[i].sprite = (Texture2D){0};
        botoes[i].area = (Rectangle){
            120.0f,
            80 + i * 100.0f,
            180.0f,
            64.0f
        };

        botoes[i].posicao = (Vector2){
            botoes[i].area.x + botoes[i].area.width / 2.0f,
            botoes[i].area.y + botoes[i].area.height / 2.0f
        };

        botoes[i].escalaX = 1.0f;
        botoes[i].escalaY = 1.0f;
        botoes[i].cor = Fade(SKYBLUE, 0.95f);
        botoes[i].hoverCor = Fade(LIGHTGRAY, 0.95f);
        botoes[i].corAtual = botoes[i].cor;
        botoes[i].hover = 0;
        botoes[i].pressionado = 0;
    }

    // Finaliza inicialização
    botoes[BOTAO_INICIAR].texto = "Jogar";
    botoes[BOTAO_COMO_JOGAR].texto = "Como jogar";
    botoes[BOTAO_SAIR].texto = "Sair";
}
void atualizarBotoes() {
    switch (telaAtual) {
        case TELA_INICIAL: {
            for (int i = 0; i < 3; i++) {
                resetBotaoState(&botoes[i]);
                if (checarBotao(&botoes[i])) {
                    if (i == BOTAO_INICIAR) {
                        // TODO: iniciar a lógica do jogo (trocar para tela de jogo quando existir)
                    } else if (i == BOTAO_COMO_JOGAR) {
                        telaAtual = TELA_COMO_JOGAR;
                    } else if (i == BOTAO_SAIR) {
                        CloseWindow();
                        exit(1);
                    }
                }
            }
        } break;

        case TELA_COMO_JOGAR: {
            // voltar para a tela inicial com ESC, ENTER ou clique
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                telaAtual = TELA_INICIAL;
            }
        } break;
    }
}
void desenharTelas() {
    switch (telaAtual) {
         case TELA_INICIAL: {
            for (int i = 0; i < 3; i++) {
                desenharBotao(botoes + i);
            }
        } break;

        case TELA_COMO_JOGAR: {
            DrawText("adivnha jgo e dai acerta e dai ve e dai tentar\n denovo e dai ate acertar :)", 100, 100, 20, WHITE);
        } break;
    }
}

int main(void)
{
    const int larguraTela = 800;
    const int alturaTela = 450;

    InitWindow(larguraTela, alturaTela, "Hello World Raylib 5.5");
    SetTargetFPS(60);

    inicializarBotoes();
    while (!WindowShouldClose()) {
        atualizarBotoes();

        BeginDrawing();
        ClearBackground(BLACK);
        
        desenharTelas();

        EndDrawing();
    }

    CloseWindow();        
    return 0;
}