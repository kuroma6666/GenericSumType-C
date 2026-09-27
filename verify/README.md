# verify/

CBMC(Bounded Model Checker for C)が使える環境での、任意の追加検証を置くディレクトリ。

**通常のビルド・テスト・CIには一切影響しない。** `.github/workflows/ci.yml` からは呼ばれておらず、`cbmc`が入っていない環境ではこのディレクトリを無視して構わない。

## 内容

- `cbmc_destroy_harness.c` — `SUM_DEFINE_DESTROY` の呼び出し忘れ、`__attribute__((cleanup(...)))` で自動化した場合のコピー後の二重解放、同じ値への二重 destroy を CBMC で検証するハーネス（[C-03](../docs/constraints.md#c-03)）
- `cbmc_verify_destroy.sh` — 上記ハーネスの4ケースをまとめて実行し、期待通りの結果か判定するスクリプト

## 使い方

```sh
sudo apt-get install cbmc   # または該当ディストロのパッケージマネージャ
cd verify
chmod +x cbmc_verify_destroy.sh
./cbmc_verify_destroy.sh
```

## 確認結果（CBMC 5.95.1）

| ケース | 期待 | 結果 |
|---|---|---|
| `safe_case` | 成功 | `VERIFICATION SUCCESSFUL`（0 of 142 failed） |
| `leaked_case` | 検出 | `VERIFICATION FAILED`（memory-leak） |
| `double_free_case` | 検出 | `VERIFICATION FAILED`（double free。構造体コピーした別変数の二重解放は v2 でも防げない） |
| `double_destroy_case` | 成功 | `VERIFICATION SUCCESSFUL`（v2 の DESTROY は解放後に空状態へ戻す。v1 のヘッダでは double free を検出） |

4ケースともループ・再帰を含まないため、CBMC のシンボリック実行は近似（bounded）ではなく状態空間を網羅的に探索している。
v1 との比較は [VERIFICATION_LOG.md](../VERIFICATION_LOG.md) の v2 節にある。
