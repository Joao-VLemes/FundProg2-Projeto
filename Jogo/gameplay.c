#include "gameplay.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#pragma region Globais e estados

/**
 * Definições globais de resolução e estados de vitória/derrota.
 * Estas variáveis controlam o fluxo macro do jogo.
 */
int screen_width = 1280;
int screen_height = 720;
bool win = false;
bool lose = false;

/**
 * Gerenciamento dinâmico da lista de jogos.
 * "games" armazena o banco de dados completo carregado do CSV.
 * "search_results" é um buffer para mostrar sugestões enquanto o usuário digita.
 */
static int game_properties_amount = 8; // Atualizado para 8 propriedades
static int game_amount = 0;
static Game *games = NULL;
static Game *search_results = NULL;
static int search_capacity = 0;

static Game correct_game;
static Game *attempts = NULL;

static int attempt_count = -1;
static int selected_attempt_index = 0;

static int max_search_results = 0;
static int selected_search_index = 0;

// Variáveis para controlar a entrada de texto e o estado das dicas
static char user_input[50] = "";
static int input_index = 0;
static bool hint_1 = false;
static bool hint_2 = false;
static bool hint_3 = false;
static bool game_finished = false;

// Recursos gráficos e shaders usados para efeitos visuais na rodada
static Shader blur_shader = {0};
static RenderTexture2D blurred_object_rt = {0};
static Camera2D game_camera = {0};
static Vector2 base_camera_offset = {0};

// Controle do efeito de "tremer" a tela (screen shake) quando erra
static bool shaking = false;
static bool moving_up = false;
static float shake_time = 0;
static float shake_power = 0;

static Texture2D heart_texture;
static Texture2D arrow_texture;
static Texture2D cover_texture;

#pragma endregion

#pragma region Utilitários

/**
 * Quebra uma string contendo barras (ex: "Ação/Aventura") em até 3 partes.
 * Útil para comparar gêneros ou plataformas separadamente.
 */
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

/**
 * Compara dois arrays de strings (atributos do jogo) para determinar a similaridade.
 * Retorna 1 se for exato, 2 se houver correspondência parcial, e 0 se nada bater.
 */
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

/**
 * Calcula o deslocamento aleatório da câmera para o efeito de impacto.
 */
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

/**
 * Ativa o efeito de tremor na tela com uma força e duração específicas.
 */
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

#pragma endregion

#pragma region Gerenciamento de dados

/**
 * Interface de terminal para cadastrar um novo jogo no banco de dados (CSV).
 * Solicita os dados passo a passo e anexa aos arquivos correspondentes.
 */
static void terminal_add_game() {
    char name[50], origin[50], genre[50], theme[50], gamemode[50], platform[50], company[50], phrase[200];
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
    printf("Empresa (Ex: Nintendo): ");
    get_user_string(company, 50);
    printf("Frase (Dica): ");
    get_user_string(phrase, 200);

    // Abre os arquivos para adicionar conteúdo no final
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

    fprintf(list_file, "%s;%d;%s;%s;%s;%s;%s;%s;\n",
            name, year, origin, genre, theme, gamemode, platform, company);
    
    fprintf(phrases_file, "%s\n", phrase);

    fclose(list_file);
    fclose(phrases_file);
    fclose(capa_file);
    fclose(logo_file);

    printf("Jogo '%s' adicionado com sucesso!\n", name);
}

/**
 * Lista todos os jogos presentes no arquivo CSV no console.
 */
static void terminal_list_games() {
    FILE *list_file = fopen("list.csv", "r");
    if (list_file == NULL) {
        perror("Nao foi possivel ler 'list.csv'");
        return;
    }

    printf("Lista de jogos cadastrados:\n");
    char name[50];
    int count = 0;
    
    // Lê apenas o primeiro campo (nome) ignorando o resto da linha
    while (fscanf(list_file, "%49[^;];%*[^\n]\n", name) == 1) {
        printf("%d. %s\n", ++count, name);
    }

    fclose(list_file);
    if (count == 0) {
        printf("Nenhum jogo encontrado.\n");
    }
    printf("----------------------------------\n");
}

