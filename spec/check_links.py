#!/usr/bin/env python3
"""リポジトリ内の相互参照を検査する。

1. Markdown のリンク（ファイルパスとアンカー）が解決すること。
2. ソースコード（.c / .h / .sh / .yml / .py）のコメント中の参照が解決すること。
   - `docs/...md` などのパス、`docs/adr/NNNN` が存在すること
   - `C-01` / `G-001` という ID が constraints.md / GUARANTEES.md に存在すること
   - 旧 design_spec.md の節番号参照（`design_spec`、`4.10節` 等）が残っていないこと
     （design_spec.md は移設マップのみで、節番号は改版で動くため ID で参照する規約）

制約と保証には C-01 / G-001 という安定した ID を振り、他の文書からはその ID で
参照する規約にしている。この規約は、参照が実際に解決することを機械的に
確かめて初めて意味を持つ。

    python3 spec/check_links.py

外部 URL（http / https / mailto）は検査しない。
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SKIP_DIRS = {".git", "node_modules"}
LINK = re.compile(r"\[[^\]]*\]\(([^)\s]+)\)")
EXPLICIT_ANCHOR = re.compile(r'<a\s+id="([^"]+)"')
HEADING = re.compile(r"^#{1,6}\s+(.+)$", re.M)


def slug(heading: str) -> str:
    """GitHub の見出しアンカー生成をおおまかに再現する。"""
    s = re.sub(r"[^\w\s\-ぁ-んァ-ヶ一-龠]", "", heading)
    return s.strip().lower().replace(" ", "-")


def markdown_files():
    for dirpath, dirnames, filenames in os.walk(ROOT):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
        for name in filenames:
            if name.endswith(".md"):
                yield os.path.join(dirpath, name)


SOURCE_EXT = (".c", ".h", ".sh", ".yml", ".yaml", ".py")
SRC_PATH = re.compile(r"\b((?:docs|spec|examples|include|tests|verify|scripts)/[\w./-]+\.(?:md|c|h|sh|tsv|py)"
                      r"|(?:api_reference|VERIFICATION_LOG|CHANGELOG|README)\.md)")
SRC_ADR = re.compile(r"\bdocs/adr/(\d{4})\b")
SRC_ID = re.compile(r"\b([CG])-(\d{2,3})\b")
SRC_STALE = re.compile(r"design_spec|\d+(?:\.\d+)?節")


def source_files():
    for dirpath, dirnames, filenames in os.walk(ROOT):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
        for name in filenames:
            if name.endswith(SOURCE_EXT):
                yield os.path.join(dirpath, name)


def check_sources(anchors):
    problems = []
    adr_dir = os.path.join(ROOT, "docs", "adr")
    adrs = {n[:4] for n in os.listdir(adr_dir)} if os.path.isdir(adr_dir) else set()
    this = os.path.abspath(__file__)
    for path in source_files():
        if os.path.abspath(path) == this:
            continue
        here = rel(path)
        for lineno, line in enumerate(open(path, encoding="utf-8", errors="replace"), 1):
            for m in SRC_PATH.finditer(line):
                target = m.group(1).rstrip(".")
                if not os.path.exists(os.path.join(ROOT, target)):
                    problems.append(f"{here}:{lineno}: 存在しないパス {target}")
            for m in SRC_ADR.finditer(line):
                if m.group(1) not in adrs:
                    problems.append(f"{here}:{lineno}: 存在しない ADR docs/adr/{m.group(1)}")
            for m in SRC_ID.finditer(line):
                kind, num = m.group(1), m.group(2)
                doc = "docs/constraints.md" if kind == "C" else "docs/GUARANTEES.md"
                if f"{kind.lower()}-{num}" not in anchors.get(doc, set()):
                    problems.append(f"{here}:{lineno}: {doc} に存在しない ID {kind}-{num}")
            if SRC_STALE.search(line):
                problems.append(f"{here}:{lineno}: 旧 design_spec の節番号参照が残っている")
    return problems


def rel(path: str) -> str:
    return os.path.relpath(path, ROOT).replace(os.sep, "/")


def main() -> int:
    anchors = {}
    for path in markdown_files():
        text = open(path, encoding="utf-8", errors="replace").read()
        found = set(EXPLICIT_ANCHOR.findall(text))
        found.update(slug(h) for h in HEADING.findall(text))
        anchors[rel(path)] = found

    problems = []
    for path in markdown_files():
        here = rel(path)
        text = open(path, encoding="utf-8", errors="replace").read()
        for match in LINK.finditer(text):
            link = match.group(1)
            if link.startswith(("http://", "https://", "mailto:", "#!")):
                continue
            target, _, anchor = link.partition("#")

            if not target:  # 同一ファイル内のアンカー
                if anchor and anchor not in anchors[here]:
                    problems.append(f"{here}: 自己アンカー #{anchor} が存在しない")
                continue

            resolved = os.path.normpath(os.path.join(os.path.dirname(path), target))
            if not os.path.exists(resolved):
                problems.append(f"{here}: リンク切れ -> {link}")
                continue

            target_rel = rel(resolved)
            if anchor and target_rel in anchors and anchor not in anchors[target_rel]:
                problems.append(f"{here}: アンカーが存在しない -> {link}")

    problems.extend(check_sources(anchors))

    # 経緯を記録する文書以外の Markdown に、旧 design_spec の節番号参照が残っていないか
    history_docs = {"design_spec.md", "VERIFICATION_LOG.md", "CHANGELOG.md"}
    for path in markdown_files():
        here = rel(path)
        if here in history_docs:
            continue
        for lineno, line in enumerate(open(path, encoding="utf-8", errors="replace"), 1):
            if re.search(r"design_spec(?:\.md)?\s*\d", line):
                problems.append(f"{here}:{lineno}: 旧 design_spec の節番号参照が残っている")

    if problems:
        print(f"NG  リンク不良 {len(problems)} 件:")
        for p in problems:
            print("      " + p)
        return 1

    total = sum(len(LINK.findall(open(p, encoding="utf-8", errors="replace").read()))
                for p in markdown_files())
    print(f"OK  Markdown リンク {total} 件・ソースコード中の参照すべて解決")
    return 0


if __name__ == "__main__":
    sys.exit(main())
