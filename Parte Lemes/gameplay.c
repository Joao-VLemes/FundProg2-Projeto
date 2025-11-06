#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

static bool shaking = false;

// Splits the string in str_array[0] by "/" and stores parts in str_array[1] and str_array[2]
void split_string(char str_array[3][50]) {
    int index = 0;
    for (int i = 1; i < 3; i++) strcpy(str_array[i], "");
    if (strlen(str_array[0]) != strcspn(str_array[0], "/")) {
        char* token = strtok(str_array[0], "/");

        while (token != NULL) {
            strcpy(str_array[index], token);
            index++;
            token = strtok(NULL, "/");
        }
    } 
}

// Checks for equality or partial match between two string arrays
int check_equality(char a[3][50], char b[3][50]) {
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
    // printf("CountB: %d countA: %d\n", count_b, count_a);
    if (count_a == equal_count && count_a == count_b) return 1; // Full match
    if (equal_count > 0) return 2; // Partial match
    return 0; // No match
}

Vector2 shake (float power, float *time) {
    Vector2 offset = {0.0f, 0.0f};

    *time -= GetFrameTime();
            
    if (*time <= 0) {
        *time = 0;
        shaking = false;
    }
    else {
        float offsetX = (float)GetRandomValue(-100, 100) / 100.0f;
        float offsetY = (float)GetRandomValue(-100, 100) / 100.0f;
        
        // Apply the intensity to the random direction
        offset.x = offsetX * power;
        offset.y = offsetY * power;
    }
    return offset;
}

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
} game_t;