/**
 * Busca simples por substring no arquivo CSV para encontrar jogos específicos.
 */
static void terminal_search_game() {
    char search_name[50];
    printf("Digite o nome (ou parte) do jogo a pesquisar: ");
    get_user_string(search_name, 50);

    FILE *list_file = fopen("list.csv", "r");
    if (list_file == NULL) {
        perror("Nao foi possivel ler 'list.csv'");
        return;
    }

    char line_buffer[512]; 
    char game_name[50];
    bool found = false;

    printf("Resultados da busca:\n");
    while (fgets(line_buffer, sizeof(line_buffer), list_file)) {
        // Usamos um buffer temporário para não destruir a linha original com o strtok
        char temp_line[512];
        strcpy(temp_line, line_buffer);
        
        char *token = strtok(temp_line, ";");
        if (token != NULL) {
            strcpy(game_name, token);
            
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

/**
 * Função auxiliar robusta para modificar ou excluir registros.
 * Ela cria arquivos temporários, copia os dados (alterando ou pulando a linha alvo)
 * e depois substitui os arquivos originais.
 * * Se new_data for NULL, a função age como "Excluir".
 * Se new_data tiver conteúdo, a função age como "Alterar".
 */
static int rewrite_files(const char* target_name, const Game* new_data) {
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
        return 0;
    }

    char line_buffer[512], phrase_buffer[256];
    char game_name[50];
    int found = 0;

    // Processa linha a linha mantendo a sincronia entre os dois arquivos
    while (fgets(line_buffer, sizeof(line_buffer), list_in) &&
           fgets(phrase_buffer, sizeof(phrase_buffer), phrases_in)) 
    {
        char temp_line[512];
        strcpy(temp_line, line_buffer);
        char *token = strtok(temp_line, ";");
        strcpy(game_name, token ? token : "");

        // Se encontrou o jogo alvo
        if (strcmp(game_name, target_name) == 0) {
            found = 1;
            if (new_data != NULL) {
                // Reescreve com os dados novos
                fprintf(list_out, "%s;%d;%s;%s;%s;%s;%s;%s;\n",
                        new_data->name, new_data->year, new_data->origin,
                        new_data->genre[0], 
                        new_data->theme,
                        new_data->gamemode[0],
                        new_data->platform[0],
                        new_data->company);
                fprintf(phrases_out, "%s\n", new_data->phrase);
            }
            // Se for NULL (excluir), simplesmente não escreve nada no arquivo de saída
        } else {
            // Mantém os dados originais
            fputs(line_buffer, list_out);
            fputs(phrase_buffer, phrases_out);
        }
    }

    fclose(list_in);
    fclose(phrases_in);
    fclose(list_out);
    fclose(phrases_out);

    if (found) {
        remove("list.csv");
        remove("frases.csv");
        rename("list.tmp", "list.csv");
        rename("phrases.tmp", "frases.csv");
    } else {
        remove("list.tmp");
        remove("phrases.tmp");
    }
    
    return found;
}

// Função para excluir os dados do jogo que desejar
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

// Função para modificar os dados do jogo que desejar
static void terminal_modify_game() {
    char name_to_modify[50];
    printf("Alterar jogo:\n");
    printf("Digite o nome EXATO do jogo a alterar: ");
    get_user_string(name_to_modify, 50);

    Game new_data;

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
    printf("Nova Empresa: ");
    get_user_string(new_data.company, 50);
    printf("Nova Frase (Dica): ");
    get_user_string(new_data.phrase, 200);

    if (rewrite_files(name_to_modify, &new_data)) {
        printf("Jogo '%s' alterado para '%s' com sucesso.\n", name_to_modify, new_data.name);
    } else {
        printf("Jogo '%s' nao encontrado.\n", name_to_modify);
    }
}

void initialize_list() {
    games = NULL;
    game_amount = 0;
    search_results = NULL;
    search_capacity = 0;
    attempts = NULL;
}

/**
 * Menu principal do modo terminal.
 * Permite gerenciar o banco de dados antes de iniciar o loop gráfico do jogo.
 */
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
            choice = -1;
        }
        clear_stdin_buffer(); 

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
                return;
            default:
                printf("Opcao invalida. Tente novamente.\n");
                break;
        }
        
        printf("\nPressione ENTER para continuar...");
        getchar();
    }
}

