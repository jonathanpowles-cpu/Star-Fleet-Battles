import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.lang.reflect.*;

/** Step 1: find campaign hexes where opposing fleets are co-located = a battle.
 *  Reports each engagement with both OOBs, flagship, and the F&E command-limit
 *  picture (S8.0 + Campaign Designer's Handbook halved-rating house rule). */
public class Detect {
  static Object fld(Object o, String n) throws Exception {
    if (o == null) return null;
    for (Class<?> c = o.getClass(); c != null; c = c.getSuperclass())
      try { Field f = c.getDeclaredField(n); f.setAccessible(true); return f.get(o); } catch (NoSuchFieldException e) {}
    return null;
  }
  @SuppressWarnings("unchecked")
  static Hashtable<String,Object> A(Object el) throws Exception { return (Hashtable<String,Object>) fld(el, "attributes"); }
  static String S(Object o) { return o == null ? "" : String.valueOf(o); }
  static String hexOf(Object p) throws Exception {
    Object bl = A(p).get("boardLocation");
    if (bl == null) return null;
    Object x = fld(bl, "x"), y = fld(bl, "y");
    if (x == null || y == null) return null;
    return String.format("%02d%02d", ((Number) x).intValue(), ((Number) y).intValue());
  }
  /** F&E command rating by hull class - used until we read the real value from the
   *  ship .def; the tactical side already carries f&e_command_rating natively. */
  static int cmdRating(String ut) {
    String k = ut == null ? "" : ut.replaceAll("[+pP]+$", "");
    switch (k) {
      case "DN": case "BB":            return 10;
      case "BC": case "BCH":           return 8;
      case "CC": case "CVA":           return 9;
      case "CA": case "CV":            return 7;
      case "CW": case "CVL": case "CS":return 6;
      case "CL": case "CLE": case "CVE": return 6;
      case "DD": case "DW": case "DF": return 4;
      case "FF": case "EFF": case "SF":return 3;
      default:                          return 3;
    }
  }
  public static void main(String[] args) throws Exception {
    byte[] raw = Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(args[0]))).trim());
    Object board;
    try (ObjectInputStream in = new ObjectInputStream(new ByteArrayInputStream(raw))) { board = in.readObject(); }
    Vector<?> on = (Vector<?>) fld(board, "onBoardPieces");

    Map<String,List<Object>> byHex = new TreeMap<>();
    for (Object p : on) {
      String h = hexOf(p);
      if (h != null) byHex.computeIfAbsent(h, k -> new ArrayList<>()).add(p);
    }
    System.out.println("scanned " + on.size() + " ships across " + byHex.size() + " occupied hexes\n");

    int battles = 0;
    for (Map.Entry<String,List<Object>> e : byHex.entrySet()) {
      Map<String,List<Object>> sides = new LinkedHashMap<>();
      for (Object p : e.getValue()) sides.computeIfAbsent(S(A(p).get("race")), k -> new ArrayList<>()).add(p);
      if (sides.size() < 2) continue;
      battles++;
      System.out.println("=========================================================");
      System.out.println("BATTLE at hex " + e.getKey() + "   (" + e.getValue().size() + " ships, "
                       + sides.size() + " sides)");
      for (Map.Entry<String,List<Object>> s : sides.entrySet()) {
        // flagship = highest command rating
        Object flag = null; int best = -1;
        for (Object p : s.getValue()) {
          int r = cmdRating(S(A(p).get("unit_type")));
          if (r > best) { best = r; flag = p; }
        }
        int halved = Math.max(3, best / 2);           // Handbook house rule
        System.out.println("  " + s.getKey() + ": " + s.getValue().size() + " ships"
          + "   flagship=" + S(A(flag).get("Label")) + " (rating " + best + ")"
          + "   command limit: " + best + "+2=" + (best + 2)
          + "  | halved: " + halved + "+2=" + (halved + 2));
        if (s.getValue().size() > halved + 2)
          System.out.println("      !! " + s.getValue().size() + " ships exceeds the halved-rating limit ("
            + (halved + 2) + ") -> split into waves, or use full ratings");
        for (Object p : s.getValue())
          System.out.println("        " + String.format("%-28s %-6s cond=%-4s %s",
            S(A(p).get("Label")), S(A(p).get("unit_type")), S(A(p).get("condition")), S(A(p).get("ID"))));
      }
      System.out.println("  -> would export: <year>-<turn>-" + e.getKey() + ".SFB");
    }
    if (battles == 0) System.out.println("No contested hexes - no battles this turn.");
    else System.out.println("\n" + battles + " battle(s) detected.");
  }
}