int main() {
    srand(time(NULL));

    #pragma region LOAD LIST
    FILE *list_file;
    game_t games[100];

    list_file = fopen("list.txt", "r");
    if (list_file == NULL){
        perror("LIST NOT LOADED\n");
        exit(1);
    }
    for (int i = 0; fscanf(list_file, "%49[^;];%d;%49[^;];%49[^;];%49[^;];%49[^;];%49[^;];\n", games[i].name, &games[i].year, games[i].origin, games[i].genre[0], games[i].theme, games[i].gamemode[0], games[i].platform[0]) == 7; i++) {
        split_string(games[i].genre);
        split_string(games[i].gamemode);
        split_string(games[i].platform);
        // for (int j = 0; j < 3; j++) printf("%s: %s\n", games[i].name, games[i].platform[j]);
    }

    fclose(list_file);
    #pragma endregion

    #pragma region LOAD PHRASES

    FILE *phrases_file;

    phrases_file = fopen("frases.txt", "r");
    if (phrases_file == NULL){
        perror("PHRASE LIST NOT LOADED\n");
        exit(1);
    }

    for (int i = 0; fscanf(phrases_file, " %199[^\n]\n", games[i].phrase) == 1; i++) {
        // printf("%s: %s\n", games[i].name, games[i].phrase);
    }

    #pragma endregion

    #pragma region WINDOW
    int screen_width = 1280;
    int screen_height = 720;
    InitWindow(screen_width, screen_height, "GAMEPLAY");
    #pragma endregion

    #pragma region GAMES
    char user_input[50] = "";

    game_t *attempts;
    int attempt_count = -1;
    int selected_attempt_index = 0;

    game_t search_results[5];
    int selected_search_index = 0;

    attempts = calloc(attempt_count + 1, sizeof(game_t));

    game_t correct_game;
    // int correct_index = (rand() % (100));
    // correct_game = games[0];
    correct_game = games[0];

    printf("%s\n", correct_game.name);
    #pragma endregion 

    int input_index = 0;
    int capslock_on = 0;

    bool hint_1 = false;
    bool hint_2 = false;
    bool hint_3 = false;

    //Arrow to indicate higher or lower year
    Image arrow_image = LoadImage("sources/flecha.png");
    Texture2D arrow_texture = LoadTextureFromImage(arrow_image);

    //Health heart
    Image heart_image = LoadImage("sources/coracao.png");
    Texture2D heart_texture = LoadTextureFromImage(heart_image);

    //Flag
    Image flag_image = LoadImage(TextFormat("sources/flags/%s.png", correct_game.origin));
    Texture2D flag_texture = LoadTextureFromImage(flag_image);

    //Cover Art
        //Cover art filename
    char game_name_formatted[40];

    // for (int i = 0; i < 100; i++) {
    //     strcpy(game_name_formatted, games[i].name);

    //     for (int i = 0; i < strlen(game_name_formatted); i++) {
    //         if (game_name_formatted[i] == ' ' || game_name_formatted[i] == ',' || game_name_formatted[i] == '-' || game_name_formatted[i] == ':'){ 
    //             if (i < 5) {
    //                 game_name_formatted[i] = '_';
    //             } else {
    //                 game_name_formatted[i] = '\0';
    //                 break;
    //             }
    //         }
    //     }

    //     printf("%s\n", game_name_formatted);
    // }

    strcpy(game_name_formatted, correct_game.name);
    for (int i = 0; i < (int)strlen(game_name_formatted); i++) {
        if (game_name_formatted[i] == ' ' || game_name_formatted[i] == ',' || game_name_formatted[i] == '-' || game_name_formatted[i] == ':'){ 
            if (i < 5) {
                game_name_formatted[i] = '_';
            } else {
                game_name_formatted[i] = '\0';
                break;
            }
        }
    }

        //Load the cover art
    char image_path[50];
    sprintf(image_path, "capas/capa_%s.jpg", game_name_formatted); 

    Image image = LoadImage(image_path);
    Texture2D cover_texture = LoadTextureFromImage(image);
    UnloadImage(image);

    //Shader for blurring
    Shader blur_shader = LoadShader(0, "capas/blur.fs");
    int view_size_loc = GetShaderLocation(blur_shader, "viewSize");
    RenderTexture2D blurred_object_rt = LoadRenderTexture(cover_texture.width, cover_texture.height);
    float texture_size[2] = { (float)cover_texture.width, (float)cover_texture.height };
    SetShaderValue(blur_shader, view_size_loc, texture_size, SHADER_UNIFORM_VEC2);

    // Camera
    Camera2D camera = {0};
    Vector2 baseCameraOffset = (Vector2){ screen_width/2, screen_height/2.0f };
    camera.offset = baseCameraOffset;
    camera.target = (Vector2){screen_width/2, screen_height/2};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    float shake_time = 0;
    float shake_power = 0;

    char name_temp[100];

    while (!WindowShouldClose()) {
        camera.offset = Vector2Add(baseCameraOffset, shake(shake_power, &shake_time));

        //Blur the image
        BeginTextureMode(blurred_object_rt);        
            ClearBackground(BLANK);
            
            BeginShaderMode(blur_shader);   
                DrawTexture(cover_texture, 0,0, WHITE);
            EndShaderMode();            

        EndTextureMode();

        int max_selection_index = 4;
        for (int i = 0; i < 5; i++) if (strcmp(search_results[i].name, "") == 0) max_selection_index--;

        //Arrows
        if (IsKeyPressed(KEY_DOWN)) {
            selected_search_index++;
            if (selected_search_index > max_selection_index) selected_search_index = 0;
        }
        if (IsKeyPressed(KEY_UP)) {
            selected_search_index--;
            if (selected_search_index < 0) selected_search_index = max_selection_index;
        }

        if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) {
            selected_attempt_index--;
        }

        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) {
            selected_attempt_index++;
            if (selected_attempt_index > attempt_count) selected_attempt_index = attempt_count;
        }
        if (selected_attempt_index <= 0) selected_attempt_index = 0;

        //Write user input
        int key_pressed = GetKeyPressed();
        if ((key_pressed >= 32) && (key_pressed <= 125) && input_index < 50) {
            if (capslock_on == 1 || (int)IsKeyDown(KEY_LEFT_SHIFT) == 1 || (int)IsKeyDown(KEY_RIGHT_SHIFT) == 1) user_input[input_index] = (char)key_pressed;
            else user_input[input_index] = tolower((char)key_pressed);
            
            selected_search_index = 0;
            input_index++;
            user_input[input_index] = '\0';
            key_pressed = GetCharPressed(); // Consumes the char press
        }

        if ((IsKeyPressedRepeat(KEY_BACKSPACE) || IsKeyPressed(KEY_BACKSPACE))&& input_index > 0) {
            if (IsKeyDown(KEY_LEFT_CONTROL)) {
                user_input[0] = '\0';
                input_index = 0;
            } else { // Check added to avoid out-of-bounds on ctrl+backspace
                input_index--;
                user_input[input_index] = '\0';
            }
        }

        //Search
        for (int l = 0; l < 5; l++) strcpy(search_results[l].name, "");
        if (user_input[0] != '\0') {
            for (int l = 0, j = 0; l < 100 && j < 5; l++) {
                char temp_substr[strlen(user_input)+1];
                for (int k = 0; k <= (int)strlen(user_input); k++) {
                    temp_substr[k] = games[l].name[k];
                }
                temp_substr[strlen(user_input)] = '\0';

                if (strcasecmp(user_input, temp_substr) == 0)   {
                    search_results[j] = games[l];
                    j++;
                }
            }
        }

        //Attempt
        if (IsKeyPressed(KEY_ENTER) && strcmp(search_results[selected_search_index].name, "") != 0) {
            attempt_count++;
            selected_attempt_index = attempt_count;
            // printf("%d\n", attempt_count);
            attempts = realloc(attempts, (attempt_count + 1) * sizeof(game_t));
            attempts[attempt_count] = search_results[selected_search_index];

            if (strcmp(search_results[selected_search_index].name, correct_game.name) != 0) {
                shaking = true;
                shake_time = 0.5;
                shake_power = 1.5;
            }

            strcpy(user_input, " ");
            for (int i = 0; i < 100; i++) if (strcmp(search_results[selected_search_index].name, games[i].name) == 0) strcpy(games[i].name, "");
            input_index = 0;

            if (strlen(attempts[attempt_count].name) >= 10) {
                for (int i = (int)strlen(attempts[attempt_count].name)/3; i < (int)strlen(attempts[attempt_count].name); i++) {
                    if (attempts[attempt_count].name[i] == ' ') {
                        attempts[attempt_count].name[i] = '\n';
                        break;
                    }

                }
            }
        }

        //Draw on screen
        BeginDrawing();

        BeginMode2D(camera);
        ClearBackground((Color){30,30,30,255});
        DrawText(user_input, screen_width/2-MeasureText(user_input, 30)/2, 30/2, 30, RAYWHITE);
        for (int j = 0; j < 5; j++) {
            DrawText(search_results[j].name, screen_width/2-MeasureText(search_results[j].name, 30)/2, 30*(2+1.5*j), 30, (selected_search_index == j ? BLUE : PINK));
        }

        if (attempt_count != -1) DrawText(TextFormat("%d", selected_attempt_index + 1), screen_width - MeasureText(TextFormat("%d", selected_attempt_index + 1), 30) - 30, 15, 30, WHITE);
        if (attempt_count != -1 && strcmp(attempts[selected_attempt_index].name, "") != 0) { // Added attempt_count check
            game_t selected_game = attempts[selected_attempt_index];
            
            DrawText(selected_game.name, 20, screen_height - 180, 30, PINK);

            int x_offset = 20;

            //YEAR
            if (selected_game.year == correct_game.year) DrawText(TextFormat("%d",selected_game.year), 20, screen_height - 80, 30, GREEN);
            else {
                // if (selected_game.year > correct_game.year) arrow_texture.height = -16; // Original comment was broken
                // else arrow_texture.height = 16;
                DrawText(TextFormat("%d",selected_game.year), x_offset, screen_height - 80, 30, RED);
                DrawTexturePro(arrow_texture, (Rectangle){0, 0, 15, 14}, (Rectangle){MeasureText(TextFormat("%d",selected_game.year), 30) + 35, screen_height - 67, 15, 14}, (Vector2){7,7}, (selected_game.year > correct_game.year ? 180 : 0), WHITE);   
                // DrawTextureEx(arrow_texture, (Vector2){MeasureText(year, 30) + 35, 413},  (selected_game.year > correct_game.year ? 180 : 0), 1, WHITE);
            }

            x_offset += MeasureText(TextFormat("%d",selected_game.year), 30) + 33;

            //ORIGIN

            if (strcmp(name_temp, selected_game.origin) != 0) {
                UnloadImage(flag_image);
                UnloadTexture(flag_texture);
                flag_image = LoadImage(TextFormat("sources/flags/%s.png", selected_game.origin));
                flag_texture = LoadTextureFromImage(flag_image);

                strcpy(name_temp, selected_game.origin);
            }

            

            DrawTexturePro(flag_texture, (Rectangle){0, 0, (float)flag_texture.width, (float)flag_texture.height}, (Rectangle){x_offset, screen_height - 80, flag_texture.width*2, flag_texture.height*2}, (Vector2){0,0}, 0, WHITE);
            DrawRectangleLinesEx((Rectangle){x_offset, screen_height - 80, flag_texture.width*2, flag_texture.height*2}, 2.5, (strcmp(selected_game.origin, correct_game.origin) == 0) ? GREEN : RED);
            // DrawText(selected_game.origin, x_offset, screen_height - 80, 30, (strcmp(selected_game.origin, correct_game.origin) == 0) ? GREEN : RED);

            x_offset += flag_texture.width*2 + 20;

            //GENRE
            int genre_equality = check_equality(correct_game.genre, selected_game.genre);
            if (genre_equality == 1) DrawText(TextFormat("%s\n%s", selected_game.genre[0], selected_game.genre[1]), x_offset, screen_height - 80, 30, GREEN);
            else DrawText(TextFormat("%s\n%s", selected_game.genre[0], selected_game.genre[1]), x_offset, screen_height - 80, 30, (genre_equality == 0) ? RED : YELLOW);

            x_offset += MeasureText(TextFormat("%s\n%s", selected_game.genre[0], selected_game.genre[1]), 30) + 20;

            //THEME
            if (strlen(selected_game.theme) != strcspn(selected_game.theme, " ")) {
                selected_game.theme[strcspn(selected_game.theme, " ")] = '\n';
            }

            DrawText(selected_game.theme, x_offset, screen_height - 80, 30, (strcmp(selected_game.theme, correct_game.theme) == 0) ? GREEN : RED);
            
            x_offset += MeasureText(selected_game.theme, 30) + 20;

            //GAMEMODE
            int gamemode_equality = check_equality(correct_game.gamemode, selected_game.gamemode);
            if (gamemode_equality == 1) DrawText(TextFormat("%s\n%s", selected_game.gamemode[0], selected_game.gamemode[1]), x_offset, screen_height - 80, 30, GREEN);
            else DrawText(TextFormat("%s\n%s", selected_game.gamemode[0], selected_game.gamemode[1]), x_offset, screen_height - 80, 30, (gamemode_equality == 0) ? RED : YELLOW);

            x_offset += MeasureText(TextFormat("%s\n%s", selected_game.gamemode[0], selected_game.gamemode[1]), 30) + 20;

            //PLATFORM
            int platform_equality = check_equality(correct_game.platform, selected_game.platform);
            int platform_count = 2;
            for (int i = 0; i < 3; i++) if (strcmp(selected_game.platform[i], "") == 0) platform_count--;
            if (platform_equality == 1) DrawText(TextFormat("%s\n%s\n%s", selected_game.platform[0], selected_game.platform[1], selected_game.platform[2]), x_offset, screen_height - 80 - 30*platform_count/2, 30, GREEN);
            else DrawText(TextFormat("%s\n%s\n%s", selected_game.platform[0], selected_game.platform[1], selected_game.platform[2]), x_offset, screen_height - 80 - 30*platform_count/2, 30, (platform_equality == 0) ? RED : YELLOW);

            x_offset += MeasureText(TextFormat("%s\n%s\n%s", selected_game.platform[0], selected_game.platform[1], selected_game.platform[2]), 30) + 10;
        }

        //BUTTON
        // Vector2 mouse_pos = GetMousePosition();
        // Vector2 button_1 = {screen_width-100, 30};
        // Vector2 button_2 = {screen_width-100, 75};
        // Vector2 button_3 = {screen_width-100, 120};

        // if (attempt_count > 3) DrawCircle(button_1.x, button_1.y, 15, (hint_1) ? BLUE : WHITE);
        // if (attempt_count > 6) DrawCircle(button_2.x, button_2.y, 15, (hint_2) ? BLUE : WHITE);
        // if (attempt_count > 8) DrawCircle(button_3.x, button_3.y, 15, (hint_3) ? BLUE : WHITE);


        // float dist_1 = sqrt(pow(mouse_pos.x - button_1.x, 2) + pow(mouse_pos.y - button_1.y, 2));
        // float dist_2 = sqrt(pow(mouse_pos.x - button_2.x, 2) + pow(mouse_pos.y - button_2.y, 2));
        // float dist_3 = sqrt(pow(mouse_pos.x - button_3.x, 2) + pow(mouse_pos.y - button_3.y, 2));

        // if (dist_1 < 15 && attempt_count > 3) if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) hint_1 = true;
        // if (dist_2 < 15 && attempt_count > 6) if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) hint_2 = true;
        // if (dist_3 < 15 && attempt_count > 8) if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) hint_3 = true;

        //Lives
        int lives = 20 - attempt_count - 1;

        for (int i = 0; i < (int)ceil((double)lives / 2); i++) {
            int sprite = 0;
            Color color = WHITE;
            if (i == lives / 2 && lives % 2 != 0) sprite = 1;
            if (i == 15 || i == 7 || i == 3 ) color = (Color){240,255,0,255}; // Example: special color for thresholds
            DrawTexturePro(heart_texture, (Rectangle){8.0f * sprite, 8.0f * sprite, 8, 8}, (Rectangle){24.0f * i, 0, 32, 32}, (Vector2){0,0}, 0, color);
        }

        if (lives <= 15) hint_1 = true;
        if (lives <= 7) hint_2 = true;
        if (lives <= 3) hint_3 = true;

        //Hints
        if (hint_1) {
            // Example: Draw the hint text instead of printing to console
            DrawText(correct_game.phrase, screen_width / 2 - MeasureText(correct_game.phrase, 20) / 2, 250, 20, YELLOW);
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

        EndMode2D();
        EndDrawing();
    }

    UnloadRenderTexture(blurred_object_rt);
    UnloadShader(blur_shader);
    UnloadTexture(arrow_texture);
    UnloadImage(arrow_image);
    UnloadTexture(flag_texture);
    UnloadImage(flag_image);
    UnloadTexture(heart_texture);
    UnloadImage(heart_image);
    UnloadTexture(cover_texture);
    free(attempts);
    return 0;
}