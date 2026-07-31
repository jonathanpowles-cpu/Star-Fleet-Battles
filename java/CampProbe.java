import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.lang.reflect.*;

/** Probe the campaign save: what attributes do ships carry, and is battle_damage present?
 *  java CampProbe <campaign> [labelFilter] */
public class CampProbe {
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
    String filter = args.length > 1 ? args[1].toLowerCase() : null;

    Vector<?> on = (Vector<?>) fld(board, "onBoardPieces");
    System.out.println("onBoardPieces: " + on.size());

    // Class histogram
    Map<String,Integer> hist = new TreeMap<>();
    for (Object p : on) hist.merge(p.getClass().getSimpleName(), 1, Integer::sum);
    System.out.println("piece classes: " + hist);

    int shown = 0, withDamage = 0, ships = 0;
    Set<String> allKeys = new TreeSet<>();
    for (Object p : on) {
      Hashtable<String,Object> a = A(p);
      if (a == null) continue;
      String label = S(a.get("Label"));
      if (label.isEmpty()) continue;
      ships++;
      allKeys.addAll(a.keySet());
      if (a.get("battle_damage") != null && !S(a.get("battle_damage")).isBlank()) withDamage++;
      if (filter != null && !label.toLowerCase().contains(filter)) continue;
      if (shown++ >= 4) continue;
      System.out.println("\n--- " + label + "  [" + p.getClass().getSimpleName() + "] ---");
      for (String k : new TreeSet<>(a.keySet())) {
        String v = S(a.get(k));
        if (v.length() > 90) v = v.substring(0, 90) + "...";
        System.out.println(String.format("   %-22s %s", k, v));
      }
    }
    System.out.println("\nlabelled pieces: " + ships + "   with battle_damage: " + withDamage);
    System.out.println("\nALL attribute keys seen across labelled pieces:");
    for (String k : allKeys) System.out.println("   " + k);
  }
}
