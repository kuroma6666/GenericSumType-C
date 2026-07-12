# ログ収集レコードの解放・複製 — 仕様

対応デモ: [`examples/resource_demo.c`](../resource_demo.c)

## 背景・要件
ログ収集器のレコードは「テキストログ（heap を所有）」と「数値メトリクス（POD）」が混在する。破棄時は所有 heap を確実に解放し、複製時はテキストをディープコピーして別々に破棄できるようにしたい。リーク・二重解放を起こさないこと。

## 仕様
- `LogRecord` は `text`(char* を所有) / `number`(int の POD)。
- `LogRecord_destroy()`: variant ごとの後始末（text は free、number は no-op）。
- `LogRecord_copy()`: variant ごとの複製（text は strdup、number は値コピー）。

## 実装対応
- `DEFINE_SUM_DESTROY` / `DEFINE_SUM_COPY`。
- 資源を持たない number には `SUM_DEFINE_NOOP_DESTROY` / `SUM_DEFINE_IDENTITY_COPY`。

## 受け入れ基準
- text のコピーが別アドレス（ディープコピー）になり、元と複製を独立に破棄できる。
- AddressSanitizer 付きビルドでリーク・二重解放が出ない。
