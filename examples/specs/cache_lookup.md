# キャッシュ参照 Hit/Miss (Either) — 仕様

対応デモ: [`examples/either_demo.c`](../either_demo.c)

## 背景・要件
キャッシュ参照は「ヒット（値がある）」か「ミス（無いのでフェッチが要る）」の二択。失敗/成功ではなく対等な二分岐なので、Result ではなく Either<Miss, Hit> で表したい。

## 仕様
- `Lookup` は `left`=miss（要フェッチのキー）/ `right`=hit（値）。
- `is_left`/`is_right` で分岐、fold で「ヒットなら値、ミスならフェッチ」に畳み込む。

## 実装対応
- `DEFINE_SUM_TYPE(tag=left/right)` + `DEFINE_EITHER_HELPERS`（is_left/is_right）。
- fold は `DEFINE_SUM_MATCH_CONST` の左右2ハンドラ、取り出しは const ゲッター。

## 受け入れ基準
- 参照結果を Hit/Miss で分岐し、fold で値へ解決できる。
- L と R を別 struct にしているため、左右ハンドラの取り違えが型で検出される（design_spec 3節）。
