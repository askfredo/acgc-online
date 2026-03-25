#!/usr/bin/env python3
"""
merge_deluxe.py - Merge VER_DELUXE compile-time guards from ac-decomp into ac-pc-port.

Finds all #if VERSION >= VER_DELUXE / #if VERSION < VER_DELUXE blocks in ac-decomp,
extracts them with surrounding context, fuzzy-matches the insertion point in ac-pc-port,
and inserts the blocks.

Usage:
    python3 tools/merge_deluxe.py           # dry-run (report only)
    python3 tools/merge_deluxe.py --apply   # actually modify files
    python3 tools/merge_deluxe.py --verbose # show detailed context matching info
"""

import argparse
import os
import re
import sys
from dataclasses import dataclass, field
from difflib import SequenceMatcher
from pathlib import Path
from typing import Optional


# ── Configuration ──────────────────────────────────────────────────────────

DECOMP_ROOT = Path("/home/bryan/ac-decomp")
PCPORT_ROOT = Path("/home/bryan/ac-pc-port")

SCAN_DIRS = ["src", "include"]

# How many context lines to capture around each block
CONTEXT_LINES = 12

# Minimum similarity for a single line to count as "matching"
LINE_MATCH_THRESHOLD = 0.60

# Minimum number of sequential context lines that must match to anchor
MIN_SEQ_MATCH = 2


# ── Data structures ────────────────────────────────────────────────────────

@dataclass
class DeluxeBlock:
    """A single VER_DELUXE preprocessor block extracted from decomp."""
    file_rel: str
    start_line: int         # 0-indexed
    end_line: int           # 0-indexed, inclusive (#endif line)
    guard_type: str         # "gte" or "lt"
    has_else: bool
    else_line: Optional[int]
    block_lines: list[str]
    context_before: list[str]
    context_after: list[str]
    old_code_lines: list[str] = field(default_factory=list)

    @property
    def description(self) -> str:
        guard = "#if VERSION >= VER_DELUXE" if self.guard_type == "gte" else "#if VERSION < VER_DELUXE"
        return f"{self.file_rel}:{self.start_line + 1} {guard} ({len(self.block_lines)} lines)"


@dataclass
class MergeResult:
    block: DeluxeBlock
    status: str             # "inserted", "already_present", "skipped_no_match", "skipped_no_file"
    detail: str = ""
    insert_line: int = -1
    action: str = "insert"  # "insert" or "replace"
    replace_end: int = -1   # For replace: end line (exclusive)


# ── Extraction ─────────────────────────────────────────────────────────────

def extract_deluxe_blocks(filepath: Path, file_rel: str) -> list[DeluxeBlock]:
    """Parse a file and extract all VER_DELUXE guarded blocks with context."""
    with open(filepath, "r", errors="replace") as f:
        raw_lines = [l.rstrip("\n") for l in f.readlines()]

    blocks = []
    i = 0
    while i < len(raw_lines):
        stripped = raw_lines[i].strip()

        guard_type = None
        if stripped == "#if VERSION >= VER_DELUXE":
            guard_type = "gte"
        elif stripped == "#if VERSION < VER_DELUXE":
            guard_type = "lt"

        if guard_type is None:
            i += 1
            continue

        start_line = i
        nesting = 1
        else_line = None
        j = i + 1

        while j < len(raw_lines) and nesting > 0:
            s = raw_lines[j].strip()
            if s.startswith("#if ") or s.startswith("#ifdef ") or s.startswith("#ifndef "):
                nesting += 1
            elif s == "#endif" or s.startswith("#endif ") or s.startswith("#endif\t"):
                nesting -= 1
                if nesting == 0:
                    break
            elif (s == "#else" or s.startswith("#else ") or s.startswith("#else\t")) and nesting == 1:
                else_line = j
            j += 1

        if nesting != 0:
            i += 1
            continue

        end_line = j

        block_lines = raw_lines[start_line:end_line + 1]
        ctx_start = max(0, start_line - CONTEXT_LINES)
        context_before = raw_lines[ctx_start:start_line]
        ctx_end = min(len(raw_lines), end_line + 1 + CONTEXT_LINES)
        context_after = raw_lines[end_line + 1:ctx_end]

        old_code_lines = []
        if else_line is not None:
            if guard_type == "gte":
                old_code_lines = raw_lines[else_line + 1:end_line]
            else:
                old_code_lines = raw_lines[start_line + 1:else_line]

        blocks.append(DeluxeBlock(
            file_rel=file_rel,
            start_line=start_line,
            end_line=end_line,
            guard_type=guard_type,
            has_else=else_line is not None,
            else_line=else_line,
            block_lines=block_lines,
            context_before=context_before,
            context_after=context_after,
            old_code_lines=old_code_lines,
        ))

        i = end_line + 1

    return blocks


