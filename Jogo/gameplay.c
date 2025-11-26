#include "gameplay.h" // Inclui nossas próprias definições
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

// #define GAME_AMOUNT 100 // REMOVIDO

int screen_width = 1280;
int screen_height = 720;
bool win = false;
bool lose = false;

static int game_properties_amount = 7;
static int game_amount = 0; // MODIFICADO: Agora é dinâmico, começa em 0
static Game *games = NULL; // MODIFICADO: Agora é um ponteiro dinâmico
static Game *search_results = NULL; // MODIFICADO: Agora é um ponteiro dinâmico
static int search_capacity = 0; // MODIFICADO: Capacidade do array de busca

static Game correct_game;
static Game *attempts = NULL;

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
static Vector2 base_camera_offset = {0};

static bool shaking = false;
static bool moving_up = false;
static float shake_time = 0;
static float shake_power = 0;

static Texture2D heart_texture;
static Texture2D arrow_texture;
static Texture2D cover_texture;


static void split_string(char str_array[3][50]) {
    int index = 0;
    for (int i = 1; i < 3; i++) strcpy(str_array[i], "");
    if (strcspn(str_array[0], "/") == strlen(str_array[0])) {
        return; 
    }
    
    char temp_str[50];
    strcpy(temp_str, str_array[0]);
    
    char* token = strtok(temp_str, "/");
    while (token != NULL && index < 3) {
        strcpy(str_array[index], token);
        index++;
        token = strtok(NULL, "/");
    }
}
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
    if (count_a == equal_count && count_a == count_b) return 1;
    if (equal_count > 0) return 2;
    return 0;
}

// (Funções 'shake_offset' e 'shake' originais mantidas)
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

static void clear_stdin_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

static void get_user_string(char* buffer, int size) {
    fgets(buffer, size, stdin);
    buffer[strcspn(buffer, "\n")] = 0;
}

static void terminal_add_game() {
    char name[50], origin[50], genre[50], theme[50], gamemode[50], platform[50], phrase[200];
    int year;

    printf("Adicionar novo jogo:\n");
    printf("Nome: ");
    get_user_string(name, 50);
    printf("Ano: ");
    scanf("%d", &year);
    clear_stdin_buffer();
    printf("Origem (Ex: EUA): ");
    get_user_string(origin, 50);
    printf("Genero (Ex: Acao/Aventura): ");
    get_user_string(genre, 50);
    printf("Tema (Ex: Fantasia): ");
    get_user_string(theme, 50);
    printf("Modo de Jogo (Ex: Single/Multi): ");
    get_user_string(gamemode, 50);
    printf("Plataforma (Ex: PC/Console): ");
    get_user_string(platform, 50);
    printf("Frase (Dica): ");
    get_user_string(phrase, 200);

    // Abre os arquivos em modo "append" (a)
    FILE *list_file = fopen("list.csv", "a");
    FILE *phrases_file = fopen("frases.csv", "a");
    FILE *capa_file = fopen(TextFormat("capas/capa %s.png", name), "wb");
    FILE *logo_file = fopen(TextFormat("logos/logo %s.png", name), "w");

    if (list_file == NULL || phrases_file == NULL) {
        perror("Erro ao abrir arquivos para escrita");
        if (list_file) fclose(list_file);
        if (phrases_file) fclose(phrases_file);
        return;
    }

    // Escreve nos arquivos
    fprintf(list_file, "%s;%d;%s;%s;%s;%s;%s;\n",
            name, year, origin, genre, theme, gamemode, platform);
    
    fprintf(phrases_file, "%s\n", phrase);

    fclose(list_file);
    fclose(phrases_file);
    fclose(capa_file);
    fclose(logo_file);

    printf("Jogo '%s' adicionado com sucesso!\n", name);
}

