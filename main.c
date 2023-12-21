#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include <raylib.h>

#define QUADTREE_POINT_CAPACITY 5

struct point {
    float x;
    float y;
};

struct rect {
    float x;
    float y;
    float w;
    float h;
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
    float sub_w = quadtree->boundary.w / 2;
    float sub_h = quadtree->boundary.h / 2;
    struct rect par_boundary = quadtree->boundary;
    quadtree->north_west = quadtree_new((struct rect){ .x = par_boundary.x - sub_w / 2, .y = par_boundary.y + sub_h / 2, .w = sub_w, .h = sub_h });
    quadtree->north_east = quadtree_new((struct rect){ .x = par_boundary.x + sub_w / 2, .y = par_boundary.y + sub_h / 2, .w = sub_w, .h = sub_h });
    quadtree->south_west = quadtree_new((struct rect){ .x = par_boundary.x - sub_w / 2, .y = par_boundary.y - sub_h / 2, .w = sub_w, .h = sub_h });
    quadtree->south_east = quadtree_new((struct rect){ .x = par_boundary.x + sub_w / 2, .y = par_boundary.y - sub_h / 2, .w = sub_w, .h = sub_h });
}

bool quadtree_insert(struct quadtree *quadtree, struct point point) { // NOLINT(*-no-recursion)
    if (!rect_contains_point(quadtree->boundary, point)) {
        return false;
    }

    if (quadtree->taken <= quadtree->capacity && quadtree->north_west == NULL) {
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

int main() {
    struct rect world_boundary = { .x = 500, .y = 500, .w = 1000, .h = 1000 };
    struct quadtree *world_quadtree = quadtree_new(world_boundary);
    for (int i=0;i<1;i++) {
        struct point point = {
                .x = rand() % (int)world_boundary.w,  // NOLINT(*-narrowing-conversions, *-msc50-cpp)
                .y = rand() % (int)world_boundary.h,  // NOLINT(*-narrowing-conversions, *-msc50-cpp)
                };
        quadtree_insert(world_quadtree, point);
    }

    quadtree_free(world_quadtree);
    return 0;
}