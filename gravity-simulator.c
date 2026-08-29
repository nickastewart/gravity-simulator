#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include "raylib.h" 

typedef struct {
    double x, y;
    double vx, vy;
    double mass;
} Object;

void compute_forces(Object *objects, int n, double G, double eps2, int dt);

int main() {
    const int screenWidth = 1200;
    const int screenHeight = 800;

    InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");
    SetTargetFPS(10);

    const int numObjects = 100;

    Object objects[numObjects]; 

    for (int i = 0; i < numObjects; i++) {
      int x = (rand() % screenWidth) + 1;
      int y = (rand() % screenHeight) + 1;
      int mass = (rand() % 2500) + 50;
      Object object = {x, y, 0, 0, mass};
      objects[i] = object;
    }
    
    while (!WindowShouldClose()) {
        compute_forces(objects, numObjects, 1, 1, 2);
        BeginDrawing();
            ClearBackground(BLACK);
            for (int i = 0; i < numObjects; i++){
              DrawCircle(objects[i].x, objects[i].y, cbrt(objects[i].mass) * 0.5f, WHITE);
            }
        EndDrawing();
    }

    CloseWindow();

    return 0;
}

void compute_forces(Object *objects, int n, double G, double eps2, int dt) {
    for (int i = 0; i < n; i++) {
      double ax = 0, ay = 0; 
      for (int j = 0; j < n; j++) {
          if (i == j) {
              continue;
          }

          double dx = objects[j].x - objects[i].x;
          double dy = objects[j].y - objects[i].y; 
          double r2 = dx*dx + dy*dy + eps2;
          double inv_r3 = 1.0 / (r2 * sqrt(r2));
          double f = G * objects[j].mass * inv_r3;
          ax += f * dx;
          ay += f * dy;
      }
      objects[i].vx += ax;
      objects[i].vy += ay;
    }

    for (int i = 0; i < n; i++) {
      for (int j = 0; j < n; j++) {
        if (i == j) {
            continue;
        }
        objects[i].x = objects[i].x + objects[i].vx * dt;
        objects[i].y = objects[i].y + objects[i].vy * dt;
      }
    }
}