def find_all_deluxe_blocks() -> list[DeluxeBlock]:
    """Scan decomp src/ and include/ for VER_DELUXE blocks."""
    all_blocks = []
    for scan_dir in SCAN_DIRS:
        root = DECOMP_ROOT / scan_dir
        if not root.exists():
            continue
        for dirpath, _, filenames in os.walk(root):
            for fname in sorted(filenames):
                if not any(fname.endswith(ext) for ext in (".c", ".h", ".c_inc", ".cpp")):
                    continue
                filepath = Path(dirpath) / fname
                file_rel = str(filepath.relative_to(DECOMP_ROOT))
                try:
                    content = filepath.read_text(errors="replace")
                except OSError:
                    continue
                if "VER_DELUXE" not in content:
                    continue
                all_blocks.extend(extract_deluxe_blocks(filepath, file_rel))
    return all_blocks


# ── Fuzzy matching engine ──────────────────────────────────────────────────

_id_re = re.compile(r'\b[A-Za-z_][A-Za-z0-9_]{2,}\b')
_comment_re1 = re.compile(r'//.*$')
_comment_re2 = re.compile(r'/\*.*?\*/')


def extract_identifiers(line: str) -> frozenset[str]:
    """Extract C identifiers (3+ chars) from a line."""
    line = _comment_re1.sub('', line)
    line = _comment_re2.sub('', line)
    return frozenset(_id_re.findall(line))


def line_similarity_fast(a_s: str, b_s: str,
                         a_ids: frozenset[str], b_ids: frozenset[str]) -> float:
    """Fast similarity score using precomputed stripped lines and identifier sets."""
    if a_s == b_s:
        return 1.0
    if not a_s and not b_s:
        return 1.0
    if not a_s or not b_s:
        return 0.0

    # Identifier Jaccard (very fast)
    if a_ids and b_ids:
        intersection = len(a_ids & b_ids)
        union = len(a_ids | b_ids)
        if union > 0:
            jaccard = intersection / union
            if jaccard >= LINE_MATCH_THRESHOLD:
                return jaccard
            # If identifiers are very different, skip expensive SequenceMatcher
            if jaccard < 0.15:
                return jaccard

    # Fall back to SequenceMatcher only for borderline cases
    return SequenceMatcher(None, a_s, b_s, autojunk=False).ratio()


def line_similarity(a: str, b: str) -> float:
    """Convenience wrapper (non-precomputed)."""
    return line_similarity_fast(a.strip(), b.strip(),
                                extract_identifiers(a), extract_identifiers(b))


# Noisy lines that don't carry enough identity to anchor on
_NOISY_LINES = frozenset((
    "{", "}", "};", ");", ")", "*/", "/*", "else {", "} else {",
    "return;", "break;", "default:", "return 0;", "return 1;",
    "return TRUE;", "return FALSE;", "return NULL;",
))


def is_anchor_worthy(line: str) -> bool:
    """True if the line carries enough identity to serve as an anchor."""
    s = line.strip()
    if not s or s in _NOISY_LINES:
        return False
    return bool(extract_identifiers(s))


@dataclass
class _PreparedTarget:
    """Precomputed data for a target file to speed up matching."""
    lines: list[str]            # raw lines
    stripped: list[str]         # stripped versions
    ids: list[frozenset[str]]   # identifier sets per line
    # Inverted index: identifier -> set of line indices that contain it
    id_index: dict[str, set[int]]

    @staticmethod
    def build(lines: list[str]) -> '_PreparedTarget':
        stripped = [l.strip() for l in lines]
        ids = [extract_identifiers(l) for l in lines]
        id_index: dict[str, set[int]] = {}
        for i, id_set in enumerate(ids):
            for ident in id_set:
                id_index.setdefault(ident, set()).add(i)
        return _PreparedTarget(lines=lines, stripped=stripped, ids=ids, id_index=id_index)