/**
 * Carrega todos os dados dos arquivos CSV para a memória.
 * Aloca dinamicamente os arrays de jogos e de resultados de busca, 
 * além de carregar as texturas (bandeiras, logos).
 */
void load_list() {
    FILE *list_file = fopen("list.csv", "r");
    if (list_file == NULL) { perror("Erro list.csv"); exit(1); }
    
    game_amount = 0;
    char buffer[512];
    while (fgets(buffer, sizeof(buffer), list_file)) game_amount++;
    rewind(list_file); // Volta ao inicio do arquivo

    if (game_amount == 0) exit(1);

    games = (Game*)calloc(game_amount, sizeof(Game));
    search_results = (Game*)calloc(game_amount, sizeof(Game));
    search_capacity = game_amount;

    for (int i = 0; i < game_amount; i++) {
        int read_count = fscanf(list_file, "%49[^;];%d;%49[^;];%49[^;];%49[^;];%49[^;];%49[^;];%49[^;\n]", 
            games[i].name,
            &games[i].year,
            games[i].origin,
            games[i].genre[0], 
            games[i].theme,
            games[i].gamemode[0],
            games[i].platform[0],
            games[i].company 
        );

        // Limpa o resto da linha (o \n e possíveis ; extras)
        // Isso garante que o ponteiro do arquivo comece limpo na próxima linha
        int c;
        while ((c = fgetc(list_file)) != '\n' && c != EOF);

        if (read_count != game_properties_amount) {
            fprintf(stderr, "Erro de formato linha %d. Lido %d campos.\n", i + 1, read_count);
            // Não damos break para tentar ler os próximos, mas idealmente deveria arrumar o CSV
        }
        
        // Backup e Split (mantido igual)
        char original_genre[50], original_gamemode[50], original_platform[50];
        strcpy(original_genre, games[i].genre[0]);
        strcpy(original_gamemode, games[i].gamemode[0]);
        strcpy(original_platform, games[i].platform[0]);

        split_string(games[i].genre);
        split_string(games[i].gamemode);
        split_string(games[i].platform);

        strcpy(games[i].genre[0], original_genre);
        strcpy(games[i].gamemode[0], original_gamemode);
        strcpy(games[i].platform[0], original_platform);

        // Assets
        Image flag_image = LoadImage(TextFormat("sources/flags/%s.png", games[i].origin));
        if (flag_image.data == NULL) flag_image = LoadImage("sources/flags/flag.png");
        games[i].flag_texture = LoadTextureFromImage(flag_image);
        UnloadImage(flag_image);

        Image logo_image = LoadImage(TextFormat("logos/logo %s.png", games[i].name));
        games[i].logo_texture = LoadTextureFromImage(logo_image);
        UnloadImage(logo_image);
        
        if (games[i].logo_texture.height != 0) games[i].logo_texture.width = 64 * games[i].logo_texture.width/games[i].logo_texture.height;
        games[i].logo_texture.height = 64;
    }
    fclose(list_file);

    FILE *phrases_file = fopen("frases.csv", "r");
    if (phrases_file) {
        for (int i = 0; i < game_amount; i++) fscanf(phrases_file, " %199[^\n]\n", games[i].phrase);
        fclose(phrases_file);
    }
}

