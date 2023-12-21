#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <raylib.h>

#define QUADTREE_POINT_CAPACITY 5
#define QUADTREE_MIN_WIDTH 1
#define QUADTREE_MIN_HEIGHT 1

struct point {
    int x;
    int y;
};

struct rect {
    int x;
    int y;
    int w;
    int h;
};

struct quadtree {
    struct quadtree *north_west;
    struct quadtree *north_east;
    struct quadtree *south_west;
    struct quadtree *south_east;
    struct point points[QUADTREE_POINT_CAPACITY];
    struct rect boundary;
    size_t capacity;
    size_t taken;
};

struct point_list {
    struct point *points;
    size_t capacity;
    size_t length;
};

struct point_list *point_list_new(size_t default_capacity) {
    struct point_list *list = calloc(1, sizeof(struct point_list));
    list->points = calloc(default_capacity, sizeof(struct point));
    list->capacity = default_capacity;
    return list;
}

void point_list_free(struct point_list *list) {
    if (list->points != NULL) {
        free(list->points);
    }

    free(list);
}

void point_list_push(struct point_list *list, struct point point) {
    if (list->length >= list->capacity-1) {
        struct point *expanded = realloc(list->points, sizeof(struct point) * list->capacity * 2);
        list->points = expanded;
        list->capacity += list->capacity;
    }

    list->points[list->length] = point;
    list->length += 1;
}

struct quadtree *quadtree_new(struct rect boundary) {
    struct quadtree *quadtree = calloc(1, sizeof(struct quadtree));
    quadtree->capacity = QUADTREE_POINT_CAPACITY;
    quadtree->boundary = boundary;
    return quadtree;
}

void quadtree_free(struct quadtree *quadtree) { // NOLINT(*-no-recursion)
    if (quadtree->north_west != NULL) {
        quadtree_free(quadtree->north_west);
    }

    if (quadtree->north_east != NULL) {
        quadtree_free(quadtree->north_east);
    }

    if (quadtree->south_west != NULL) {
        quadtree_free(quadtree->south_west);
    }

    if (quadtree->south_east != NULL) {
        quadtree_free(quadtree->south_east);
    }

    free(quadtree);
}

bool rect_contains_point(struct rect rect, struct point point) {
    return (
            point.x >= rect.x - rect.w / 2
            && point.x < rect.x + rect.w / 2
            && point.y >= rect.y - rect.h / 2
            && point.y < rect.y + rect.h / 2
    );
}

bool rect_outside_rect(struct rect first, struct rect second) {
    return (
            first.x + first.w / 2 < second.x - second.w / 2
            || first.x - first.w / 2 > second.x + second.w / 2
            || first.y + first.h / 2 < second.y - second.h / 2
            || first.y - first.h / 2 > second.y + second.h / 2
    );
}

bool quadtree_subdivide(struct quadtree *quadtree) {
    int sub_w = quadtree->boundary.w / 2;
    int sub_h = quadtree->boundary.h / 2;
    if (sub_w < QUADTREE_MIN_WIDTH || sub_h < QUADTREE_MIN_HEIGHT) {
        TraceLog(LOG_ERROR, "cannot subdivide quadtree! (%d %d %d %d)",
                 quadtree->boundary.x, quadtree->boundary.y, quadtree->boundary.w, quadtree->boundary.h);
        return false;
    }

    struct rect par_boundary = quadtree->boundary;
    quadtree->north_west = quadtree_new(
            (struct rect) {.x = par_boundary.x - sub_w / 2, .y = par_boundary.y + sub_h / 2, .w = sub_w, .h = sub_h});
    quadtree->north_east = quadtree_new(
            (struct rect) {.x = par_boundary.x + sub_w / 2, .y = par_boundary.y + sub_h / 2, .w = sub_w, .h = sub_h});
    quadtree->south_west = quadtree_new(
            (struct rect) {.x = par_boundary.x - sub_w / 2, .y = par_boundary.y - sub_h / 2, .w = sub_w, .h = sub_h});
    quadtree->south_east = quadtree_new(
            (struct rect) {.x = par_boundary.x + sub_w / 2, .y = par_boundary.y - sub_h / 2, .w = sub_w, .h = sub_h});
    return true;
}