def _candidate_start_lines(anchor_lines: list[tuple[int, str]], prep: _PreparedTarget) -> list[int]:
    """
    Use the inverted index to find candidate start lines in target
    where the first anchor line might appear. Much faster than scanning every line.
    """
    if not anchor_lines:
        return list(range(len(prep.lines)))

    # Use the first anchor line's identifiers to narrow candidates
    first_ids = extract_identifiers(anchor_lines[0][1])
    if not first_ids:
        return list(range(len(prep.lines)))

    # Intersect line sets for each identifier in the first anchor
    candidates = None
    for ident in first_ids:
        lines_with_id = prep.id_index.get(ident, set())
        if candidates is None:
            candidates = set(lines_with_id)
        else:
            candidates &= lines_with_id
        if not candidates:
            # Relax: try union instead
            candidates = set()
            for ident2 in first_ids:
                candidates |= prep.id_index.get(ident2, set())
            break

    if not candidates:
        return list(range(len(prep.lines)))

    # Expand each candidate to include a small window before it
    expanded = set()
    for c in candidates:
        for offset in range(max(0, c - 5), c + 1):
            expanded.add(offset)

    return sorted(expanded)


def find_sequential_anchor(context: list[str], prep: _PreparedTarget,
                           direction: str) -> Optional[tuple[int, float, int]]:
    """
    Find the best sequential alignment of context lines within target.

    direction:
      "forward"  - context is BEFORE the block; returns insertion point AFTER matched span
      "backward" - context is AFTER the block; returns insertion point BEFORE matched span

    Returns (target_line_index, score, matched_count) or None.
    """
    if not context:
        return None

    anchor_lines = [(i, line) for i, line in enumerate(context) if is_anchor_worthy(line)]
    if not anchor_lines:
        return None

    # Precompute context identifiers
    ctx_stripped = [l.strip() for l in context]
    ctx_ids = [extract_identifiers(l) for l in context]

    # Get candidate start positions using inverted index
    candidates = _candidate_start_lines(anchor_lines, prep)

    best_pos = -1
    best_score = 0.0
    best_matched = 0

    for t_start in candidates:
        if t_start >= len(prep.lines):
            continue

        t_idx = t_start
        matched = 0
        total_sim = 0.0
        last_match_t = t_start

        for ctx_idx, ctx_line in anchor_lines:
            found = False
            c_stripped = ctx_stripped[ctx_idx]
            c_ids = ctx_ids[ctx_idx]
            lookahead = min(8, len(prep.lines) - t_idx)
            for offset in range(lookahead):
                tt = t_idx + offset
                if tt >= len(prep.lines):
                    break
                sim = line_similarity_fast(c_stripped, prep.stripped[tt],
                                           c_ids, prep.ids[tt])
                if sim >= LINE_MATCH_THRESHOLD:
                    matched += 1
                    total_sim += sim
                    t_idx = tt + 1
                    last_match_t = tt
                    found = True
                    break
            # If not found, skip this anchor line

        if matched < MIN_SEQ_MATCH or matched < len(anchor_lines) * 0.4:
            continue

        score = (matched / len(anchor_lines)) * 0.5 + (total_sim / max(1, matched)) * 0.3
        # Bonus for matching the last anchor line
        last_c = anchor_lines[-1]
        last_sim = line_similarity_fast(ctx_stripped[last_c[0]], prep.stripped[last_match_t],
                                        ctx_ids[last_c[0]], prep.ids[last_match_t])
        if last_sim >= LINE_MATCH_THRESHOLD:
            score += 0.2

        if score > best_score:
            best_score = score
            best_matched = matched
            if direction == "forward":
                # Scan past trailing non-anchor context lines
                last_anchor_ctx_idx = anchor_lines[-1][0]
                trail_pos = last_match_t + 1
                for trailing_idx in range(last_anchor_ctx_idx + 1, len(context)):
                    ctx_trail = ctx_stripped[trailing_idx]
                    if not ctx_trail:
                        if trail_pos < len(prep.lines) and not prep.stripped[trail_pos]:
                            trail_pos += 1
                        else:
                            trail_pos += 1
                    elif trail_pos < len(prep.lines):
                        if ctx_trail == prep.stripped[trail_pos]:
                            trail_pos += 1
                        elif line_similarity_fast(ctx_trail, prep.stripped[trail_pos],
                                                  ctx_ids[trailing_idx], prep.ids[trail_pos]) >= 0.5:
                            trail_pos += 1
                        else:
                            break
                    else:
                        break
                best_pos = trail_pos
            else:
                best_pos = t_start

    if best_pos >= 0:
        return (best_pos, best_score, best_matched)
    return None


