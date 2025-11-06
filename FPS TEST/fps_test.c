#include "raylib.h"
#include "raymath.h" // Necessário para Vector3Normalize e Vector3Subtract

int main(void)
{
    // --- 1. CONFIGURAÇÃO (SETUP) ---
    const int screen_width = 1280;
    const int screen_height = 720;

    InitWindow(screen_width, screen_height, "Raylib - FPS Mínimo 3D (Versão Antiga)");

    // Define a câmera 3D
    Camera camera = { 0 };
    camera.position = (Vector3){ 0.0f, 1.0f, -5.0f };
    camera.target = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 70.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    
    // SetCameraMode(camera, CAMERA_FIRST_PERSON); // << REMOVIDO: Esta função não existe na sua versão
    
    DisableCursor();

    // Cria um "mundo" (chão e parede)
    Mesh floor_mesh = GenMeshPlane(50.0f, 50.0f, 1, 1);
    Model floor_model = LoadModelFromMesh(floor_mesh);
    floor_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = GRAY;

    Mesh wall_mesh = GenMeshCube(8.0f, 5.0f, 1.0f);
    Model wall_model = LoadModelFromMesh(wall_mesh);
    wall_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = DARKGRAY;
    
    // Variáveis para o "tiro" (hitscan)
    Ray bullet_ray = { 0 };
    RayCollision hit_info = { 0 };
    Vector3 hit_marker_pos = { 0 };
    bool show_hit_marker = false;
    float hit_marker_timer = 0.0f;

    SetTargetFPS(60);

    // --- 2. LOOP PRINCIPAL DO JOGO ---
    while (!WindowShouldClose())
    {
        // --- 3. ATUALIZAÇÃO (LÓGICA) ---
        float delta_time = GetFrameTime();

        // MUDANÇA AQUI:
        // Passamos o modo de câmera em todo frame, como a versão antiga exige
        UpdateCamera(&camera, CAMERA_FIRST_PERSON);

        // Atualiza o timer da marca de acerto
        if (hit_marker_timer > 0)
        {
            hit_marker_timer -= delta_time;
        }
        else
        {
            show_hit_marker = false;
        }
        
        // Lógica de "Tiro"
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            bullet_ray.position = camera.position;

            // MUDANÇA AQUI:
            // Calculamos o vetor "para frente" manualmente
            // (GetCameraForward() não existe na sua versão)
            bullet_ray.direction = Vector3Normalize(Vector3Subtract(camera.target, camera.position));

            hit_info.hit = false;
            
            // MUDANÇA AQUI:
            // Usamos GetRayCollisionMesh, pois GetRayCollisionModel não existe
            // Precisamos checar a malha (mesh) dentro do modelo
            RayCollision wall_hit = GetRayCollisionMesh(bullet_ray, wall_model.meshes[0], wall_model.transform);
            if (wall_hit.hit)
            {
                hit_info = wall_hit;
            }

            // MUDANÇA AQUI:
            RayCollision floor_hit = GetRayCollisionMesh(bullet_ray, floor_model.meshes[0], floor_model.transform);
            
            if (floor_hit.hit && (floor_hit.distance < hit_info.distance || !hit_info.hit))
            {
                hit_info = floor_hit;
            }

            if (hit_info.hit)
            {
                hit_marker_pos = hit_info.point;
                show_hit_marker = true;
                hit_marker_timer = 0.5f;
            }
        }

        // --- 4. DESENHO ---
        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode3D(camera);

            DrawModel(floor_model, Vector3Zero(), 1.0f, WHITE);
            DrawModel(wall_model, (Vector3){ 0, 2.5f, 5.0f }, 1.0f, WHITE);
            DrawGrid(50, 1.0f);
            
            if (show_hit_marker)
            {
                DrawSphere(hit_marker_pos, 0.1f, RED);
            }

        EndMode3D();

        // --- Desenho 2D (UI) ---
        DrawLine(screen_width / 2, screen_height / 2 - 10, screen_width / 2, screen_height / 2 + 10, WHITE);
        DrawLine(screen_width / 2 - 10, screen_height / 2, screen_width / 2 + 10, screen_height / 2, WHITE);
        DrawText("Mova: W,A,S,D | Olhe: Mouse | Atire: Clique Esquerdo", 10, 10, 20, GREEN);
        DrawFPS(10, 35);

        EndDrawing();
    }

    // --- 5. LIMPEZA (CLEANUP) ---
    UnloadModel(floor_model);
    UnloadModel(wall_model);
    
    EnableCursor();
    CloseWindow();

    return 0;
}