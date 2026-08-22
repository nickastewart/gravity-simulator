#include<stdio.h>
#include<math.h>

typedef struct {
    double x, y;
    double vx, vy;
    double mass;
} Object;

void compute_forces(Object *objects, int n, double G, double eps2, int dt);

int main() {

    Object objects[] = {
        { 50, 50, 0, 0, 10}, 
        { 100, 100, 0, 0, 20},
        { 150, 150, 0, 0, 10}
    };

    for (int t = 0; t < 100; t++) {
        compute_forces(objects, 3, 1, 1, 2);
        for (int i = 0; i < 3; i++) {
            printf("object = %i, x = %f, y=%f \n", i, objects[i].x, objects[i].y);
        }
    }
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
        objects[i].x = objects[i].x + objects[i].vx * dt;
        objects[i].y = objects[i].y + objects[i].vy * dt;
    }

}