def find_old_code_in_target(old_code: list[str], target: list[str]) -> Optional[tuple[int, int]]:
    """
    Find where old_code lines exist sequentially in target.
    Returns (start, end_exclusive) or None.
    """
    if not old_code:
        return None

    old_nonblank = [l for l in old_code if l.strip()]
    if not old_nonblank:
        return None

    # Precompute old code
    old_stripped = [l.strip() for l in old_code]
    old_ids = [extract_identifiers(l) for l in old_code]

    # Precompute target (only once per call, but these are typically small spans)
    tgt_stripped = [l.strip() for l in target]
    tgt_ids = [extract_identifiers(l) for l in target]

    for t_start in range(len(target)):
        t_idx = t_start
        old_idx = 0

        while old_idx < len(old_code) and t_idx < len(target):
            o_s = old_stripped[old_idx]
            t_s = tgt_stripped[t_idx]

            if not o_s and not t_s:
                old_idx += 1
                t_idx += 1
                continue
            if not o_s:
                old_idx += 1
                continue
            if not t_s:
                t_idx += 1
                continue

            sim = line_similarity_fast(o_s, t_s, old_ids[old_idx], tgt_ids[t_idx])
            if sim >= LINE_MATCH_THRESHOLD:
                old_idx += 1
                t_idx += 1
            else:
                break

        if old_idx >= len(old_code):
            return (t_start, t_idx)

    return None


# ── Already-present check ─────────────────────────────────────────────────

def block_already_present(block: DeluxeBlock, prep: _PreparedTarget) -> bool:
    """Check if this VER_DELUXE block is already in the target."""
    guard = block.block_lines[0].strip()

    for i, s in enumerate(prep.stripped):
        if s != guard:
            continue

        # Verify by checking subsequent lines
        check_n = min(5, len(block.block_lines))
        matches = 0
        for j in range(check_n):
            if i + j < len(prep.lines):
                bl_s = block.block_lines[j].strip()
                if bl_s == prep.stripped[i + j]:
                    matches += 1
                elif line_similarity_fast(bl_s, prep.stripped[i + j],
                                          extract_identifiers(block.block_lines[j]),
                                          prep.ids[i + j]) >= 0.75:
                    matches += 1

        if matches >= max(2, check_n * 0.6):
            return True

    return False


# ── Core merge logic for one block ────────────────────────────────────────

