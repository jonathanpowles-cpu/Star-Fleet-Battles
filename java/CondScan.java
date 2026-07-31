import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.lang.reflect.*;

/** Distribution of the campaign's durable state fields. java CondScan <campaign> */
public class CondScan {
  static Object fld(Object o, String n) throws Exception {
    if (o == null) return null;
    for (Class<?> c = o.getClass(); c != null; c = c.getSuperclass())
      try { Field f = c.getDeclaredField(n); f.setAccessible(true); return f.get(o); } catch (NoSuchFieldException e) {}
    return null;
  }
  @SuppressWarnings("unchecked")
  static Hashtable<String,Object> A(Object el) throws Exception { return (Hashtable<String,Object>) fld(el, "attributes"); }
  static String S(Object o) { return o == null ? "" : String.valueOf(o); }

  @SuppressWarnings("unchecked")
  public static void main(String[] args) throws Exception {
    byte[] raw = Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(args[0]))).trim());
    Object board;
    try (ObjectInputStream in = new ObjectInputStream(new ByteArrayInputStream(raw))) { board = in.readObject(); }
    Vector<?> on = (Vector<?>) fld(board, "onBoardPieces");

    Map<String,Integer> cond = new TreeMap<>(), status = new TreeMap<>(), pile = new TreeMap<>(), race = new TreeMap<>();
    int nonEmptyCrew = 0, nonEmptyExp = 0, nonEmptyFtr = 0, nonEmptyRefit = 0;
    for (Object p : on) {
      Hashtable<String,Object> a = A(p);
      if (a == null || S(a.get("Label")).isEmpty()) continue;
      cond.merge(S(a.get("condition")), 1, Integer::sum);
      status.merge(S(a.get("status")), 1, Integer::sum);
      pile.merge(S(a.get("boardPile")), 1, Integer::sum);
      race.merge(S(a.get("race")), 1, Integer::sum);
      if (!S(a.get("crew")).isBlank()) nonEmptyCrew++;
      if (!S(a.get("expendables")).isBlank() && !S(a.get("expendables")).equals("[]")) nonEmptyExp++;
      if (!S(a.get("fighters_pfs")).isBlank()) nonEmptyFtr++;
      if (!S(a.get("refit")).isBlank()) nonEmptyRefit++;
    }
    System.out.println("condition  : " + cond);
    System.out.println("status     : " + status);
    System.out.println("boardPile  : " + pile);
    System.out.println("race       : " + race);
    System.out.println("non-blank  : crew=" + nonEmptyCrew + " expendables=" + nonEmptyExp
                       + " fighters_pfs=" + nonEmptyFtr + " refit=" + nonEmptyRefit);

    // Sample any ship that is NOT at condition 100
    for (Object p : on) {
      Hashtable<String,Object> a = A(p);
      if (a == null || S(a.get("Label")).isEmpty()) continue;
      if (!"100".equals(S(a.get("condition")))) {
        System.out.println("\nexample damaged: " + S(a.get("Label"))
            + "  condition=" + S(a.get("condition")) + " status=" + S(a.get("status"))
            + " crew=" + S(a.get("crew")) + " pile=" + S(a.get("boardPile")));
        break;
      }
    }
  }
}
