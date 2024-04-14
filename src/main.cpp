#define RAYGUI_IMPLEMENTATION

#include <iostream>
#include <thread>
#include <future>
#include <cmath>

#include "raylib.h"
#include "raygui.h"
#include "flecs.h"

#define SCREEN_WIDTH 500
#define SCREEN_HEIGHT 525
#define LOGIC_UPDATE_INTERVAL 0.5f
#define POPULATION_LIMIT_INCREASE_SPEED 1.01

#define RESIDENTIAL_COLOR GREEN
#define COMMERCIAL_COLOR YELLOW

#define RESIDENTIAL_BUILDING_RESOURCE_COST 50
#define COMMERICAL_BUILDING_RESOURCE_COST 100

#define RESIDENTIAL_POPULATION_COUNT 3
#define COMMERICAL_RESOURCE_COUNT 10
#define COMMERCIAL_POPULATION_COST 5

using namespace std;

int roundToFloor25(int value) {
    return (value / 25) * 25;
}

enum BUILDING_TYPE{
    RESIDENTIAL,
    COMMERICAL
};

std::string BUILDING_TYPE_to_string(BUILDING_TYPE value) {
    switch (value) {
        case BUILDING_TYPE::RESIDENTIAL: return "RESI";
        case BUILDING_TYPE::COMMERICAL: return "COMM";
        default: return "Unknown";
    }
}

struct WorldValues{
    int available_population;
    float available_resources;
    int total_population;
    int resource_income_per_frame;
    BUILDING_TYPE current_type;
    double last_updated;
    float maintain_population; // no lower than this level else player loses
    double game_start_time;
};

struct Building{
    Vector2 position;
    BUILDING_TYPE current_type;
};

flecs::world world;
Camera2D camera = { 0 };
WorldValues world_value_singleton = {
    20,
    100,
    20,
    0,
    RESIDENTIAL,
    GetTime(),
    0,
    GetTime()
};

void DrawGrid2D(int width, int height, float cellSize, Color color) {
    for (int x = 0; x <= width; x++) {
        DrawLine(
            (float)x * cellSize, 25,
            ((float)x * cellSize), ((float)height * cellSize) + 25,
            color
        );
    }

    for (int y = 0; y <= height; y++) {
        DrawLine(
            0, ((float)y * cellSize) + 25,
            (float)width * cellSize, ((float)y * cellSize) + 25,
            color
        );
    }
}

void cameraSetup(){
    camera.target = (Vector2){ 0.0f, 0.0f };
    camera.offset = (Vector2){ 0, 0 };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;
}

void restartWorldValues(){
    world_value_singleton = {
        20,
        100,
        20,
        0,
        RESIDENTIAL,
        GetTime(),
        0,
        GetTime()
    };
    world.reset();
    auto logic_loop = world.system<Building>()
    .iter([](flecs::iter it, Building *b){
        if(world_value_singleton.maintain_population > world_value_singleton.total_population){
            return;
        }
        int total_population = 20;

        int potential_resource_income_rate = 0;
        for(auto i: it){
            if(b[i].current_type == RESIDENTIAL){
                total_population += RESIDENTIAL_POPULATION_COUNT;
            }else{
                potential_resource_income_rate += COMMERICAL_RESOURCE_COUNT;
            }
        }

        int available_population = total_population;
        int resource_income_rate  = 0;

        for(auto i: it){
            if(b[i].current_type == COMMERICAL){
                if(available_population > 0){
                    available_population -= COMMERCIAL_POPULATION_COST;
                    resource_income_rate += COMMERICAL_RESOURCE_COUNT;
                }
            }
        }

        world_value_singleton.total_population = total_population;
        world_value_singleton.resource_income_per_frame = resource_income_rate;
        world_value_singleton.available_resources += resource_income_rate * GetFrameTime();
        world_value_singleton.available_population = available_population;
        world_value_singleton.maintain_population += pow(POPULATION_LIMIT_INCREASE_SPEED, (GetTime() - world_value_singleton.game_start_time)/3) * GetFrameTime();
});
    logic_loop.add(flecs::OnUpdate);
}

