#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <raylib.h>

#define QUADTREE_POINT_CAPACITY 5

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
    if (list->length >= list->capacity) {
        struct point *expanded = realloc(list->points, list->capacity * 2);
        assert(expanded != NULL && "error: not enough memory!");
        memset(expanded + list->capacity * sizeof(struct point), 0L, list->capacity);
        list->capacity *= 2;
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

void quadtree_subdivide(struct quadtree *quadtree) {
    int sub_w = quadtree->boundary.w / 2;
    int sub_h = quadtree->boundary.h / 2;
    struct rect par_boundary = quadtree->boundary;
    quadtree->north_west = quadtree_new(
            (struct rect) {.x = par_boundary.x - sub_w / 2, .y = par_boundary.y + sub_h / 2, .w = sub_w, .h = sub_h});
    quadtree->north_east = quadtree_new(
            (struct rect) {.x = par_boundary.x + sub_w / 2, .y = par_boundary.y + sub_h / 2, .w = sub_w, .h = sub_h});
    quadtree->south_west = quadtree_new(
            (struct rect) {.x = par_boundary.x - sub_w / 2, .y = par_boundary.y - sub_h / 2, .w = sub_w, .h = sub_h});
    quadtree->south_east = quadtree_new(
            (struct rect) {.x = par_boundary.x + sub_w / 2, .y = par_boundary.y - sub_h / 2, .w = sub_w, .h = sub_h});
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
        quadtree_subdivide(quadtree);
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
    struct quadtree *world_quadtree = quadtree_new(world_boundary);
    for (size_t i = 0; i < 1000; i++) {
        struct point point = {
                .x = GetRandomValue(0, world_boundary.w),  // NOLINT(*-narrowing-conversions, *-msc50-cpp)
                .y = GetRandomValue(0, world_boundary.h),  // NOLINT(*-narrowing-conversions, *-msc50-cpp)
        };
        quadtree_insert(world_quadtree, point);
    }

    struct point_list *result = point_list_new(100);
    struct rect query = {120, 120, 155, 155};
    quadtree_query_range(world_quadtree, query, result);

    while (!WindowShouldClose()) {
        BeginDrawing();
        {
            ClearBackground(BLACK);
            quadtree_draw(world_quadtree);
            quadtree_query_result_draw(query, result);
        }
        EndDrawing();
    }

    point_list_free(result);
    quadtree_free(world_quadtree);
    CloseWindow();
    return 0;
}