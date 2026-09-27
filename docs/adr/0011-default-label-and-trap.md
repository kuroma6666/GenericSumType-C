# ADR-0011: 全 switch に `default:` を置き、値を返すディスパッチャは不正 tag で停止する

- **状態:** 採用（[ADR-0010](./0010-unhandled-tag-hook.md) を置き換え、[ADR-0001](./0001-x-macro-code-generation.md) の「`default:` を置かない」を覆す）
- **関連:** [G-019](../GUARANTEES.md#g-019), [G-020](../GUARANTEES.md#g-020), [G-027](../GUARANTEES.md#g-027), [C-05](../constraints.md#c-05), [C-07](../constraints.md#c-07)

## 文脈

v1 は値を返すディスパッチャ（MATCH / MATCH_CONST / COPY）で、不正 tag を受けると `SUM_UNHANDLED_TAG` を呼んだ後に
`__builtin_unreachable()` へ進んでいた。フックの既定は no-op なので、本番ビルドでは未定義動作になる。
実測では、`-O2` で tag=99 を MATCH に渡すと gcc 13 / clang 18 のどちらでも**無関係なハンドラが呼ばれ、その戻り値が返った**。
クラッシュせずに誤った処理が進むので、組込み・通信系の用途（[requirements.md](../requirements.md)）では最も悪い壊れ方になる。

v1 の判断根拠には誤りが2つあった。

| v1 の根拠 | 誤りの内容 |
|---|---|
| 「戻り値を用意できないため `SUM_UNREACHABLE()` 以外に手がない」（ADR-0010） | 停止（trap / abort）すれば戻り値を用意する必要がない。未定義動作は避けられる |
| 「`default:` を置くと `-Wswitch` が無効になるので置かない」（ADR-0001, ADR-0010） | enum も case 列も同じ VARIANTS から生成されるので、両者はずれようがない。`-Wswitch` が守る対象が存在しない |

さらに `default:` が無いことで、生成コードは `-Wswitch-default` の警告を出し（v1 の examples だけで 14 件）、
MISRA C:2012 Rule 16.4（switch には default を置く）にも抵触していた。

## 決定

- 生成するすべての switch に、空状態の `case`（[ADR-0013](./0013-empty-tag-zero.md)）と `default:` を置く。
- `void` を返すもの（DISPATCH / DISPATCH_LOCKED / DESTROY）は、`SUM_UNHANDLED_TAG(self)` を呼んで復帰する。
- 値を返すもの（MATCH / MATCH_CONST / COPY / COPY_CHECKED / DISPATCH_RET）は、`SUM_UNHANDLED_TAG(self)` を呼んだ後に
  `SUM_TRAP()` で停止する。
  - `SUM_TRAP()` の既定は、GCC/Clang では `__builtin_trap()`、それ以外では `abort()`。
    `__builtin_trap()` はヘッダを要さないのでフリースタンディング環境でも使える。
  - 利用者は `SUM_TRAP()` を自前のフォールトハンドラに差し替えられる。契約は「戻らないこと」。
  - 誤って戻る実装を差し込まれた場合に備えて `SUM_TRAP()` の後ろに無限ループを置く。
    未定義動作にはならず、ウォッチドッグでのリセットという形で表面化する。
- 性能を優先して不正 tag を未定義動作として扱いたい利用者向けに、`SUM_ASSUME_VALID_TAG` を用意する（オプトイン）。
  定義すると `default:` 節は `__builtin_unreachable()` になる。

## 検討した代替案

| 案 | 不採用の理由 |
|---|---|
| フックを必須にする（既定を定義しない） | include するだけのユーザーに定義を強制することになる。安全側の既定値（trap）で足りる |
| 値を返すディスパッチャに「不正時の既定値」引数を足す | 呼び出し側全部の署名が変わる。不正 tag はプログラムの誤りなので、既定値で続行させる意味が薄い |
| DISPATCH / DESTROY も trap する | v1 からの挙動変更が大きい。void 系はフック→復帰で安全に続行できる（ロックの対応も崩れない） |

## 帰結

- v1 の `sum_handled_` フラグ（DISPATCH で不正 tag を判定するためのもの）が不要になった。
- 生成コードは `-Wswitch-default` / `-Wswitch-enum` を出さない（[G-027](../GUARANTEES.md#g-027)）。
- 不正 tag・空状態でハンドラが呼ばれないことを、値を返す5種について検査する（[G-020](../GUARANTEES.md#g-020)）。
- DISPATCH_RET はロック区間内で trap する。`SUM_TRAP` を longjmp で実装する場合、ロックは保持されたままになる。
  `SUM_TRAP` は復帰しない前提の設計なので、これは許容する。
- コードサイズが増える。Cortex-M4 `-Os` の実測では、4 variant の MATCH 呼び出しで 32 → 42 バイト（範囲検査と trap 命令の分）。
  `SUM_ASSUME_VALID_TAG` を定義すると 34 バイト（[VERIFICATION_LOG.md](../../VERIFICATION_LOG.md) v2 節）。

