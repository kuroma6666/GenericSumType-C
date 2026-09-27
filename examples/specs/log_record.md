# ログ収集レコードの解放・複製 — 仕様

対応デモ: [`examples/resource_demo.c`](../resource_demo.c)

## 背景・要件
ログ収集器のレコードは「テキストログ（heap を所有）」と「数値メトリクス（POD）」が混在する。破棄時は所有 heap を確実に解放し、複製時はテキストをディープコピーして別々に破棄できるようにしたい。リーク・二重解放を起こさないこと。

## 仕様
- `LogRecord` は `text`(char* を所有) / `number`(int の POD)。
- `LogRecord_destroy()`: variant ごとの後始末（text は free、number は no-op）。
- `LogRecord_copy()`: variant ごとの複製（text は strdup、number は値コピー）。
- `LogRecord_try_copy()`: 複製の失敗を戻り値（`ENOMEM`）で返し、失敗時は複製先を変更しない。
- destroy 後のレコードは空状態になり、同じレコードへの2回目の destroy は何もしない。

## 実装対応
- `SUM_DEFINE_DESTROY` / `SUM_DEFINE_COPY` / `SUM_DEFINE_COPY_CHECKED`（[ADR-0015](../../docs/adr/0015-copy-checked.md)）。
- 資源を持たない number には `SUM_DEFINE_NOOP_DESTROY` / `SUM_DEFINE_IDENTITY_COPY`。

## 受け入れ基準
- text のコピーが別アドレス（ディープコピー）になり、元と複製を独立に破棄できる。
- 確保失敗を模擬したとき `try_copy` が `ENOMEM` を返し、複製先が元の値のまま残る。
- 同じレコードへ destroy を2回呼んでも二重解放にならない。
- AddressSanitizer 付きビルドでリーク・二重解放が出ない。
