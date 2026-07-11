/*
 * DEFINE_SUM_MATCH_CONST / NAME_get_<tag>_const（read-only 版 API）の使用例。
 *
 * shape_demo.c の可変版 match に対し、こちらは「Shape を書き換えずに検査するだけ」
 * という read-only の関心を、型（const）で表明する例。
 *
 * ポイント: inspect() / total_area() は引数を const Shape*（あるいは const Shape 配列）
 * で受け取れる。可変版 DEFINE_SUM_MATCH は self が非const NAME* のため const Shape*
 * からはキャストなしに呼べない（-Werror下で discards const）が、const 版なら
 * 「この関数は Shape を書き換えない」という契約を保ったまま呼べる。
 *
 * ビルド: gcc -std=c11 -pedantic -Wall -Wextra -Werror -Iinclude examples/const_view_demo.c -o const_view_demo
 */
#include <stdio.h>
#include "generic_sum_type.h"

typedef struct { double radius; } Circle;
typedef struct { double width, height; } Rectangle;
typedef struct { double base, height; } Triangle;

#define SHAPE_VARIANTS(X, NAME, EXTRA)     \
    X(NAME, EXTRA, circle,    Circle)      \
    X(NAME, EXTRA, rectangle, Rectangle)   \
    X(NAME, EXTRA, triangle,  Triangle)

DEFINE_SUM_TYPE(Shape, SHAPE_VARIANTS)

/* read-only 版の match。ハンドラは payload を const で受け取り、書き換えられない。
 * 面積(double)と種別名(const char*)の2種類を、同じ Shape に対して定義している。 */
DEFINE_SUM_MATCH_CONST(Shape, SHAPE_VARIANTS, Shape_area,  double)
DEFINE_SUM_MATCH_CONST(Shape, SHAPE_VARIANTS, Shape_kind,  const char *)

/* --- 面積ハンドラ（const payload。読むだけ） --- */
static double area_circle(const Circle *c)       { return 3.14159265358979 * c->radius * c->radius; }
static double area_rectangle(const Rectangle *r) { return r->width * r->height; }
static double area_triangle(const Triangle *t)   { return t->base * t->height / 2.0; }

/* --- 種別名ハンドラ --- */
static const char *kind_circle(const Circle *c)       { (void)c; return "circle"; }
static const char *kind_rectangle(const Rectangle *r) { (void)r; return "rectangle"; }
static const char *kind_triangle(const Triangle *t)   { (void)t; return "triangle"; }

/* Shape を書き換えないことを型で保証した検査関数（引数が const Shape*）。 */
static void inspect(const Shape *s) {
    const char *kind = Shape_kind(s, kind_circle, kind_rectangle, kind_triangle);
    double area = Shape_area(s, area_circle, area_rectangle, area_triangle);
    printf("%-9s area = %.4f\n", kind, area);
}

/* const 配列を走査して合計面積を求める（要素も const Shape として扱える）。 */
static double total_area(const Shape *shapes, size_t n) {
    double sum = 0.0;
    for (size_t i = 0; i < n; ++i) {
        sum += Shape_area(&shapes[i], area_circle, area_rectangle, area_triangle);
    }
    return sum;
}

int main(void) {
    const Shape shapes[] = {
        Shape_new_circle((Circle){ .radius = 2.0 }),
        Shape_new_rectangle((Rectangle){ .width = 3.0, .height = 4.0 }),
        Shape_new_triangle((Triangle){ .base = 5.0, .height = 2.0 }),
    };
    const size_t n = sizeof shapes / sizeof shapes[0];

    for (size_t i = 0; i < n; ++i) {
        inspect(&shapes[i]);
    }
    printf("total area = %.4f\n", total_area(shapes, n));

    /* const ゲッター: const Shape* から const payload* を得る（書き換え不可）。 */
    const Circle *c = Shape_get_circle_const(&shapes[0]);
    const Rectangle *r = Shape_get_rectangle_const(&shapes[0]); /* shapes[0]はcircleなのでNULL */
    printf("shapes[0]: get_circle_const=%s, get_rectangle_const=%s\n",
           c ? "非NULL" : "NULL", r ? "非NULL" : "NULL");

    return 0;
}
