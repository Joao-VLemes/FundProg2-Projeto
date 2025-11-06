#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

void divisao(char div[3][50]) {
    int index = 0;
    for (int i = 1; i < 3; i++) strcpy(div[i], "");
    if (strlen(div[0]) != strcspn(div[0], "/")) {
        char* token = strtok(div[0], "/");

        while (token != NULL) {
            strcpy(div[index], token);
            index++;
            token = strtok(NULL, "/");
        }
    } 
}

int igualdade(char a[3][50], char b[3][50]) {
    int igual = 0;
    int contA = 0; int contB = 0;

    for (int i = 0; i < 3; i++) {
        if (strlen(b[i]) == 0) continue; 
        contB++;
    }   

    for (int i = 0; i < 3; i++) {
        if (strlen(a[i]) == 0) continue; 
        contA++;

        bool achou = false;
        for (int j = 0; j < 3; j++) {
            if (strlen(b[j]) == 0) continue; 
            if ((strcmp(a[i],b[j]) == 0)) {
                achou = true;
                break;
            }
        }
        if (achou) igual++;
    }
    // printf("ContB: %d contA: %d\n", contB, contA);
    if (contA == igual && contA == contB) return 1;
    if (igual > 0) return 2;
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
    char frase[200];
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

    #pragma region CARREGAR FRASES

    FILE *listaFrases;

    listaFrases = fopen("frases.txt", "r");
    if (listaFrases == NULL){
        perror("LISTA NÃO CARREGADA\n");
        exit(1);
    }

    for (int i = 0; fscanf(listaFrases, " %199[^\n]\n", jogos[i].frase) == 1; i++) {
        // printf("%s: %s\n", jogos[i].nome,jogos[i].frase);
    }

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
    // correto = jogos[0];
    correto = jogos[c];

    printf("%s\n", correto.nome);
    #pragma endregion 

    int index = 0;
    int capslock = 0;

    bool ajuda1 = false;
    bool ajuda2 = false;
    bool ajuda3 = false;

    //Flecha que indica maior ou menor ano
    Image flechaIm = LoadImage("sources/flecha.png");
    Texture2D flecha = LoadTextureFromImage(flechaIm);

    //Coração de vida
    Image coracaoIm = LoadImage("sources/coracao.png");
    Texture2D coracao = LoadTextureFromImage(coracaoIm);

    //Bandeira
    Image bandeiraIM = LoadImage(TextFormat("sources/flags/%s.png", correto.origem));
    Texture2D bandeira = LoadTextureFromImage(bandeiraIM);

    //Capa
        //Nome do arquivo da capa
    char nomeJogo[40];

    // for (int i = 0; i < 100; i++) {
    //     strcpy(nomeJogo, jogos[i].nome);

    //     for (int i = 0; i < strlen(nomeJogo); i++) {
    //         if (nomeJogo[i] == ' ' || nomeJogo[i] == ',' || nomeJogo[i] == '-' || nomeJogo[i] == ':'){ 
    //             if (i < 5) {
    //                 nomeJogo[i] = '_';
    //             } else {
    //                 nomeJogo[i] = '\0';
    //                 break;
    //             }
    //         }
    //     }

    //     printf("%s\n", nomeJogo);
    // }

    strcpy(nomeJogo, correto.nome);
    for (int i = 0; i < (int)strlen(nomeJogo); i++) {
        if (nomeJogo[i] == ' ' || nomeJogo[i] == ',' || nomeJogo[i] == '-' || nomeJogo[i] == ':'){ 
            if (i < 5) {
                nomeJogo[i] = '_';
            } else {
                nomeJogo[i] = '\0';
                break;
            }
        }
    }

        //Carregar a capa
    char imagem[50];
    sprintf(imagem, "capas/capa_%s.jpg", nomeJogo); 

    Image image = LoadImage(imagem);
    Texture2D capa = capa = LoadTextureFromImage(image);
    UnloadImage(image);

    //Shader para borrar
    Shader borrar = LoadShader(0, "capas/blur.fs");
    int viewSizeLoc = GetShaderLocation(borrar, "viewSize");
    RenderTexture2D objetoBorrado = LoadRenderTexture(capa.width, capa.height);
    float tamanhoText[2] = { (float)capa.width, (float)capa.height };
    SetShaderValue(borrar, viewSizeLoc, tamanhoText, SHADER_UNIFORM_VEC2);

    while (!WindowShouldClose()) {
        // printf("%s\n", nomeJogo);

        //Borrar a imagem

        BeginTextureMode(objetoBorrado);     
            ClearBackground(BLANK);
            
            BeginShaderMode(borrar);  
                DrawTexture(capa, 0,0, WHITE);
            EndShaderMode();                

        EndTextureMode();

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
                for (int k = 0; k <= (int)strlen(entrada); k++) {
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
                for (int i = (int)strlen(tentativas[t].nome)/3; i < (int)strlen(tentativas[t].nome); i++) {
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
        DrawText(entrada, largura/2-MeasureText(entrada, 30)/2, 30/2, 30, RAYWHITE);
        for (int j = 0; j < 5; j++) {
            DrawText(pesquisa[j].nome, largura/2-MeasureText(pesquisa[j].nome, 30)/2, 30*(2+1.5*j), 30, (selecionado == j ? BLUE : PINK));
        }

        if (t!=-1) DrawText(TextFormat("%d", tSelec+1), largura-MeasureText(TextFormat("%d", tSelec+1), 30)-30, 15, 30, WHITE);
        if (strcmp(tentativas[tSelec].nome, "") != 0) {
            GAME jogoSelec = tentativas[tSelec];
            
            DrawText(jogoSelec.nome, 20, altura - 180, 30, PINK);

            int pulo = 20;

            //ANO
            if (jogoSelec.ano == correto.ano) DrawText(TextFormat("%d",jogoSelec.ano), 20, altura - 80, 30, GREEN);
            else {
                // if (jogoSelec.ano > correto.ano) flecha. = -16;
                // else flecha.height = 16;
                DrawText(TextFormat("%d",jogoSelec.ano), pulo, altura - 80, 30, RED);
                DrawTexturePro(flecha, (Rectangle){0, 0, 15, 14}, (Rectangle){MeasureText(TextFormat("%d",jogoSelec.ano), 30) + 35, altura - 67, 15, 14}, (Vector2){7,7}, (jogoSelec.ano > correto.ano ? 180 : 0), WHITE);    
                // DrawTextureEx(flecha, (Vector2){MeasureText(ano, 30) + 35, 413},  (jogoSelec.ano > correto.ano ? 180 : 0), 1, WHITE);
            }

            pulo += MeasureText(TextFormat("%d",jogoSelec.ano), 30) + 33;

            //ORIGEM
            UnloadImage(bandeiraIM);
            UnloadTexture(bandeira);
            bandeiraIM = LoadImage(TextFormat("sources/flags/%s.png", jogoSelec.origem));
            bandeira = LoadTextureFromImage(bandeiraIM);

            DrawTexturePro(bandeira, (Rectangle){0, 0, (float)bandeira.width, (float)bandeira.height}, (Rectangle){pulo, altura - 80, bandeira.width*2, bandeira.height*2}, (Vector2){0,0}, 0, WHITE);
            DrawRectangleLinesEx((Rectangle){pulo, altura - 80, bandeira.width*2, bandeira.height*2}, 2.5, (strcmp(jogoSelec.origem, correto.origem) == 0) ? GREEN : RED);
            // DrawText(jogoSelec.origem, pulo, altura - 80, 30, (strcmp(jogoSelec.origem, correto.origem) == 0) ? GREEN : RED);

            pulo += bandeira.width*2 + 20;

            //GENERO
            int gIgualdade = igualdade(correto.genero, jogoSelec.genero);
            if (gIgualdade == 1) DrawText(TextFormat("%s\n%s", jogoSelec.genero[0], jogoSelec.genero[1]), pulo, altura - 80, 30, GREEN);
            else DrawText(TextFormat("%s\n%s", jogoSelec.genero[0], jogoSelec.genero[1]), pulo, altura - 80, 30, (gIgualdade == 0) ? RED : YELLOW);

            pulo += MeasureText(TextFormat("%s\n%s", jogoSelec.genero[0], jogoSelec.genero[1]), 30) + 20;

            // //TEMA
            if (strlen(jogoSelec.tema) != strcspn(jogoSelec.tema, " ")) {
                jogoSelec.tema[strcspn(jogoSelec.tema, " ")] = '\n';
            }

            DrawText(jogoSelec.tema, pulo, altura - 80, 30, (strcmp(jogoSelec.tema, correto.tema) == 0) ? GREEN : RED);
            
            pulo += MeasureText(jogoSelec.tema, 30) + 20;

            //GAMEMODE
            int gaIgualdade = igualdade(correto.gamemode, jogoSelec.gamemode);
            if (gaIgualdade == 1) DrawText(TextFormat("%s\n%s", jogoSelec.gamemode[0], jogoSelec.gamemode[1]), pulo, altura - 80, 30, GREEN);
            else DrawText(TextFormat("%s\n%s", jogoSelec.gamemode[0], jogoSelec.gamemode[1]), pulo, altura - 80, 30, (gaIgualdade == 0) ? RED : YELLOW);

            pulo += MeasureText(TextFormat("%s\n%s", jogoSelec.gamemode[0], jogoSelec.gamemode[1]), 30) + 20;

            //PLATAFORMA
            int pIgualdade = igualdade(correto.plataforma, jogoSelec.plataforma);
            int qntPlata = 2;
            for (int i = 0; i < 3; i++) if (strcmp(jogoSelec.plataforma[i], "") == 0) qntPlata--;
            if (pIgualdade == 1) DrawText(TextFormat("%s\n%s\n%s", jogoSelec.plataforma[0], jogoSelec.plataforma[1], jogoSelec.plataforma[2]), pulo, altura - 80 - 30*qntPlata/2, 30, GREEN);
            else DrawText(TextFormat("%s\n%s\n%s", jogoSelec.plataforma[0], jogoSelec.plataforma[1], jogoSelec.plataforma[2]), pulo, altura - 80 - 30*qntPlata/2, 30, (pIgualdade == 0) ? RED : YELLOW);

            pulo += MeasureText(TextFormat("%s\n%s\n%s", jogoSelec.plataforma[0], jogoSelec.plataforma[1], jogoSelec.plataforma[2]), 30) + 10;
        }

        //BOTÃO
        // Vector2 mousePos = GetMousePosition();
        // Vector2 botao1 = {largura-100, 30};
        // Vector2 botao2 = {largura-100, 75};
        // Vector2 botao3 = {largura-100, 120};

        // if (t > 3) DrawCircle(botao1.x, botao1.y, 15, (ajuda1) ? BLUE : WHITE);
        // if (t > 6) DrawCircle(botao2.x, botao2.y, 15, (ajuda2) ? BLUE : WHITE);
        // if (t > 8) DrawCircle(botao3.x, botao3.y, 15, (ajuda3) ? BLUE : WHITE);


        // float dist1 = sqrt(pow(mousePos.x - botao1.x, 2) + pow(mousePos.y - botao1.y, 2));
        // float dist2 = sqrt(pow(mousePos.x - botao2.x, 2) + pow(mousePos.y - botao2.y, 2));
        // float dist3 = sqrt(pow(mousePos.x - botao3.x, 2) + pow(mousePos.y - botao3.y, 2));

        // if (dist1 < 15 && t > 3) if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ajuda1 = true;
        // if (dist2 < 15 && t > 6) if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ajuda2 = true;
        // if (dist3 < 15 && t > 8) if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ajuda3 = true;

        //Vidas
        int vidas = 20-t-1;

        for (int i = 0; i < (int)ceil((double)vidas/2); i++) {
            int sprite = 0;
            Color color = WHITE;
            if (i == vidas/2 && vidas%2 != 0) sprite = 1;
            if (i == 15 || i == 7 || i == 3 ) color = (Color){240,255,0,(((int)GetTime() % 2 == 0 ) ? 255 : 0)};
            DrawTexturePro(coracao, (Rectangle){8*sprite,8*sprite,8,8}, (Rectangle){24*i,0,32,32}, (Vector2){0,0}, 0, color);
        }

        if (vidas <= 15) ajuda1 = true;
        if (vidas <= 7) ajuda2 = true;
        if (vidas <= 3) ajuda3 = true;

        //Ajudas
        if (ajuda1) printf("%s\n", correto.frase);
        if (ajuda2) {
            Rectangle sourceRect = { 0, 0, (float)objetoBorrado.texture.width, (float)-objetoBorrado.texture.height };
            Vector2 destPos = { largura-capa.width-20, altura-capa.height-20};
            DrawTextureRec(objetoBorrado.texture, sourceRect, destPos, WHITE);
        }
        if (ajuda3) {
            DrawTexture(capa, largura-capa.width-20, altura-capa.height-20, WHITE);
        }

        DrawRectangleLines(largura-capa.width-20, altura-capa.height-20, objetoBorrado.texture.width, objetoBorrado.texture.height, DARKBLUE);

        EndDrawing();
    }

    UnloadRenderTexture(objetoBorrado);
    UnloadShader(borrar);
    UnloadTexture(flecha);
    UnloadImage(flechaIm);
    UnloadTexture(bandeira);
    UnloadImage(bandeiraIM);
    UnloadTexture(coracao);
    UnloadImage(coracaoIm);
    UnloadTexture(capa);
    free(tentativas);
    return 0;
}