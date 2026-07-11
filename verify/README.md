# verify/

CBMC(Bounded Model Checker for C)が使える環境での、任意の追加検証を置くディレクトリ。

**通常のビルド・テスト・CIには一切影響しない。** `.github/workflows/ci.yml` からは呼ばれておらず、`cbmc`が入っていない環境ではこのディレクトリを無視して構わない。

## 内容

- `cbmc_destroy_harness.c` — `DEFINE_SUM_DESTROY`の呼び出し忘れ、および`__attribute__((cleanup(...)))`で自動化しようとした場合に発生する二重解放を、CBMCで検証するためのハーネス(design_spec.md 8節参照)
- `cbmc_verify_destroy.sh` — 上記ハーネスの3ケースをまとめて実行し、期待通りの結果か判定するスクリプト

## 使い方

```sh
sudo apt-get install cbmc   # または該当ディストロのパッケージマネージャ
cd verify
chmod +x cbmc_verify_destroy.sh
./cbmc_verify_destroy.sh
```

## 実機での確認結果(CBMC 5.95.1)

| ケース | 結果 |
|---|---|
| `safe_case` | `VERIFICATION SUCCESSFUL`(0 of 115 failed) |
| `leaked_case` | `VERIFICATION FAILED`(memory-leakを検出) |
| `double_free_case` | `VERIFICATION FAILED`(double freeを検出) |

3ケースともループ・再帰を含まないため、CBMCのシンボリック実行は近似(bounded)ではなく状態空間を網羅的に探索している。詳細はdesign_spec.md 8節を参照。
