#!/usr/bin/env python3
"""リポジトリ内の Markdown 相互リンク（ファイルパスとアンカー）を検査する。

制約と保証には C-01 / G-001 という安定した ID を振り、他の文書からはその ID で
参照する規約にしている。この規約は、リンクが実際に解決することを機械的に
確かめて初めて意味を持つ。ID を振り替えたりファイルを移動したときに、
参照側の直し忘れをここで落とす。

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

    if problems:
        print(f"NG  リンク不良 {len(problems)} 件:")
        for p in problems:
            print("      " + p)
        return 1

    total = sum(len(LINK.findall(open(p, encoding="utf-8", errors="replace").read()))
                for p in markdown_files())
    print(f"OK  Markdown リンク検査: {total} 件すべて解決")
    return 0


if __name__ == "__main__":
    sys.exit(main())