static void terminal_list_games() {
    FILE *list_file = fopen("list.csv", "r");
    if (list_file == NULL) {
        perror("Nao foi possivel ler 'list.csv'");
        return;
    }

    printf("Lista de jogos cadastrados:\n");
    char name[50];
    int count = 0;
    
    // Formato de leitura simplificado
    // Apenas lê o nome (o primeiro campo antes do ';')
    while (fscanf(list_file, "%49[^;];%*[^\n]\n", name) == 1) {
        printf("%d. %s\n", ++count, name);
    }

    fclose(list_file);
    if (count == 0) {
        printf("Nenhum jogo encontrado.\n");
    }
    printf("----------------------------------\n");
}

static void terminal_search_game() {
    char search_name[50];
    printf("Digite o nome (ou parte) do jogo a pesquisar: ");
    get_user_string(search_name, 50);

    FILE *list_file = fopen("list.csv", "r");
    if (list_file == NULL) {
        perror("Nao foi possivel ler 'list.csv'");
        return;
    }

    char line_buffer[512]; // Buffer para ler a linha inteira
    char game_name[50];
    bool found = false;

    printf("Resultados da busca:\n");
    while (fgets(line_buffer, sizeof(line_buffer), list_file)) {
        // Copia a linha para extrair o nome sem destruir o buffer
        char temp_line[512];
        strcpy(temp_line, line_buffer);
        
        char *token = strtok(temp_line, ";");
        if (token != NULL) {
            strcpy(game_name, token);
            // strcasestr?
            if (strstr(game_name, search_name) != NULL) {
                printf("Encontrado: %s", line_buffer);
                found = true;
            }
        }
    }
    
    if (!found) {
        printf("Nenhum jogo encontrado com '%s'.\n", search_name);
    }
    printf("----------------------------\n");
    fclose(list_file);
}

//Função auxiliar para reescrever arquivos
static int rewrite_files(const char* target_name, const Game* new_data) {
    // new_data == NULL significa EXCLUIR
    // new_data != NULL significa ALTERAR
    
    FILE *list_in = fopen("list.csv", "r");
    FILE *phrases_in = fopen("frases.csv", "r");
    FILE *list_out = fopen("list.tmp", "w");
    FILE *phrases_out = fopen("phrases.tmp", "w");

    if (!list_in || !phrases_in || !list_out || !phrases_out) {
        perror("Erro ao abrir arquivos (originais ou temporarios)");
        if(list_in) fclose(list_in);
        if(phrases_in) fclose(phrases_in);
        if(list_out) fclose(list_out);
        if(phrases_out) fclose(phrases_out);
        return 0; // Falha
    }

    char line_buffer[512], phrase_buffer[256];
    char game_name[50];
    int found = 0;

    // Itera pelos dois arquivos linha por linha
    while (fgets(line_buffer, sizeof(line_buffer), list_in) &&
           fgets(phrase_buffer, sizeof(phrase_buffer), phrases_in)) 
    {
        // Extrai o nome da linha atual
        char temp_line[512];
        strcpy(temp_line, line_buffer);
        char *token = strtok(temp_line, ";");
        strcpy(game_name, token ? token : "");

        // Compara com o nome alvo
        if (strcmp(game_name, target_name) == 0) {
            found = 1;
            if (new_data != NULL) {
                // MODO ALTERAR: Escreve os *novos* dados
                fprintf(list_out, "%s;%d;%s;%s;%s;%s;%s;\n",
                        new_data->name, new_data->year, new_data->origin,
                        new_data->genre[0], // Salva o formato original (antes do split)
                        new_data->theme,
                        new_data->gamemode[0],
                        new_data->platform[0]);
                fprintf(phrases_out, "%s\n", new_data->phrase);
            }
            // MODO EXCLUIR: Não faz nada (pula a escrita)
        } else {
            // Nome não bateu, apenas reescreve a linha original
            fputs(line_buffer, list_out);
            fputs(phrase_buffer, phrases_out);
        }
    }

    fclose(list_in);
    fclose(phrases_in);
    fclose(list_out);
    fclose(phrases_out);

    if (found) {
        // Substitui os arquivos antigos pelos novos
        remove("list.csv");
        remove("frases.csv");
        rename("list.tmp", "list.csv");
        rename("phrases.tmp", "frases.csv");
    } else {
        // Se não achou, apaga os temporários
        remove("list.tmp");
        remove("phrases.tmp");
    }
    
    return found;
}

