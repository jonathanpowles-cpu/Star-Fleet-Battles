import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.lang.reflect.*;

public class Rebuild {
  static boolean APPLY = false;

  static Object fld(Object o, String n) throws Exception {
    if (o == null) return null;
    for (Class<?> c = o.getClass(); c != null; c = c.getSuperclass())
      try { Field f = c.getDeclaredField(n); f.setAccessible(true); return f.get(o); } catch (NoSuchFieldException e) {}
    return null;
  }
  static void setfld(Object o, String n, Object v) throws Exception {
    for (Class<?> c = o.getClass(); c != null; c = c.getSuperclass())
      try { Field f = c.getDeclaredField(n); f.setAccessible(true); f.set(o, v); return; } catch (NoSuchFieldException e) {}
    throw new NoSuchFieldException(n);
  }
  @SuppressWarnings("unchecked")
  static Hashtable<String,Object> A(Object el) throws Exception { return (Hashtable<String,Object>) fld(el, "attributes"); }
  static String S(Object o) { return o == null ? "" : String.valueOf(o); }
  static String lab(Object el) throws Exception { return S(A(el).get("Label")); }
  static String uid(Object el) throws Exception { return S(A(el).get("ID")); }
  static String utype(Object el) throws Exception { return S(A(el).get("unit_type")); }
  static String race(Object el) throws Exception { return S(A(el).get("race")); }

  static Object deepCopy(Object o) throws Exception {
    ByteArrayOutputStream b = new ByteArrayOutputStream();
    try (ObjectOutputStream out = new ObjectOutputStream(b)) { out.writeObject(o); }
    try (ObjectInputStream in = new ObjectInputStream(new ByteArrayInputStream(b.toByteArray()))) { return in.readObject(); }
  }
  static Object mkLoc(int x, int y) throws Exception {
    Class<?> c = Class.forName("pd.pfranz.map.BoardLocation");
    try { return c.getConstructor(int.class, int.class).newInstance(x, y); }
    catch (NoSuchMethodException e) {
      Object o = c.getDeclaredConstructor().newInstance();
      setfld(o, "x", x); setfld(o, "y", y); return o;
    }
  }
  static void place(Object el, String hex) throws Exception {
    int x = Integer.parseInt(hex.substring(0, 2)), y = Integer.parseInt(hex.substring(2));
    A(el).put("boardLocation", mkLoc(x, y));
    A(el).put("saved_boardLocation", mkLoc(x, y));
  }
  static Object byId(Vector<?> v, String idSuffix) throws Exception {
    for (Object o : v) if (uid(o).endsWith(idSuffix)) return o;
    return null;
  }
  static Object byLabel(Vector<?> v, String lb) throws Exception {
    for (Object o : v) if (lab(o).equalsIgnoreCase(lb)) return o;
    return null;
  }
  static Object srcOfType(Vector<?> v, String rc, String ut) throws Exception {
    for (Object o : v) if (race(o).equals(rc) && utype(o).equals(ut)) return o;
    return null;
  }

  static final String[] RANKS = {"Arch Duke","King","Duke","Countess","Count","Baron","Marquis",
                                 "Marshall","Captain","Commodore","Admiral","Commander"};
  static String commander(String note) {
    if (note == null || note.isBlank()) return "";
    String rank = null; int at = -1;
    for (String r : RANKS) {
      int i = -1;
      while ((i = note.indexOf(r, i + 1)) >= 0) {
        int end = i + r.length();
        // must be a standalone rank followed by a space + a capitalised name:
        // rejects "Count's Fleet" in a destruction record
        if (end >= note.length() || note.charAt(end) != ' ') continue;
        int k = end + 1;
        if (k >= note.length() || !Character.isUpperCase(note.charAt(k))) continue;
        if (at < 0 || i < at) { rank = r; at = i; }
        break;
      }
    }
    if (rank == null) return "";
    String rest = note.substring(at + rank.length());
    List<String> name = new ArrayList<>();
    for (String t : rest.trim().split("\\s+")) {
      String w = t.replaceAll("^[,;.]+", "").replaceAll("[,;.]+$", "");
      if (w.isEmpty()) continue;
      if (w.equalsIgnoreCase("Exp")) break;
      if (w.equals("(LCap)")) { name.add(w); break; }
      if (w.toUpperCase().equals(w) && w.length() > 2) break;
      if (!Character.isUpperCase(w.charAt(0))) break;
      name.add(w);
      if (name.size() >= 2) break;
    }
    String out = (rank + " " + String.join(" ", name)).trim();
    if (note.toUpperCase().contains("LEGENDARY COMMANDER")) out += " LEGENDARY COMMANDER";
    if (note.contains("(LCap)") && !out.contains("(LCap)")) out += " (LCap)";
    return out;
  }

