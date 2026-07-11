#!/usr/bin/env bash
#
# cbmc_verify_destroy.sh
#
# cbmc_destroy_harness.c の3ケース(safe_case / leaked_case / double_free_case)を
# CBMCで検証し、期待通りの結果になっているかをまとめて確認するスクリプト。
# CIには組み込んでおらず、CBMCが使える環境での任意の追加検証という位置づけ。
#
# 使い方(verify/ディレクトリで実行する想定):
#   chmod +x cbmc_verify_destroy.sh
#   ./cbmc_verify_destroy.sh
#   ./cbmc_verify_destroy.sh cbmc_destroy_harness.c /path/to/include  # include_dirを明示する場合
#
#   harness_file : デフォルト cbmc_destroy_harness.c
#   include_dir  : generic_sum_type.h があるディレクトリ(デフォルト: ../include)
#
# 期待される結果:
#   safe_case        -> VERIFICATION SUCCESSFUL (exit code 0)
#   leaked_case      -> VERIFICATION FAILED     (メモリリーク検出、exit code != 0)
#   double_free_case -> VERIFICATION FAILED     (二重解放検出、exit code != 0)
#
# 注意: CBMCのexit codeは「0=成功」「0以外=検証失敗 or ツールエラー」の大まかな区別しかできない。
# 0以外だった場合、実際にリーク/二重解放を検出したのか、単なる構文エラー等かは
# 画面に出力される本文(Trace等)を目視で確認すること。

set -u

HARNESS="${1:-cbmc_destroy_harness.c}"
INCLUDE_DIR="${2:-../include}"
CBMC_OPTS="--pointer-check --memory-leak-check --bounds-check"

if [ ! -f "$HARNESS" ]; then
    echo "エラー: ハーネスファイルが見つかりません: $HARNESS" >&2
    exit 1
fi

if ! command -v cbmc >/dev/null 2>&1; then
    echo "エラー: cbmc が見つかりません。'sudo apt-get install cbmc' 等でインストールしてください。" >&2
    exit 1
fi

FAILCOUNT=0

run_case() {
    local fn="$1"
    local expect="$2"   # "success" または "fail"

    echo "=================================================="
    echo "== $fn (期待: $expect) =="
    echo "=================================================="
    cbmc $CBMC_OPTS -I "$INCLUDE_DIR" --function "$fn" "$HARNESS"
    local rc=$?

    if [ "$expect" = "success" ] && [ "$rc" -eq 0 ]; then
        echo "-> OK (期待通り成功)"
    elif [ "$expect" = "fail" ] && [ "$rc" -ne 0 ]; then
        echo "-> OK (期待通り検出された。詳細は上記本文で確認)"
    else
        echo "-> NG (期待と異なる結果。exit code=$rc)"
        FAILCOUNT=$((FAILCOUNT + 1))
    fi
    echo
}

run_case safe_case        success
run_case leaked_case      fail
run_case double_free_case fail

echo "=================================================="
if [ "$FAILCOUNT" -eq 0 ]; then
    echo "全ケースが期待通りの結果になりました。"
    exit 0
else
    echo "$FAILCOUNT 件のケースが期待と異なる結果でした。上記ログを確認してください。"
    exit 1
fi
