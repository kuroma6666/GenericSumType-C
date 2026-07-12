# 描画プリミティブの read-only 検査 — 仕様

対応デモ: [`examples/const_view_demo.c`](../const_view_demo.c)

## 背景・要件
監視・集計処理は描画プリミティブ列を「書き換えずに」総塗りつぶし面積や種別を集計したい。集計コードが誤ってプリミティブを変更しないことを、レビューや規約ではなく型（const）で強制したい。

## 仕様
- 集計関数は `const Shape*` / `const Shape[]` を受け取る（書き換え不可）。
- fill_cost 合計を求め、個々のプリミティブは const のまま検査する。

## 実装対応
- `DEFINE_SUM_MATCH_CONST`（self=const NAME*, ハンドラ=Type const*）。可変版は const NAME* から呼べない（discards const）ため read-only 契約に不適。
- 取り出しは const ゲッター `NAME_get_<tag>_const`（Type const* を返す）。

## 受け入れ基準
- `const Shape*` を保持したまま面積集計・種別判定ができる（キャスト不要）。
- ハンドラが payload を書き換えようとするとコンパイルエラー（design_spec 4.14節）。
