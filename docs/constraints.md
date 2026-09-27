# 既知の制約

このライブラリが**できないこと**の一覧。判断の根拠は [`adr/`](./adr/)、コンパイル時に
何が保証されるかは [`GUARANTEES.md`](./GUARANTEES.md)、そもそも何を解くライブラリかは
[`requirements.md`](./requirements.md) にある。

各項目には `C-NN` という安定した ID を付けてある。他の文書・コミット・Issue からは
節番号ではなくこの ID で参照すること（節番号は改版で動くため）。

| ID | 制約 | 機械検査 |
|---|---|---|
| [C-01](#c-01) | variant 間でペイロード型が重複すると取り違えを検出できない | C11 以降のみ（[G-009](./GUARANTEES.md#g-009), [G-017](./GUARANTEES.md#g-017)） |
| [C-02](#c-02) | ペイロードに置ける型は宣言子の形で制限される | あり（[G-005](./GUARANTEES.md#g-005), [G-006](./GUARANTEES.md#g-006)） |
| [C-03](#c-03) | 解放・コピーの呼び出し忘れと、コピー後の二重解放は検出できない | なし（同じ値への二重 destroy は no-op。[G-023](./GUARANTEES.md#g-023)） |
| [C-04](#c-04) | 値としての自己埋め込み（再帰的な直和型）は表現できない | なし（言語仕様上の制約。ポインタ経由は可能: [G-032](./GUARANTEES.md#g-032)） |
| [C-05](#c-05) | カプセル化がない | なし |
| [C-06](#c-06) | 1 variant = 1 型 | なし（運用ルール） |
| [C-07](#c-07) | コンパイラ拡張に依存している | なし |
| [C-08](#c-08) | `SUM_DEFINE_MATCH` / `_DISPATCH_RET` に `void` を渡せない | `-pedantic` 依存 |
| [C-09](#c-09) | グローバルロックフックの再定義漏れと再入デッドロック | なし |
| [C-10](#c-10) | メモリレイアウトは処理系定義 | なし（規格が意図的に未規定） |
| [C-11](#c-11) | マクロ呼び出しのコピー&ペースト運用は未検証 | なし（運用ルール） |
| [C-12](#c-12) | デバッグ容易性のトレードオフ | なし |
| [C-13](#c-13) | payload 型が変わる関数合成は汎用化できない | なし（言語仕様上の制約） |
| [C-14](#c-14) | 予約された tag 名がある | あり（[G-029](./GUARANTEES.md#g-029)） |
| [C-15](#c-15) | 変数経由の NULL ハンドラは検出できない | なし（定数 NULL は検出: [G-021](./GUARANTEES.md#g-021), [G-022](./GUARANTEES.md#g-022)） |

---

<a id="c-01"></a>

## C-01 variant 間でペイロード型が重複すると取り違えを検出できない

### 守るべき不変条件

> **同一の SumType に属する2つ以上の variant が、互いに互換な（ISO C の意味で compatible な）
> ペイロード型を持ってはならない。**

ハンドラの順序取り違え検出（[G-002](./GUARANTEES.md#g-002)）は、各ハンドラの関数ポインタ型が
variant ごとに固有であることに依存している。ペイロード型が一致すると関数ポインタ型も完全に一致し、
順序を入れ替えてもコンパイラには区別できない（[G-003](./GUARANTEES.md#g-003)）。

### この不変条件を満たすための推奨手段

**ペイロードに素の基本型（`int`, `double` 等）を直接使わず、たとえ1フィールドでも専用の
`struct` にラップする**（実質的に Rust の newtype パターン）。

ただしこれは**十分条件ではない**。`typedef` は別名を作るだけで別の型を作らないため、
次のコードは推奨手段を守りながら不変条件を破る。

```c
typedef int32_t DeviceId;
typedef int32_t SensorId;   /* 素の基本型は避けたが DeviceId と同じ型 */
```

守るべきは不変条件であって推奨手段ではない、という関係を取り違えないこと。
上の例は [G-017](./GUARANTEES.md#g-017) として機械検査に登録してある。

### 機械検査（C11 以降のみ）

`SUM_DEFINE_NEW_GENERIC` を呼ぶと `_Generic` の連想リスト重複としてコンパイルエラーになる
（[ADR-0008](./adr/0008-generic-constructor-selection.md)）。`SUM_NEW` を実際に使う予定がなくても、
**この不変条件を検査させる目的だけで `SUM_DEFINE_NEW_GENERIC` を呼んでおく価値がある。**

C99 には `_Generic` が無いためこの検査は働かず、設計ガイドラインとコードレビューに頼るしかない
（[G-018](./GUARANTEES.md#g-018)）。

### 残る前提

`-Wall -Wextra -Werror` の運用が必須である。`-Werror` を付けないと
`-Wincompatible-pointer-types` は警告止まりで通ってしまい、順序取り違えの検出が成立しない。

---

<a id="c-02"></a>

## C-02 ペイロードに置ける型は宣言子の形で制限される

`SUM__UNION_MEMBER` / `SUM__CTOR` / `SUM__GETTER` が生成する宣言は `TYPE 識別子;` という
「型名を前に置き識別子をそのまま続ける」パターンを前提にしている。

**関数ポインタ型**は宣言子の構文上、識別子を型の内側（`TYPE (*識別子)(引数型...)`）に埋め込む
必要があるため、このパターンに当てはまらずコンパイルエラーになる（[G-005](./GUARANTEES.md#g-005)）。
原因はマクロ引数のコンマ分割ではない（括弧で保護されている）点に注意。

対策は `typedef` で型名を単一トークンにしてから渡すこと（[G-006](./GUARANTEES.md#g-006)）。

```c
typedef void (*Callback)(int, int);
#define V(X, NAME, EXTRA) X(NAME, EXTRA, cb, Callback)
```

### 配列型は typedef だけでは解決しない

`typedef int IntArr3[3];` で型名を単一トークンにすれば構造体メンバー宣言としては機能する。
しかし配列型は関数引数境界で暗黙にポインタ型へ読み替えられる（ISO C 6.7.6.3p7）ため、
typedef 後もコンストラクタ関数の引数としては正しく機能しない（`sizeof` を前提にしたコピー処理が
範囲外読み出しになり、実機で不定値が返ることを確認済み）。

配列型はそもそも C において代入・値渡し・値の戻り値のいずれもできない型であり、
**typedef は宣言構文の問題を解決するだけで値セマンティクスの問題を解決しない**。

配列をペイロードにしたい場合は [C-01](#c-01) が既に推奨している「専用 struct でラップする」が
唯一の正しい解決策である。struct は通常の値セマンティクスを持つため、
`struct { int v[3]; } IntArr3Box;` のように包めば構造体代入だけで正しく値コピーされる。

### 一般化した運用ルール

> ペイロードには**単純な型・ポインタ型・typedef 済みの関数ポインタ型**か、
> **配列を含む場合は必ず struct でラップした型**のみを渡す。

---

<a id="c-03"></a>

## C-03 解放・コピーの呼び出し忘れと、コピー後の二重解放は検出できない

`SUM_DEFINE_DESTROY` / `SUM_DEFINE_COPY` は「呼べば全 variant を網羅的に処理する」ところまでを
保証する。**呼ぶこと自体は強制できない**（[ADR-0004](./adr/0004-destroy-copy-separate-macros.md)）。
`Message_destroy()` を呼び忘れてもコンパイルエラーにはならず、単にリークする。

DESTROY は解放後に値を空状態へ戻すため、**同じ値**への2回目の DESTROY は no-op になる
（[ADR-0013](./adr/0013-empty-tag-zero.md)、[G-023](./GUARANTEES.md#g-023)）。しかし構造体代入で作った**別の変数**は
同じ資源を指したままなので、両方を DESTROY すれば二重解放になる。C には move がないため防げない。

コピーの失敗（確保失敗など）を扱う場合は `SUM_DEFINE_COPY_CHECKED` を使う（[ADR-0015](./adr/0015-copy-checked.md)）。

Rust の ownership / borrow checker に相当するものは C に存在しない。これは
「渡し忘れの検出」（[G-001](./GUARANTEES.md#g-001)）とは別の話である。

`__attribute__((cleanup(...)))` による自動化も検討したが、C に move semantics がないため
値コピー時に二重解放を引き起こすリスクがあり根本解決にならない（ASan・CBMC 双方の実機検証で確認済み。
経緯は [VERIFICATION_LOG.md](../VERIFICATION_LOG.md) 8節）。
CBMC ハーネス（[verify/](../verify/README.md)）は、呼び忘れ・コピー後の二重解放・同じ値への二重 destroy の3つを検証している。

---

<a id="c-04"></a>

## C-04 値としての自己埋め込み（再帰的な直和型）は表現できない

payload に自身の型を**値として**埋め込むことはできない。
**これはマクロ実装の制約ではなく言語仕様そのものの制約である**（ISO C 6.7.2.1）。
詳細と判断の根拠は [ADR-0006](./adr/0006-no-recursive-sum-types.md)。

ポインタ経由の自己参照（`struct NAME *`）は書ける（[ADR-0017](./adr/0017-self-reference-by-pointer.md)、[G-032](./GUARANTEES.md#g-032)）。
所有権（確保・解放の責任）はライブラリが管理しない。

---

<a id="c-05"></a>

## C-05 カプセル化がない

`NAME_get_<tag>()` はタグを検査して不一致なら `NULL` を返す安全なアクセサだが、
`self->as.TAG` に直接アクセスすればこの検査をすり抜けられる。`union` メンバは常に公開であり、
C には真のカプセル化機構がない。

この安全性は「ゲッター経由でのみ触る」という規約に依存し、**コードレビューで担保するしかない**。

なお tag が破壊された状態・空状態でディスパッチされた場合の挙動は
[ADR-0011](./adr/0011-default-label-and-trap.md) で定義してある（`SUM_UNHANDLED_TAG` フックと `SUM_TRAP`）。

---

<a id="c-06"></a>

## C-06 1 variant = 1 型

Rust の `Variant(i32, String)` のような複数フィールドを持つタプルバリアントは直接表現できない。
事前に `struct` にまとめて渡す。

これは設計上の制約というより運用ルールであり、[C-01](#c-01) の「専用 struct でラップする」推奨と
方向が一致するため、実務上の追加負担はほとんどない。

---

<a id="c-07"></a>

## C-07 コンパイラ拡張に依存している

GCC / Clang では次の処理系拡張を使う。いずれも ISO C の規定するキーワード・標準ライブラリ関数ではない。

| 内部マクロ | GCC / Clang | それ以外 | 影響 |
|---|---|---|---|
| `SUM__MAYBE_UNUSED` | `__attribute__((unused))` | 空 | Clang の `-Wunused-function` が生成関数に出る |
| `SUM__NONNULL` | `__attribute__((nonnull))` | 空 | 定数 NULL ハンドラの検出（[G-021](./GUARANTEES.md#g-021)）が働かない |
| `SUM_TRAP()` の既定 | `__builtin_trap()` | `abort()`（`<stdlib.h>`） | フリースタンディング環境で `abort` が無ければ `SUM_TRAP` を定義する |
| `SUM_ASSUME_VALID_TAG` 時 | `__builtin_unreachable()` | `abort()` | オプトイン時のみ |

この制約は「規格を読み込めば回避できる」ものではなく、**そもそも規格の対象外の機能に依存している**
という性質のものであり、[C-04](#c-04) / [C-10](#c-10) のような規格条文との突き合わせが
原理的に成立しない領域である。

MSVC は対象外と決定しているため、これ以上の検討は行わない（[ADR-0007](./adr/0007-msvc-out-of-scope.md)）。

---

<a id="c-08"></a>

## C-08 `SUM_DEFINE_MATCH` / `_DISPATCH_RET` に `void` を渡せない

`SUM_DEFINE_MATCH(..., void)` は多くの環境でコンパイルが通るが、`-pedantic` を付けると
`ISO C forbids 'return' with expression, in function returning void` という警告が出る
（`case NAME_TAG_x: return handler();` の `handler()` が void を返す関数呼び出しのため）。
`SUM_DEFINE_DISPATCH_RET` に `void` を渡すと、`void` 型のローカル変数を宣言することになりコンパイルできない。

ISO C 6.8.6.4 により診断が要求される明確な制約違反であり、コンパイラが警告止まりで許容しているだけである。
`-Werror -pedantic` を運用しているプロジェクトではビルドが壊れる。

**void 用途には `SUM_DEFINE_DISPATCH`（または `SUM_DEFINE_DESTROY`）を使うこと**
（[ADR-0002](./adr/0002-separate-match-and-dispatch.md)）。

---

<a id="c-09"></a>

## C-09 グローバルロックフックの再定義漏れと再入デッドロック

### 再定義漏れ

`SUM_DEFINE_DISPATCH` / `SUM_DEFINE_DISPATCH_RET` が使う `SUM_CTX_LOCK` / `SUM_CTX_UNLOCK` の既定は
no-op であり、**再定義し忘れると気づかず排他制御なしで動く**。コンパイル時にロックの有無を強制する
仕組みはない（[ADR-0003](./adr/0003-lock-hook-injection.md)）。

ロックが必要な DISPATCH は `SUM_DEFINE_DISPATCH_LOCKED` / `SUM_DEFINE_DISPATCH_RET_LOCKED` で定義する
（[ADR-0014](./adr/0014-dispatch-ret-and-locked.md)）。ロック関数を定義箇所で渡すため書き忘れが見え、
同じ翻訳単位に型ごとに異なるロック方針を置ける（[G-025](./GUARANTEES.md#g-025)）。ロック不要なら
`SUM_NO_LOCK` を渡して明示する。

フックは `SUM_DEFINE_DISPATCH` を**展開した時点**で評価される。翻訳単位の分割はロック方針の切り替えに
必須ではない（DEFINE の間で `#undef` / 再定義しても切り替わる）。

### 再入デッドロック

ロック区間は `SUM_CTX_LOCK(ctx)` からハンドラ本体を含む `switch` 全体を経て
`SUM_CTX_UNLOCK(ctx)` までを1つの区間とする。この粒度の帰結として、**あるハンドラが同一の
`ctx` に対して直接または間接に別の dispatch を呼び返すと、非再帰ロック
（`pthread_mutex` のデフォルトや多くの RTOS セマフォ）では自己デッドロックする。**

これは実装バグではなく、ロック区間がハンドラ実行全体を包むという設計選択の必然的な帰結である。

対策は次のいずれか。

- **規約で回避する（推奨）** — ハンドラ内で同一 `ctx` への再 dispatch を行わない。
  dispatch はコマンド1件の適用で完結させ、連鎖が必要ならロック区間の外で次を発行する
- **診断可能性を上げる** — `PTHREAD_MUTEX_ERRORCHECK` で初期化した mutex を差し込む。
  デッドロックせず `EDEADLK` が即座に返るため、**ハングという最も原因究明が高コストな
  失敗の仕方を避けられる**
- **再入対応ロックを差し込む** — 再入が設計上避けられない場合のみ。
  `PTHREAD_MUTEX_RECURSIVE` や RTOS の再帰対応ミューテックス

`SUM_DEFINE_DISPATCH`（void）は不正 tag・空状態でもフックを呼んでから `UNLOCK` に到達するため、
**LOCK / UNLOCK の対応は崩れない**（[G-019](./GUARANTEES.md#g-019)）。
`SUM_DEFINE_DISPATCH_RET` は不正 tag でロック区間内のまま `SUM_TRAP()` に進む
（[ADR-0011](./adr/0011-default-label-and-trap.md)。`SUM_TRAP` は復帰しない前提）。

---

<a id="c-10"></a>

## C-10 メモリレイアウトは処理系定義

`union` のサイズは最大 variant のサイズになるが、padding / alignment の詳細はコンパイラ依存であり、
Rust の enum が持つ niche optimization に相当する仕組みはない。

ISO C 6.7.2.1 では `union` のサイズは「最大のメンバを保持できるだけの大きさ」とだけ規定され、
末尾に追加のパディングが入ってよいことも明記されている。struct についても、メンバ間および
最終メンバの後ろに名前のないパディングが入ってよいとされる。

tag の数値も安定 ABI として約束しない。v2 で空状態 `NAME_TAG__EMPTY = 0` を予約したため、
variant の tag 値は v1 から 1 ずれた（[ADR-0013](./adr/0013-empty-tag-zero.md)）。tag を永続化・通信する場合は
自前の変換表を持つこと。

つまりこれは実装の手抜きではなく、**規格が意図的に処理系定義としている領域**であり、
`sizeof` の値をコンパイラ間で一致させる保証は最初から存在しない。本ライブラリの制約というより
C の struct / union 全般に共通する性質である。

---

<a id="c-11"></a>

## C-11 マクロ呼び出しのコピー&ペースト運用は未検証

推奨運用（型定義を1つの共通ヘッダに1回だけ書き、複数の `.c` から include する）は
問題なく動作する（[G-007](./GUARANTEES.md#g-007)）。`-flto` を含めて検証済み。

生成される関数はすべて `static inline`（内部リンケージ）であり、TU 間で共有される外部シンボルが
一つも存在しないため、匿名 union / struct の型が ISO C 6.2.7 の意味で複数 TU 間互換とみなされるか
という論点自体が、この使い方の範囲では問う必要のない問いになっている。

**「同じマクロ呼び出しを、共通ヘッダを介さずに複数の `.c` へコピー&ペーストする」運用は避けること。**
その場合は字面上同一でも独立した定義として扱われ、上記の検証結果は当てはまらない（未検証）。
この違反を機械的に検出する手段はないため、運用ルールとして守る。

---

<a id="c-12"></a>

## C-12 デバッグ容易性のトレードオフ

X-Macro によるコード生成は、手書き関数と比べて次のトレードオフがある。

### しづらくなる点

- **マクロ定義自体のバグが追いにくい。** 実際に v1 の `SUM_DISPATCH_CASE` が `return;` で抜けるせいで
  後段の `SUM_CTX_UNLOCK` がスキップされ、ロックを取得したまま解放しないバグを作り込んだ
  （[VERIFICATION_LOG.md](../VERIFICATION_LOG.md) 7節）。生成された関数の「見た目」を読んでも気づきにくい
- **最適化ビルドでは `static inline` 関数がインライン化されて消える。** `-O0 -g` では
  `Command_dispatch` がシンボルとして残るが、`-O2 -g` では消えて名前でのブレークポイント設定ができない。
  デバッグ時は `-O0 -g` でビルドし直す運用が実質必須
- **型リスト定義の構文ミスは、間違えた箇所ではなく `SUM_DEFINE_TYPE(...)` の呼び出し行にエラーが出る。**
  継続行の `\` を付け忘れた際に「unterminated #ifndef」という一見無関係なエラーが出た実例がある

### 緩和されている点

- 生成される関数名は C++ テンプレートのようにマングルされず、素の C 関数としてバックトレースに出る
- enum のタグ値も `Command_TAG_move` のように意味のある名前を持つため、デバッガで `self->tag` を
  表示すればそのまま読める
- 生成された関数はすべて普通の C 関数なので、個別に単体テストが書ける

### 運用ルール

- デバッグ時は `-O0 -g` でビルドする
- マクロ展開後の実コードは `gcc -E -std=c11 foo.c | clang-format` で見る
- エラーは「呼び出し行」だけでなく「マクロ定義側の行」まで含めて note を最後まで読む

---

<a id="c-13"></a>

## C-13 payload 型が変わる関数合成は汎用化できない

Rust / F# の `map` / `and_then` / `map_err` のように **payload 型が変わる**関数合成
（Railway Oriented Programming）は、C では汎用に書けない。

`map: Result<T,E> → Result<U,E>` の変換先 `Result<U,E>` は `Result<T,E>` とは別の具体型
（別 typedef）になるが、C にはパラメトリック多相も戻り値型推論もなく、マクロは呼び出し側で
目標型を生成できない（マクロ展開結果から `#define` を起こせない制約と同根。
[ADR-0008](./adr/0008-generic-constructor-selection.md)）。

したがって書けるのは次のいずれか。

- **同型変換**（`T → Result<T,E>`。`examples/result_demo.c` の `and_then` がこれ）
- **型を変える段ごとに目標の `Result` 型を明示して手書きする合成**

流れるようなチェーンは言語機能の制約でそのままは再現できない。これは
「値としてのエラー表現（直和型・網羅 match・述語・fold）は素直に書けるが、
関数**合成**は言語特性に依存する」という一般論の一例でもある。

---

<a id="c-14"></a>

## C-14 予約された tag 名がある

生成される識別子は `NAME_TAG_<tag>`（enum 定数）、`NAME_new_<tag>` / `NAME_get_<tag>` /
`NAME_get_<tag>_const` / `NAME_is_<tag>`（関数）である（[ADR-0012](./adr/0012-naming-v2.md)）。
v1 の衝突（tag `get_a` と tag `a` のゲッター等）は解消したが、次は残る。

| 予約 | 理由 |
|---|---|
| 末尾が `_const` で、他の tag 名 + `_const` と一致する tag 名（`a` と `a_const`） | `NAME_get_a_const` が2つ生成される（[G-029](./GUARANTEES.md#g-029)） |
| `_` で始まる tag 名 | `NAME_TAG__EMPTY` と衝突し得る。また `SUM_DEFINE_NEW_GENERIC` の内部識別子（`NAME_new_unreachable_` 等） |
| `SUM_LEGACY_TAG_NAMES` 有効時の、v1 と同じ衝突 | 移行期間だけ使うこと |

いずれも衝突時はコンパイルエラーになり、黙って壊れることはない。

---

<a id="c-15"></a>

## C-15 変数経由の NULL ハンドラは検出できない

生成関数は `__attribute__((nonnull))` 付きで、ハンドラに定数 `NULL` を渡すと `-Wnonnull` で落ちる
（[ADR-0016](./adr/0016-nonnull-handlers.md)、[G-021](./GUARANTEES.md#g-021)）。変数・関数ポインタテーブル経由で
NULL を渡した場合はコンパイラが値を追えず、検出できない（[G-022](./GUARANTEES.md#g-022)）。
その variant が来た時点で実行時に落ちる。

