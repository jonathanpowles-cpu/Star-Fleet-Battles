import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.lang.reflect.*;

/**
 * Campaign -> tactical export.
 *   java ExportBattle <campaign> <KzTemplates> <LyTemplates> <out.SFB> <hexA> [hexB ...]
 * Ships at the given campaign hexes are cloned from the class templates (authentic SSDs),
 * relabelled with the campaign Label (the join key for re-import), healed or damaged per the
 * campaign's durable damage record, and deployed as two facing battle lines.
 */
public class ExportBattle {
  // ---------- reflection helpers ----------
  static Object fld(Object o, String n) throws Exception {
    if (o == null) return null;
    for (Class<?> c = o.getClass(); c != null; c = c.getSuperclass())
      try { Field f = c.getDeclaredField(n); f.setAccessible(true); return f.get(o); } catch (NoSuchFieldException e) {}
    return null;
  }
  static void setfld(Object o, String n, Object v) throws Exception {
    for (Class<?> c = o.getClass(); c != null; c = c.getSuperclass())
      try { Field f = c.getDeclaredField(n); f.setAccessible(true); f.set(o, v); return; } catch (NoSuchFieldException e) {}
  }
  @SuppressWarnings("unchecked")
  static Hashtable<String,Object> A(Object el) throws Exception { return (Hashtable<String,Object>) fld(el, "attributes"); }
  static String S(Object o) { return o == null ? "" : String.valueOf(o); }
  static String cls(String ut) { return ut == null ? "" : ut.replaceAll("[+pP]+$", ""); }
  static Object deepCopy(Object o) throws Exception {
    ByteArrayOutputStream b = new ByteArrayOutputStream();
    try (ObjectOutputStream out = new ObjectOutputStream(b)) { out.writeObject(o); }
    try (ObjectInputStream in = new ObjectInputStream(new ByteArrayInputStream(b.toByteArray()))) { return in.readObject(); }
  }
  static Object load(String p) throws Exception {
    byte[] raw = Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(p))).trim());
    try (ObjectInputStream in = new ObjectInputStream(new ByteArrayInputStream(raw))) { return in.readObject(); }
  }
  static Object mkLoc(int x, int y) throws Exception {
    Class<?> c = Class.forName("pd.pfranz.map.BoardLocation");
    try { return c.getConstructor(int.class, int.class).newInstance(x, y); }
    catch (NoSuchMethodException e) { Object o = c.getDeclaredConstructor().newInstance(); setfld(o,"x",x); setfld(o,"y",y); return o; }
  }
  static String hexOf(Object p) throws Exception {
    Object bl = A(p).get("boardLocation");
    if (bl == null) return "-";
    Object x = fld(bl,"x"), y = fld(bl,"y");
    if (x == null || y == null) return "-";
    return String.format("%02d%02d", ((Number)x).intValue(), ((Number)y).intValue());
  }

  // ---------- damage ----------
  /** Set every SSD box to intact (1), restore numOfBoxes, clear armed/used state. */
  static void healFully(Object ship) throws Exception {
    Object ssd = A(ship).get("SSD");
    if (ssd != null) {
      Object boxes = fld(ssd, "boxes");
      for (int i = 0; i < Array.getLength(boxes); i++) {
        Object bx = Array.get(boxes, i);
        Object bs = fld(bx, "boxStatus");
        if (bs instanceof int[]) { int[] arr = (int[]) bs; Arrays.fill(arr, 1); }
        Object mx = fld(bx, "maxNumOfBoxes");
        if (mx != null) setfld(bx, "numOfBoxes", mx);   // damage count reads numOfBoxes
      }
    }
    Object sbs = A(ship).get("system_box_status");
    if (sbs != null && sbs.getClass().isArray())
      for (int i = 0; i < Array.getLength(sbs); i++) {
        Object row = Array.get(sbs, i);
        if (row == null) continue;
        setfld(row, "status", (short) 0);
        setfld(row, "usedThisTurn", (short) 0);
        setfld(row, "lastUsedTurn", (short) 0);
        setfld(row, "lastUsedImpulse", (short) 0);
        setfld(row, "prevUsedTurn", (short) 0);
        setfld(row, "prevUsedImpulse", (short) 0);
      }
  }
  /** durable record: "g0:1,1,2,1|g1:1,1,..." (group index -> per-box status) */
  static void applyDamage(Object ship, String rec) throws Exception {
    if (rec == null || rec.isBlank()) return;
    Object ssd = A(ship).get("SSD");
    if (ssd == null) return;
    Object boxes = fld(ssd, "boxes");
    for (String grp : rec.split("\\|")) {
      int c = grp.indexOf(':');
      if (c < 0) continue;
      int gi = Integer.parseInt(grp.substring(1, c));
      if (gi >= Array.getLength(boxes)) continue;
      Object box = Array.get(boxes, gi);
      Object bs = fld(box, "boxStatus");
      if (!(bs instanceof int[])) continue;
      int[] arr = (int[]) bs;
      String[] vals = grp.substring(c + 1).split(",");
      for (int i = 0; i < Math.min(arr.length, vals.length); i++) arr[i] = Integer.parseInt(vals[i].trim());
      int intact = 0; for (int v : arr) if (v != 2) intact++;
      setfld(box, "numOfBoxes", Integer.valueOf(intact));   // keep count consistent with boxStatus
    }
  }
  static int[] boxTally(Object ship) throws Exception {   // {total, destroyed}
    Object ssd = A(ship).get("SSD");
    if (ssd == null) return new int[]{0,0};
    Object boxes = fld(ssd, "boxes");
    int t = 0, d = 0;
    for (int i = 0; i < Array.getLength(boxes); i++) {
      Object bs = fld(Array.get(boxes, i), "boxStatus");
      if (bs instanceof int[]) for (int v : (int[]) bs) { t++; if (v == 2) d++; }
    }
    return new int[]{t, d};
  }

  @SuppressWarnings("unchecked")
  public static void main(String[] args) throws Exception {
    String campaignPath = args[0], kzPath = args[1], lyPath = args[2], outPath = args[3];
    Set<String> hexes = new LinkedHashSet<>(Arrays.asList(args).subList(4, args.length));

    Object camp = load(campaignPath);
    Vector<Object> campOn = (Vector<Object>) fld(camp, "onBoardPieces");

    // ---- template index: race|class -> tactical ship ----
    Map<String,Object> tmpl = new LinkedHashMap<>();
    for (String tp : new String[]{kzPath, lyPath}) {
      Object tb = load(tp);
      for (String v : new String[]{"onBoardPieces","offBoardPieces","discardedPieces"}) {
        Vector<?> vec = (Vector<?>) fld(tb, v);
        if (vec == null) continue;
        for (Object p : vec) {
          if (!p.getClass().getName().contains("ShipAttributes")) continue;
          Hashtable<String,Object> h = A(p);
          String race = S(h.get("race"));
          tmpl.putIfAbsent(race + "|" + cls(S(h.get("Label"))), p);
          tmpl.putIfAbsent(race + "|" + cls(S(h.get("ship_type"))), p);
        }
      }
    }
    System.out.println("templates indexed: " + tmpl.size() + " keys");

    // ---- gather campaign combatants ----
    List<Object> combatants = new ArrayList<>();
    for (Object p : campOn) {
      Hashtable<String,Object> h = A(p);
      if ("Fleet".equals(S(h.get("unit_type")))) continue;            // stack markers
      if ("true".equals(S(h.get("IsStackPiece")))) continue;
      if (!hexes.contains(hexOf(p))) continue;
      combatants.add(p);
    }
    Map<String,List<Object>> sides = new LinkedHashMap<>();
    for (Object p : combatants) sides.computeIfAbsent(S(A(p).get("race")), k -> new ArrayList<>()).add(p);
    System.out.println("combatants at " + hexes + ":");
    for (Map.Entry<String,List<Object>> e : sides.entrySet())
      System.out.println("   " + e.getKey() + " x" + e.getValue().size());
    if (sides.size() < 2) System.out.println("   (warning: only one side present - exporting anyway)");

    // ---- build the tactical board from a template ----
    Object board = load(kzPath);
    Vector<Object> on = (Vector<Object>) fld(board, "onBoardPieces");
    Vector<Object> off = (Vector<Object>) fld(board, "offBoardPieces");
    Vector<Object> disc = (Vector<Object>) fld(board, "discardedPieces");
    on.clear(); off.clear(); disc.clear();
    setfld(board, "currentTurn", 0);
    setfld(board, "currentImpulse", 0);
    setfld(board, "firstImpulse", Boolean.TRUE);
    Map<String,Object> shared = (Map<String,Object>) fld(board, "sharedAttributes");
    int BW = 60, BH = 40;
    if (shared != null) {
      shared.put("board_size", mkLoc(BW, BH));
      // current_turn_impulse is a TurnImpulse OBJECT (its toString just looks like a
      // double) — writing a Double crashes loadGame with a ClassCastException.
      // Keep the template's object and reset its fields to turn 1 / impulse 1.
      // Client displays stored TurnImpulse + 1 (0-indexed internally), so store 0/0
      // to show "Turn 1, Impulse 1" at battle start.
      Object ti = shared.get("current_turn_impulse");
      if (ti != null && ti.getClass().getName().endsWith("TurnImpulse")) {
        for (Field f : ti.getClass().getDeclaredFields()) {
          if (Modifier.isStatic(f.getModifiers())) continue;
          f.setAccessible(true);
          Object v = f.get(ti);
          if (v instanceof Integer) f.set(ti, Integer.valueOf(0));
          else if (v instanceof Short) f.set(ti, Short.valueOf((short) 0));
        }
      }
      shared.put("first_impulse", Boolean.TRUE);
    }
    Object log = fld(board, "log");
    if (log != null) { Vector<Object> ent = (Vector<Object>) fld(log, "entries"); if (ent != null) ent.clear(); }

    // ---- clone each combatant ----
    // Pieces must carry the SAME boardID as the board they live on, or the client
    // treats them as belonging to another board and drops them.
    String boardId = S(fld(board, "boardId"));
    System.out.println("board boardId = " + boardId);
    long nid = 1784400000000L;   // realistic owner:timestamp-style ids
    int made = 0; List<String> skipped = new ArrayList<>();
    String[] raceOrder = sides.keySet().toArray(new String[0]);
    for (int si = 0; si < raceOrder.length; si++) {
      String race = raceOrder[si];
      List<Object> fleet = sides.get(race);
      int rowY = (si == 0) ? 8 : BH - 8;              // opposing battle lines
      int facing = (si == 0) ? 3 : 0;                 // 0 = north, 3 = south -> face each other
      int startX = Math.max(2, (BW - fleet.size() * 2) / 2);
      for (int i = 0; i < fleet.size(); i++) {
        Object cs = fleet.get(i);
        Hashtable<String,Object> ch = A(cs);
        String label = S(ch.get("Label")), ut = S(ch.get("unit_type"));
        Object t = tmpl.get(race + "|" + cls(ut));
        if (t == null) { skipped.add(label + " (" + race + " " + ut + ": no template)"); continue; }
        Object ns = deepCopy(t);
        Hashtable<String,Object> nh = A(ns);
        nh.put("Label", label);                        // JOIN KEY back to the campaign
        nh.put("ship_type", ut);
        nh.put("race", race);
        if (!S(ch.get("refit")).isBlank()) nh.put("refit", S(ch.get("refit")));
        nh.put("ID", "Skylark:" + (nid++));
        nh.put("boardID", boardId);                    // must match the board or it won't render
        // templates parked off-board carry boardPile=2 -> client keeps them off the map
        Object bp = nh.get("boardPile");
        nh.put("boardPile", (bp instanceof Integer || bp == null) ? Integer.valueOf(0) : (Object) "0");
        nh.put("Name", S(ch.get("Name")));
        nh.put("Note", S(ch.get("Note")));             // carry commander through
        // crew_units: keep the template's value (and its type) — campaign crew was cleared
        healFully(ns);
        applyDamage(ns, S(ch.get("battle_damage")));   // durable record, if the campaign has one
        nh.put("boardLocation", mkLoc(startX + i * 2, rowY));
        nh.put("saved_boardLocation", mkLoc(startX + i * 2, rowY));
        nh.put("facing", facing);
        nh.put("saved_facing", facing);
        nh.put("current_speed", 0);
        nh.put("previous_speed", 0);
        nh.put("moved_this_turn", Boolean.FALSE);
        on.add(ns);
        made++;
        int[] tal = boxTally(ns);
        System.out.println(String.format("   + %-26s %-6s -> %s  boxes %d/%d intact",
          label, ut, S(nh.get("boardLocation")), tal[0]-tal[1], tal[0]));
      }
    }
    if (!skipped.isEmpty()) { System.out.println("SKIPPED:"); for (String s : skipped) System.out.println("   " + s); }

    ByteArrayOutputStream b = new ByteArrayOutputStream();
    try (ObjectOutputStream out = new ObjectOutputStream(b)) { out.writeObject(board); }
    byte[] bytes = b.toByteArray();
    try (ObjectInputStream in = new ObjectInputStream(new ByteArrayInputStream(bytes))) { in.readObject(); }
    Files.write(Paths.get(outPath), Base64.getEncoder().encode(bytes));
    System.out.println("\nWROTE " + outPath + "  (" + made + " ships, " + bytes.length + " raw bytes, reload-verified)");
  }
}
