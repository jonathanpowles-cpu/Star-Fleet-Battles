"""
Geometry-aware extraction of multi-column tables from the SFB rulebook PDFs.

WHY THIS EXISTS
---------------
`pdftotext -layout` flattens a two-column page by interleaving the columns into
single lines. For prose that is merely ugly; for a TABLE it is destructive - the
row labels end up beside a different row's numbers. That failure has already
caused two real defects in this project:

  * the D3.32 shield-cost chart, where FULL/TOTAL columns drifted up one row and
    the engine over-booked every cruiser by 2 energy a turn until the values
    were measured in the client;
  * Annex #7G carrier deck crews, where the hull labels are torn away from their
    data entirely, so the numbers cannot be attributed at all.

Reading these by eye does not scale - there are dozens of annex tables - and
every manual reading is another chance to repeat the shield-cost mistake.

APPROACH
--------
pdfplumber reports every word with a bounding box. So:
  1. find the GUTTER: the x with the fewest words across the page body, i.e.
     the vertical whitespace channel between the two columns;
  2. split words into column blocks at that gutter;
  3. cluster each block's words into ROWS by y, with a tolerance of roughly one
     line height;
  4. within a row, bucket words into FIELDS by their x centre, using column
     anchors derived from the whole block rather than from any one row (a short
     row must not redefine the layout).

The result is real cells with real coordinates, which can be checked against the
page image if anything looks wrong.
"""
from __future__ import annotations

import collections
import statistics


def body_words(page, top_frac=0.06, bottom_frac=0.94):
    """Words in the page BODY, excluding the running head and footer.

    Those span the full page width ("152 - CAPTAIN'S MODULE G3: ANNEXES -
    Revised 4/13/09"), so they occupy every x and make the gutter look occupied
    on every page - which defeated cross-page gutter detection entirely.
    """
    words = page.extract_words(use_text_flow=False, keep_blank_chars=False)
    lo, hi = page.height * top_frac, page.height * bottom_frac
    return [w for w in words if lo <= (w["top"] + w["bottom"]) / 2 <= hi]


def _pages(pdf_path, pages):
    import pdfplumber
    with pdfplumber.open(pdf_path) as pdf:
        for i in pages:
            yield i, pdf.pages[i - 1]


def find_gutter(words, page_width, lo_frac=0.35, hi_frac=0.65, bins=40):
    """x of the emptiest vertical channel in the middle of the page.

    Searched only across the middle third: page margins are always empty and
    would otherwise win.
    """
    if not words:
        return page_width / 2
    # Taking the single emptiest x is not enough: a table's own inter-field gaps
    # are empty too, and on some pages one of those is emptier than the real page
    # gutter. That chose 265 on one annex page against 311 on its neighbour and
    # bled right-column words into the left block. The page gutter is
    # distinguished by being WIDE - a continuous empty band - so measure runs of
    # empty columns and take the middle of the longest.
    lo, hi = page_width * lo_frac, page_width * hi_frac
    step = 2.0
    xs, empty = [], []
    x = lo
    while x <= hi:
        xs.append(x)
        empty.append(not any(w["x0"] < x < w["x1"] for w in words))
        x += step
    best_len, best_mid, run_start = 0, (lo + hi) / 2, None
    for i, e in enumerate(empty + [False]):
        if e and run_start is None:
            run_start = i
        elif not e and run_start is not None:
            if i - run_start > best_len:
                best_len = i - run_start
                best_mid = (xs[run_start] + xs[i - 1]) / 2
            run_start = None
    return best_mid


def find_gutter_across(pdf_path, pages, lo_frac=0.30, hi_frac=0.70, step=2.0):
    """The gutter shared by EVERY page of a multi-page table.

    Per-page inference is unreliable on these annexes: a table's own field gaps
    can be wider than the page gutter on any given page, which produced gutters
    of 310, 344 and 272 on three consecutive pages of the same table. But the
    real gutter is empty on ALL of them, whereas a field gap that happens to be
    empty on one page is occupied on another. So intersect the empty channels
    across the whole run and take the middle of the widest surviving band.
    """
    import pdfplumber
    grid, common = None, None
    with pdfplumber.open(pdf_path) as pdf:
        for n in pages:
            pg = pdf.pages[n - 1]
            words = body_words(pg)
            lo, hi = pg.width * lo_frac, pg.width * hi_frac
            xs, empty = [], []
            x = lo
            while x <= hi:
                xs.append(x)
                empty.append(not any(w["x0"] < x < w["x1"] for w in words))
                x += step
            grid = xs
            common = empty if common is None else [a and b for a, b in zip(common, empty)]
    if not grid or not common or not any(common):
        return None
    best_len, best_mid, run_start = 0, None, None
    for i, e in enumerate(list(common) + [False]):
        if e and run_start is None:
            run_start = i
        elif not e and run_start is not None:
            if i - run_start > best_len:
                best_len = i - run_start
                best_mid = (grid[run_start] + grid[i - 1]) / 2
            run_start = None
    return best_mid


