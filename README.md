# GenericSumType-C

[![CI](https://github.com/kuroma6666/GenericSumType-C/actions/workflows/ci.yml/badge.svg)](https://github.com/kuroma6666/GenericSumType-C/actions/workflows/ci.yml)

C で Rust の `enum`（タグ付き共用体）に近い書き味を実現する、依存ゼロ・ヘッダオンリーのマクロライブラリ。
型リストを1箇所に定義すると、そこから N 分岐の直和型を生成する（X-Macro）。
ハンドラの**渡し忘れ**と**取り違え**はコンパイルエラーになる。

採用を検討する場合は先に [docs/requirements.md](./docs/requirements.md) を読むこと。
このライブラリに価値があるのは、**variant が今後増えること**と **`-Wall -Wextra -Werror` を運用できること**の両方が成り立つときだけである。
似たライブラリ（datatype99 など）との比較も requirements.md に載せている。

## 目次

- [特徴](#特徴)
- [クイックスタート](#クイックスタート)
- [ディレクトリ構成](#ディレクトリ構成)
- [ビルド・テスト](#ビルドテスト)
- [ドキュメント](#ドキュメント)
- [参考資料](#参考資料)
- [License](#license)

## 特徴

- **ワイルドカードの無い網羅性検査**: ハンドラは variant の数だけ必須引数になる。variant を1つ足すと、すべての呼び出し箇所が
  `too few arguments` で落ちる。「残り全部」を受ける逃げ道が無い（[ADR-0018](./docs/adr/0018-exhaustiveness-by-arity.md)）。
- **取り違え検査**: 型が異なる variant 間でハンドラの順序を入れ替えると `-Wincompatible-pointer-types` になる（`-Werror` が前提）。
  定数の `NULL` をハンドラとして渡した場合も `-Wnonnull` で落ちる。
- **未定義動作を持ち込まない**: 不正な tag、未初期化の値（tag 0 = 空状態）は、フックを呼んだ後に trap（値を返すもの）または復帰（void のもの）する。
  無関係なハンドラが呼ばれることは無い（[ADR-0011](./docs/adr/0011-default-label-and-trap.md), [ADR-0013](./docs/adr/0013-empty-tag-zero.md)）。
- **用途別のディスパッチャ**: 純粋な変換（`MATCH`）、副作用（`DISPATCH`）、副作用と結果（`DISPATCH_RET`）、
  型ごとのロック（`_LOCKED`）、解放（`DESTROY`）、ディープコピー（`COPY` / 失敗を返す `COPY_CHECKED`）。
- **厳格な警告設定でもそのまま入る**: 生成コードは `-Wswitch-default`、`-Wswitch-enum`、`-Wconversion`、`-Wshadow` などで警告を出さない（CI の strict モードで検査）。
- **依存ゼロ**: 標準 C ヘッダのみ。C99 以降、GCC / Clang（MSVC は対象外。[ADR-0007](./docs/adr/0007-msvc-out-of-scope.md)）。

## クイックスタート

```c
#include <stdio.h>
#include "generic_sum_type.h"

/* ペイロードは variant ごとに別の型にする（C-01） */
typedef struct { int port; }          Port;
typedef struct { const char *name; }  Host;
typedef struct { double factor; }     Scale;

#define CONFIG(X, NAME, EXTRA)    \
    X(NAME, EXTRA, port,  Port)   \
    X(NAME, EXTRA, host,  Host)   \
    X(NAME, EXTRA, scale, Scale)

SUM_DEFINE_TYPE(Config, CONFIG)
SUM_DEFINE_MATCH_CONST(Config, CONFIG, Config_print, int)

static int print_port(const Port *p)   { return printf("port  %d\n", p->port); }
static int print_host(const Host *h)   { return printf("host  %s\n", h->name); }
static int print_scale(const Scale *s) { return printf("scale %f\n", s->factor); }

int main(void) {
    Config c = Config_new_port((Port){ 8080 });
    Config_print(&c, print_port, print_host, print_scale);
    return 0;
}
```

`print_host` を渡し忘れても、`print_port` と `print_host` の順序を入れ替えても、コンパイルエラーになる。
全 API は [api_reference.md](./api_reference.md) を参照。

## ディレクトリ構成

```
include/generic_sum_type.h   ライブラリ本体（ヘッダオンリー）
examples/                    実務シナリオ風のデモ。各デモの要件・仕様は examples/specs/
tests/                       ランタイムの単体テスト（assert + ASan）
spec/                        コンパイル時保証の実行可能仕様（manifest.tsv が単一ソース）
verify/                      CBMC による任意の形式検証（CI 外）
scripts/check.sh             CI と同じ検査を手元で実行する
docs/                        requirements / GUARANTEES（生成物）/ constraints / adr
```

`examples/internal_command.h` / `protocol_frame.h` / `recipe_command.h` は、型定義とハンドラを同名の `.h` に切り出してある。
デモの `main()` と `tests/` の単体テストが、同じ実装を共有するためである（共通ヘッダ方式は [C-11](./docs/constraints.md#c-11) / [G-007](./docs/GUARANTEES.md#g-007)）。

## ビルド・テスト

ヘッダオンリーなので、`include/` にインクルードパスを通すだけで使える。

```sh
gcc -std=c11 -pedantic -Wall -Wextra -Werror -Iinclude examples/demo.c -o demo
```

CI と同じ検査を手元で実行する:

```sh
scripts/check.sh                          # gcc / c11: examples, tests(ASan), spec, 文書の同期とリンク
CC=clang STD=c99 scripts/check.sh         # コンパイラ・C 標準を切り替える
MODE=sanitize scripts/check.sh            # -O2 + ASan + UBSan
MODE=strict scripts/check.sh              # 厳格警告（-Wswitch-default, -Wconversion 等）。gcc では -fanalyzer も
MODE=cross CC=arm-none-eabi-gcc CROSS_CFLAGS="-mcpu=cortex-m4 -mthumb" scripts/check.sh   # クロスコンパイルのみ
spec/run.sh G-020 G-021                   # 指定した保証だけ検査する
```

CI は gcc / clang × c99 / c11 / c17 × default / sanitize / strict の 18 通りと、arm-none-eabi-gcc（Cortex-M4）× 3 標準のクロスコンパイルで [`scripts/check.sh`](./scripts/check.sh) を実行する。

このライブラリが約束しているのは、主に「**どんなコードがコンパイルを通り、どんなコードが通らないか**」である。
その仕様は [`spec/manifest.tsv`](./spec/manifest.tsv) で、判定器はコンパイラそのものである。
compile-fail のケースには、原因の箇所だけを直すと通ることを確かめる対照コンパイルを必須にしている（[spec/README.md](./spec/README.md)）。
[docs/GUARANTEES.md](./docs/GUARANTEES.md) は spec から生成しているので、手で編集しないこと。

CBMC が使える環境では、[verify/](./verify/README.md) で解放忘れと二重解放を形式的に検証できる（CI 外の任意検証）。

## ドキュメント

| 文書 | 引きたいこと |
|---|---|
| [docs/requirements.md](./docs/requirements.md) | **使うべきか**。想定利用者、採用すべきでない状況、既存の選択肢との比較 |
| [api_reference.md](./api_reference.md) | **何ができるか**。マクロとシグネチャ、フック、不正 tag での挙動 |
| [docs/GUARANTEES.md](./docs/GUARANTEES.md) | **何がコンパイル時に保証されるか**。`spec/` からの生成物 |
| [docs/constraints.md](./docs/constraints.md) | **何ができないか**。`C-01`〜`C-15` |
| [docs/adr/](./docs/adr/) | **なぜそう設計したか**。1判断1ファイル、追記型 |
| [CHANGELOG.md](./CHANGELOG.md) | 版ごとの変更と v1 からの移行手順 |
| [VERIFICATION_LOG.md](./VERIFICATION_LOG.md) | 結論に至った過程と訂正の記録 |

制約と保証には `C-01` / `G-001` という安定 ID を振っている。ほかの文書・コミット・Issue からは、この ID で参照すること。
ソースコード中の参照が切れていないかは、`spec/check_links.py` で検査している。

## 参考資料

- [internal_command_demo.c](./examples/internal_command_demo.c) / [protocol_frame_demo.c](./examples/protocol_frame_demo.c) / [recipe_param_demo.c](./examples/recipe_param_demo.c) は、[PALLASER社 レーザーマーカー 通信コマンド仕様書 Ver.2.0](https://pallaser.co.jp/dwl/mother/R2_0_Communication_Manual.pdf) のコマンド名・役割に着想を得て、独自に簡略化して書き起こしたサンプルです。仕様書の転載ではなく、実際のバイト列フォーマットや詳細パラメータまでは再現していません。

## License

[MIT](./LICENSE)
