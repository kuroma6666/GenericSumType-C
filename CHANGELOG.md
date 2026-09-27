# CHANGELOG

## 目次

- [2.0.0](#200)
  - [破壊的変更](#破壊的変更)
  - [追加](#追加)
  - [修正](#修正)
  - [検証・CI](#検証ci)
  - [v1 からの移行手順](#v1-からの移行手順)
- [1.x](#1x)

## 2.0.0

### 破壊的変更

| 変更 | 影響を受けるコード | 根拠 |
|---|---|---|
| tag 定数を `NAME_<tag>` から `NAME_TAG_<tag>` に改名 | tag 定数を直接参照するコード（`v.tag == Foo_bar`、`case Foo_bar:`） | [ADR-0012](./docs/adr/0012-naming-v2.md) |
| tag 値 0 を空状態 `NAME_TAG__EMPTY` として予約。variant の tag 値が 1 ずれる | tag の数値を保存・通信しているコード | [ADR-0013](./docs/adr/0013-empty-tag-zero.md) |
| 値を返すディスパッチャは、不正 tag・空状態で `SUM_TRAP()` によって停止する（v1 は未定義動作） | ゼロ初期化した値を MATCH / COPY に渡していたコード（v1 でも誤りだが黙って通っていた） | [ADR-0011](./docs/adr/0011-default-label-and-trap.md) |
| DESTROY は解放後に値を空状態に戻す | destroy 後の値の tag を読んでいたコード | [ADR-0013](./docs/adr/0013-empty-tag-zero.md) |
| 生成関数に `__attribute__((nonnull))` を付与 | ハンドラ・ctx・self に定数 `NULL` を渡していたコード | [ADR-0016](./docs/adr/0016-nonnull-handlers.md) |
| 公開マクロ名を `SUM_` 接頭辞に統一（`DEFINE_SUM_*` → `SUM_DEFINE_*`）。v1 名は非推奨の別名として残す | なし（別名が有効な限り） | [ADR-0012](./docs/adr/0012-naming-v2.md) |
| 内部マクロを `SUM__` 接頭辞に改名。`SUM_MAYBE_UNUSED` / `SUM_UNREACHABLE` は非推奨の別名としてのみ残す | 内部マクロを直接使っていたコード | [ADR-0012](./docs/adr/0012-naming-v2.md) |
| struct にタグ名 `NAME` を付与（`typedef struct NAME {...} NAME;`） | 同名の `struct NAME` を別に定義していたコード | [ADR-0017](./docs/adr/0017-self-reference-by-pointer.md) |

### 追加

- `SUM_DEFINE_DISPATCH_RET` / `SUM_DEFINE_DISPATCH_LOCKED` / `SUM_DEFINE_DISPATCH_RET_LOCKED` と `SUM_NO_LOCK`（[ADR-0014](./docs/adr/0014-dispatch-ret-and-locked.md)）
- `SUM_DEFINE_COPY_CHECKED`（[ADR-0015](./docs/adr/0015-copy-checked.md)）
- `SUM_EMPTY(NAME)`、`SUM_TRAP()` フック、`SUM_ASSUME_VALID_TAG`、`SUM_NO_LEGACY_NAMES`、`SUM_LEGACY_TAG_NAMES`
- `SUM_VERSION_MAJOR` / `_MINOR` / `_PATCH`
- payload から `struct NAME *` で自身を指せるようになった（[ADR-0017](./docs/adr/0017-self-reference-by-pointer.md)）

### 修正

- 不正 tag を MATCH に渡すと、`-O2` で無関係なハンドラが呼ばれていた（gcc 13 / clang 18 で再現）。
- ハンドラに `NULL` を渡すとコンパイルが通り、実行時に SIGSEGV になっていた。
- tag 名 `get_a` / `new_a` / `tag_t` / `is_a` が、生成関数・型名と衝突していた。
- 生成コードが `-Wswitch-default` の警告を出していた（MISRA C:2012 Rule 16.4 に抵触）。
- 同じ値への2回目の DESTROY で二重解放になっていた（CBMC で確認）。
- ヘッダ中に、存在しない節への参照があった。ソースコード中に旧 `design_spec.md` の節番号参照が19箇所あった。

### 検証・CI

- `spec/run.sh`: compile-fail のケースに、`-DSPEC_FIXED` を付けると通ることを確かめる対照コンパイルを必須にした。
  無関係な理由（typo 等）で落ちているのに OK と判定されることが無くなった。失敗件数が256件のとき終了コードが0になる問題も直した。
- 保証を G-020〜G-032 の13件追加した（計32件）。
- `scripts/check.sh` を追加し、CI はそれを呼ぶだけにした。デモの追加フラグは各ファイルの `ci-flags:` 行で宣言する（手書きの実行リストを廃止）。
- CI に `sanitize`（`-O2` + ASan + UBSan）と `strict`（厳格警告。gcc では `-fanalyzer` も `-Werror` で実行）のモードを追加した（計18ジョブ）。
- CI に arm-none-eabi-gcc（Cortex-M4、newlib）でのクロスコンパイルジョブを追加した。`examples/demo.c` の `printf("%d", int32_t)` が newlib（`int32_t` = `long`）で `-Wformat` エラーになる問題を見つけ、`PRId32` に直した。
- `spec/check_links.py` は、ソースコード中の文書パス、ADR 番号、`C-xx` / `G-xxx` の ID、旧節番号参照も検査するようになった。

### v1 からの移行手順

1. **まずそのままビルドする。** v1 のマクロ名は別名として有効なので、ほとんどのコードはそのまま通る。
   tag 定数の参照がエラーになる場合は、次の行を include 前に置けば v1 の定数名も生成される。
   ```c
   #define SUM_LEGACY_TAG_NAMES
   ```
2. **`NULL` ハンドラ・`NULL` ctx のエラーを直す。** `-Wnonnull` で指摘された箇所は、v1 では実行時に落ちていた箇所。
3. **マクロ名を置き換える。**
   ```sh
   sed -i -E 's/\bDEFINE_SUM_(TYPE|MATCH_CONST|MATCH|DISPATCH|DESTROY|COPY|NEW_GENERIC)\b/SUM_DEFINE_\1/g;
              s/\bDEFINE_(EITHER|RESULT)_HELPERS\b/SUM_DEFINE_\1_HELPERS/g' $(git ls-files '*.c' '*.h')
   ```
4. **tag 定数を置き換える。** tag 名は型ごとに異なるので、機械的な一括置換はできない。
   `SUM_LEGACY_TAG_NAMES` を外してビルドし、エラーになった箇所の `Foo_bar` を `Foo_TAG_bar` に直す。
5. **`SUM_NO_LEGACY_NAMES` を定義してビルドが通ることを確認する。** v1 名が残っていないことを、ビルドで保証できる。
6. **ゼロ初期化した値を MATCH / COPY に渡している箇所が無いか確認する。** v2 では trap する。
   開発ビルドで `SUM_UNHANDLED_TAG` に `assert` を差し込むと、発生箇所を特定しやすい。
7. tag の数値を永続化・通信している場合は、変換表を用意する（値が1ずれる）。

## 1.x

`cc940a0`（2026-07-25）までの版。変更の経緯は [VERIFICATION_LOG.md](./VERIFICATION_LOG.md) を参照。
