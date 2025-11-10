#include "raylib.h"
#include <string.h>

typedef enum GameState {
    STATE_JOGANDO,
    STATE_VITORIA
    
} GameState;

int main(void) {

    const int screenWidth = 800;
    const int screenHeight = 450;
    InitWindow(screenWidth, screenHeight, "Tela Vitoria");

    // --- Variáveis do Jogo ---
    const char* palavraSecreta = "GOAT";      // A palavra que precisa ser acertada
    char inputUsuario[100] = {0};               // Onde guardamos o que o usuário digita
    int contadorChars = 0;                      // Controla a posição no array 'inputUsuario'
    bool palavraErrada = false;                 // Feedback se o usuário errar

    GameState estadoAtual = STATE_JOGANDO;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        
        switch (estadoAtual) {
            
            case STATE_JOGANDO: {

                int charCode = GetCharPressed();

                if ((charCode >= 32) && (charCode <= 125) && (contadorChars < 99)) {
                    inputUsuario[contadorChars] = (char)charCode;
                    inputUsuario[contadorChars + 1] = '\0'; 
                    contadorChars++;
                    palavraErrada = false; 
                }

                if (IsKeyPressed(KEY_BACKSPACE)) {
                    if (contadorChars > 0) {
                        contadorChars--;
                        inputUsuario[contadorChars] = '\0';
                    }
                }


                if (IsKeyPressed(KEY_ENTER)) {

                    if (strcmp(inputUsuario, palavraSecreta) == 0) {

                        estadoAtual = STATE_VITORIA;
                    } else {

                        palavraErrada = true;
                        contadorChars = 0;
                        inputUsuario[0] = '\0';
                    }
                }
            } break;

            case STATE_VITORIA: {

                if (IsKeyPressed(KEY_ENTER)) {

                    estadoAtual = STATE_JOGANDO;
                    contadorChars = 0;
                    inputUsuario[0] = '\0';
                    palavraErrada = false;
                }
            } break;
        }


        BeginDrawing();
        ClearBackground(RAYWHITE); 

        switch (estadoAtual) {

            case STATE_JOGANDO: {
                DrawText("Digite a palavra secreta e aperte ENTER:", 50, 150, 20, DARKGRAY);


                DrawText(inputUsuario, 50, 200, 40, BLACK);
                

                if ((GetTime() * 2.0f) - (int)(GetTime() * 2.0f) > 0.5f) {
                     DrawText("_", 50 + MeasureText(inputUsuario, 40), 200, 40, MAROON);
                }

                if (palavraErrada) {
                    DrawText("Palavra errada! Tente novamente.", 50, 250, 20, RED);
                }
                
                DrawText("(Dica: é o nome da biblioteca)", 50, 300, 10, GRAY);

            } break;

            case STATE_VITORIA: {

                

                const char* msgVitoria = "VOCE VENCEU!";
                int tamTexto = MeasureText(msgVitoria, 80); // Mede o tamanho do texto
                int posX = (screenWidth - tamTexto) / 2;    // Calcula a posição X
                int posY = screenHeight / 2 - 40;           // Posição Y (um pouco acima do meio)

                DrawText(msgVitoria, posX, posY, 80, GREEN); // Desenha o texto de vitória!

                const char* msgRestart = "Pressione ENTER para jogar novamente";
                int tamRestart = MeasureText(msgRestart, 20);
                posX = (screenWidth - tamRestart) / 2;
                posY = screenHeight / 2 + 60; // Um pouco abaixo

                DrawText(msgRestart, posX, posY, 20, DARKGRAY);

            } break;
        }

        EndDrawing();
    }

    // --- Finalização ---
    CloseWindow();
    return 0;
    
}