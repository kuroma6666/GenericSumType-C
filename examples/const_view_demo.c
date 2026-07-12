/*
 * 描画プリミティブの read-only 検査（監視・集計）
 *
 * 【要件】
 *   監視・集計処理は、描画プリミティブ列を「書き換えずに」総塗りつぶし面積や
 *   種別を集計したい。集計コードが誤ってプリミティブを変更しないことを、型（const）
 *   で保証したい（レビューや規約に頼らず、コンパイル時に強制する）。
 *
 * 【仕様】
 *   - 集計関数は const Shape* / const Shape[] を受け取る（書き換え不可）。
 *   - fill_cost 合計を求める。個々のプリミティブは const のまま検査する。
 *
 * 【実装方針】
 *   - DEFINE_SUM_MATCH_CONST（self=const NAME*, ハンドラ=Type const*）を使う。
 *     可変版 DEFINE_SUM_MATCH は self が非const のため const Shape* から呼べず
 *     （discards const）、read-only 契約に不適。
 *   - 取り出しは const ゲッター NAME_get_<tag>_const（Type const* を返す）。
 *
 * 仕様の詳細: examples/specs/draw_inspect.md
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
DEFINE_SUM_MATCH_CONST(Shape, SHAPE_VARIANTS, Shape_fill_cost, double)
DEFINE_SUM_MATCH_CONST(Shape, SHAPE_VARIANTS, Shape_label, const char *)

static double cost_circle(const Circle *c)       { return 3.14159265358979 * c->radius * c->radius; }
static double cost_rectangle(const Rectangle *r) { return r->width * r->height; }
static double cost_triangle(const Triangle *t)   { return t->base * t->height / 2.0; }

static const char *label_circle(const Circle *c)       { (void)c; return "circle"; }
static const char *label_rectangle(const Rectangle *r) { (void)r; return "rectangle"; }
static const char *label_triangle(const Triangle *t)   { (void)t; return "triangle"; }

/* Shape を書き換えないことを型で保証した集計（引数は const Shape*） */
static double total_fill_cost(const Shape *shapes, size_t n) {
    double sum = 0.0;
    for (size_t i = 0; i < n; ++i)
        sum += Shape_fill_cost(&shapes[i], cost_circle, cost_rectangle, cost_triangle);
    return sum;
}

int main(void) {
    const Shape scene[] = {
        Shape_new_circle((Circle){ .radius = 2.0 }),
        Shape_new_rectangle((Rectangle){ .width = 3.0, .height = 4.0 }),
        Shape_new_triangle((Triangle){ .base = 5.0, .height = 2.0 }),
    };
    const size_t n = sizeof scene / sizeof scene[0];

    for (size_t i = 0; i < n; ++i)
        printf("%-9s %.4f px\n",
               Shape_label(&scene[i], label_circle, label_rectangle, label_triangle),
               Shape_fill_cost(&scene[i], cost_circle, cost_rectangle, cost_triangle));
    printf("total = %.4f px\n", total_fill_cost(scene, n));

    const Circle *c = Shape_get_circle_const(&scene[0]);   /* const のまま取り出す */
    printf("scene[0] is circle: %s\n", c ? "yes" : "no");
    return 0;
}
