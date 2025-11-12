#include "gameplay.h" // Inclui nossas próprias definições
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

// --- Variáveis Globais (Públicas) ---
int screen_width = 1280;
int screen_height = 720;
bool win = false; // 'win' precisa ser público para o menu checar

// --- Variáveis Estáticas Globais (Privadas deste arquivo) ---
static bool shaking = false;
static bool moving_up = false;
static float shake_time = 0;
static float shake_power = 0;

static Texture2D heart_texture;
static Texture2D arrow_texture;
static Texture2D cover_texture;

static GAME games[100];
static GAME correct_game;
static GAME search_results[100];
static GAME *attempts = NULL; // Importante inicializar com NULL

static int attempt_count = -1;
static int selected_attempt_index = 0;

static int max_search_results = 0; // Será resetado para -1 no init/search
static int selected_search_index = 0;

// Variáveis de estado do Gameplay
static char user_input[50] = "";
static int input_index = 0;
static bool hint_1 = false;
static bool hint_2 = false;
static bool hint_3 = false;
static bool game_finished = false;

// Assets da rodada
static Shader blur_shader = {0};
static RenderTexture2D blurred_object_rt = {0};
static Camera2D game_camera = {0};
static Vector2 baseCameraOffset = {0};


// --- Funções de "Helper" (Privadas) ---

// Divide a string em str_array[0] por "/" e armazena as partes em str_array[1] e str_array[2]
static void split_string(char str_array[3][50]) {
    int index = 0;
    for (int i = 1; i < 3; i++) strcpy(str_array[i], "");
    // Verifica se o caractere '/' existe
    if (strcspn(str_array[0], "/") == strlen(str_array[0])) {
        return; // Não há '/', nada a fazer
    }
    
    // Cria uma cópia temporária para tokenizar, pois strtok modifica a string
    char temp_str[50];
    strcpy(temp_str, str_array[0]);
    
    char* token = strtok(temp_str, "/");
    while (token != NULL && index < 3) {
        strcpy(str_array[index], token);
        index++;
        token = strtok(NULL, "/");
    }
}


// Verifica a igualdade ou correspondência parcial entre dois arrays de strings
static int check_equality(char a[3][50], char b[3][50]) {
    int equal_count = 0;
    int count_a = 0; int count_b = 0;

    for (int i = 0; i < 3; i++) {
        if (strlen(b[i]) == 0) continue; 
        count_b++;
    }   

    for (int i = 0; i < 3; i++) {
        if (strlen(a[i]) == 0) continue; 
        count_a++;

        bool found = false;
        for (int j = 0; j < 3; j++) {
            if (strlen(b[j]) == 0) continue; 
            if ((strcmp(a[i],b[j]) == 0)) {
                found = true;
                break;
            }
        }
        if (found) equal_count++;
    }
    if (count_a == equal_count && count_a == count_b) return 1; // Combina total
    if (equal_count > 0) return 2; // Combina parcial
    return 0; // Sem combinação
}

static Vector2 shake_offset () {
    Vector2 offset = {0.0f, 0.0f};

    shake_time -= GetFrameTime();
            
    if (shake_time <= 0) {
        shake_time = 0;
        moving_up = true;
        shaking = false;
    }
    else {
        float offsetX = (float)GetRandomValue(-100, 100) / 100.0f;
        float offsetY = (float)GetRandomValue(-100, 100) / 100.0f;
        
        offset.x = offsetX * shake_power;
        offset.y = offsetY * shake_power;
    }
    return offset;
}

static void shake(float power, float time) {
    shake_time = time;
    shake_power = power;
    shaking = true;
}


// --- Funções Públicas (Definidas no .h) ---

