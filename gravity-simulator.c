#include <math.h>
#include <raylib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WORLD 1000
#define CELL 100
#define ROWS (WORLD / CELL)
#define COLS (WORLD / CELL)
#define NBINS (COLS * ROWS)
#define BINSIZE 200
#define MAXPAIRS 500

typedef struct {
  double x, y;
  double vx, vy;
  double mass;
  double radius;
  bool merged;
} Object;

typedef struct {
  int bins[NBINS][BINSIZE];
  int count[NBINS];
} Grid;

typedef struct {
  int a, b;
} Pair;

typedef struct {
  Pair pairs[MAXPAIRS];
  int count;
} PairList;
void init_objects(Object *objects, int n, int screenWidth, int screenHeight,
                  double G);
void compute_forces(Object *objects, int n, double G, double eps2, double dt);
void collision_detection(Grid *grid, PairList *pairList, Object *objects,
                         int n);
void grid_insert(Grid *grid, Object *objects, int n);
int constrain(int i, int min, int max);
int grid_index(int x, int y);
int overlap(const Object *a, const Object *b);
void grid_init(Grid *grid);
void pair_list_init(PairList *pairList);
void grid_reset(Grid *grid);
void pair_list_reset(PairList *pairList);
void merge_objects(Object *objects, PairList *pairList, int numObjects);

int main() {
  const int screenWidth = 1200;
  const int screenHeight = 1000;

  InitWindow(screenWidth, screenHeight, "Gravity Simulator");

  int monitor = GetCurrentMonitor();
  int mx = GetMonitorPosition(monitor).x;
  int my = GetMonitorPosition(monitor).y;
  int mw = GetMonitorWidth(monitor);
  int mh = GetMonitorHeight(monitor);

  if (mw > 0 && mh > 0) {
    int x = mx + (mw - screenWidth) / 2;
    int y = my + (mh - screenHeight) / 2;
    SetWindowPosition(x, y);
  }
  SetTargetFPS(30);

  const int numObjects = 2000;

  Object objects[numObjects];

  init_objects(objects, numObjects, screenWidth, screenHeight, 1.0);
  Grid grid;
  PairList pairList;
  grid_init(&grid);
  pair_list_init(&pairList);

  while (!WindowShouldClose()) {

    compute_forces(objects, numObjects, 1, 1, 0.25);
    collision_detection(&grid, &pairList, objects, numObjects);
    merge_objects(objects, &pairList, numObjects);

    BeginDrawing();
    ClearBackground(BLACK);
    for (int i = 1; i < numObjects; i++) {
      if (!objects[i].merged) {
        DrawCircle(objects[i].x, objects[i].y, objects[i].radius, WHITE);
      }
    }
    if (!objects[0].merged) {
      DrawCircle((int)objects[0].x, (int)objects[0].y, (float)objects[0].radius,
                 YELLOW);
    }
    EndDrawing();
  }

  CloseWindow();

  return 0;
}
void init_objects(Object *objects, int n, int screenWidth, int screenHeight,
                  double G) {
  double cx = screenWidth * 0.5;
  double cy = screenHeight * 0.5;

  double mass_center = 20000.0;
  objects[0].x = cx;
  objects[0].y = cy;
  objects[0].vx = 0.0;
  objects[0].vy = 0.0;
  objects[0].mass = mass_center;
  objects[0].radius = cbrt(mass_center) * 0.5;
  objects[0].merged = false;

  double cloud_r = 750.0;

  for (int i = 1; i < n; i++) {
    double ang = (double)rand() / (double)RAND_MAX * 2.0 * 3.141592653589793;
    double rad = 40.0 + cloud_r * sqrt((double)rand() / (double)RAND_MAX);

    double mass = 10;

    objects[i].x = cx + rad * cos(ang);
    objects[i].y = cy + rad * sin(ang);
    objects[i].mass = mass;
    objects[i].radius = cbrt(mass) * 0.5;
    objects[i].merged = false;

    double v = sqrt(G * mass_center / rad);
    objects[i].vx = -v * sin(ang);
    objects[i].vy = v * cos(ang);
  }
}

void compute_forces(Object *objects, int n, double G, double eps2, double dt) {
  for (int i = 1; i < n; i++) {
    if (objects[i].merged) {
      continue;
    }
    double ax = 0, ay = 0;
    for (int j = 0; j < n; j++) {
      if (i == j) {
        continue;
      }

      if (objects[j].merged) {
        continue;
      }

      double dx = objects[j].x - objects[i].x;
      double dy = objects[j].y - objects[i].y;
      double r2 = dx * dx + dy * dy + eps2;
      double inv_r3 = 1.0 / (r2 * sqrt(r2));
      double f = G * objects[j].mass * inv_r3;
      ax += f * dx;
      ay += f * dy;
    }
    objects[i].vx += ax * dt;
    objects[i].vy += ay * dt;
  }

  for (int i = 1; i < n; i++) {
    if (objects[i].merged) {
      continue;
    }
    objects[i].x = objects[i].x + objects[i].vx * dt;
    objects[i].y = objects[i].y + objects[i].vy * dt;
  }
}

