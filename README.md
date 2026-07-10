# GenericSumType-C

[![CI](https://github.com/kuroma6666/GenericSumType-C/actions/workflows/ci.yml/badge.svg)](https://github.com/kuroma6666/GenericSumType-C/actions/workflows/ci.yml)

C言語で、Rustの `enum`(代数的直和型/タグ付き共用体)に近い書き味を実現するための、依存ゼロ・ヘッダオンリーのジェネリックマクロライブラリ。X-Macroパターンにより、N個の任意型を持つ直和型を1箇所の型リスト定義から生成し、ハンドラの渡し忘れ・渡し順の取り違えを**コンパイルエラーとして**検出する。

## 特徴

- **N分岐対応**: variant数を問わない。型リストに1行足すだけで拡張できる
- **網羅性検査**: ハンドラの渡し忘れ・型の異なるvariant間での順序取り違えは、通常のC言語の関数プロトタイプチェックだけでコンパイルエラーになる(特別なビルドフラグ不要)
- **リソース管理**: `DEFINE_SUM_DESTROY` / `DEFINE_SUM_COPY` により、ポインタ資源(heap確保文字列など)の解放・ディープコピーも網羅性検査付きで書ける
- **スレッドセーフ対応**: `SUM_CTX_LOCK` / `SUM_CTX_UNLOCK` フックにより、RTOS・pthread等のロックプリミティブを差し込める(デフォルトはno-opでコストゼロ)
- **依存ゼロ**: 標準Cヘッダのみに依存。外部ライブラリ不要
- **対応環境**: C99以降 / GCC・Clang(MSVCは意図的に対象外。詳細は [design_spec.md](./design_spec.md) 参照)

## クイックスタート

```c
#include <stdint.h>
#include <stdio.h>
#include "generic_sum_type.h"

/* 型リストを1箇所だけで定義する */
#define INT_OR_STR_OR_FLOAT(X, NAME, EXTRA) \
    X(NAME, EXTRA, i, int32_t)              \
    X(NAME, EXTRA, s, const char*)          \
    X(NAME, EXTRA, f, double)

DEFINE_SUM_TYPE(IntOrStrOrFloat, INT_OR_STR_OR_FLOAT)
DEFINE_SUM_MATCH(IntOrStrOrFloat, INT_OR_STR_OR_FLOAT, IntOrStrOrFloat_print, int)

static int print_i(int32_t *v)     { printf("int:   %d\n", *v);   return 0; }
static int print_s(const char **v) { printf("str:   %s\n", *v);   return 0; }
static int print_f(double *v)      { printf("float: %f\n", *v);   return 0; }

int main(void) {
    IntOrStrOrFloat v = IntOrStrOrFloat_new_i(42);
    IntOrStrOrFloat_print(&v, print_i, print_s, print_f);
    return 0;
}
```

`print_s` を渡し忘れたり、`print_i` と `print_s` の順序を入れ替えたりすると、`gcc`/`clang`単体で(特別な警告フラグなしで)コンパイルエラーになる。実例は [`examples/demo.c`](./examples/demo.c) を参照。

## ディレクトリ構成

```
GenericSumType-C/
├── include/
│   └── generic_sum_type.h        # ライブラリ本体(ヘッダオンリー)
├── examples/
│   ├── demo.c                    # 基本例(int/str/float)
│   ├── shape_demo.c               # 図形の面積計算・表示(DEFINE_SUM_MATCH複数定義)
│   ├── command_demo.c             # コマンドディスパッチ(DEFINE_SUM_DISPATCH)
│   ├── resource_demo.c            # ポインタ資源の解放/コピー(DEFINE_SUM_DESTROY/COPY)
│   └── threadsafe_dispatch_demo.c # pthreadでのロックフック検証
├── tests/
│   ├── test_generic_sum_type.c     # 単体テスト(assert()ベース)
│   └── test_compile_guarantees.sh  # コンパイル時保証の回帰テスト
├── .github/workflows/ci.yml       # GitHub Actions(gcc/clang × c99/c11/c17)
├── design_spec.md                 # 設計仕様書(判断根拠・トレードオフ・既知の制約)
└── api_reference.md               # APIリファレンス(マクロ一覧・シグネチャ)
```

## ビルド・テスト

ヘッダオンリーのため、`include/` にインクルードパスを通すだけで使える。

```sh
gcc -std=c11 -pedantic -Wall -Wextra -Werror -Iinclude examples/demo.c -o demo
./demo
```

単体テスト(AddressSanitizer付き):

```sh
gcc -std=c11 -pedantic -Wall -Wextra -Werror -Iinclude -fsanitize=address -g \
    tests/test_generic_sum_type.c -o test_gst
./test_gst
```

コンパイル時保証(渡し忘れ・順序取り違えが実際に検出されるか)の回帰テスト:

```sh
tests/test_compile_guarantees.sh          # デフォルト: gcc, -std=c11
CC=clang STD=c17 tests/test_compile_guarantees.sh  # コンパイラ/C標準を切り替え
```

CIでは `gcc`/`clang` × `c99`/`c11`/`c17` の全6組み合わせで、上記に加え全デモの実行確認まで自動実行している([ci.yml](./.github/workflows/ci.yml))。

## ドキュメント

- [api_reference.md](./api_reference.md) — 提供するマクロ・関数のシグネチャ一覧(「何ができるか」を引くためのリファレンス)
- [design_spec.md](./design_spec.md) — 設計思想、各マクロを採用した判断根拠とトレードオフ、実装していないこと・既知の制約、検証で見つかったバグの記録(「なぜそうなっているか」)

## License

未定義。ライセンスを設定する場合は `LICENSE` ファイルを追加してください。