static void terminal_delete_game() {
    char name_to_delete[50];
    printf("Excluir jogo:\n");
    printf("Digite o nome EXATO do jogo a excluir: ");
    get_user_string(name_to_delete, 50);

    if (rewrite_files(name_to_delete, NULL)) {
        printf("Jogo '%s' excluido com sucesso.\n", name_to_delete);
    } else {
        printf("Jogo '%s' nao encontrado.\n", name_to_delete);
    }
}

static void terminal_modify_game() {
    char name_to_modify[50];
    printf("Alterar jogo:\n");
    printf("Digite o nome EXATO do jogo a alterar: ");
    get_user_string(name_to_modify, 50);

    Game new_data; // Struct temporária para os novos dados

    printf("Digite os NOVOS dados para '%s'\n", name_to_modify);
    
    printf("Novo Nome: ");
    get_user_string(new_data.name, 50);
    printf("Novo Ano: ");
    scanf("%d", &new_data.year);
    clear_stdin_buffer();
    printf("Nova Origem: ");
    get_user_string(new_data.origin, 50);
    printf("Novo Genero (Ex: Acao/Aventura): ");
    get_user_string(new_data.genre[0], 50);
    printf("Novo Tema: ");
    get_user_string(new_data.theme, 50);
    printf("Novo Modo de Jogo: ");
    get_user_string(new_data.gamemode[0], 50);
    printf("Nova Plataforma: ");
    get_user_string(new_data.platform[0], 50);
    printf("Nova Frase (Dica): ");
    get_user_string(new_data.phrase, 200);

    if (rewrite_files(name_to_modify, &new_data)) {
        printf("Jogo '%s' alterado para '%s' com sucesso.\n", name_to_modify, new_data.name);
    } else {
        printf("Jogo '%s' nao encontrado.\n", name_to_modify);
    }
}

void initialize_list() {
    // Preparar variáiveis globais
    games = NULL;
    game_amount = 0;
    search_results = NULL;
    search_capacity = 0;
    attempts = NULL;
}

void update_list() {
    int choice = 0;

    while(1) {
        printf("\nGerenciador de dados:\n");
        printf("1. Adicionar jogo\n");
        printf("2. Listar todos os jogos\n");
        printf("3. Pesquisar jogo\n");
        printf("4. Alterar jogo\n");
        printf("5. Excluir jogo\n");
        printf("0. Sair e iniciar jogo\n");
        printf("\n");
        printf("Escolha uma opcao: ");

        if (scanf("%d", &choice) != 1) {
            choice = -1; // Escolha inválida
        }
        clear_stdin_buffer(); // Limpa o \n ou entrada inválida

        switch(choice) {
            case 1:
                terminal_add_game();
                break;
            case 2:
                terminal_list_games();
                break;
            case 3:
                terminal_search_game();
                break;
            case 4:
                terminal_modify_game();
                break;
            case 5:
                terminal_delete_game();
                break;
            case 0:
                printf("Iniciando o jogo...\n");
                return; // Sai da função e continua para o InitWindow
            default:
                printf("Opcao invalida. Tente novamente.\n");
                break;
        }
        
        printf("\nPressione ENTER para continuar...");
        getchar(); // Pausa
    }
}

