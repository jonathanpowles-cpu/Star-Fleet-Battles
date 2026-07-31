import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.lang.reflect.*;

/**
 * Tactical -> campaign import: the other half of ExportBattle.
 *
 *   java ImportBattle <campaign> <tacticalSave> [--apply] [--json <out>]
 *
 * Reads a finished tactical battle, works out how each ship fared from its SSD
 * box status, and writes the outcome back onto the matching campaign ship.
 *
 * JOIN KEY: the piece Label. ExportBattle deliberately relabels exported ships
 * with the campaign Label so they can be matched on the way home. A tactical
 * ship whose Label matches no campaign ship is REPORTED AND SKIPPED, never
 * guessed at.
 *
 * WHAT IT WRITES (the campaign's own durable fields, confirmed by inspection):
 *   condition      0-100, the share of SSD boxes still intact. This is what the
 *                  campaign UI displays and what ExportBattle's healer honours.
 *   battle_damage  exact per-box record "g0:1,1,2,1|g1:..." (2 = destroyed), so a
 *                  later export can restore the precise damage rather than an
 *                  approximation from the percentage. ExportBattle already reads
 *                  this attribute; nothing wrote it until now.
 *   boardPile      set to 1 (off-board) when a ship is destroyed.
 *   status         annotated "destroyed" for a killed ship.
 *
 * DRY RUN BY DEFAULT. Without --apply it changes nothing and just prints the
 * table of what it would do. With --apply it writes a timestamped backup of the
 * campaign file first, then saves.
 *
 * The client must be FULLY EXITED before applying: it holds the campaign in
 * memory and will overwrite anything written underneath it.
 */
public class ImportBattle {

