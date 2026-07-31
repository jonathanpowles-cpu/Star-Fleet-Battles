"""
Campaign reset planner: deploy Lyran ships to their county home provinces
(Lyran MSSB hull-number -> county hex), redeploy Kzinti by section-700 fleet
zones, and reconcile the board against the manual order of battle to find losses.
Dry-run analysis only; no writes.
"""
import importlib.util, sys, re
from collections import defaultdict, Counter


def load(n):
    s = importlib.util.spec_from_file_location(n, n + ".py")
    m = importlib.util.module_from_spec(s); sys.modules[n] = m; s.loader.exec_module(m)
    return m


for _n in ["sfb_rules", "sfb_hex", "sfb_ssd", "sfb_events", "sfb_client"]:
    load(_n)
import sfb_client as client
import sfb_events


def load_roster(logpath="sfb_relay_run.log"):
    gs = client.GameState()
    c = client.SFBGameClient.__new__(client.SFBGameClient)
    c.state = gs; c.nick = "x"; c.room = "#SFBCampaign_Game1"
    c.combat_log = []; c.events = sfb_events.EventReassembler(on_event=c._on_event)
    for ln in open(logpath, encoding="utf-8", errors="replace").read().splitlines():
        m = re.search(r"<- \w+: ((?:setattr|addobj) #SFBCampaign_Game1 .*)", ln)
        if m:
            c._on_line(":S!S@local " + m.group(1))
    return [p for p in gs.snapshot()
            if p.obj_id.startswith("gp*") and p.attrs.get("Name", "").startswith(("Lyran", "Kzinti"))]


def hull_num(lbl):
    m = re.match(r"^\s*(\d+)", lbl or "")
    return int(m.group(1)) if m else None


def cls(name):                       # 'Lyran - CA+' -> 'CA'
    h = name.split("-", 1)[-1].strip()
    h = re.sub(r"[+pP]+$", "", h)
    return {"BATS": "MB", "MB2": "MB", "DWM": "DW"}.get(h, h)


# Lyran hull-number range -> county home hex  (MSSB pp.4/6)
LYR = [(100, 199, "0408", "Homeworld/Royal"), (200, 219, "0707", "Foremost"),
       (220, 239, "0306", "Apex"), (240, 259, "0609", "Silver Moon"),
       (260, 279, "0107", "Black Claw"), (280, 299, "0105", "Night Roar"),
       (300, 319, "0310", "Enemy's Blood"), (320, 339, "0109", "White Stripe"),
       (340, 359, "0312", "Hidden Dagger"), (360, 379, "0111", "Bloody Claw"),
       (540, 559, "0103", "Blood Star"), (700, 719, "0404", "Red Claw"),
       (720, 739, "0101", "Black Stripe"), (740, 759, "0301", "Pelt Hunter"),
       (760, 779, "0402", "Golden Fang"), (780, 799, "0604", "Predator")]


def lyr_deploy(num):
    for lo, hi, hx, nm in LYR:
        if num and lo <= num <= hi:
            return hx, nm
    return None, "off-map (Far Stars)"


def kzin_deploy(xy):                 # section-700 fleet zones
    col = xy[0] if xy else 0
    if col >= 14:
        return "1401", "Home Fleet"
    if col >= 10:
        return "1205", "Marquis' Fleet"
    return "0901", "Count's Fleet (W of 09xx)"


# section-700 orders of battle (Kzinti-front fleets only)
LYR_OOB = Counter({"DN": 1, "BC": 1, "CC": 2, "CA": 8, "CW": 10, "CL": 10,
                   "DD": 10, "DW": 3, "FF": 10, "SC": 2, "TGC": 1, "TGP": 1,
                   "FRD": 1, "MB": 2})
KZN_OOB = Counter({"DN": 1, "CV": 3, "CVL": 3, "CVE": 3, "CLE": 6, "EFF": 9,
                   "CC": 3, "BC": 9, "CL": 3, "DD": 3, "FF": 5, "DF": 3,
                   "SF": 3, "TGC": 2, "TGT": 1, "FRD": 1, "MB": 1})


def recon(title, oob, board):
    print("\n" + "=" * 68)
    print(f"RECONCILIATION - {title}: manual OOB vs board")
    print("=" * 68)
    print(f"  {'class':6} {'manual':>6} {'board':>6} {'diff':>6}")
    miss = extra = 0
    for k in sorted(set(oob) | set(board)):
        d = board[k] - oob[k]
        flag = "  <-- losses" if d < 0 else ("  (reinforced/extra)" if d > 0 else "")
        if oob[k] or board[k]:
            print(f"  {k:6} {oob[k]:>6} {board[k]:>6} {d:>+6}{flag}")
        miss += max(0, -d); extra += max(0, d)
    print(f"  {'TOTAL':6} {sum(oob.values()):>6} {sum(board.values()):>6} "
          f"{sum(board.values()) - sum(oob.values()):>+6}")
    print(f"  => {miss} short of OOB (likely destroyed), {extra} extra")


def main():
    units = load_roster()
    lyr = [u for u in units if u.attrs.get("Name", "").startswith("Lyran")]
    kzn = [u for u in units if u.attrs.get("Name", "").startswith("Kzinti")]

    print("=" * 68); print("LYRAN DEPLOYMENT (hull number -> county province)"); print("=" * 68)
    byhex = defaultdict(list)
    for u in lyr:
        n = hull_num(u.attrs.get("Label", "")); hx, nm = lyr_deploy(n)
        byhex[(hx, nm)].append((n, cls(u.attrs.get("Name", ""))))
    for (hx, nm), grp in sorted(byhex.items(), key=lambda k: str(k[0][0])):
        print(f"  {hx or '----'} {nm:16} ({len(grp)}): "
              + ", ".join(f"{g[0]}{g[1]}" for g in sorted(grp, key=lambda x: x[0] or 0)))

    print("\n" + "=" * 68); print("KZINTI DEPLOYMENT (section-700 fleet zones)"); print("=" * 68)
    kbyhex = defaultdict(list)
    for u in kzn:
        hx, nm = kzin_deploy(u.xy); kbyhex[(hx, nm)].append(cls(u.attrs.get("Name", "")))
    for (hx, nm), grp in sorted(kbyhex.items()):
        print(f"  {hx} {nm:24} ({len(grp)}): "
              + ", ".join(f"{k}x{v}" for k, v in Counter(grp).most_common()))

    recon("LYRAN", LYR_OOB, Counter(cls(u.attrs.get("Name", "")) for u in lyr))
    recon("KZINTI", KZN_OOB, Counter(cls(u.attrs.get("Name", "")) for u in kzn))


if __name__ == "__main__":
    main()