def merge_block(block: DeluxeBlock, target: list[str], prep: _PreparedTarget,
                claimed_lines: set[int], verbose: bool = False) -> MergeResult:
    """Attempt to merge a single VER_DELUXE block into the target."""
    if block_already_present(block, prep):
        return MergeResult(block=block, status="already_present")

    # Strategy 1: #else blocks - find old code that should be wrapped
    if block.has_else and block.old_code_lines:
        old_pos = find_old_code_in_target(block.old_code_lines, target)
        # Keep searching if the first match overlaps with already-claimed lines
        while old_pos is not None:
            start, end = old_pos
            if not any(i in claimed_lines for i in range(start, end)):
                break
            # Search again starting after the claimed region
            remainder = target[end:]
            next_pos = find_old_code_in_target(block.old_code_lines, remainder)
            if next_pos is not None:
                old_pos = (next_pos[0] + end, next_pos[1] + end)
            else:
                old_pos = None

        if old_pos is not None:
            start, end = old_pos
            # Claim these lines
            for i in range(start, end):
                claimed_lines.add(i)
            return MergeResult(
                block=block,
                status="inserted",
                detail=f"wrap existing code lines {start + 1}-{end} with guard",
                insert_line=start,
                action="replace",
                replace_end=end,
            )

    # Strategy 2: Sequential anchor matching using before context
    anchor_before = find_sequential_anchor(block.context_before, prep, "forward")

    # Strategy 3: Sequential anchor matching using after context
    anchor_after = find_sequential_anchor(block.context_after, prep, "backward")

    if verbose:
        print(f"  Context matching for {block.description}:")
        if anchor_before:
            pos, score, cnt = anchor_before
            print(f"    Before anchor: line {pos}, score={score:.3f}, matched={cnt}")
        else:
            print(f"    Before anchor: NONE")
        if anchor_after:
            pos, score, cnt = anchor_after
            print(f"    After anchor: line {pos}, score={score:.3f}, matched={cnt}")
        else:
            print(f"    After anchor: NONE")

    insert_pos = None
    detail = ""

    if anchor_before and anchor_after:
        b_pos, b_score, b_cnt = anchor_before
        a_pos, a_score, a_cnt = anchor_after

        # Cross-validate: both anchors should agree within a few lines
        if abs(b_pos - a_pos) <= 5:
            insert_pos = b_pos
            detail = f"dual-anchor (before={b_score:.2f}/{b_cnt}m, after={a_score:.2f}/{a_cnt}m)"
        else:
            # Anchors disagree - use positional plausibility
            # The expected position is proportional to the decomp file position
            # Use the after anchor if before anchor seems wrong (too far from expected)
            decomp_ratio = block.start_line / max(1, block.start_line + len(target))
            expected_pos = int(decomp_ratio * len(target))
            b_dist = abs(b_pos - expected_pos)
            a_dist = abs(a_pos - expected_pos)

            # Strongly prefer the closer anchor if the other is far off
            if b_dist < a_dist * 0.5:
                insert_pos = b_pos
                detail = f"before-anchor (score={b_score:.2f}, {b_cnt} matched)"
            elif a_dist < b_dist * 0.5:
                insert_pos = a_pos
                detail = f"after-anchor (score={a_score:.2f}, {a_cnt} matched)"
            elif b_score > a_score + 0.1:
                insert_pos = b_pos
                detail = f"before-anchor (score={b_score:.2f}, {b_cnt} matched)"
            elif a_score > b_score + 0.1:
                insert_pos = a_pos
                detail = f"after-anchor (score={a_score:.2f}, {a_cnt} matched)"
            else:
                # Tie-break: prefer the one closer to expected position
                insert_pos = b_pos if b_dist <= a_dist else a_pos
                winner = "before" if b_dist <= a_dist else "after"
                detail = f"{winner}-anchor by proximity (b={b_pos} a={a_pos} exp~{expected_pos})"
    elif anchor_before:
        b_pos, b_score, b_cnt = anchor_before
        insert_pos = b_pos
        detail = f"before-anchor (score={b_score:.2f}, {b_cnt} matched)"
    elif anchor_after:
        a_pos, a_score, a_cnt = anchor_after
        insert_pos = a_pos
        detail = f"after-anchor (score={a_score:.2f}, {a_cnt} matched)"

    if insert_pos is not None:
        claimed_lines.add(insert_pos)
        return MergeResult(
            block=block,
            status="inserted",
            detail=detail,
            insert_line=insert_pos,
        )

    return MergeResult(
        block=block,
        status="skipped_no_match",
        detail="no matching context found in target file",
    )


# ── File-level application ─────────────────────────────────────────────────

def apply_merges(target_lines: list[str], results: list[MergeResult]) -> list[str]:
    """Apply all successful merges, returning new file content."""
    ops = []

    for r in results:
        if r.status != "inserted":
            continue
        block = r.block

        if r.action == "replace" and block.has_else and block.old_code_lines:
            # Re-find old code position (in case ordering matters)
            stripped = [l.rstrip("\n") for l in target_lines]
            old_pos = find_old_code_in_target(block.old_code_lines, stripped)
            if old_pos:
                ops.append(("replace", old_pos[0], old_pos[1], block.block_lines))
                continue

        ops.append(("insert", r.insert_line, r.insert_line, block.block_lines))

    # Apply in reverse order to preserve line numbers
    ops.sort(key=lambda x: x[1], reverse=True)

    new_lines = list(target_lines)

    for action, start, end, block_lines in ops:
        text = [l + "\n" for l in block_lines]

        # Clamp to valid range
        start = max(0, min(start, len(new_lines)))
        end = max(start, min(end, len(new_lines)))

        if action == "replace":
            new_lines[start:end] = text
        else:
            # Insert - add spacing if needed
            prefix = []
            suffix = []

            if start > 0 and start - 1 < len(new_lines) and new_lines[start - 1].strip():
                if block_lines[0].strip():
                    prefix = ["\n"]

            if start < len(new_lines) and new_lines[start].strip():
                if block_lines[-1].strip():
                    suffix = ["\n"]

            new_lines[start:start] = prefix + text + suffix

    return new_lines