void load_list() {
    FILE *list_file = fopen("list.csv", "r");
    if (list_file == NULL){
        perror("Error reading list!\n");
        exit(1);
    }
    
    game_amount = 0; // Reseta a contagem
    char buffer[512];
    while (fgets(buffer, sizeof(buffer), list_file)) {
        // Simplesmente conta as linhas
        game_amount++;
    }
    fclose(list_file);

    if (game_amount == 0) {
        printf("Nenhum jogo encontrado em list.csv! Saindo.\n");
        exit(1);
    }

    printf("Total de %d jogos encontrados. Carregando...\n", game_amount);

    // Alocar memória dinamicamente
    games = (Game*)calloc(game_amount, sizeof(Game));
    search_results = (Game*)calloc(game_amount, sizeof(Game));
    
    if (games == NULL || search_results == NULL) {
        perror("Falha ao alocar memoria para a lista de jogos");
        exit(1);
    }
    search_capacity = game_amount; // Define a capacidade de busca

    list_file = fopen("list.csv", "r");
    if (list_file == NULL) {
        exit(1);
    }

    for (int i = 0; i < game_amount; i++) {
        int _game = fscanf(list_file, "%49[^;];%d;%49[^;];%49[^;];%49[^;];%49[^;];%49[^;];\n", 
            games[i].name,
            &games[i].year,
            games[i].origin,
            games[i].genre[0], 
            games[i].theme,
            games[i].gamemode[0],
            games[i].platform[0]
        );

        if (_game != game_properties_amount) {
            fprintf(stderr, "Erro de formato na linha %d de list.csv\n", i + 1);
            break; 
        }
        
        char original_genre[50];
        char original_gamemode[50];
        char original_platform[50];
        
        strcpy(original_genre, games[i].genre[0]);
        strcpy(original_gamemode, games[i].gamemode[0]);
        strcpy(original_platform, games[i].platform[0]);

        split_string(games[i].genre);
        split_string(games[i].gamemode);
        split_string(games[i].platform);

        strcpy(games[i].genre[0], original_genre);
        strcpy(games[i].gamemode[0], original_gamemode);
        strcpy(games[i].platform[0], original_platform);

        Image flag_image = LoadImage(TextFormat("sources/flags/%s.png", games[i].origin));
        if (flag_image.data == NULL) flag_image = LoadImage("sources/flags/flag.png");
        games[i].flag_texture = LoadTextureFromImage(flag_image);
        UnloadImage(flag_image);

        Image logo_image = LoadImage(TextFormat("logos/logo %s.png", games[i].name));
        games[i].logo_texture = LoadTextureFromImage(logo_image);
        UnloadImage(logo_image);

        if (games[i].logo_texture.height != 0) games[i].logo_texture.width = 32* games[i].logo_texture.width/games[i].logo_texture.height;
        games[i].logo_texture.height = 32;
    }
    fclose(list_file);

    // Frases
    FILE *phrases_file = fopen("frases.csv", "r");
    if (phrases_file == NULL){
        perror("PHRASE LIST NOT LOADED\n");
        exit(1);
    }

    for (int i = 0; i < game_amount; i++) {
        if (fscanf(phrases_file, " %199[^\n]\n", games[i].phrase) != 1) {
            fprintf(stderr, "Erro de formato ou falta de linhas em frases.csv (esperava %d, parou em %d)\n", game_amount, i);
            break;
        }
    }
    fclose(phrases_file);
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
    if (game_amount <= 0) {
        printf("ERRO FATAL: load_games chamada sem jogos carregados!\n");
        exit(1);
    }
    int correct_game_index = (rand() % (game_amount)); // MODIFICADO
    correct_game = games[correct_game_index];
    
    split_string(correct_game.genre);
    split_string(correct_game.gamemode);
    split_string(correct_game.platform);
}

void init_gameplay() {
    win = false;
    lose = false;
    game_finished = false;
    attempt_count = -1;
    selected_attempt_index = 0;
    max_search_results = -1;
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

    attempts = calloc(1, sizeof(Game)); 

    blur_shader = LoadShader(0, "capas/blur.fs");
    int view_size_loc = GetShaderLocation(blur_shader, "viewSize");
    blurred_object_rt = LoadRenderTexture(cover_texture.width, cover_texture.height);
    float texture_size[2] = { (float)cover_texture.width, (float)cover_texture.height };
    SetShaderValue(blur_shader, view_size_loc, texture_size, SHADER_UNIFORM_VEC2);

    base_camera_offset = (Vector2){ screen_width/2, screen_height/2.0f };
    game_camera.offset = base_camera_offset;
    game_camera.target = (Vector2){screen_width/2, screen_height/2};
    game_camera.rotation = 0.0f;
    game_camera.zoom = 1.0f;
}