void load_list() {
    FILE *list_file;

    list_file = fopen("list.txt", "r");
    if (list_file == NULL){
        perror("LIST NOT LOADED\n");
        exit(1);
    }
    for (int i = 0; i < 100; i++) { // Lê até 100 jogos
        if (fscanf(list_file, "%49[^;];%d;%49[^;];%49[^;];%49[^;];%49[^;];%49[^;];\n", 
            games[i].name, &games[i].year, games[i].origin, games[i].genre[0], 
            games[i].theme, games[i].gamemode[0], games[i].platform[0]) != 7) {
            break; // Para se a linha não for lida corretamente ou o arquivo acabar
        }
        
        // Salva os originais antes de 'split_string' modificar
        char original_genre[50];
        char original_gamemode[50];
        char original_platform[50];
        
        strcpy(original_genre, games[i].genre[0]);
        strcpy(original_gamemode, games[i].gamemode[0]);
        strcpy(original_platform, games[i].platform[0]);

        split_string(games[i].genre);
        split_string(games[i].gamemode);
        split_string(games[i].platform);

        // Restaura os originais para 'split_string' funcionar nas próximas rodadas
        // (pois strtok é destrutivo)
        strcpy(games[i].genre[0], original_genre);
        strcpy(games[i].gamemode[0], original_gamemode);
        strcpy(games[i].platform[0], original_platform);

        Image flag_image = LoadImage(TextFormat("sources/flags/%s.png", games[i].origin));
        games[i].flag_texture = LoadTextureFromImage(flag_image);
        UnloadImage(flag_image);

        Image logo_image = LoadImage(TextFormat("logos/logo %s.png", games[i].name));
        games[i].logo_texture = LoadTextureFromImage(logo_image);
        UnloadImage(logo_image);

        if (games[i].logo_texture.height != 0) games[i].logo_texture.width = 32* games[i].logo_texture.width/games[i].logo_texture.height;
        games[i].logo_texture.height = 32;
    }

    fclose(list_file);

    FILE *phrases_file;

    phrases_file = fopen("frases.txt", "r");
    if (phrases_file == NULL){
        perror("PHRASE LIST NOT LOADED\n");
        exit(1);
    }

    for (int i = 0; i < 100; i++) {
        if (fscanf(phrases_file, " %199[^\n]\n", games[i].phrase) != 1) {
            break; // Para se o arquivo acabar
        }
    }
    fclose(phrases_file); // Fechar o arquivo de frases
}

void load_texture() {
    Image arrow_image = LoadImage("sources/flecha.png");
    arrow_texture = LoadTextureFromImage(arrow_image);
    UnloadImage(arrow_image);

    Image heart_image = LoadImage("sources/coracao.png");
    heart_texture = LoadTextureFromImage(heart_image);
    UnloadImage(heart_image);

    char image_path[100];
    sprintf(image_path, "capas/capa %s.png", correct_game.name); 

    Image image = LoadImage(image_path);
    cover_texture = LoadTextureFromImage(image);
    cover_texture.height = 334;
    cover_texture.width = 236;
    UnloadImage(image);
}

void load_games() {
    // 'attempts' é limpo em unload_gameplay_round
    int correct_index = (rand() % (100));
    correct_game = games[correct_index];
    
    // Copia os dados que 'split_string' vai modificar
    // para que a 'correct_game' tenha os dados divididos
    split_string(correct_game.genre);
    split_string(correct_game.gamemode);
    split_string(correct_game.platform);
}

void init_gameplay() {
    // Reseta o estado do jogo para uma nova rodada
    win = false;
    game_finished = false;
    attempt_count = -1;
    selected_attempt_index = 0;
    max_search_results = -1; // Resetado para vazio
    selected_search_index = 0;
    
    strcpy(user_input, "");
    input_index = 0;
    
    hint_1 = false;
    hint_2 = false;
    hint_3 = false;
    
    shaking = false;
    moving_up = false;
    shake_time = 0;
    shake_power = 0;

    // Aloca memória inicial para 'attempts'
    // (Será 'realloc'ada depois)
    attempts = calloc(1, sizeof(GAME)); 

    //Shader para o blur
    blur_shader = LoadShader(0, "capas/blur.fs");
    int view_size_loc = GetShaderLocation(blur_shader, "viewSize");
    blurred_object_rt = LoadRenderTexture(cover_texture.width, cover_texture.height);
    float texture_size[2] = { (float)cover_texture.width, (float)cover_texture.height };
    SetShaderValue(blur_shader, view_size_loc, texture_size, SHADER_UNIFORM_VEC2);

    // Câmera
    baseCameraOffset = (Vector2){ screen_width/2, screen_height/2.0f };
    game_camera.offset = baseCameraOffset;
    game_camera.target = (Vector2){screen_width/2, screen_height/2};
    game_camera.rotation = 0.0f;
    game_camera.zoom = 1.0f;
}