# ── Main ───────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(
        description="Merge VER_DELUXE blocks from ac-decomp to ac-pc-port")
    parser.add_argument("--apply", action="store_true",
                        help="Actually modify files (default: dry-run)")
    parser.add_argument("--verbose", "-v", action="store_true",
                        help="Show detailed matching info")
    parser.add_argument("--file", "-f", type=str,
                        help="Only process a specific relative path (e.g. src/game/m_lib.c)")
    args = parser.parse_args()

    mode = "APPLY" if args.apply else "DRY-RUN"
    print("=" * 72)
    print("VER_DELUXE Merge Tool")
    print(f"  Source: {DECOMP_ROOT}")
    print(f"  Target: {PCPORT_ROOT}")
    print(f"  Mode:   {mode}")
    print("=" * 72)
    print()

    # 1. Extract all blocks
    print("Scanning decomp for VER_DELUXE blocks...")
    all_blocks = find_all_deluxe_blocks()
    files_with_blocks = len(set(b.file_rel for b in all_blocks))
    print(f"  Found {len(all_blocks)} blocks across {files_with_blocks} files")
    print()

    # Group by file
    by_file: dict[str, list[DeluxeBlock]] = {}
    for b in all_blocks:
        if args.file and b.file_rel != args.file:
            continue
        by_file.setdefault(b.file_rel, []).append(b)

    # 2. Process files
    stats = dict(files=0, modified=0, no_target=0,
                 inserted=0, present=0, no_match=0)
    no_match_list: list[MergeResult] = []

    for file_rel in sorted(by_file):
        blocks = by_file[file_rel]
        target_path = PCPORT_ROOT / file_rel

        if not target_path.exists():
            for b in blocks:
                no_match_list.append(MergeResult(
                    block=b, status="skipped_no_file",
                    detail=f"no file: {target_path}"))
            stats["no_target"] += 1
            continue

        stats["files"] += 1

        with open(target_path, "r", errors="replace") as f:
            target_raw = f.readlines()
        target_stripped = [l.rstrip("\n") for l in target_raw]
        prep = _PreparedTarget.build(target_stripped)
        claimed_lines: set[int] = set()

        file_results = []
        for block in blocks:
            r = merge_block(block, target_stripped, prep, claimed_lines,
                            verbose=args.verbose)
            file_results.append(r)

            if r.status == "inserted":
                stats["inserted"] += 1
            elif r.status == "already_present":
                stats["present"] += 1
            elif r.status == "skipped_no_match":
                stats["no_match"] += 1
                no_match_list.append(r)

        ins = [r for r in file_results if r.status == "inserted"]
        skip = [r for r in file_results if r.status == "skipped_no_match"]
        pres = [r for r in file_results if r.status == "already_present"]

        if ins:
            stats["modified"] += 1
            summary_parts = []
            if ins:
                summary_parts.append(f"{len(ins)} inserted")
            if pres:
                summary_parts.append(f"{len(pres)} present")
            if skip:
                summary_parts.append(f"{len(skip)} skipped")
            print(f"  {file_rel}: {', '.join(summary_parts)}")
            for r in ins:
                act = "wrap" if r.action == "replace" else "insert@"
                line_info = f"line {r.insert_line + 1}"
                print(f"    + {act} {line_info}: {r.detail}")
            for r in skip:
                print(f"    ! SKIP: {r.block.description}")

            if args.apply:
                new_content = apply_merges(target_raw, file_results)
                with open(target_path, "w") as f:
                    f.writelines(new_content)
                print(f"    -> WRITTEN")
        elif skip:
            print(f"  {file_rel}: {len(pres)} present, {len(skip)} skipped")
            for r in skip:
                print(f"    ! SKIP: {r.block.description}")
        elif args.verbose and pres:
            print(f"  {file_rel}: all {len(pres)} blocks already present")

    # 3. Summary
    print()
    print("=" * 72)
    print("SUMMARY")
    print("=" * 72)
    print(f"  Files processed:         {stats['files']}")
    print(f"  Files modified:          {stats['modified']}")
    print(f"  Files missing in target: {stats['no_target']}")
    print(f"  Blocks inserted:         {stats['inserted']}")
    print(f"  Blocks already present:  {stats['present']}")
    print(f"  Blocks unmatched:        {stats['no_match']}")
    print()

    if no_match_list:
        print("UNMATCHED BLOCKS (manual review needed):")
        print("-" * 72)
        for r in no_match_list:
            b = r.block
            print(f"  {b.description}")
            if b.context_before:
                anchor_lines = [l for l in b.context_before if is_anchor_worthy(l)]
                if anchor_lines:
                    print(f"    Context: {anchor_lines[-1].strip()[:100]}")
            if r.detail:
                print(f"    Reason: {r.detail}")
            print()

    if not args.apply and stats["inserted"] > 0:
        print(f"Run with --apply to write {stats['inserted']} blocks "
              f"to {stats['modified']} files.")

    return 0 if stats["no_match"] == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