  // ---------- reflection helpers ----------
  static Object fld(Object o, String n) throws Exception {
    if (o == null) return null;
    for (Class<?> c = o.getClass(); c != null; c = c.getSuperclass())
      try { Field f = c.getDeclaredField(n); f.setAccessible(true); return f.get(o); } catch (NoSuchFieldException e) {}
    return null;
  }
  @SuppressWarnings("unchecked")
  static Hashtable<String,Object> A(Object el) throws Exception {
    return (Hashtable<String,Object>) fld(el, "attributes");
  }
  static String S(Object o) { return o == null ? "" : String.valueOf(o); }
  static Object load(String p) throws Exception {
    byte[] raw = Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(p))).trim());
    try (ObjectInputStream in = new ObjectInputStream(new ByteArrayInputStream(raw))) { return in.readObject(); }
  }
  static void save(Object board, String p) throws Exception {
    ByteArrayOutputStream b = new ByteArrayOutputStream();
    try (ObjectOutputStream out = new ObjectOutputStream(b)) { out.writeObject(board); }
    // The client stores the serialized board base64-encoded, NOT as raw bytes.
    Files.write(Paths.get(p), Base64.getEncoder().encodeToString(b.toByteArray()).getBytes());
  }
  static String jesc(String s) { return s == null ? "" : s.replace("\\", "\\\\").replace("\"", "\\\""); }

  // ---------- damage measurement ----------
  /** {totalBoxes, destroyedBoxes} across the whole SSD. */
  static int[] tally(Object ship) throws Exception {
    Object ssd = A(ship).get("SSD");
    if (ssd == null) return new int[]{0, 0};
    Object boxes = fld(ssd, "boxes");
    if (boxes == null) return new int[]{0, 0};
    int t = 0, d = 0;
    for (int i = 0; i < Array.getLength(boxes); i++) {
      Object bs = fld(Array.get(boxes, i), "boxStatus");
      if (bs instanceof int[]) for (int v : (int[]) bs) { t++; if (v == 2) d++; }
    }
    return new int[]{t, d};
  }

  /** Encode the SSD as ExportBattle's durable record: "g0:1,1,2|g3:1,2,2". Only
   *  groups that actually contain damage are emitted, to keep it compact. */
  static String encodeDamage(Object ship) throws Exception {
    Object ssd = A(ship).get("SSD");
    if (ssd == null) return "";
    Object boxes = fld(ssd, "boxes");
    if (boxes == null) return "";
    StringBuilder sb = new StringBuilder();
    for (int i = 0; i < Array.getLength(boxes); i++) {
      Object bs = fld(Array.get(boxes, i), "boxStatus");
      if (!(bs instanceof int[])) continue;
      int[] arr = (int[]) bs;
      boolean any = false;
      for (int v : arr) if (v == 2) { any = true; break; }
      if (!any) continue;
      if (sb.length() > 0) sb.append('|');
      sb.append('g').append(i).append(':');
      for (int k = 0; k < arr.length; k++) { if (k > 0) sb.append(','); sb.append(arr[k]); }
    }
    return sb.toString();
  }

  static class Outcome {
    String label, race, type;
    int total, destroyed, condition, oldCondition;
    boolean killed, matched;
    String damageRec = "";
  }

  @SuppressWarnings("unchecked")
  public static void main(String[] args) throws Exception {
    if (args.length < 2) {
      System.out.println("usage: ImportBattle <campaign> <tacticalSave> [--apply] [--json <out>]");
      return;
    }
    String campPath = args[0], tacPath = args[1];
    boolean apply = false; String jsonOut = null;
    for (int i = 2; i < args.length; i++) {
      if (args[i].equals("--apply")) apply = true;
      else if (args[i].equals("--json") && i + 1 < args.length) jsonOut = args[++i];
    }

    Object tac = load(tacPath);
    Object camp = load(campPath);
    Vector<?> tacPieces = (Vector<?>) fld(tac, "onBoardPieces");
    Vector<?> campPieces = (Vector<?>) fld(camp, "onBoardPieces");

    // Index campaign ships by Label
    Map<String,Object> byLabel = new LinkedHashMap<>();
    for (Object p : campPieces) {
      Hashtable<String,Object> a = A(p);
      if (a == null) continue;
      String l = S(a.get("Label"));
      if (!l.isEmpty()) byLabel.put(l, p);
    }

    List<Outcome> outcomes = new ArrayList<>();
    for (Object p : tacPieces) {
      if (!p.getClass().getName().contains("ShipAttributes")) continue;
      Hashtable<String,Object> a = A(p);
      if (a == null) continue;
      Outcome o = new Outcome();
      o.label = S(a.get("Label"));
      o.race = S(a.get("race"));
      o.type = S(a.get("ship_type"));
      int[] t = tally(p);
      o.total = t[0]; o.destroyed = t[1];
      o.condition = (o.total == 0) ? 100 : (int) Math.round(100.0 * (o.total - o.destroyed) / o.total);
      // A ship is counted killed only when the tactical game itself removed it
      // from the board, or its SSD is entirely gone. We do NOT infer death from
      // a low percentage - a badly mauled ship that survived must stay alive.
      boolean offBoard = !"0".equals(S(a.get("boardPile"))) && !S(a.get("boardPile")).isEmpty();
      o.killed = offBoard || (o.total > 0 && o.destroyed >= o.total);
      o.damageRec = encodeDamage(p);
      Object cp = byLabel.get(o.label);
      o.matched = cp != null;
      if (cp != null) o.oldCondition = parseIntSafe(S(A(cp).get("condition")), 100);
      outcomes.add(o);
    }

    // ---------- report ----------
    System.out.println("campaign : " + campPath);
    System.out.println("tactical : " + tacPath);
    System.out.println("campaign ships: " + byLabel.size() + "   tactical ships: " + outcomes.size());
    System.out.println();
    System.out.println(String.format("%-26s %-10s %8s %10s %9s  %s",
        "LABEL", "RACE", "BOXES", "CONDITION", "MATCH", "OUTCOME"));
    System.out.println("-".repeat(88));
    int matched = 0, changed = 0, kills = 0;
    for (Outcome o : outcomes) {
      if (o.matched) matched++;
      String cond = o.matched ? (o.oldCondition + " -> " + o.condition) : String.valueOf(o.condition);
      String verdict = !o.matched ? "NO CAMPAIGN MATCH - skipped"
                     : o.killed ? "DESTROYED"
                     : (o.condition == o.oldCondition) ? "unchanged"
                     : "damaged";
      if (o.matched && (o.killed || o.condition != o.oldCondition)) changed++;
      if (o.matched && o.killed) kills++;
      System.out.println(String.format("%-26s %-10s %4d/%-4d %10s %9s  %s",
          trunc(o.label, 26), trunc(o.race, 10), o.total - o.destroyed, o.total,
          cond, o.matched ? "yes" : "NO", verdict));
    }
    System.out.println("-".repeat(88));
    System.out.println("matched " + matched + "/" + outcomes.size()
        + "   would change " + changed + "   destroyed " + kills);

    if (jsonOut != null) writeJson(jsonOut, outcomes);

    if (!apply) {
      System.out.println("\nDRY RUN - nothing written. Re-run with --apply to commit.");
      if (matched == 0)
        System.out.println("NOTE: no labels matched. These tactical ships were not exported from\n"
                         + "      the campaign, so there is nothing to import them into.");
      return;
    }
    if (matched == 0) {
      System.out.println("\nREFUSING to apply: no tactical ship matched a campaign ship.");
      return;
    }

    // ---------- apply ----------
    String stamp = new java.text.SimpleDateFormat("yyyyMMdd_HHmmss").format(new Date());
    Path backup = Paths.get(campPath + ".preimport_" + stamp);
    Files.copy(Paths.get(campPath), backup, StandardCopyOption.REPLACE_EXISTING);
    System.out.println("\nbackup: " + backup);

    int wrote = 0;
    for (Outcome o : outcomes) {
      if (!o.matched) continue;
      Object cp = byLabel.get(o.label);
      Hashtable<String,Object> ca = A(cp);
      ca.put("condition", String.valueOf(o.condition));
      ca.put("battle_damage", o.damageRec);
      if (o.killed) {
        ca.put("boardPile", "1");
        // Idempotent: re-running the import must not stack up "destroyed
        // destroyed destroyed" on the status flag.
        String st = S(ca.get("status"));
        if (!st.toLowerCase().contains("destroyed"))
          ca.put("status", st.isBlank() ? "destroyed" : st + " destroyed");
      }
      wrote++;
    }
    save(camp, campPath);
    System.out.println("applied to " + wrote + " campaign ships; saved " + campPath);
    System.out.println("Reload the campaign in the client to see it.");
  }

  static int parseIntSafe(String s, int dflt) {
    try { return Integer.parseInt(s.trim()); } catch (Exception e) { return dflt; }
  }
  static String trunc(String s, int n) { return s == null ? "" : (s.length() <= n ? s : s.substring(0, n)); }

  static void writeJson(String path, List<Outcome> outs) throws Exception {
    StringBuilder sb = new StringBuilder("[\n");
    for (int i = 0; i < outs.size(); i++) {
      Outcome o = outs.get(i);
      if (i > 0) sb.append(",\n");
      sb.append(String.format(
          "  {\"label\":\"%s\",\"race\":\"%s\",\"type\":\"%s\",\"boxes_total\":%d,"
          + "\"boxes_destroyed\":%d,\"condition\":%d,\"old_condition\":%d,"
          + "\"killed\":%s,\"matched\":%s}",
          jesc(o.label), jesc(o.race), jesc(o.type), o.total, o.destroyed,
          o.condition, o.oldCondition, o.killed, o.matched));
    }
    sb.append("\n]\n");
    Files.write(Paths.get(path), sb.toString().getBytes());
    System.out.println("json: " + path);
  }
}
