#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

void divisao(char div[3][50]) {
    int index = 1;
    for (int i = 1; i < 3; i++) strcpy(div[i], "");

    if (strlen(div[0]) != strcspn(div[0], "/")) {
        char* token = strtok(div[0], "/");

        while (token != NULL) {
            strcpy(div[index], token);

            token = strtok(NULL, "/");
        }
    } else index = 0;
}

int igualdade(char a[3][50], char b[3][50]) {
    int igual = 0;
    int contA = 3;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (strcmp(a[i], "") == 0) {
                contA--;
                break;
            }
            if (strcmp(b[j], "") == 0) break;
            if ((strcmp(a[i],b[j]) == 0) != 0) igual++;
        }
    }
    if (contA == igual) return 1;
    else if (igual > 0) return 2;
    return 0;
}

typedef struct
{
    char nome[50];
    int ano;
    char origem[50];
    char genero[3][50];
    char tema[50];
    char gamemode[3][50];
    char plataforma[3][50];
} GAME;

int main() {
    srand(time(NULL));

    #pragma region CARREGAR LISTA
    FILE *lista;
    GAME jogos[100];

    lista = fopen("list.txt", "r");
    if (lista == NULL){
        perror("LISTA NÃO CARREGADA\n");
        exit(1);
    }
    for (int i = 0; fscanf(lista, "%49[^;];%d;%49[^;];%49[^;];%49[^;];%49[^;];%49[^;];\n", jogos[i].nome, &jogos[i].ano, jogos[i].origem, jogos[i].genero[0], jogos[i].tema, jogos[i].gamemode[0], jogos[i].plataforma[0]) == 7; i++) {
        divisao(jogos[i].genero);
        divisao(jogos[i].gamemode);
        divisao(jogos[i].plataforma);
        for (int j = 0; j < 3; j++) printf("%s: %s\n", jogos[i].nome,jogos[i].plataforma[j]);
    }

    fclose(lista);
    #pragma endregion

    #pragma region JANELA
    int largura = 1280;
    int altura = 720;
    InitWindow(largura, altura, "GAMEPLAY");
    #pragma endregion

    #pragma region JOGOS
    char entrada[50] = "";

    GAME *tentativas;
    int t = -1;
    int tSelec = 0;

    GAME pesquisa[5];
    int selecionado = 0;

    tentativas = calloc(t+1, sizeof(GAME));

    GAME correto;
    int c = (rand() % (100));
    correto = jogos[0];
    // correto = jogos[c];

    printf("%s\n", correto.nome);
    #pragma endregion 

    int index = 0;
    int capslock = 0;

    Image flechaIm = LoadImage("sources/flecha.png");
    Texture2D flecha = LoadTextureFromImage(flechaIm);

    while (!WindowShouldClose()) {
        int maxSelec = 4;
        for (int i = 0; i < 5; i++) if (strcmp(pesquisa[i].nome, "") == 0) maxSelec--;

        //Setas
        if (IsKeyPressed(KEY_DOWN)) {
            selecionado++;
            if (selecionado > maxSelec) selecionado = 0;
        }
        if (IsKeyPressed(KEY_UP)) {
            selecionado--;
            if (selecionado < 0) selecionado = maxSelec;
        }

        if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) {
            tSelec--;
        }

        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) {
            tSelec++;
            if (tSelec > t) tSelec = t;
        }
        if (tSelec <= 0) tSelec = 0;

        //Escrever a entrada do usuário
        int tecla = GetKeyPressed();
        if ((tecla >= 32) && (tecla <= 125) && index < 50) {
            if (capslock == 1 || (int)IsKeyDown(KEY_LEFT_SHIFT) == 1 || (int)IsKeyDown(KEY_RIGHT_SHIFT) == 1) entrada[index] = (char)tecla;
            else entrada[index] = tolower((char)tecla);
            
            selecionado = 0;
            index++;
            entrada[index] = '\0';
            tecla = GetCharPressed();
        }

        if ((IsKeyPressedRepeat(KEY_BACKSPACE) || IsKeyPressed(KEY_BACKSPACE))&& index > 0) {
            if (IsKeyDown(KEY_LEFT_CONTROL)) {
                entrada[0] = '\0';
                index = 0;
            }
            index--;
            entrada[index] = '\0';
        }

        //Pesquisa
        for (int l = 0; l < 5; l++) strcpy(pesquisa[l].nome, "");
        if (entrada[0] != '\0') {
            for (int l = 0, j = 0; l < 100 && j < 5; l++) {
                char a[strlen(entrada)+1];
                for (int k = 0; k <= strlen(entrada); k++) {
                    a[k] = jogos[l].nome[k];
                }
                a[strlen(entrada)] = '\0';

                if (strcasecmp(entrada, a) == 0)  {
                    pesquisa[j] = jogos[l];
                    j++;
                }
            }
        }

        //Tentativa
        if (IsKeyPressed(KEY_ENTER) && strcmp(pesquisa[selecionado].nome, "") != 0) {
            t++;
            tSelec = t;
            // printf("%d\n", t);
            tentativas = realloc(tentativas, (t+1) * sizeof(GAME));
            tentativas[t] = pesquisa[selecionado];
            strcpy(entrada, " ");
            for (int i = 0; i < 100; i++) if (strcmp(pesquisa[selecionado].nome, jogos[i].nome) == 0) strcpy(jogos[i].nome, "");
            index = 0;

            if (strlen(tentativas[t].nome) >= 10) {
                for (int i = strlen(tentativas[t].nome)/3; i < strlen(tentativas[t].nome); i++) {
                    if (tentativas[t].nome[i] == ' ') {
                        tentativas[t].nome[i] = '\n';
                        break;
                    }

                }
            }
        }

        //Desenhar na tela
        BeginDrawing();

        ClearBackground((Color){30,30,30,255});
        DrawText(entrada, 20, 30/2, 30, RAYWHITE);
        for (int j = 0; j < 5; j++) {
            DrawText(pesquisa[j].nome, 20, 30*(2+1.5*j), 30, (selecionado == j ? BLUE : PINK));
        }

        char textoTentativas[20];
        sprintf(textoTentativas,"%d", tSelec+1);
        if (t!=-1) DrawText(textoTentativas, largura-MeasureText(textoTentativas, 30)-30, 15, 30, WHITE);
        if (strcmp(tentativas[tSelec].nome, "") != 0) {
            GAME jogoSelec = tentativas[tSelec];
            
            DrawText(jogoSelec.nome, 20, 315, 30, RED);

            int pulo = 20;

            //ANO
            char ano[5];
            sprintf(ano,"%d",jogoSelec.ano);
            if (jogoSelec.ano == correto.ano) DrawText(ano, 20, 400, 30, GREEN);
            else {
                // if (jogoSelec.ano > correto.ano) flecha. = -16;
                // else flecha.height = 16;
                DrawText(ano, pulo, 400, 30, RED);
                DrawTexturePro(flecha, (Rectangle){0, 0, 15, 14}, (Rectangle){MeasureText(ano, 30) + 35, 413, 15, 14}, (Vector2){7,7}, (jogoSelec.ano > correto.ano ? 180 : 0), WHITE);    
                // DrawTextureEx(flecha, (Vector2){MeasureText(ano, 30) + 35, 413},  (jogoSelec.ano > correto.ano ? 180 : 0), 1, WHITE);
            }

            pulo += MeasureText(ano, 30) + 33;

            //ORIGEM
            DrawText(jogoSelec.origem, pulo, 400, 30, (strcmp(jogoSelec.origem, correto.origem) == 0) ? GREEN : RED);
            
            pulo += MeasureText(jogoSelec.origem, 30) + 20;

            //GENERO
            char genero[50];
            int gIgualdade = igualdade(jogoSelec.genero, correto.genero);
            sprintf(genero, "%s\n%s", jogoSelec.genero[0], jogoSelec.genero[1]);
            if (gIgualdade == 1) DrawText(genero, pulo, 400, 30, GREEN);
            else DrawText(genero, pulo, 400, 30, (gIgualdade == 0) ? RED : YELLOW);

            pulo += MeasureText(genero, 30) + 20;

            //TEMA
            if (strlen(jogoSelec.tema) != strcspn(jogoSelec.tema, " ")) {
                jogoSelec.tema[strcspn(jogoSelec.tema, " ")] = '\n';
            }

            DrawText(jogoSelec.tema, pulo, 400, 30, (strcmp(jogoSelec.tema, correto.tema) == 0) ? GREEN : RED);
            
            pulo += MeasureText(jogoSelec.tema, 30) + 20;

            //GAMEMODE
            char gamemode[50];
            int gaIgualdade = igualdade(jogoSelec.gamemode, correto.gamemode);
            sprintf(gamemode, "%s\n%s", jogoSelec.gamemode[0], jogoSelec.gamemode[1]);
            if (gaIgualdade == 1) DrawText(gamemode, pulo, 400, 30, GREEN);
            else DrawText(gamemode, pulo, 400, 30, (gaIgualdade == 0) ? RED : YELLOW);

            pulo += MeasureText(gamemode, 30) + 20;

            int tamanhoNome = 15 + strcspn(jogoSelec.nome, "\n");
            char imagem[tamanhoNome];
            sprintf(imagem, "capas/capa_%s.jpg", correto.nome); 

            // Image image = LoadImage(imagem);
            // if (image.format != NULL && t > 4) {
            //     texture = LoadTextureFromImage(image);
            //     UnloadImage(image);
            //     DrawTexture(texture, 530, 20, WHITE);
            // }

            //PLATAFORMA
            // char plataforma[100];
            // int pIgualdade = igualdade(jogoSelec.plataforma, correto.plataforma);
            // sprintf(plataforma, "%s\n%s\n%s", jogoSelec.plataforma[0], jogoSelec.plataforma[1], jogoSelec.plataforma[2]);
            // if (pIgualdade == 1) DrawText(plataforma, pulo, 360, 30, GREEN);
            // else DrawText(plataforma, pulo, 360, 30, (pIgualdade == 0) ? RED : YELLOW);

            // pulo += MeasureText(plataforma, 30) + 10;
        }

        //BOTÃO
        Vector2 mousePos = GetMousePosition();
        Vector2 botao = {largura-100, 30};

        DrawCircle(botao.x, botao.y, 15, (t > 3) ? WHITE : RED);


        float distance = sqrt(pow(mousePos.x - botao.x, 2) + pow(mousePos.y - botao.y, 2));
        if (distance < 15 && t > 3)
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                printf("a\n");
            }
        }

        EndDrawing();
    }

    UnloadTexture(flecha);
    UnloadImage(flechaIm);
    free(tentativas);
    return 0;
}