void update_gameplay(void) {
    // --- LÓGICA DE SAÍDA ---
    if (IsKeyPressed(KEY_ESCAPE)) {
        game_finished = true; // Sinaliza para o menu
        return; // Para de processar
    }
    
    if (win) { // Se já ganhou, não processa mais
        game_finished = true;
        return;
    }

    // --- LÓGICA DE UPDATE ---
    if (shaking) game_camera.offset = Vector2Add((Vector2){baseCameraOffset.x, game_camera.offset.y}, shake_offset());

    //if (moving_up) {
    //    game_camera.offset.y = Lerp(game_camera.offset.y, baseCameraOffset.y, 0.01);
    //}

    // Processa o blur (isso é lógica, não desenho final)
    BeginTextureMode(blurred_object_rt);        
        ClearBackground(BLANK);
        BeginShaderMode(blur_shader);   
            DrawTexture(cover_texture, 0,0, WHITE);
        EndShaderMode();            
    EndTextureMode();

    // Input - Setas
    if (IsKeyPressed(KEY_DOWN)) {
        selected_search_index++;
        if (selected_search_index > max_search_results) selected_search_index = 0;
    }
    if (IsKeyPressed(KEY_UP)) {
        selected_search_index--;
        if (selected_search_index < 0) selected_search_index = max_search_results;
    }

    if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) {
        selected_attempt_index--;
    }

    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) {
        selected_attempt_index++;
        if (selected_attempt_index > attempt_count) selected_attempt_index = attempt_count;
    }
    if (selected_attempt_index <= 0) selected_attempt_index = 0;

    // Input - Texto
    int key_pressed = GetCharPressed();
    if ((key_pressed >= 32) && (key_pressed <= 125) && input_index < 50) {
        user_input[input_index] = (char)key_pressed;
        
        selected_search_index = 0; // Reseta a seleção ao digitar
        input_index++;
        user_input[input_index] = '\0';
    }

    if ((IsKeyPressedRepeat(KEY_BACKSPACE) || IsKeyPressed(KEY_BACKSPACE))&& input_index > 0) {
        if (IsKeyDown(KEY_LEFT_CONTROL)) {
            user_input[0] = '\0';
            input_index = 0;
        } else {
            input_index--;
            user_input[input_index] = '\0';
        }
        selected_search_index = 0; // Reseta a seleção ao apagar
    }

    // Lógica - Search
    
    // 1. Limpa *todos* os resultados de busca anteriores
    for (int i = 0; i < 100; i++) strcpy(search_results[i].name, "");
    
    int j = 0; // Índice do próximo resultado de busca
    max_search_results = -1; // *** CORREÇÃO: Inicializa como -1 (vazio) ***

    if (user_input[0] != '\0') {
        for (int l = 0; l < 100; l++) { // Itera sobre a lista MESTRA 'games'
            if (strlen(games[l].name) == 0) continue; // Pula jogos inválidos

            // 2. Compara o input com o nome do jogo
            char temp_substr[strlen(user_input)+1];
            for (int k = 0; k <= (int)strlen(user_input); k++) {
                temp_substr[k] = games[l].name[k];
            }
            temp_substr[strlen(user_input)] = '\0';

            if (strcasecmp(user_input, temp_substr) == 0) {
                
                // 3. VERIFICA SE JÁ FOI TENTADO (A LÓGICA QUE FALTAVA)
                bool already_attempted = false;
                for (int i = 0; i <= attempt_count; i++) {
                    // *** AGORA ISSO FUNCIONA, POIS O NOME EM 'attempts' ESTÁ LIMPO ***
                    if (strcmp(games[l].name, attempts[i].name) == 0) {
                        already_attempted = true;
                        break;
                    }
                }

                // 4. Se não foi tentado, adiciona aos resultados da busca
                if (!already_attempted) {
                    search_results[j] = games[l];
                    max_search_results = j; // Atualiza o índice máximo (se j=0, max_idx=0. se j=1, max_idx=1)
                    j++;
                    if (j >= 100) break; // Para se o array de busca estiver cheio
                }
            }
        }
    }

    // Lógica - Tentativa
    if (IsKeyPressed(KEY_ENTER) && max_search_results >= 0 && strcmp(search_results[selected_search_index].name, "") != 0) {
        attempt_count++;
        selected_attempt_index = attempt_count;
        
        // Realoca o array de tentativas
        GAME* new_attempts = realloc(attempts, (attempt_count + 1) * sizeof(GAME));
        if (new_attempts == NULL) {
            perror("Failed to realloc attempts");
            exit(1); // Falha crítica
        }
        attempts = new_attempts;
        
        // Copia os dados do jogo selecionado
        attempts[attempt_count] = search_results[selected_search_index];
        
        // *IMPORTANTE*: Copia os dados divididos para a tentativa
        // (pois search_results[i] tem os dados da 'games[l]' que não estão divididos)
        split_string(attempts[attempt_count].genre);
        split_string(attempts[attempt_count].gamemode);
        split_string(attempts[attempt_count].platform);


        //moving_up = true;

        if (strcmp(attempts[attempt_count].name, correct_game.name) != 0) shake(3, 0.4);
        else win = true; // Jogo ganho!

        strcpy(user_input, " ");
        
        input_index = 0;

        // Limpa os resultados da busca (para a UI sumir)
        for (int i = 0; i < 100; i++) strcpy(search_results[i].name, "");
        max_search_results = -1; // *** CORREÇÃO: Reseta a busca ***
        selected_search_index = 0; // *** CORREÇÃO: Reseta o índice ***
        
        // *** REMOVIDO ***: A lógica de adicionar '\n' foi removida daqui
        // pois estava corrompendo o dado em 'attempts[...].name'
    }

    // Lógica - Câmera Scroll
    
    game_camera.offset.y += (int)(GetMouseWheelMove()*35);
    
    if (game_camera.offset.y >= screen_height/2-2) {
        game_camera.offset.y = screen_height/2;
    }        

    int y_gap = screen_height/2 - (attempt_count)*185;
    if (attempt_count == -1) y_gap = screen_height/2;
    if (game_camera.offset.y <= y_gap) game_camera.offset.y = y_gap;
}

