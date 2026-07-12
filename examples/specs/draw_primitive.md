# 描画プリミティブのコスト見積り — 仕様

対応デモ: [`examples/shape_demo.c`](../shape_demo.c)

## 背景・要件
2D描画エンジンで、描画プリミティブ（円・矩形・三角形）ごとに「塗りつぶしピクセル数の近似（面積）」と「ログ用ラベル」を求めたい。種別を増やしたら、面積計算・ラベル生成の双方で更新漏れをコンパイル時に検出したい。

## 仕様
- `Shape` は `circle` / `rectangle` / `triangle`。各 payload は専用 struct。
- `Shape_fill_cost()`: 面積（double, ピクセル近似）。`Shape_label()`: 種別ラベル（const char*）。
- 同一 SumType に対し用途の異なる match を2つ定義する。

## 実装対応
- `DEFINE_SUM_TYPE(Shape, SHAPE_VARIANTS)`。
- `DEFINE_SUM_MATCH(..., Shape_fill_cost, double)` と `DEFINE_SUM_MATCH(..., Shape_label, const char *)` の2本。

## 受け入れ基準
- 各プリミティブの面積とラベルが得られ、合計が計算できる。
- 各 variant を専用 struct にしているため、面積ハンドラと別ハンドラの取り違えも型で検出される（design_spec 3節）。
