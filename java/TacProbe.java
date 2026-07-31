import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.lang.reflect.*;

public class TacProbe {
  static Object fld(Object o, String n) throws Exception {
    if (o == null) return null;
    for (Class<?> c = o.getClass(); c != null; c = c.getSuperclass())
      try { Field f = c.getDeclaredField(n); f.setAccessible(true); return f.get(o); } catch (NoSuchFieldException e) {}
    return null;
  }
  static void dumpFields(Object o, String tag, int max) {
    System.out.println("--- " + tag + " : " + o.getClass().getName());
    int n = 0;
    for (Class<?> c = o.getClass(); c != null && !c.equals(Object.class); c = c.getSuperclass()) {
      if (c.getName().startsWith("javax.swing") || c.getName().startsWith("java.awt")) break;
      for (Field f : c.getDeclaredFields()) {
        if (Modifier.isStatic(f.getModifiers())) continue;
        try {
          f.setAccessible(true);
          Object v = f.get(o);
          String d;
          if (v == null) d = "null";
          else if (v instanceof Collection) d = v.getClass().getSimpleName() + " size=" + ((Collection<?>) v).size();
          else if (v instanceof Map) d = v.getClass().getSimpleName() + " size=" + ((Map<?,?>) v).size();
          else if (v.getClass().isArray()) d = v.getClass().getSimpleName() + " len=" + java.lang.reflect.Array.getLength(v);
          else { String s = String.valueOf(v); d = v.getClass().getSimpleName() + " = " + s.substring(0, Math.min(46, s.length())); }
          System.out.println("     " + c.getSimpleName() + "." + f.getName() + " : " + d);
          if (++n > max) { System.out.println("     ..."); return; }
        } catch (Throwable t) {}
      }
    }
  }
  public static void main(String[] args) throws Exception {
    byte[] raw = Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(args[0]))).trim());
    Object board;
    try (ObjectInputStream in = new ObjectInputStream(new ByteArrayInputStream(raw))) { board = in.readObject(); }
    dumpFields(board, "TACTICAL BOARD", 40);
    Vector<?> on = (Vector<?>) fld(board, "onBoardPieces");
    System.out.println("\nonBoardPieces = " + (on == null ? "null" : on.size()));
    if (on != null && !on.isEmpty()) {
      Map<String,Integer> kinds = new TreeMap<>();
      for (Object p : on) kinds.merge(p.getClass().getSimpleName(), 1, Integer::sum);
      System.out.println("piece kinds: " + kinds);
      Object ship = null;
      for (Object p : on) if (p.getClass().getName().contains("ShipAttributes")) { ship = p; break; }
      if (ship != null) {
        dumpFields(ship, "\nA SHIP", 45);
        @SuppressWarnings("unchecked")
        Hashtable<String,Object> a = (Hashtable<String,Object>) fld(ship, "attributes");
        if (a != null) {
          System.out.println("\n  ship attribute keys (" + a.size() + "):");
          System.out.println("   " + new TreeSet<>(a.keySet()));
          for (String k : new String[]{"Label","race","unit_type","ID","boardLocation","facing","current_speed","crew","condition"})
            if (a.containsKey(k)) System.out.println("     " + k + " = " + String.valueOf(a.get(k)).substring(0, Math.min(50, String.valueOf(a.get(k)).length())));
        }
        Object ssd = fld(ship, "ssd");
        if (ssd == null && a != null) ssd = a.get("ssd");
        if (ssd != null) dumpFields(ssd, "\nSSD", 22);
      }
    }
  }
}
