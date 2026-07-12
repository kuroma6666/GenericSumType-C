/*
 * 描画プリミティブのコスト見積り（塗りつぶし面積とラベル）
 *
 * 【要件】
 *   組み込みディスプレイ／2D描画エンジンで、描画プリミティブ（円・矩形・三角形）
 *   ごとに「塗りつぶしに要するピクセル数の近似（=面積）」と「ログ用のラベル」を
 *   求めたい。プリミティブ種別を増やしたら、面積計算・ラベル生成の双方で
 *   更新漏れをコンパイル時に検出したい。
 *
 * 【仕様】
 *   - Shape は circle / rectangle / triangle のいずれか。各 payload は専用 struct。
 *   - Shape_fill_cost(): 面積（double, ピクセル近似）。
 *   - Shape_label():    種別ラベル（const char*）。
 *   - 同一 SumType に対し用途の異なる match を2つ定義する。
 *
 * 【実装方針】
 *   - DEFINE_SUM_MATCH を用途別に2回（fill_cost / label）呼ぶ。
 *   - 各 variant を専用 struct にすることで、面積ハンドラと別ハンドラの
 *     取り違えも型で検出できる（design_spec 3節: 素の基本型を避ける）。
 *
 * 仕様の詳細: examples/specs/draw_primitive.md
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
DEFINE_SUM_MATCH(Shape, SHAPE_VARIANTS, Shape_fill_cost, double)
DEFINE_SUM_MATCH(Shape, SHAPE_VARIANTS, Shape_label, const char *)

static double cost_circle(Circle *c)       { return 3.14159265358979 * c->radius * c->radius; }
static double cost_rectangle(Rectangle *r) { return r->width * r->height; }
static double cost_triangle(Triangle *t)   { return t->base * t->height / 2.0; }

static const char *label_circle(Circle *c)       { (void)c; return "circle"; }
static const char *label_rectangle(Rectangle *r) { (void)r; return "rectangle"; }
static const char *label_triangle(Triangle *t)   { (void)t; return "triangle"; }

int main(void) {
    Shape prims[] = {
        Shape_new_circle((Circle){ .radius = 2.0 }),
        Shape_new_rectangle((Rectangle){ .width = 3.0, .height = 4.0 }),
        Shape_new_triangle((Triangle){ .base = 5.0, .height = 2.0 }),
    };

    double total = 0.0;
    for (size_t i = 0; i < sizeof prims / sizeof prims[0]; ++i) {
        double c = Shape_fill_cost(&prims[i], cost_circle, cost_rectangle, cost_triangle);
        printf("%-9s fill_cost = %.4f px\n",
               Shape_label(&prims[i], label_circle, label_rectangle, label_triangle), c);
        total += c;
    }
    printf("total fill_cost = %.4f px\n", total);
    return 0;
}