#pragma endregion

#pragma region Configurações

// Função para carregar as texturas e as imagens auxiliares, como corações, bandeiras etc
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

/**
 * Seleciona aleatoriamente um jogo da lista para ser o desafio da rodada.
 */
void load_games() {
    if (game_amount <= 0) {
        printf("ERRO FATAL: load_games chamada sem jogos carregados!\n");
        exit(1);
    }
    int correct_game_index = (rand() % (game_amount));
    correct_game = games[correct_game_index];
    
    // Prepara os dados do jogo correto para facilitar comparações
    split_string(correct_game.genre);
    split_string(correct_game.gamemode);
    split_string(correct_game.platform);
}

/**
 * Reseta todas as variáveis de estado do gameplay para iniciar uma nova partida.
 * Inclui resets de input, câmera, shader de blur e contadores.
 */
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

#pragma endregion

#pragma region Lógica de gameplay

/**
 * Função principal de atualização do jogo (game Loop).
 * Processa inputs, atualiza a câmera, gerencia a barra de busca e valida tentativas.
 */
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

    // Renderiza a capa do jogo borrada para usar como dica visual
    BeginTextureMode(blurred_object_rt);        
        ClearBackground(BLANK);
        BeginShaderMode(blur_shader);   
            DrawTexture(cover_texture, 0,0, WHITE);
        EndShaderMode();            
    EndTextureMode();

    // Navegação na lista de sugestões
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

    // Captura de texto do usuário
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

    // Lógica de busca e autocompletar
    int j = 0;
    max_search_results = -1;

    if (user_input[0] != '\0') {
        for (int l = 0; l < game_amount; l++) { 
            if (strlen(games[l].name) == 0) continue;

            // Cria substring do nome do jogo do tamanho do input para comparar
            char temp_substr[strlen(user_input)+1];
            for (int k = 0; k <= (int)strlen(user_input); k++) {
                temp_substr[k] = games[l].name[k];
            }
            temp_substr[strlen(user_input)] = '\0';

            if (strcasecmp(user_input, temp_substr) == 0) {
                // Evita mostrar jogos que já foram tentados
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
                    if (j >= search_capacity) break; 
                }
            }
        }
    }

    // Confirmação da tentativa (enter)
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
        
        // Processa os dados da tentativa para renderização
        split_string(attempts[attempt_count].genre);
        split_string(attempts[attempt_count].gamemode);
        split_string(attempts[attempt_count].platform);

        // Verifica vitória ou aplica penalidade (shake)
        if (strcmp(attempts[attempt_count].name, correct_game.name) != 0) shake(3, 0.4);
        else win = true;

        // Limpa o input
        strcpy(user_input, " ");
        input_index = 0;
        
        max_search_results = -1;
        selected_search_index = 0;
    }

    // Controle da câmera (scroll do mouse)
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

#pragma endregion

#pragma region Renderização

/**
 * Renderiza o "mundo" do jogo, ou seja, a lista de tentativas passadas.
 * Compara cada atributo (ano, gênero, plataforma) com o jogo correto
 * e desenha as dicas coloridas (Verde = Certo, Vermelho = Errado, Amarelo = Parcial).
 */
