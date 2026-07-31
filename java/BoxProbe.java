import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.lang.reflect.*;

public class BoxProbe {
  static Object fld(Object o, String n) throws Exception {
    if (o == null) return null;
    for (Class<?> c = o.getClass(); c != null; c = c.getSuperclass())
      try { Field f = c.getDeclaredField(n); f.setAccessible(true); return f.get(o); } catch (NoSuchFieldException e) {}
    return null;
  }
  @SuppressWarnings("unchecked")
  static Hashtable<String,Object> A(Object el) throws Exception { return (Hashtable<String,Object>) fld(el, "attributes"); }
  static String S(Object o) { return o == null ? "" : String.valueOf(o); }

  static void shape(String tag, Object o) throws Exception {
    if (o == null) { System.out.println(tag + " = null"); return; }
    System.out.println(tag + " : " + o.getClass().getName());
    for (Class<?> c = o.getClass(); c != null && !c.equals(Object.class); c = c.getSuperclass()) {
      if (c.getName().startsWith("java.")) break;
      for (Field f : c.getDeclaredFields()) {
        if (Modifier.isStatic(f.getModifiers())) continue;
        f.setAccessible(true);
        Object v = f.get(o);
        String d = v == null ? "null"
          : v.getClass().isArray() ? v.getClass().getSimpleName() + " len=" + Array.getLength(v)
          : v instanceof Collection ? v.getClass().getSimpleName() + " size=" + ((Collection<?>) v).size()
          : v.getClass().getSimpleName() + " = " + S(v).substring(0, Math.min(36, S(v).length()));
        System.out.println("      ." + f.getName() + " : " + d);
      }
    }
  }

  public static void main(String[] args) throws Exception {
    byte[] raw = Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(args[0]))).trim());
    Object board;
    try (ObjectInputStream in = new ObjectInputStream(new ByteArrayInputStream(raw))) { board = in.readObject(); }

    // pick a ship that actually took damage: prefer discarded (destroyed), else onboard
    Object ship = null;
    for (String vn : new String[]{"onBoardPieces","discardedPieces"}) {
      Vector<?> v = (Vector<?>) fld(board, vn);
      if (v == null) continue;
      for (Object p : v) if (p.getClass().getName().contains("ShipAttributes")) { ship = p; break; }
      if (ship != null) { System.out.println("using ship from " + vn); break; }
    }
    Hashtable<String,Object> a = A(ship);
    System.out.println("SHIP: " + S(a.get("Label")) + "  ship_type=" + S(a.get("ship_type")) + "\n");

    Object sbs = a.get("system_box_status");
    System.out.println("===== system_box_status : " + sbs.getClass().getSimpleName() + " len=" + Array.getLength(sbs));
    shape("  [0]", Array.get(sbs, 0));
    System.out.println("\n  all entries:");
    for (int i = 0; i < Array.getLength(sbs); i++) {
      Object e = Array.get(sbs, i);
      if (e == null) { System.out.println("   " + i + ": null"); continue; }
      StringBuilder sb = new StringBuilder("   " + i + ": ");
      for (Class<?> c = e.getClass(); c != null && !c.equals(Object.class); c = c.getSuperclass())
        for (Field f : c.getDeclaredFields()) {
          if (Modifier.isStatic(f.getModifiers())) continue;
          f.setAccessible(true); Object v = f.get(e);
          String vs = v == null ? "null" : v.getClass().isArray() ? "[len=" + Array.getLength(v) + "]" : S(v);
          if (vs.length() > 24) vs = vs.substring(0, 24);
          sb.append(f.getName()).append("=").append(vs).append("  ");
        }
      System.out.println(sb);
    }

    Object ssd = a.get("SSD");
    Object boxes = fld(ssd, "boxes");
    System.out.println("\n===== SSD.boxes len=" + Array.getLength(boxes));
    shape("  Box[0]", Array.get(boxes, 0));
    System.out.println("\n  first 12 boxes:");
    for (int i = 0; i < Math.min(12, Array.getLength(boxes)); i++) {
      Object b = Array.get(boxes, i);
      if (b == null) continue;
      StringBuilder sb = new StringBuilder("   box" + i + ": ");
      for (Class<?> c = b.getClass(); c != null && !c.equals(Object.class); c = c.getSuperclass())
        for (Field f : c.getDeclaredFields()) {
          if (Modifier.isStatic(f.getModifiers())) continue;
          f.setAccessible(true); Object v = f.get(b);
          String vs = v == null ? "null" : v.getClass().isArray() ? "[len=" + Array.getLength(v) + "]" : S(v);
          if (vs.length() > 18) vs = vs.substring(0, 18);
          sb.append(f.getName()).append("=").append(vs).append(" ");
        }
      System.out.println(sb);
    }
  }
}