def rows_from_words(words, y_tol=None):
    """Cluster words into rows by their vertical centre."""
    if not words:
        return []
    heights = [w["bottom"] - w["top"] for w in words]
    tol = y_tol if y_tol is not None else max(2.0, statistics.median(heights) * 0.6)
    ws = sorted(words, key=lambda w: ((w["top"] + w["bottom"]) / 2, w["x0"]))
    rows, cur, cur_y = [], [], None
    for w in ws:
        yc = (w["top"] + w["bottom"]) / 2
        if cur_y is None or abs(yc - cur_y) <= tol:
            cur.append(w)
            cur_y = yc if cur_y is None else (cur_y + yc) / 2
        else:
            rows.append(sorted(cur, key=lambda z: z["x0"]))
            cur, cur_y = [w], yc
    if cur:
        rows.append(sorted(cur, key=lambda z: z["x0"]))
    return rows


def column_anchors(words, min_share=0.04, merge_within=12.0):
    """x positions where fields start, derived from the WHOLE block.

    Deriving anchors per row would let a short row redefine the layout; taking
    them from every word in the block means a row with blank cells still lands
    its values in the right fields.
    """
    if not words:
        return []
    hist = collections.Counter(round(w["x0"] / 4) * 4 for w in words)
    floor = max(1, int(len(words) * min_share))
    peaks = sorted(x for x, n in hist.items() if n >= floor)
    merged = []
    for x in peaks:
        if merged and x - merged[-1] <= merge_within:
            continue
        merged.append(x)
    return merged


def split_row_by_gaps(row, gap=None):
    """Split one row's words into cells wherever the horizontal gap is large.

    This replaces page-wide gutter detection, which cannot work on these annex
    pages: the two table columns' x-ranges OVERLAP, so no single x is empty down
    the whole page. Within a ROW, though, the gaps are unambiguous - the space
    between fields is several times the space between words inside a field.

    `gap` defaults to a multiple of the row's own median inter-word gap, so it
    adapts to font size rather than being a magic number.
    """
    if not row:
        return []
    ws = sorted(row, key=lambda w: w["x0"])
    gaps = [b["x0"] - a["x1"] for a, b in zip(ws, ws[1:])]
    if gap is None:
        pos = [g for g in gaps if g > 0]
        gap = max(statistics.median(pos) * 2.5 if pos else 6.0, 5.0)
    cells, cur = [], [ws[0]]
    for prev, w in zip(ws, ws[1:]):
        if w["x0"] - prev["x1"] >= gap:
            cells.append(cur)
            cur = [w]
        else:
            cur.append(w)
    cells.append(cur)
    return cells


def extract_rows_by_gaps(pdf_path, page_no, y_tol=None, gap=None):
    """[(xs, texts)] - every body row on the page, cells split by gap."""
    out = []
    for _i, pg in _pages(pdf_path, [page_no]):
        for row in rows_from_words(body_words(pg), y_tol):
            cells = split_row_by_gaps(row, gap)
            texts = [" ".join(w["text"] for w in c).strip() for c in cells]
            xs = [c[0]["x0"] for c in cells]
            if any(texts):
                out.append((xs, texts))
    return out


def row_to_cells(row, anchors):
    """Bucket a row's words into fields by nearest anchor at or left of x0."""
    if not anchors:
        return [" ".join(w["text"] for w in row)]
    cells = [[] for _ in anchors]
    for w in row:
        idx = 0
        for i, a in enumerate(anchors):
            if w["x0"] >= a - 3:
                idx = i
        cells[idx].append(w["text"])
    return [" ".join(c).strip() for c in cells]


def extract_table(pdf_path, page_no, side="both", y_tol=None):
    """[(side, [cells...])] for one page, columns separated geometrically."""
    out = []
    for _i, pg in _pages(pdf_path, [page_no]):
        words = pg.extract_words(use_text_flow=False, keep_blank_chars=False)
        gut = find_gutter(words, pg.width)
        blocks = {"left": [w for w in words if w["x1"] <= gut],
                  "right": [w for w in words if w["x0"] >= gut]}
        for name, block in blocks.items():
            if side not in ("both", name):
                continue
            anchors = column_anchors(block)
            for row in rows_from_words(block, y_tol):
                cells = row_to_cells(row, anchors)
                if any(c for c in cells):
                    out.append((name, cells))
    return out


def dump(pdf_path, page_no, side="both", width=18):
    for name, cells in extract_table(pdf_path, page_no, side):
        print(f"[{name:5}] " + " | ".join(c[:width].ljust(width) for c in cells))