void draw_gameplay_world(void) {
    if (attempt_count != -1 ) {
        // Layout
        int row_height = 185;
        int font_size = 30;
        int padding = 20;
        
        int y_off_logo = 180;
        int y_off_header = 110;
        int y_off_value = 80;
        
        // Configuração da sombra
        int shadow_offset = 2; 

        // Paleta de cores
        Color col_header = GRAY; // Títulos das colunas (YEAR, ORIGIN...)
        Color col_game_name = PINK; // Nome do jogo ao lado do logo
        Color col_correct = GREEN; // Valor correto
        Color col_wrong = RED; // Valor incorreto
        Color col_partial = YELLOW; // Valor parcialmente correto
        Color col_shadow = BLACK; // Sombra/outline de texturas
        Color col_tint = WHITE; // Cor padrão

        for (int i = 0; i < attempt_count+1; i++){
            Game selected_game = attempts[attempt_count-i];
            
            // Cálculos de posição y
            int current_row_y = i * row_height;
            int pos_y_logo = screen_height - y_off_logo + current_row_y;
            int pos_y_header = screen_height - y_off_header + current_row_y;
            int pos_y_value = screen_height - y_off_value + current_row_y;

            // Formatação do nome
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
            
            // Desenha nome e logo
            // Shadow
            DrawText(name_to_draw, 50 + selected_game.logo_texture.width + shadow_offset, pos_y_logo + shadow_offset, font_size, col_shadow);
            // Main Text
            DrawText(name_to_draw, 50 + selected_game.logo_texture.width, pos_y_logo, font_size, col_game_name);
            
            DrawTextureEx(selected_game.logo_texture, (Vector2){20 + shadow_offset, pos_y_logo + shadow_offset}, 0, 1, col_shadow);
            DrawTexture(selected_game.logo_texture, 20, pos_y_logo, col_tint); 
            
            int x_offset = 20;
            int x_diff = 0;

            // YEAR
            DrawText("ANO", x_offset + shadow_offset, pos_y_header + shadow_offset, font_size, col_shadow);
            DrawText("ANO", x_offset, pos_y_header, font_size, col_header);
            
            if (selected_game.year == correct_game.year) {
                DrawText(TextFormat("%d", selected_game.year), padding + shadow_offset, pos_y_value + shadow_offset, font_size, col_shadow);
                DrawText(TextFormat("%d", selected_game.year), padding, pos_y_value, font_size, col_correct);
            } else {
                DrawText(TextFormat("%d", selected_game.year), x_offset + shadow_offset, pos_y_value + shadow_offset, font_size, col_shadow);
                DrawText(TextFormat("%d", selected_game.year), x_offset, pos_y_value, font_size, col_wrong);
                
                int arrow_x = MeasureText(TextFormat("%d", selected_game.year), font_size) + 35;
                int arrow_y = screen_height - 67 + current_row_y; 
                
                // Sombra para a seta (opcional, mas consistente)
                DrawTexturePro(arrow_texture, (Rectangle){0, 0, 15, 14}, (Rectangle){arrow_x + 2, arrow_y + 2, 15, 14}, (Vector2){7,7}, (selected_game.year > correct_game.year ? 180 : 0), col_shadow);    
                DrawTexturePro(arrow_texture, (Rectangle){0, 0, 15, 14}, (Rectangle){arrow_x, arrow_y, 15, 14}, (Vector2){7,7}, (selected_game.year > correct_game.year ? 180 : 0), col_tint);    
            }
            
            x_diff = MeasureText(TextFormat("%d", selected_game.year), font_size) + 33;
            x_offset += x_diff + fmaxf(0, MeasureText("ANO", font_size) + padding - x_diff);
            

            // ORIGIN
            DrawText("ORIGEM", x_offset + shadow_offset, pos_y_header + shadow_offset, font_size, col_shadow);
            DrawText("ORIGEM", x_offset, pos_y_header, font_size, col_header);
            
            // Sombra da bandeira (offset manual no rect de destino)
            DrawTexturePro(selected_game.flag_texture, (Rectangle){0, 0, (float)selected_game.flag_texture.width, (float)selected_game.flag_texture.height}, (Rectangle){x_offset + 2, pos_y_value + 2, selected_game.flag_texture.width*2, selected_game.flag_texture.height*2}, (Vector2){0,0}, 0, col_shadow);
            DrawTexturePro(selected_game.flag_texture, (Rectangle){0, 0, (float)selected_game.flag_texture.width, (float)selected_game.flag_texture.height}, (Rectangle){x_offset, pos_y_value, selected_game.flag_texture.width*2, selected_game.flag_texture.height*2}, (Vector2){0,0}, 0, col_tint);
            
            Color border_color = (strcmp(selected_game.origin, correct_game.origin) == 0) ? col_correct : col_wrong;
            DrawRectangleLinesEx((Rectangle){x_offset,  pos_y_value, selected_game.flag_texture.width*2, selected_game.flag_texture.height*2 }, 2.5, border_color);
            
            x_diff = selected_game.flag_texture.width*2 + padding;
            x_offset += x_diff + fmaxf(0, MeasureText("ORIGEM", font_size) + padding - x_diff);

            // GENRE
            DrawText("GÊNERO", x_offset + shadow_offset, pos_y_header + shadow_offset, font_size, col_shadow);
            DrawText("GÊNERO", x_offset, pos_y_header, font_size, col_header);
            int genre_equality = check_equality(correct_game.genre, selected_game.genre);
            
            if (genre_equality == 1) {
                DrawText(TextFormat("%s\n%s", selected_game.genre[0], selected_game.genre[1]), x_offset + shadow_offset, pos_y_value + shadow_offset, font_size, col_shadow);
                DrawText(TextFormat("%s\n%s", selected_game.genre[0], selected_game.genre[1]), x_offset,  pos_y_value, font_size, col_correct);
            } else {
                DrawText(TextFormat("%s\n%s", selected_game.genre[0], selected_game.genre[1]), x_offset + shadow_offset, pos_y_value + shadow_offset, font_size, col_shadow);
                DrawText(TextFormat("%s\n%s", selected_game.genre[0], selected_game.genre[1]), x_offset,  pos_y_value, font_size, (genre_equality == 0) ? col_wrong : col_partial);
            }
            
            x_diff = MeasureText(TextFormat("%s\n%s", selected_game.genre[0], selected_game.genre[1]), font_size) + padding;
            x_offset += x_diff + fmaxf(0, MeasureText("GÊNERO", font_size) + padding - x_diff);

            // THEME
            DrawText("TEMA", x_offset + shadow_offset, pos_y_header + shadow_offset, font_size, col_shadow);
            DrawText("TEMA", x_offset, pos_y_header, font_size, col_header);
            
            char theme_to_draw[50];
            strcpy(theme_to_draw, selected_game.theme);
            if (strlen(theme_to_draw) != strcspn(theme_to_draw, " ")) {
                theme_to_draw[strcspn(theme_to_draw, " ")] = '\n';
            }
            
            Color theme_color = (strcmp(selected_game.theme, correct_game.theme) == 0) ? col_correct : col_wrong;
            DrawText(theme_to_draw, x_offset + shadow_offset, pos_y_value + shadow_offset, font_size, col_shadow);
            DrawText(theme_to_draw, x_offset,  pos_y_value, font_size, theme_color);
            
            x_diff = MeasureText(theme_to_draw, font_size) + padding;
            x_offset += x_diff + fmaxf(0, MeasureText("TEMA", font_size) + padding - x_diff);

            // GAMEMODE
            DrawText("GAMEMODE", x_offset + shadow_offset, pos_y_header + shadow_offset, font_size, col_shadow);
            DrawText("GAMEMODE", x_offset, pos_y_header, font_size, col_header);
            int gamemode_equality = check_equality(correct_game.gamemode, selected_game.gamemode);
            
            if (gamemode_equality == 1) {
                DrawText(TextFormat("%s\n%s", selected_game.gamemode[0], selected_game.gamemode[1]), x_offset + shadow_offset, pos_y_value + shadow_offset, font_size, col_shadow);
                DrawText(TextFormat("%s\n%s", selected_game.gamemode[0], selected_game.gamemode[1]), x_offset,  pos_y_value, font_size, col_correct);
            } else {
                DrawText(TextFormat("%s\n%s", selected_game.gamemode[0], selected_game.gamemode[1]), x_offset + shadow_offset, pos_y_value + shadow_offset, font_size, col_shadow);
                DrawText(TextFormat("%s\n%s", selected_game.gamemode[0], selected_game.gamemode[1]), x_offset,  pos_y_value, font_size, (gamemode_equality == 0) ? col_wrong : col_partial);
            }
            
            x_diff = MeasureText(TextFormat("%s\n%s", selected_game.gamemode[0], selected_game.gamemode[1]), font_size) + padding;
            x_offset += x_diff + fmaxf(0, MeasureText("GAMEMODE", font_size) + padding - x_diff);

            // PLATFORM
            int platform_equality = check_equality(correct_game.platform, selected_game.platform);
            int platform_count = 0;
            for (int k = 0; k < 3; k++) if (strlen(selected_game.platform[k]) > 0) platform_count++;
            
            int platform_y_adjustment = (platform_count > 1 ? 15 * (platform_count-1) : 0);

            DrawText("PLATAFORMA", x_offset + shadow_offset, pos_y_header - platform_y_adjustment + shadow_offset, font_size, col_shadow);
            DrawText("PLATAFORMA", x_offset, pos_y_header - platform_y_adjustment, font_size, col_header);
            
            if (platform_equality == 1) {
                DrawText(TextFormat("%s\n%s\n%s", selected_game.platform[0], selected_game.platform[1], selected_game.platform[2]), x_offset + shadow_offset, pos_y_value - platform_y_adjustment + shadow_offset, font_size, col_shadow);
                DrawText(TextFormat("%s\n%s\n%s", selected_game.platform[0], selected_game.platform[1], selected_game.platform[2]), x_offset,  pos_y_value - platform_y_adjustment, font_size, col_correct);
            } else {
                DrawText(TextFormat("%s\n%s\n%s", selected_game.platform[0], selected_game.platform[1], selected_game.platform[2]), x_offset + shadow_offset, pos_y_value - platform_y_adjustment + shadow_offset, font_size, col_shadow);
                DrawText(TextFormat("%s\n%s\n%s", selected_game.platform[0], selected_game.platform[1], selected_game.platform[2]), x_offset,  pos_y_value - platform_y_adjustment, font_size, (platform_equality == 0) ? col_wrong : col_partial);
            }
            
            x_diff = MeasureText(TextFormat("%s\n%s\n%s", selected_game.platform[0], selected_game.platform[1], selected_game.platform[2]), font_size) + 10;
            x_offset += x_diff + fmaxf(0, MeasureText("PLATAFORMA", font_size) + padding - x_diff);

            // EMPRESA
            DrawText("EMPRESA", x_offset + shadow_offset, pos_y_header + shadow_offset, font_size, col_shadow);
            DrawText("EMPRESA", x_offset, pos_y_header, font_size, col_header);
            
            Color company_color = (strcmp(selected_game.company, correct_game.company) == 0) ? col_correct : col_wrong;
            
            // Formatação do nome da empresa para quebra de linha se longo
            char company_draw[50];
            strcpy(company_draw, selected_game.company);
            if (strlen(company_draw) != strcspn(company_draw, " ")) {
                company_draw[strcspn(company_draw, " ")] = '\n';
            }

            DrawText(company_draw, x_offset + shadow_offset, pos_y_value + shadow_offset, font_size, col_shadow);
            DrawText(company_draw, x_offset, pos_y_value, font_size, company_color);
            
            x_diff = MeasureText(company_draw, font_size) + padding;
            x_offset += x_diff + fmaxf(0, MeasureText("EMPRESA", font_size) + padding - x_diff);
        }
    }

    DrawRectangle(screen_width - cover_texture.width - 20, 20, blurred_object_rt.texture.width, blurred_object_rt.texture.height, Fade(BLACK, 0.25f));

    // Renderiza as dicas (frase, imagem borrada, imagem nítida)
    if (hint_1) {
        DrawText(correct_game.phrase, screen_width / 2 - MeasureText(correct_game.phrase, 30) / 2 + 2, 500 + 2, 30, BLACK);
        DrawText(correct_game.phrase, screen_width / 2 - MeasureText(correct_game.phrase, 30) / 2, 500, 30, YELLOW);
    }
    if (hint_2) {
        Rectangle sourceRect = { 0, 0, (float)blurred_object_rt.texture.width, (float)-blurred_object_rt.texture.height };
        Vector2 destPos = { screen_width - cover_texture.width - 20.0f, 20};
        DrawTextureRec(blurred_object_rt.texture, sourceRect, destPos, WHITE);
    }
    if (hint_3) {
        DrawTexture(cover_texture, screen_width - cover_texture.width - 20, 20, WHITE);
    }
}