// Nova função get_gameplay_camera
Camera2D get_gameplay_camera(void) {
    return game_camera;
}

// Nova função draw_gameplay_world
// (Contém tudo que estava DENTRO do BeginMode2D)
void draw_gameplay_world(void) {
    if (attempt_count != -1 ) {
        for (int i = 0; i < attempt_count+1; i++){
            GAME selected_game = attempts[attempt_count-i];
            
            // --- INÍCIO DA CORREÇÃO DE DESENHO (NOME) ---
            // Cria uma cópia temporária do nome para desenhar
            char name_to_draw[50];
            strcpy(name_to_draw, selected_game.name);
            
            if (strlen(name_to_draw) >= 10) {
                for (int k = (int)strlen(name_to_draw)/3; k < (int)strlen(name_to_draw); k++) {
                    if (name_to_draw[k] == ' ') {
                        name_to_draw[k] = '\n'; // Modifica a cópia, não o original
                        break;
                    }
                }
            }
            // Desenha a cópia modificada
            DrawText(name_to_draw, 30 + selected_game.logo_texture.width, screen_height - 180 +  i*185, 30, PINK);
            // --- FIM DA CORREÇÃO DE DESENHO (NOME) ---
            
            DrawTextureEx(selected_game.logo_texture, (Vector2){20, screen_height - 180 +  i*185}, 0, 1.1, GRAY);
            DrawTexture(selected_game.logo_texture, 20, screen_height - 180 +  i*185, WHITE); 
            int x_offset = 20;

            //YEAR
            if (selected_game.year == correct_game.year) DrawText(TextFormat("%d",selected_game.year), 20, screen_height - 80 + i*185, 30, GREEN);
            else {
                DrawText(TextFormat("%d",selected_game.year), x_offset, screen_height - 80 + i*185, 30, RED);
                DrawTexturePro(arrow_texture, (Rectangle){0, 0, 15, 14}, (Rectangle){MeasureText(TextFormat("%d",selected_game.year), 30) + 35, screen_height - 67 + i*185, 15, 14}, (Vector2){7,7}, (selected_game.year > correct_game.year ? 180 : 0), WHITE);   
            }

            x_offset += MeasureText(TextFormat("%d",selected_game.year), 30) + 33;

            //ORIGIN
            DrawTexturePro(selected_game.flag_texture, (Rectangle){0, 0, (float)selected_game.flag_texture.width, (float)selected_game.flag_texture.height}, (Rectangle){x_offset, screen_height - 80 + i*185, selected_game.flag_texture.width*2, selected_game.flag_texture.height*2}, (Vector2){0,0}, 0, WHITE);
            DrawRectangleLinesEx((Rectangle){x_offset,  screen_height - 80 + i*185, selected_game.flag_texture.width*2, selected_game.flag_texture.height*2 }, 2.5, (strcmp(selected_game.origin, correct_game.origin) == 0) ? GREEN : RED);
            x_offset += selected_game.flag_texture.width*2 + 20;

            //GENRE
            int genre_equality = check_equality(correct_game.genre, selected_game.genre);
            if (genre_equality == 1) DrawText(TextFormat("%s\n%s", selected_game.genre[0], selected_game.genre[1]), x_offset,  screen_height - 80 + i*185, 30, GREEN);
            else DrawText(TextFormat("%s\n%s", selected_game.genre[0], selected_game.genre[1]), x_offset,  screen_height - 80 + i*185, 30, (genre_equality == 0) ? RED : YELLOW);
            x_offset += MeasureText(TextFormat("%s\n%s", selected_game.genre[0], selected_game.genre[1]), 30) + 20;

            // --- INÍCIO DA CORREÇÃO DE DESENHO (TEMA) ---
            // Cria uma cópia temporária do tema
            char theme_to_draw[50];
            strcpy(theme_to_draw, selected_game.theme);

            if (strlen(theme_to_draw) != strcspn(theme_to_draw, " ")) {
                theme_to_draw[strcspn(theme_to_draw, " ")] = '\n'; // Modifica a cópia
            }
            // Desenha a cópia
            DrawText(theme_to_draw, x_offset,  screen_height - 80 + i*185, 30, (strcmp(selected_game.theme, correct_game.theme) == 0) ? GREEN : RED);
            // --- FIM DA CORREÇÃO DE DESENHO (TEMA) ---
            
            x_offset += MeasureText(theme_to_draw, 30) + 20; // Usa a cópia para medir


            //GAMEMODE
            int gamemode_equality = check_equality(correct_game.gamemode, selected_game.gamemode);
            if (gamemode_equality == 1) DrawText(TextFormat("%s\n%s", selected_game.gamemode[0], selected_game.gamemode[1]), x_offset,  screen_height - 80 + i*185, 30, GREEN);
            else DrawText(TextFormat("%s\n%s", selected_game.gamemode[0], selected_game.gamemode[1]), x_offset,  screen_height - 80 + i*185, 30, (gamemode_equality == 0) ? RED : YELLOW);
            x_offset += MeasureText(TextFormat("%s\n%s", selected_game.gamemode[0], selected_game.gamemode[1]), 30) + 20;

            //PLATFORM
            int platform_equality = check_equality(correct_game.platform, selected_game.platform);
            int platform_count = 0; // Começa em 0
            for (int k = 0; k < 3; k++) if (strlen(selected_game.platform[k]) > 0) platform_count++; // Conta quantos existem
            
            if (platform_equality == 1) DrawText(TextFormat("%s\n%s\n%s", selected_game.platform[0], selected_game.platform[1], selected_game.platform[2]), x_offset,  screen_height - 80 + i*185 - (platform_count > 1 ? 15 * (platform_count-1) : 0), 30, GREEN);
            else DrawText(TextFormat("%s\n%s\n%s", selected_game.platform[0], selected_game.platform[1], selected_game.platform[2]), x_offset,  screen_height - 80 + i*185 - (platform_count > 1 ? 15 * (platform_count-1) : 0), 30, (platform_equality == 0) ? RED : YELLOW);
            x_offset += MeasureText(TextFormat("%s\n%s\n%s", selected_game.platform[0], selected_game.platform[1], selected_game.platform[2]), 30) + 10;
        }
    }

    //Hints
    if (hint_1) {
        DrawText(correct_game.phrase, screen_width / 2 - MeasureText(correct_game.phrase, 20) / 2, 500, 20, YELLOW);
    }
    if (hint_2) {
        Rectangle sourceRect = { 0, 0, (float)blurred_object_rt.texture.width, (float)-blurred_object_rt.texture.height };
        Vector2 destPos = { screen_width - cover_texture.width - 20.0f, screen_height - cover_texture.height - 20.0f};
        DrawTextureRec(blurred_object_rt.texture, sourceRect, destPos, WHITE);
    }
    if (hint_3) {
        DrawTexture(cover_texture, screen_width - cover_texture.width - 20, screen_height - cover_texture.height - 20, WHITE);
    }

    DrawRectangleLines(screen_width - cover_texture.width - 20, screen_height - cover_texture.height - 20, blurred_object_rt.texture.width, blurred_object_rt.texture.height, DARKBLUE);
}

