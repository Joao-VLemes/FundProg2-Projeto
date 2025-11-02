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

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Inicialização
    //--------------------------------------------------------------------------------------
    const int larguraTela= 800;
    const int alturaTela = 450;

    InitWindow(larguraTela, alturaTela, "Hello World Raylib 5.5");

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Loop principal do jogo
    while (!WindowShouldClose())    // Detecta o botão de fechar a janela ou a tecla ESC
    {

        BeginDrawing();

            ClearBackground(RAYWHITE);

            DrawText("Parabéns! Você acertou o jogo", 120, 200, 20, BLACK);

        EndDrawing();

    }

    CloseWindow();        

    return 0;
}