#include <stdio.h>
#include "generic_sum_type.h"

/* --- variantごとの「型」をまず普通のstructとして定義する ---
 * (直和型のバリアントはそれぞれ独立したデータ構造を持つのが普通なので、
 *  シンプルなint/doubleより実務に近い例として、各図形を専用structにする) */
typedef struct { double radius; } Circle;
typedef struct { double width, height; } Rectangle;
typedef struct { double base, height; } Triangle;

/* 図形の種別リスト（Rustの enum Shape { Circle(..), Rectangle(..), Triangle(..) } に相当） */
#define SHAPE_VARIANTS(X, NAME, EXTRA)     \
    X(NAME, EXTRA, circle,    Circle)      \
    X(NAME, EXTRA, rectangle, Rectangle)   \
    X(NAME, EXTRA, triangle,  Triangle)

DEFINE_SUM_TYPE(Shape, SHAPE_VARIANTS)
DEFINE_SUM_MATCH(Shape, SHAPE_VARIANTS, Shape_area, double)   /* 面積計算用のmatch */
DEFINE_SUM_MATCH(Shape, SHAPE_VARIANTS, Shape_print, int)     /* 表示用のmatch（同じSumTypeに複数定義できる） */

/* --- 面積計算ハンドラ --- */
static double area_circle(Circle *c)       { return 3.14159265358979 * c->radius * c->radius; }
static double area_rectangle(Rectangle *r) { return r->width * r->height; }
static double area_triangle(Triangle *t)   { return t->base * t->height / 2.0; }

/* --- 表示ハンドラ --- */
static int print_circle(Circle *c)       { printf("Circle(r=%.1f)\n", c->radius); return 0; }
static int print_rectangle(Rectangle *r) { printf("Rectangle(w=%.1f, h=%.1f)\n", r->width, r->height); return 0; }
static int print_triangle(Triangle *t)   { printf("Triangle(base=%.1f, h=%.1f)\n", t->base, t->height); return 0; }

int main(void) {
    Shape shapes[] = {
        Shape_new_circle((Circle){ .radius = 2.0 }),
        Shape_new_rectangle((Rectangle){ .width = 3.0, .height = 4.0 }),
        Shape_new_triangle((Triangle){ .base = 5.0, .height = 2.0 }),
    };

    for (size_t i = 0; i < 3; ++i) {
        Shape_print(&shapes[i], print_circle, print_rectangle, print_triangle);
        double area = Shape_area(&shapes[i], area_circle, area_rectangle, area_triangle);
        printf("  area = %.4f\n", area);
    }

    /* getterで「本当にその種別か」を型安全に確認する例 */
    Circle *c = Shape_get_circle(&shapes[0]);
    Rectangle *r = Shape_get_rectangle(&shapes[0]); /* shapes[0]はcircleなのでNULLが返る */
    printf("shapes[0]: get_circle=%s, get_rectangle=%s\n",
           c ? "非NULL" : "NULL", r ? "非NULL" : "NULL");

    return 0;
}