void update_gameplay(void) {
    if (IsKeyPressed(KEY_ESCAPE)) {
        game_finished = true;
        return;
    }
    if (win || lose) {
        game_finished = true;
        return;
    }
    if (shaking) game_camera.offset = Vector2Add((Vector2){base_camera_offset.x, game_camera.offset.y}, shake_offset());

    BeginTextureMode(blurred_object_rt);        
        ClearBackground(BLANK);
        BeginShaderMode(blur_shader);   
            DrawTexture(cover_texture, 0,0, WHITE);
        EndShaderMode();            
    EndTextureMode();

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

    int key_pressed = GetCharPressed();
    if ((key_pressed >= 32) && (key_pressed <= 125) && input_index < 50) {
        user_input[input_index] = (char)key_pressed;
        selected_search_index = 0;
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
        selected_search_index = 0;
    }

    int j = 0;
    max_search_results = -1;

    if (user_input[0] != '\0') {
        for (int l = 0; l < game_amount; l++) { 
            if (strlen(games[l].name) == 0) continue;

            char temp_substr[strlen(user_input)+1];
            for (int k = 0; k <= (int)strlen(user_input); k++) {
                temp_substr[k] = games[l].name[k];
            }
            temp_substr[strlen(user_input)] = '\0';

            if (strcasecmp(user_input, temp_substr) == 0) {
                bool already_attempted = false;
                for (int i = 0; i <= attempt_count; i++) {
                    if (strcmp(games[l].name, attempts[i].name) == 0) {
                        already_attempted = true;
                        break;
                    }
                }

                if (!already_attempted) {
                    search_results[j] = games[l];
                    max_search_results = j;
                    j++;
                    // MODIFICADO: Compara com a capacidade dinâmica
                    if (j >= search_capacity) break; 
                }
            }
        }
    }

    // Lógica - Tentativa
    if (IsKeyPressed(KEY_ENTER) && max_search_results >= 0 && strcmp(search_results[selected_search_index].name, "") != 0) {
        attempt_count++;
        selected_attempt_index = attempt_count;
        
        Game* new_attempts = realloc(attempts, (attempt_count + 1) * sizeof(Game));
        if (new_attempts == NULL) {
            perror("Failed to realloc attempts");
            exit(1);
        }
        attempts = new_attempts;
        
        attempts[attempt_count] = search_results[selected_search_index];
        
        split_string(attempts[attempt_count].genre);
        split_string(attempts[attempt_count].gamemode);
        split_string(attempts[attempt_count].platform);

        if (strcmp(attempts[attempt_count].name, correct_game.name) != 0) shake(3, 0.4);
        else win = true;

        strcpy(user_input, " ");
        input_index = 0;

        // Limpa os resultados da busca (para a UI sumir)
        // MODIFICADO: Este loop não é estritamente necessário se
        // 'max_search_results' for -1, mas vamos mantê-lo dinâmico
        // for (int i = 0; i < search_capacity; i++) strcpy(search_results[i].name, "");
        // (Removido para otimizar, já que 'max_search_results = -1' resolve)
        
        max_search_results = -1;
        selected_search_index = 0;
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

Camera2D get_gameplay_camera(void) {
    return game_camera;
}

void draw_gameplay_world(void) {
    if (attempt_count != -1 ) {
        for (int i = 0; i < attempt_count+1; i++){
            Game selected_game = attempts[attempt_count-i];
            
            char name_to_draw[50];
            strcpy(name_to_draw, selected_game.name);
            if (strlen(name_to_draw) >= 10) {
                for (int k = (int)strlen(name_to_draw)/3; k < (int)strlen(name_to_draw); k++) {
                    if (name_to_draw[k] == ' ') {
                        name_to_draw[k] = '\n';
                        break;
                    }
                }
            }
            DrawText(name_to_draw, 30 + selected_game.logo_texture.width, screen_height - 180 +  i*185, 30, PINK);
            
            DrawTextureEx(selected_game.logo_texture, (Vector2){20, screen_height - 180 +  i*185}, 0, 1.1, GRAY);
            DrawTexture(selected_game.logo_texture, 20, screen_height - 180 +  i*185, WHITE); 
            int x_offset = 20;

            //YEAR
            DrawText("YEAR", x_offset, screen_height - 110 + i*185, 15, GRAY);
            if (selected_game.year == correct_game.year) DrawText(TextFormat("%d",selected_game.year), 20, screen_height - 80 + i*185, 30, GREEN);
            else {
                DrawText(TextFormat("%d",selected_game.year), x_offset, screen_height - 80 + i*185, 30, RED);
                DrawTexturePro(arrow_texture, (Rectangle){0, 0, 15, 14}, (Rectangle){MeasureText(TextFormat("%d",selected_game.year), 30) + 35, screen_height - 67 + i*185, 15, 14}, (Vector2){7,7}, (selected_game.year > correct_game.year ? 180 : 0), WHITE);   
            }
            x_offset += MeasureText(TextFormat("%d",selected_game.year), 30) + 33;

            //ORIGIN
            DrawText("ORIGIN", x_offset, screen_height - 110 + i*185, 15, GRAY);
            DrawTexturePro(selected_game.flag_texture, (Rectangle){0, 0, (float)selected_game.flag_texture.width, (float)selected_game.flag_texture.height}, (Rectangle){x_offset, screen_height - 80 + i*185, selected_game.flag_texture.width*2, selected_game.flag_texture.height*2}, (Vector2){0,0}, 0, WHITE);
            DrawRectangleLinesEx((Rectangle){x_offset,  screen_height - 80 + i*185, selected_game.flag_texture.width*2, selected_game.flag_texture.height*2 }, 2.5, (strcmp(selected_game.origin, correct_game.origin) == 0) ? GREEN : RED);
            x_offset += selected_game.flag_texture.width*2 + 20;

            //GENRE
            DrawText("GENRE", x_offset, screen_height - 110 + i*185, 15, GRAY);
            int genre_equality = check_equality(correct_game.genre, selected_game.genre);
            if (genre_equality == 1) DrawText(TextFormat("%s\n%s", selected_game.genre[0], selected_game.genre[1]), x_offset,  screen_height - 80 + i*185, 30, GREEN);
            else DrawText(TextFormat("%s\n%s", selected_game.genre[0], selected_game.genre[1]), x_offset,  screen_height - 80 + i*185, 30, (genre_equality == 0) ? RED : YELLOW);
            x_offset += MeasureText(TextFormat("%s\n%s", selected_game.genre[0], selected_game.genre[1]), 30) + 20;

            //THEME
            DrawText("THEME", x_offset, screen_height - 110 + i*185, 15, GRAY);
            char theme_to_draw[50];
            strcpy(theme_to_draw, selected_game.theme);
            if (strlen(theme_to_draw) != strcspn(theme_to_draw, " ")) {
                theme_to_draw[strcspn(theme_to_draw, " ")] = '\n';
            }
            DrawText(theme_to_draw, x_offset,  screen_height - 80 + i*185, 30, (strcmp(selected_game.theme, correct_game.theme) == 0) ? GREEN : RED);
            x_offset += MeasureText(theme_to_draw, 30) + 20;


            //GAMEMODE
            DrawText("GAMEMODE", x_offset, screen_height - 110 + i*185, 15, GRAY);
            int gamemode_equality = check_equality(correct_game.gamemode, selected_game.gamemode);
            if (gamemode_equality == 1) DrawText(TextFormat("%s\n%s", selected_game.gamemode[0], selected_game.gamemode[1]), x_offset,  screen_height - 80 + i*185, 30, GREEN);
            else DrawText(TextFormat("%s\n%s", selected_game.gamemode[0], selected_game.gamemode[1]), x_offset,  screen_height - 80 + i*185, 30, (gamemode_equality == 0) ? RED : YELLOW);
            x_offset += MeasureText(TextFormat("%s\n%s", selected_game.gamemode[0], selected_game.gamemode[1]), 30) + 20;

            //PLATFORM
            int platform_equality = check_equality(correct_game.platform, selected_game.platform);
            int platform_count = 0;
            for (int k = 0; k < 3; k++) if (strlen(selected_game.platform[k]) > 0) platform_count++;
            
            DrawText("PLATAFORM", x_offset, screen_height - 110 - (platform_count > 1 ? 15 * (platform_count-1) : 0) + i*185, 15, GRAY);
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

    DrawRectangle(screen_width - cover_texture.width - 20, screen_height - cover_texture.height - 20, blurred_object_rt.texture.width, blurred_object_rt.texture.height, BLACK);
}

void draw_gameplay_ui(void) {
    if (strcmp(user_input, "") != 0 && strcmp(user_input, " ") != 0) DrawRectangle(screen_width/2-MeasureText(user_input, 30)/2-5, 30/2-5, MeasureText(user_input, 30)+10, 30+10, LIGHTGRAY);
    DrawText(user_input, screen_width/2-MeasureText(user_input, 30)/2, 30/2, 30, BLACK);
    
    for (int j = 0; j < 5; j++) {
        char name[50];
        if (max_search_results >= 0) {
            int search_idx = j;
            if (max_search_results >= 5 && selected_search_index >= 4) {
                 search_idx = j + (selected_search_index - 4);
            }
            if (search_idx <= max_search_results) {
                strcpy(name, search_results[search_idx].name);
            } else {
                strcpy(name, "");
            }
            
            int actual_selected_index = selected_search_index;
            
            if (strcmp(name, "") != 0) DrawRectangle(screen_width/2-MeasureText(name, 30)/2-5, 30*(2+1.5*j)-5, MeasureText(name, 30)+10, 50, (Color){30,30,30,255});
            DrawText(name, screen_width/2-MeasureText(name, 30)/2, 30*(2+1.5*j), 30, (actual_selected_index == search_idx ? BLUE : PINK));

        } else {
            strcpy(name, "");
        }
    }

    //Lives
    int lives = 20 - attempt_count - 1;

    for (int i = 0; i < (int)ceil((double)lives / 2); i++) {
        int sprite = 0;
        int hint_sprite = 0;
        if (i == lives / 2 && lives % 2 != 0) sprite = 1;
        if (i == 7 || i == 3 || i == 0 ) hint_sprite = 2;
        sprite += hint_sprite;
        DrawTexturePro(heart_texture, (Rectangle){16.0f * sprite, 16.0f * sprite, 16, 16}, (Rectangle){4 + 34.0f * i, 4, 32, 32}, (Vector2){0,0}, 0, WHITE);
    }

    if (lives <= 15) hint_1 = true;
    if (lives <= 7) hint_2 = true;
    if (lives <= 1) hint_3 = true;
    if (lives <= 0) lose = true;
}

bool is_gameplay_finished(void) {
    return game_finished;
}

void unload_gameplay_round(void) {
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
    // Limpar as texturas
    for (int i = 0; i < game_amount; i++) {
        if (games[i].flag_texture.id > 0) {
            UnloadTexture(games[i].flag_texture);
        }
        if (games[i].logo_texture.id > 0) {
            UnloadTexture(games[i].logo_texture);
        }
    }

    // Liberar memória dinâmica
    if (games != NULL) {
        free(games);
        games = NULL;
    }
    
    if (search_results != NULL) {
        free(search_results);
        search_results = NULL;
    }

    // Resetar contadores
    game_amount = 0;
    search_capacity = 0;
}