  // ---------- deployment ----------
  static final int[][] CLAN_RANGES = {{100,199},{200,219},{220,239},{240,259},{260,279},{280,299},
                                      {700,719},{720,739},{740,759},{760,779},{780,799}};
  static final String[] CLAN_NAMES = {"Homeworld/Royal","Foremost","Apex","Silver Moon","Black Claw",
                                      "Night Roar","Red Claw","Black Stripe","Pelt Hunter","Golden Fang","Predator"};
  static final Set<String> BORDER = new HashSet<>(Arrays.asList("Red Claw","Black Stripe","Pelt Hunter","Golden Fang","Predator"));
  static int hullOf(String label) {
    java.util.regex.Matcher m = java.util.regex.Pattern.compile("^\\s*(\\d+)").matcher(label == null ? "" : label);
    return m.find() ? Integer.parseInt(m.group(1)) : -1;
  }
  static String clanOf(String label) {
    int h = hullOf(label);
    for (int i = 0; i < CLAN_RANGES.length; i++) if (h >= CLAN_RANGES[i][0] && h <= CLAN_RANGES[i][1]) return CLAN_NAMES[i];
    return "unknown";
  }
  static String norm(String ut) {
    String k = ut == null ? "" : ut.replaceAll("[+pP]+$", "");
    if (k.equals("DWM")) return "DW";
    if (k.equals("MB2") || k.equals("BATS")) return "MB";
    if (k.equals("TG")) return "TGP";
    if (k.equals("CS")) return "BC";     // unrefitted strike cruiser = BC-series (R5.2/R5.3)
    if (k.equals("FFK")) return "FF";
    return k;
  }
  static List<Object> take(List<Object> pool, String rc, Map<String,Integer> oob, boolean preferBorder) throws Exception {
    List<Object> got = new ArrayList<>();
    Map<String,List<Object>> byType = new LinkedHashMap<>();
    for (Object p : pool) if (race(p).equals(rc)) byType.computeIfAbsent(norm(utype(p)), k -> new ArrayList<>()).add(p);
    if (preferBorder) for (List<Object> l : byType.values())
      l.sort((a, b) -> {
        try { return Boolean.compare(!BORDER.contains(clanOf(lab(a))), !BORDER.contains(clanOf(lab(b)))); }
        catch (Exception e) { return 0; }
      });
    for (Map.Entry<String,Integer> e : oob.entrySet()) {
      List<Object> avail = byType.getOrDefault(e.getKey(), new ArrayList<>());
      for (int i = 0; i < e.getValue() && !avail.isEmpty(); i++) got.add(avail.remove(0));
    }
    pool.removeAll(got);
    return got;
  }
  static Map<String,Integer> oob(Object... kv) {
    Map<String,Integer> m = new LinkedHashMap<>();
    for (int i = 0; i < kv.length; i += 2) m.put((String) kv[i], (Integer) kv[i+1]);
    return m;
  }
  /** whole clans -> hexes, largest clan into the emptiest hex with room */
  static void byClanHexes(List<Object> ships, String[] hexes, Map<String,Integer> caps, String tag) throws Exception {
    Map<String,List<Object>> groups = new LinkedHashMap<>();
    for (Object p : ships) groups.computeIfAbsent(clanOf(lab(p)), k -> new ArrayList<>()).add(p);
    Map<String,List<Object>> out = new LinkedHashMap<>();
    for (String h : hexes) out.put(h, new ArrayList<>());
    List<Map.Entry<String,List<Object>>> gs = new ArrayList<>(groups.entrySet());
    gs.sort((a, b) -> b.getValue().size() - a.getValue().size());
    for (Map.Entry<String,List<Object>> g : gs) {
      String best = null;
      for (String h : hexes) {
        int cap = caps.getOrDefault(h, Integer.MAX_VALUE);
        if (out.get(h).size() + g.getValue().size() > cap) continue;
        if (best == null || out.get(h).size() < out.get(best).size()) best = h;
      }
      if (best == null) for (String h : hexes) if (!caps.containsKey(h)) { best = h; break; }
      if (best == null) best = hexes[0];
      out.get(best).addAll(g.getValue());
    }
    for (Map.Entry<String,List<Object>> e : out.entrySet()) {
      Set<String> clans = new LinkedHashSet<>();
      for (Object p : e.getValue()) { place(p, e.getKey()); clans.add(clanOf(lab(p))); }
      System.out.println("  DEPLOY    " + e.getKey() + " " + tag + " " + e.getValue().size() + " ships, clans: " + clans);
    }
  }
  static void deploy(Vector<Object> on) throws Exception {
    List<Object> pool = new ArrayList<>(on);
    // Marquis' pieces were just placed at 1902/1803/1704 - exclude them
    List<Object> marq = new ArrayList<>();
    for (Object p : pool) { String h = hexOf(p); if (h.equals("1902")||h.equals("1803")||h.equals("1704")) marq.add(p); }
    pool.removeAll(marq);
    List<Object> red  = take(pool, "Lyran", oob("BC",1,"CC",1,"CA",4,"CW",5,"CL",5,"DD",5,"FF",5,"SC",1,"TGC",1,"MB",1), true);
    List<Object> home = take(pool, "Lyran", oob("DN",1,"CC",1,"CA",4,"CW",5,"CL",5,"DD",5,"DW",3,"FF",5,"SC",1,"TGP",1,"FRD",1,"MB",1), false);
    System.out.println("  LYRAN     Red Claw " + red.size() + "/29   Home " + home.size() + "/33");
    byClanHexes(red,  new String[]{"0502","0504","0604"}, new HashMap<>(), "Red Claw");
    byClanHexes(home, new String[]{"0408","0608","0707"}, new HashMap<>(Map.of("0707", 6)), "Home");
    Map<String,Integer> KH = oob("DN",1,"CV",1,"CVL",1,"CVE",1,"CLE",2,"EFF",3,"CC",1,"BC",3,"CL",1,"DD",3,"FF",3,"DF",1,"SF",1,"TGC",1,"FRD",1,"MB",1);
    Map<String,Integer> KC = oob("CV",1,"CVL",1,"CVE",1,"CLE",2,"EFF",3,"CC",1,"BC",3,"CL",1,"DF",1,"FF",1,"SF",1,"TGT",1);
    Map<String,Integer> KD = oob("DN",1,"CV",1,"CVL",1,"CVE",1,"CLE",2,"EFF",3,"CC",1,"BC",3,"CL",1,"DF",1,"FF",1,"SF",1,"TGC",1);
    List<Object> kh = take(pool, "Kzinti", KH, false);
    List<Object> kc = take(pool, "Kzinti", KC, false);
    List<Object> kd = take(pool, "Kzinti", KD, false);
    for (Object p : kh) place(p, "1401");
    for (Object p : kc) place(p, "0901");
    for (Object p : kd) place(p, "1504");
    System.out.println("  KZINTI    Home " + kh.size() + "/25 @1401   Count's " + kc.size() + "/17 @0901   Duke's " + kd.size() + "/18 @1504");
    if (!pool.isEmpty()) {
      System.out.println("  !! UNASSIGNED " + pool.size() + ":");
      for (Object p : pool) System.out.println("       " + lab(p) + " (" + race(p) + " " + utype(p) + ")");
    }
  }
  static String hexOf(Object p) throws Exception {
    Object bl = A(p).get("boardLocation");
    if (bl == null) return "-";
    Object x = fld(bl, "x"), y = fld(bl, "y");
    if (x == null || y == null) return "-";
    return String.format("%02d%02d", ((Number) x).intValue(), ((Number) y).intValue());
  }