void DrawUI(){
    WorldValues& value = world_value_singleton;
    GuiDrawRectangle((Rectangle){0, 0, 500, 25}, 1, BLACK, WHITE);
    GuiDrawText(TextFormat("Population: %d/%d", value.available_population, value.total_population), (Rectangle){5, 0, 100, 25}, 0, BLACK);
    GuiDrawText(TextFormat("Selected: %s", BUILDING_TYPE_to_string(value.current_type).c_str()), (Rectangle){110, 0 ,200,25}, 0, BLACK);
    GuiDrawText(TextFormat("Maintain Pop: %d", (int)value.maintain_population), (Rectangle){250,0,100,25}, 0, BLACK);
    GuiDrawText(TextFormat("Resources: %d (+%d)", (int)value.available_resources, value.resource_income_per_frame), (Rectangle){370, 0, 125, 25}, 2, BLACK);
    if(GuiButton((Rectangle){225, 0, 25, 25}, GuiIconText(ICON_REDO, ""))){
        restartWorldValues();
    }
    if(world_value_singleton.maintain_population > world_value_singleton.total_population){
        GuiDrawText("Your Score", (Rectangle){200, 232.5, 100, 25}, 1, BLACK);
        GuiDrawText(TextFormat("Population: %d", (int)world_value_singleton.total_population), (Rectangle){200, 232.5 + 50, 100, 25}, 1, BLACK);
    }
}

void InputGameWorld(flecs::world& world, Sound& building_placement_audio, Sound& building_not_placed_audio){
    if(world_value_singleton.maintain_population > world_value_singleton.total_population){
        return;
    }
    Vector2 mousePos = GetMousePosition();
    Vector2 mouseWorldPos = GetScreenToWorld2D(mousePos, camera);

    if(IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)){
        world_value_singleton.current_type = world_value_singleton.current_type == RESIDENTIAL? COMMERICAL: RESIDENTIAL;
    }

    // Depending on the mouse location, we will try to snap it to the closest grid
    float selectionX = roundToFloor25(mouseWorldPos.x);
    float selectionY = roundToFloor25(mouseWorldPos.y);

    DrawRectangle(selectionX, selectionY, 25, 25, BLACK);

    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
        double necessary_cost = world_value_singleton.current_type == RESIDENTIAL? RESIDENTIAL_BUILDING_RESOURCE_COST: COMMERICAL_BUILDING_RESOURCE_COST;

        if(world_value_singleton.available_resources >= necessary_cost){
        auto new_building = world.entity();
            new_building.add<Building>();
            new_building.set<Building>({
                    {selectionX, selectionY}, 
                    world_value_singleton.current_type
                }
            );

            PlaySound(building_placement_audio);
            world_value_singleton.available_resources -= necessary_cost;
            return;
        }

        PlaySound(building_not_placed_audio);
    }
}

void DrawGameWorld(flecs::world& world){
    if(world_value_singleton.maintain_population > world_value_singleton.total_population){
        return;
    }
        BeginMode2D(camera);
        DrawGrid2D(20, 20, 25, BLACK);
        world.system<Building>()
            .iter([](flecs::iter it, Building *b){
                for(auto i: it){
                DrawRectangle(b[i].position.x, b[i].position.y, 25, 25, b[i].current_type == RESIDENTIAL? RESIDENTIAL_COLOR: COMMERCIAL_COLOR);
            }
        });
    EndMode2D();
}

int main(){

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Keep Building");
    InitAudioDevice();
    Sound building_placement_audio = LoadSound("resources/Audio/buildingplace.wav");
    Sound building_not_placed_audio = LoadSound("resources/Audio/buildingnotplaced.wav");

    restartWorldValues();
    cameraSetup();

    while(!WindowShouldClose()){
        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawGameWorld(world);
        InputGameWorld(world, building_placement_audio, building_not_placed_audio);

        DrawUI();
        EndDrawing();

        world.progress();
        //Custom_Update();
    }

    CloseWindow();
    CloseAudioDevice();
}