/**
 * Renderiza a interface de usuário (UI) fixa na tela.
 * Inclui barra de pesquisa, lista de sugestões e os corações de vida.
 */
void draw_gameplay_ui(void) {
    if (strcmp(user_input, "") != 0 && strcmp(user_input, " ") != 0) DrawRectangle(screen_width/2-MeasureText(user_input, 30)/2-5, 30/2-5, MeasureText(user_input, 30)+10, 30+10, LIGHTGRAY);
    DrawText(user_input, screen_width/2-MeasureText(user_input, 30)/2, 30/2, 30, BLACK);
    
    // Desenha a lista drop-down com as sugestões de pesquisa
    for (int j = 0; j < 5; j++) {
        char name[50];
        if (max_search_results >= 0) {
            int search_idx = j;
            // Lógica de scroll na lista de sugestões
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

    // Calcula e desenha as vidas restantes (corações)
    int lives = 20 - attempt_count - 1;

    for (int i = 0; i < (int)ceil((double)lives / 2); i++) {
        int sprite = 0;
        int hint_sprite = 0;
        if (i == lives / 2 && lives % 2 != 0) sprite = 1; // Meio coração
        if (i == 7 || i == 3 || i == 0 ) hint_sprite = 2; // Coração dourado/especial (indicador de dica)
        sprite += hint_sprite;
        DrawTexturePro(heart_texture, (Rectangle){16.0f * sprite, 16.0f * sprite, 16, 16}, (Rectangle){4 + 34.0f * i + 2, 4 + 2, 32, 32}, (Vector2){0,0}, 0, BLACK);
        DrawTexturePro(heart_texture, (Rectangle){16.0f * sprite, 16.0f * sprite, 16, 16}, (Rectangle){4 + 34.0f * i, 4, 32, 32}, (Vector2){0,0}, 0, WHITE);
    }

    // Ativa as dicas conforme a vida diminui
    if (lives <= 15) hint_1 = true;
    if (lives <= 7) hint_2 = true;
    if (lives <= 1) hint_3 = true;
    if (lives <= 0) lose = true;
}

#pragma endregion

#pragma region Ciclo de vida e limpeza

bool is_gameplay_finished(void) {
    return game_finished;
}

/**
 * Limpeza de recursos da rodada atual (texturas, shaders e memória de tentativas).
 */
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

/**
 * Limpeza global de recursos ao fechar o jogo.
 * Libera a memória da lista principal de jogos e texturas carregadas.
 */
void unload_global_assets(void) {
    for (int i = 0; i < game_amount; i++) {
        if (games[i].flag_texture.id > 0) {
            UnloadTexture(games[i].flag_texture);
        }
        if (games[i].logo_texture.id > 0) {
            UnloadTexture(games[i].logo_texture);
        }
    }

    if (games != NULL) {
        free(games);
        games = NULL;
    }
    
    if (search_results != NULL) {
        free(search_results);
        search_results = NULL;
    }

    game_amount = 0;
    search_capacity = 0;
}

#pragma endregion