bool quadtree_insert(struct quadtree *quadtree, struct point point) { // NOLINT(*-no-recursion)
    if (!rect_contains_point(quadtree->boundary, point)) {
        return false;
    }

    if (quadtree->taken < quadtree->capacity && quadtree->north_west == NULL) {
        quadtree->points[quadtree->taken] = point;
        quadtree->taken += 1;
        return true;
    }

    if (quadtree->north_west == NULL) {
        if (!quadtree_subdivide(quadtree)) {
            TraceLog(LOG_ERROR, "cannot insert new point: (%d %d)", point.x, point.y);
            return false;
        }
    }

    return (
            quadtree_insert(quadtree->north_west, point)
            || quadtree_insert(quadtree->north_east, point)
            || quadtree_insert(quadtree->south_west, point)
            || quadtree_insert(quadtree->south_east, point)
    );
}

void
quadtree_query_range(struct quadtree *quadtree, struct rect range, struct point_list *found) { // NOLINT(*-no-recursion)
    if (rect_outside_rect(quadtree->boundary, range)) {
        return;
    }

    for (size_t i = 0; i < quadtree->taken; i++) {
        struct point point = quadtree->points[i];
        if (rect_contains_point(range, point)) {
            point_list_push(found, point);
        }
    }

    if (quadtree->north_west == NULL) {
        return;
    }

    quadtree_query_range(quadtree->north_west, range, found);
    quadtree_query_range(quadtree->north_east, range, found);
    quadtree_query_range(quadtree->south_west, range, found);
    quadtree_query_range(quadtree->south_east, range, found);
}

void quadtree_draw(struct quadtree *quadtree) { // NOLINT(*-no-recursion)
    struct rect boundary = quadtree->boundary;
    DrawRectangleLines(boundary.x - boundary.w / 2, boundary.y - boundary.h / 2, boundary.w, boundary.h, WHITE);
    for (size_t i = 0; i < quadtree->taken; i++) {
        struct point point = quadtree->points[i];
        DrawCircle(point.x, point.y, 1.0f, WHITE);
    }

    if (quadtree->north_west != NULL) {
        quadtree_draw(quadtree->north_west);
        quadtree_draw(quadtree->north_east);
        quadtree_draw(quadtree->south_west);
        quadtree_draw(quadtree->south_east);
    }
}

void quadtree_query_result_draw(struct rect range, struct point_list *result) {
    DrawRectangleLines(range.x - range.w / 2, range.y - range.h / 2, range.w, range.h, GREEN);
    for (size_t i = 0; i < result->length; i++) {
        struct point point = result->points[i];
        DrawCircle(point.x, point.y, 1.5f, LIME);
    }
}

int main() {
    InitWindow(800, 800, "SimpleQuadTree");

    struct rect world_boundary = {.x = 400, .y = 400, .w = 800, .h = 800};
    SetTargetFPS(60);

    struct point_list *saved_points = point_list_new(5);
    struct point query_center = { 120, 120 };
    struct point query_size = { 90, 90 };
    float last_inserted_point_time = 0.0f;
    while (!WindowShouldClose()) {
        BeginDrawing();
        {
            // Add points to saved instances
            last_inserted_point_time += GetFrameTime();
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && last_inserted_point_time >= 0.01f) {
                point_list_push(saved_points, (struct point) {GetMouseX(), GetMouseY() });
                last_inserted_point_time = 0.0f;
            }

            if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
                query_center.x = GetMouseX();
                query_center.y = GetMouseY();
            }

            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
                query_size.x = abs(GetMouseX() - query_center.x) * 2;
                query_size.y = abs(GetMouseY() - query_center.y) * 2;
            }

            // Create quadtree (each frame)
            struct quadtree *world_quadtree = quadtree_new(world_boundary);
            for (int i=0; i < saved_points->length; i++) {
                struct point point = saved_points->points[i];
                quadtree_insert(world_quadtree, point);
            }

            // Query quadtree (each frame)
            struct point_list *result = point_list_new(100);
            struct rect query = {query_center.x, query_center.y, query_size.x, query_size.y};
            quadtree_query_range(world_quadtree, query, result);

            // Draw quadtree and query result
            DrawFPS(0, 0);
            ClearBackground(BLACK);
            quadtree_draw(world_quadtree);
            quadtree_query_result_draw(query, result);

            // Free memory for quadtree and result
            point_list_free(result);
            quadtree_free(world_quadtree);
        }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}