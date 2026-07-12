# examples/specs — デモの要件・仕様

各デモ（抽象例を実務シナリオに置き換えたもの）について、要件 → 仕様 → 実装対応 → 受け入れ基準をまとめたもの。デモ本体の冒頭コメントにも【要件】【仕様】【実装方針】を記載している。

| デモ | シナリオ | 仕様 | 主に使うマクロ |
|---|---|---|---|
| `demo.c` | 設定値ストア | [config_value.md](./config_value.md) | `DEFINE_SUM_MATCH` |
| `shape_demo.c` | 描画プリミティブのコスト見積り | [draw_primitive.md](./draw_primitive.md) | `DEFINE_SUM_MATCH`×2 |
| `command_demo.c` | モータ制御コマンド | [motor_command.md](./motor_command.md) | `DEFINE_SUM_DISPATCH` |
| `resource_demo.c` | ログレコードの解放・複製 | [log_record.md](./log_record.md) | `DEFINE_SUM_DESTROY`/`COPY` |
| `threadsafe_dispatch_demo.c` | テレメトリカウンタ並行更新 | [telemetry_counter.md](./telemetry_counter.md) | `DEFINE_SUM_DISPATCH` + lockフック |
| `generic_ctor_demo.c` | 型からのイベント構築 | [sensor_event.md](./sensor_event.md) | `DEFINE_SUM_NEW_GENERIC`/`SUM_NEW` |
| `const_view_demo.c` | read-only 検査 | [draw_inspect.md](./draw_inspect.md) | `DEFINE_SUM_MATCH_CONST` |
| `either_demo.c` | キャッシュ Hit/Miss | [cache_lookup.md](./cache_lookup.md) | `DEFINE_EITHER_HELPERS` |
| `result_demo.c` | 注文処理の Result | [order_result.md](./order_result.md) | `DEFINE_RESULT_HELPERS` |