  @SuppressWarnings("unchecked")
  public static void main(String[] args) throws Exception {
    APPLY = args.length > 2 && args[2].equals("--apply");
    byte[] raw = Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(args[0]))).trim());
    Object board;
    try (ObjectInputStream in = new ObjectInputStream(new ByteArrayInputStream(raw))) { board = in.readObject(); }
    Vector<Object> on = (Vector<Object>) fld(board, "onBoardPieces");
    Vector<Object> disc = (Vector<Object>) fld(board, "discardedPieces");
    System.out.println((APPLY ? "APPLYING" : "DRY RUN") + "  start: onBoard=" + on.size() + " discarded=" + disc.size());

    String[] losses = {"1589270692467","1589270692465","1589270692452","1589270692447","1589270692513"};
    for (String id : losses) {
      Object p = byId(disc, id);
      if (p != null) { disc.remove(p); on.add(p); System.out.println("  RESURRECT " + lab(p)); }
    }

    String[][] restore = {
      {"1589270692577", "",        "1401"},
      {"1589270692582", "EFF 115", "1401"},
      {"1589270692578", "",        "0901"},
      {"1589270692581", "EFF 117", "0901"},
      {"1589270692579", "",        "1504"},
      {"1589270692580", "EFF 125", "1504"},
    };
    for (String[] r : restore) {
      Object p = byId(disc, r[0]);
      if (p == null) continue;
      disc.remove(p); on.add(p);
      String old = lab(p);
      if (!r[1].isEmpty()) A(p).put("Label", r[1]);
      place(p, r[2]);
      System.out.println("  RESTORE   " + old + (r[1].isEmpty() ? "" : " -> " + r[1]) + " @" + r[2]);
    }

    // CLE Lifeguard KEPT: with the 3 CVEs restored the escort count is exactly
    // 6 CLE + 3 CVE = OOB (2 CLE per fleet x 3 fleets). It is not a duplicate.

    String[][] MARQ = {
      {"CV",  "CV Rapier",         "1902"}, {"CL", "CL Witchcraft", "1902"}, {"EFF", "EFF 134", "1902"},
      {"CC",  "CC Universe",       "1902"}, {"BC", "BC Black Hole", "1902"}, {"DF",  "DF 61",   "1902"},
      {"CVL", "CVL Zephyr",        "1803"}, {"CL", "CL Mysterion",  "1803"}, {"EFF", "EFF 195", "1803"},
      {"BC",  "BC Eclipse",        "1803"}, {"FF", "FF 231",        "1803"}, {"SF+", "SF 115",  "1803"},
      {"CVE", "CVE Conflagration", "1704"}, {"EFF","EFF 224",       "1704"},
      {"BC",  "BC Nebula",         "1704"}, {"CL", "CL Warlock",    "1704"}, {"TGT", "TGT#2",   "1704"},
    };
    long nextId = 1589270692583L;
    for (String[] spec : MARQ) {
      Object src = srcOfType(on, "Kzinti", spec[0]);
      if (src == null) src = srcOfType(disc, "Kzinti", spec[0]);
      if (src == null) { System.out.println("  !! no clone source for " + spec[0] + " (" + spec[1] + ")"); continue; }
      Object nw = deepCopy(src);
      nextId++;
      A(nw).put("ID", "Skylark:" + nextId);
      A(nw).put("Label", spec[1]);
      A(nw).put("condition", 100);
      A(nw).put("Note", ""); A(nw).put("status", ""); A(nw).put("crew", "");
      place(nw, spec[2]);
      on.add(nw);
      System.out.println("  CREATE    " + String.format("%-18s", spec[1]) + " (" + spec[0] + " <- " + lab(src) + ") @" + spec[2] + " id=Skylark:" + nextId);
    }

    // ---- DEPLOY: F&E 711.0 (Lyran) / 705.0 (Kzinti) ----
    deploy(on);

    int kept = 0, cleared = 0;
    for (Object p : on) {
      Hashtable<String,Object> a = A(p);
      String note = S(a.get("Note"));
      String cmd = commander(note);
      if (!note.isBlank()) {
        if (cmd.isEmpty()) cleared++;
        else { kept++; if (!cmd.equals(note)) System.out.println("  NOTE      " + String.format("%-26s", lab(p)) + " [" + note + "] -> [" + cmd + "]"); }
      }
      a.put("Note", cmd);
      if (!lab(p).contains("Bloodshedder")) a.put("status", "");
      a.put("crew", "");
    }
    System.out.println("  notes: kept " + kept + " commander(s), cleared " + cleared);

    Object log = fld(board, "log");
    Vector<Object> entries = (Vector<Object>) fld(log, "entries");
    System.out.println("  LOG       clearing " + entries.size() + " entries");
    entries.clear();

    System.out.println("FINAL: onBoard=" + on.size() + " discarded=" + disc.size());

    if (APPLY) {
      ByteArrayOutputStream b = new ByteArrayOutputStream();
      try (ObjectOutputStream out = new ObjectOutputStream(b)) { out.writeObject(board); }
      byte[] bytes = b.toByteArray();
      try (ObjectInputStream in = new ObjectInputStream(new ByteArrayInputStream(bytes))) { in.readObject(); }
      Files.write(Paths.get(args[1]), Base64.getEncoder().encode(bytes));
      System.out.println("WROTE " + args[1] + "  (" + bytes.length + " raw bytes, reload-verified)");
    }
  }
}
