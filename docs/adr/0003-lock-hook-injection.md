# ADR-0003: ロックプリミティブを埋め込まずフックとして外出しする

- **状態:** 採用
- **関連:** [C-09](../constraints.md#c-09)

## 文脈

`DEFINE_SUM_DISPATCH` は共有コンテキストを書き換えるため、複数スレッドから使う場合は排他制御が要る。
しかし RTOS（FreeRTOS のセマフォ等）と Linux（`pthread_mutex` 等）では使えるロック機構が異なり、
どちらか一方に依存する実装をライブラリ本体に持たせると他方の環境で使えなくなる。

## 決定

`SUM_CTX_LOCK(ctx)` / `SUM_CTX_UNLOCK(ctx)` をフックマクロとして提供し、デフォルトは no-op にする。
必要な環境ではヘッダを include する前に利用側で再定義する。

```c
#define SUM_CTX_LOCK(ctx)   pthread_mutex_lock(&(ctx)->mutex)
#define SUM_CTX_UNLOCK(ctx) pthread_mutex_unlock(&(ctx)->mutex)
#include "generic_sum_type.h"
```

ロックの粒度（dispatch 呼び出し単位で十分か、もっと粗い単位にすべきか）はアプリケーション側の
責務とし、ライブラリは関知しない。

## 検証

`threadsafe_dispatch_demo.c` で `pthread_mutex` をフックに差し込み、+1 と -1 を行うスレッドを
4本ずつ走らせて最終値が 0 になることを3回確認した。比較のためフックを no-op のままにした版では、
8スレッドが単純に increment するだけのケースでも期待値 1,600,000 に対し実測 92万〜65万まで
ばらつく lost update が再現し、**フックが実際に競合を防いでいること**を実測で裏付けた。

## 帰結

- ロックが不要な用途では追加コストがゼロ（no-op は最適化で消える）
- **フックを再定義し忘れると、気づかず排他制御なしで動く。** コンパイル時に強制する仕組みはない
  （[C-09](../constraints.md#c-09)）
- **ロック区間がハンドラ本体全体を包むため、同一 ctx への再 dispatch で自己デッドロックする。**
  これは実装バグではなく、この粒度を選んだことの必然的な帰結である（[C-09](../constraints.md#c-09)）