void collision_detection(Grid *grid, PairList *pairList, Object *objects,
                         int n) {
  grid_reset(grid);
  pair_list_reset(pairList);

  for (int i = 0; i < n; i++) {
    if (objects[i].merged) {
      continue;
    }
    grid_insert(grid, &objects[i], i);
  }

  for (int b = 0; b < NBINS; b++) {
    for (size_t i = 0; i < grid->count[b]; i++) {
      for (size_t j = i + 1; j < grid->count[b]; j++) {
        int ia = grid->bins[b][i];
        int ib = grid->bins[b][j];
        if (overlap(&objects[ia], &objects[ib])) {
          int a = ia < ib ? ia : ib;
          int b = ia < ib ? ib : ia;

          int found = 0;
          for (int p = 0; p < pairList->count; p++) {
            if (pairList->pairs[p].a == a && pairList->pairs[p].b == b) {
              found = 1;
              break;
            }
          }
          if (!found) {
            pairList->pairs[pairList->count++] = (Pair){a, b};
          }
        }
      }
    }
  }
}

int overlap(const Object *a, const Object *b) {
  float dx = a->x - b->x;
  float dy = a->y - b->y;
  float r = a->radius + b->radius;
  return dx * dx + dy * dy <= r * r;
}

void grid_insert(Grid *grid, Object *object, int index) {
  int cx0 = (int)floor((object->x - object->radius) / CELL);
  int cy0 = (int)floor((object->y - object->radius) / CELL);
  int cx1 = (int)floor((object->x + object->radius) / CELL);
  int cy1 = (int)floor((object->y + object->radius) / CELL);

  cx0 = constrain(cx0, 0, COLS - 1);
  cy0 = constrain(cy0, 0, ROWS - 1);
  cx1 = constrain(cx1, 0, COLS - 1);
  cy1 = constrain(cy1, 0, ROWS - 1);

  for (int cy = cy0; cy <= cy1; cy++) {
    for (int cx = cx0; cx <= cx1; cx++) {
      int bin = grid_index(cx, cy);
      grid->bins[bin][grid->count[bin]++] = index;
    }
  }
}

int constrain(int i, int min, int max) {
  if (i < min) {
    return min;
  }

  if (i > max) {
    return max;
  }

  return i;
}

int grid_index(int x, int y) { return y * COLS + x; }

void clear_bins(Grid *grid) {
  for (int i = 0; i < NBINS; i++) {
    for (int j = 0; j < BINSIZE; j++) {
      grid->bins[i][j] = 0;
    }
  }
}

void grid_init(Grid *grid) {
  memset(grid->count, 0, sizeof grid->count);
  memset(grid->bins, 0, sizeof grid->bins);
}

void pair_list_init(PairList *pairList) {
  pairList->count = 0;
  memset(pairList->pairs, 0, sizeof pairList->pairs);
}

void grid_reset(Grid *grid) {
  for (size_t i = 0; i < (sizeof(grid->count) / sizeof(grid->count[0])); i++) {
    grid->count[i] = 0;
  }
}

void pair_list_reset(PairList *pairList) { pairList->count = 0; }

void merge_objects(Object *objects, PairList *pairList, int numObjects) {
  for (int i = 0; i < pairList->count; i++) {
    int ia = pairList->pairs[i].a;
    int ib = pairList->pairs[i].b;
    if (objects[ia].merged || objects[ib].merged) {
      continue;
    }
    double mass_new =
        objects[pairList->pairs[i].a].mass + objects[pairList->pairs[i].b].mass;
    double x_new = ((objects[pairList->pairs[i].a].mass *
                     objects[pairList->pairs[i].a].x) +
                    (objects[pairList->pairs[i].b].mass *
                     objects[pairList->pairs[i].b].x)) /
                   mass_new;

    double y_new = ((objects[pairList->pairs[i].a].mass *
                     objects[pairList->pairs[i].a].y) +
                    (objects[pairList->pairs[i].b].mass *
                     objects[pairList->pairs[i].b].y)) /
                   mass_new;

    double vx_new = ((objects[pairList->pairs[i].a].mass *
                      objects[pairList->pairs[i].a].vx) +
                     (objects[pairList->pairs[i].b].mass *
                      objects[pairList->pairs[i].b].vx)) /
                    mass_new;

    double vy_new = ((objects[pairList->pairs[i].a].mass *
                      objects[pairList->pairs[i].a].vy) +
                     (objects[pairList->pairs[i].b].mass *
                      objects[pairList->pairs[i].b].vy)) /
                    mass_new;

    double r_new = cbrt(objects[pairList->pairs[i].a].radius *
                            objects[pairList->pairs[i].a].radius *
                            objects[pairList->pairs[i].a].radius +
                        objects[pairList->pairs[i].b].radius *
                            objects[pairList->pairs[i].b].radius *
                            objects[pairList->pairs[i].b].radius);

    int survivor = ia;
    int eaten = ib;
    if (ia == 0 || ib == 0) {
      int eaten = (ia == 0) ? ib : ia;
      objects[eaten].merged = true;
      continue;
    }
    if (objects[ib].mass > objects[ia].mass) {
      survivor = ib;
      eaten = ia;
    }

    objects[survivor].x = x_new;
    objects[survivor].y = y_new;
    objects[survivor].vy = vy_new;
    objects[survivor].vx = vx_new;
    objects[survivor].mass = mass_new;
    objects[survivor].radius = r_new;
    objects[eaten].merged = true;
  }
}