// Nova função draw_gameplay_ui
// (Contém tudo que estava FORA do BeginMode2D)
void draw_gameplay_ui(void) {
    if (strcmp(user_input, "") != 0 && strcmp(user_input, " ") != 0) DrawRectangle(screen_width/2-MeasureText(user_input, 30)/2-5, 30/2-5, MeasureText(user_input, 30)+10, 30+10, LIGHTGRAY);
    DrawText(user_input, screen_width/2-MeasureText(user_input, 30)/2, 30/2, 30, BLACK);
    
    for (int j = 0; j < 5; j++) {
        char name[50];
        // *** CORREÇÃO: Checa se max_search_results é válido antes de usar ***
        if (max_search_results >= 0) {
            // Lógica para rolagem da lista de busca
            int search_idx = j;
            if (max_search_results >= 5 && selected_search_index >= 4) {
                 search_idx = j + (selected_search_index - 4);
            }
            // Evita ler fora dos limites
            if (search_idx <= max_search_results) {
                strcpy(name, search_results[search_idx].name);
            } else {
                strcpy(name, "");
            }
            
            // Determina o índice de seleção real (para a cor)
            int actual_selected_index = selected_search_index;
            
            if (strcmp(name, "") != 0) DrawRectangle(screen_width/2-MeasureText(name, 30)/2-5, 30*(2+1.5*j)-5, MeasureText(name, 30)+10, 50, (Color){30,30,30,255});
            DrawText(name, screen_width/2-MeasureText(name, 30)/2, 30*(2+1.5*j), 30, (actual_selected_index == search_idx ? BLUE : PINK));

        } else {
            strcpy(name, ""); // Se não há resultados, string vazia
        }
    }

    //Lives
    int lives = 20 - attempt_count - 1;

    for (int i = 0; i < (int)ceil((double)lives / 2); i++) {
        int sprite = 0;
        Color color = WHITE;
        if (i == lives / 2 && lives % 2 != 0) sprite = 1;
        if (i == 15 || i == 7 || i == 3 ) color = (Color){240,255,0,255};
        DrawTexturePro(heart_texture, (Rectangle){8.0f * sprite, 8.0f * sprite, 8, 8}, (Rectangle){24.0f * i, 0, 32, 32}, (Vector2){0,0}, 0, color);
    }

    if (lives <= 15) hint_1 = true;
    if (lives <= 7) hint_2 = true;
    if (lives <= 3) hint_3 = true;
}

bool is_gameplay_finished(void) {
    return game_finished;
}

void unload_gameplay_round(void) {
    // Limpa os assets carregados para ESTA partida
    UnloadRenderTexture(blurred_object_rt);
    UnloadShader(blur_shader);
    UnloadTexture(arrow_texture);
    UnloadTexture(heart_texture);
    UnloadTexture(cover_texture);
    
    if (attempts != NULL) {
        free(attempts);
        attempts = NULL;
    }
}

void unload_global_assets(void) {
    // Limpa os assets da load_list
    for (int i = 0; i < 100; i++) {
        if (games[i].flag_texture.id > 0) UnloadTexture(games[i].flag_texture);
        if (games[i].logo_texture.id > 0) UnloadTexture(games[i].logo_texture);
    }
}
