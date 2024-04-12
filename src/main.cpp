#include <iostream>
#include <raylib.h>

using namespace std;

int main(){

    InitWindow(500, 500, "Keep Building");

    while(!WindowShouldClose()){
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Hello World", 10, 10, 20, LIGHTGRAY);
        EndDrawing();
    }

    CloseWindow();
}