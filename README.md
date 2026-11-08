# GenericSumType-C

[![CI](https://github.com/kuroma6666/GenericSumType-C/actions/workflows/ci.yml/badge.svg)](https://github.com/kuroma6666/GenericSumType-C/actions/workflows/ci.yml)

C言語で、Rustの `enum`(代数的直和型/タグ付き共用体)に近い書き味を実現するための、依存ゼロ・ヘッダオンリーのジェネリックマクロライブラリ。X-Macroパターンにより、N個の任意型を持つ直和型を1箇所の型リスト定義から生成し、ハンドラの渡し忘れ・渡し順の取り違えを**コンパイルエラーとして**検出する。

採用を検討する場合は先に [docs/requirements.md](./docs/requirements.md) を読むこと。
**variant が今後増えること**と**`-Wall -Wextra -Werror` を運用できること**の両方が成り立つときにだけ価値がある。

## 特徴

- **N分岐対応**: variant数を問わない。型リストに1行足すだけで拡張できる
- **網羅性検査**: ハンドラの渡し忘れは、通常のC言語の関数プロトタイプチェックだけでコンパイルエラーになる(`-Wswitch` 等のフラグ運用に依存しない)
- **順序取り違え検査**: 型の異なるvariant間でハンドラの順序を入れ替えると `-Wincompatible-pointer-types` になる。**`-Werror` の運用が前提**(付けないと警告止まりで通ってしまう)
- **リソース管理**: `DEFINE_SUM_DESTROY` / `DEFINE_SUM_COPY` により、ポインタ資源(heap確保文字列など)の解放・ディープコピーも網羅性検査付きで書ける
- **スレッドセーフ対応**: `SUM_CTX_LOCK` / `SUM_CTX_UNLOCK` フックにより、RTOS・pthread等のロックプリミティブを差し込める(デフォルトはno-opでコストゼロ)
- **コンストラクタ自動選択(C11以降・オプトイン)**: `DEFINE_SUM_NEW_GENERIC` / `SUM_NEW` により、`_Generic`を使って渡した値の型からコンストラクタを自動選択できる(タグ名を書かずに済む)。C99ビルドではこの機能自体が存在しない扱いになる
- **依存ゼロ**: 標準Cヘッダのみに依存。外部ライブラリ不要
- **対応環境**: C99以降 / GCC・Clang(MSVCは意図的に対象外。詳細は [ADR-0007](./docs/adr/0007-msvc-out-of-scope.md) 参照)

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

C11以降であれば、`DEFINE_SUM_NEW_GENERIC` / `SUM_NEW` でタグ名(`_i`/`_s`/`_f`)を書かずに構築できる。

```c
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
DEFINE_SUM_NEW_GENERIC(IntOrStrOrFloat, INT_OR_STR_OR_FLOAT)
/* ... */
IntOrStrOrFloat v = SUM_NEW(IntOrStrOrFloat, INT_OR_STR_OR_FLOAT, 42);
#endif
```

実例は [`examples/generic_ctor_demo.c`](./examples/generic_ctor_demo.c)、設計判断・既知の制約は [ADR-0008](./docs/adr/0008-generic-constructor-selection.md) を参照。

## ディレクトリ構成

```
GenericSumType-C/
├── include/
│   └── generic_sum_type.h        # ライブラリ本体(ヘッダオンリー)
├── examples/
│   ├── demo.c                    # 設定値ストア(int32/文字列/double を1つの値型で保持)
│   ├── shape_demo.c               # 描画プリミティブのコスト見積り(DEFINE_SUM_MATCH複数定義)
│   ├── const_view_demo.c          # 描画プリミティブのread-only検査(DEFINE_SUM_MATCH_CONST/get_<tag>_const)
│   ├── either_demo.c             # キャッシュHit/Miss(Either: left/right + DEFINE_EITHER_HELPERS)
│   ├── result_demo.c             # 注文処理のResult(ok/err + DEFINE_RESULT_HELPERS)
│   ├── command_demo.c             # モータ制御コマンドのディスパッチ(DEFINE_SUM_DISPATCH)
│   ├── resource_demo.c            # ログレコードの解放/コピー(DEFINE_SUM_DESTROY/COPY)
│   ├── threadsafe_dispatch_demo.c # テレメトリカウンタの並行更新(pthreadロックフック)
│   ├── internal_command.h         # 内部処理系Commandの型・DISPATCH・ハンドラ本体
│   ├── internal_command_demo.c    # ↑を使う薄いデモ(デバイス状態を書き換えるDISPATCH)
│   ├── protocol_frame.h           # 通信処理系Commandの型・MATCH・分類ロジック
│   ├── protocol_frame_demo.c      # ↑を使う薄いデモ(受信フレームを分類するMATCH)
│   ├── recipe_command.h           # ROM/RAMパラメータ取得系Commandの型・DISPATCH・ハンドラ本体
│   ├── recipe_param_demo.c        # ↑を使う薄いデモ(永続ストアへのDISPATCH)
│   ├── generic_ctor_demo.c        # 型からのイベント構築(DEFINE_SUM_NEW_GENERIC/SUM_NEW, C11以降限定・自己ガード付き)
│   └── specs/                     # 各demoの要件・仕様(要件→仕様→実装対応→受け入れ基準)
├── tests/
│   ├── test_generic_sum_type.c     # ライブラリ本体の単体テスト(assert()ベース)
│   ├── test_internal_command.c     # internal_command.hの単体テスト
│   ├── test_protocol_frame.c       # protocol_frame.hの単体テスト
│   ├── test_recipe_command.c       # recipe_command.hの単体テスト
│   └── test_sum_new_generic.c      # DEFINE_SUM_NEW_GENERIC/SUM_NEWの単体テスト(C11以降限定・自己ガード付き)
├── spec/                          # コンパイル時保証の実行可能仕様(単一ソース)
│   ├── README.md                   # spec/ の使い方・ケースの足し方
│   ├── manifest.tsv                # 保証の一覧。ここが仕様の実体
│   ├── cases/G-NNN.c               # ケース断片(1ファイル1ケース)
│   ├── run.sh                      # 判定(コンパイラを実際に走らせて期待結果と突き合わせる)
│   ├── emit.sh                     # docs/GUARANTEES.md を生成する
│   └── check_links.py              # Markdown 相互リンク(パス・アンカー)の検査
├── verify/
│   ├── cbmc_destroy_harness.c      # DEFINE_SUM_DESTROY呼び出し忘れ/cleanup属性の二重解放をCBMCで検証するハーネス
│   ├── cbmc_verify_destroy.sh      # 上記ハーネスの3ケースをまとめて実行するスクリプト
│   └── README.md                   # verify/の使い方(CIには組み込まれていない任意の追加検証)
├── docs/
│   ├── requirements.md            # 何を解くか・採用条件・非目標
│   ├── GUARANTEES.md              # コンパイル時に何が保証されるか(spec/ からの生成物)
│   ├── constraints.md             # できないことの一覧(C-01〜C-14)
│   └── adr/                       # 設計判断の記録(なぜAではなくBか。1判断1ファイル)
├── .github/workflows/ci.yml       # GitHub Actions(gcc/clang × c99/c11/c17)
├── api_reference.md               # APIリファレンス(マクロ一覧・シグネチャ)
├── VERIFICATION_LOG.md            # 検証ログ(検証の経緯・訂正履歴・規格条文突き合わせ・バグ記録)
└── design_spec.md                 # 旧設計仕様書。分割済みのため移設マップのみ残している
```

`internal_command_demo.c` / `protocol_frame_demo.c` / `recipe_param_demo.c` は、型定義・DISPATCH(またはMATCH)・ハンドラ本体をそれぞれ同名の`.h`に切り出し、`main()`だけの薄いドライバにしている。これは [C-11](./docs/constraints.md#c-11) / [G-007](./docs/GUARANTEES.md#g-007) で検証済みの「共通ヘッダを複数の`.c`からincludeする」パターンの応用で、デモ本体と`tests/`配下の単体テストが同じハンドラ実装を共有できるようにするための構成である(単体テストのために`main()`を分割できない、という制約への対応)。

`generic_ctor_demo.c` / `test_sum_new_generic.c` はC11以降限定の機能を使うが、ファイル自体は`__STDC_VERSION__`で中身を分岐させ、C99でもビルド・実行できる(該当部分はスキップメッセージを出すだけ)ようにしてある。これはci.ymlの既存のグロブ運用(examples/tests配下を無条件でc99/c11/c17全マトリクスにかける)を変更せずに済ませるための構成である。詳細は [ADR-0008](./docs/adr/0008-generic-constructor-selection.md) を参照。

## ビルド・テスト

ヘッダオンリーのため、`include/` にインクルードパスを通すだけで使える。

```sh
gcc -std=c11 -pedantic -Wall -Wextra -Werror -Iinclude examples/demo.c -o demo
./demo
```

単体テスト(AddressSanitizer付き)。`tests/test_*.c` はライブラリ本体とexamples/内の各サンプル(内部処理系/通信処理系/ROM・RAMパラメータ取得系/コンストラクタ自動選択)それぞれに対応する:

```sh
for tf in tests/test_*.c; do
    gcc -std=c11 -pedantic -Wall -Wextra -Werror -Iinclude -fsanitize=address -g \
        "$tf" -o "/tmp/$(basename "$tf" .c)"
    "/tmp/$(basename "$tf" .c)"
done
```

コンパイル時保証(渡し忘れ・順序取り違えが実際に検出されるか)の検査:

```sh
spec/run.sh                     # デフォルト: gcc, -std=c11
CC=clang STD=c17 spec/run.sh    # コンパイラ/C標準を切り替え
spec/run.sh G-009 G-017         # 指定したケースだけ実行

spec/emit.sh --check            # docs/GUARANTEES.md が spec/ と同期しているか検査
```

このライブラリのランタイム挙動は `switch` 一つ分しかなく、実際に約束しているのは
「**どんなコードがコンパイルを通り、どんなコードが通らないか**」である。したがって
[`spec/manifest.tsv`](./spec/manifest.tsv) がこのライブラリの仕様の中核であり、その判定器は
コンパイラそのものになる。[docs/GUARANTEES.md](./docs/GUARANTEES.md) はそこからの生成物で、
CI が同期を検査しているため手で編集してはならない。詳しくは [spec/README.md](./spec/README.md)。

CIでは `gcc`/`clang` × `c99`/`c11`/`c17` の全6組み合わせで、上記に加え全デモの実行確認と `-O2` でのビルド(最適化時にしか出ない警告の検出)まで自動実行している([ci.yml](./.github/workflows/ci.yml))。

### 任意の追加検証(CBMC)

CBMC(Bounded Model Checker for C)が使える環境であれば、`verify/`配下のハーネスで`DEFINE_SUM_DESTROY`呼び出し忘れ・`__attribute__((cleanup(...)))`による二重解放を形式的に検証できる。**CIには組み込まれておらず、通常のビルド・テストには一切影響しない任意のオプション。**

```sh
sudo apt-get install cbmc
cd verify
./cbmc_verify_destroy.sh
```

詳細・実機での検証結果は [verify/README.md](./verify/README.md) と [C-03](./docs/constraints.md#c-03) を参照。

## ドキュメント

読者と更新頻度で分けてある。「今どうなっているか」と「なぜそうしたか」と「そこに至る過程」を混ぜない。

| 文書 | 引きたいこと |
|---|---|
| [docs/requirements.md](./docs/requirements.md) | このライブラリを**使うべきか**。想定利用者・採用すべきでない状況・非目標 |
| [api_reference.md](./api_reference.md) | **何ができるか**。マクロ・関数のシグネチャ一覧 |
| [docs/GUARANTEES.md](./docs/GUARANTEES.md) | **何がコンパイル時に保証されるか**。`spec/` からの生成物 |
| [docs/constraints.md](./docs/constraints.md) | **何ができないか**。制約カタログ(`C-01`〜`C-14`) |
| [docs/adr/](./docs/adr/) | **なぜそう設計したか**。1判断1ファイルの追記型 |
| [spec/](./spec/) | 保証の実行可能仕様。ケースを足すならここ |
| [VERIFICATION_LOG.md](./VERIFICATION_LOG.md) | **結論にどう辿り着いたか**。訂正履歴・規格条文との突き合わせ・見つかったバグ |

制約と保証には `C-01` / `G-001` という安定した ID を振ってある。節番号は改版で動くため、
他の文書・コミット・Issue からはこの ID で参照すること。

旧 [design_spec.md](./design_spec.md) は上記に分割済みで、旧節番号からの移設マップだけが残っている
(`VERIFICATION_LOG.md` の節番号は旧 design_spec の節番号に対応しているため)。

## 参考資料

- [internal_command_demo.c](./examples/internal_command_demo.c) / [protocol_frame_demo.c](./examples/protocol_frame_demo.c) / [recipe_param_demo.c](./examples/recipe_param_demo.c) は、[PALLASER社 レーザーマーカー 通信コマンド仕様書 Ver.2.0](https://pallaser.co.jp/dwl/mother/R2_0_Communication_Manual.pdf) のコマンド名・役割に着想を得て、独自に簡略化して書き起こしたサンプルです。仕様書の転載ではなく、実際のバイト列フォーマットや詳細パラメータまでは再現していません。

## License

[MIT](./LICENSE)
