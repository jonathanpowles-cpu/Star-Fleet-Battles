import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.lang.reflect.*;

public class SSDProbe {
  static Object fld(Object o, String n) throws Exception {
    if (o == null) return null;
    for (Class<?> c = o.getClass(); c != null; c = c.getSuperclass())
      try { Field f = c.getDeclaredField(n); f.setAccessible(true); return f.get(o); } catch (NoSuchFieldException e) {}
    return null;
  }
  @SuppressWarnings("unchecked")
  static Hashtable<String,Object> A(Object el) throws Exception { return (Hashtable<String,Object>) fld(el, "attributes"); }
  static String S(Object o) { return o == null ? "" : String.valueOf(o); }
  static void allFields(Object o, String indent) throws Exception {
    for (Class<?> c = o.getClass(); c != null && !c.equals(Object.class); c = c.getSuperclass()) {
      if (c.getName().startsWith("java.")) break;
      for (Field f : c.getDeclaredFields()) {
        if (Modifier.isStatic(f.getModifiers())) continue;
        f.setAccessible(true);
        Object v = f.get(o);
        String d;
        if (v == null) d = "null";
        else if (v.getClass().isArray()) d = v.getClass().getSimpleName() + " len=" + Array.getLength(v);
        else if (v instanceof Collection) d = "Collection size=" + ((Collection<?>) v).size();
        else { String s = String.valueOf(v); d = v.getClass().getSimpleName() + "=" + s.substring(0, Math.min(38, s.length())); }
        System.out.println(indent + c.getSimpleName() + "." + f.getName() + " : " + d);
      }
    }
  }
  public static void main(String[] args) throws Exception {
    byte[] raw = Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(args[0]))).trim());
    Object board;
    try (ObjectInputStream in = new ObjectInputStream(new ByteArrayInputStream(raw))) { board = in.readObject(); }
    Vector<?> on = (Vector<?>) fld(board, "onBoardPieces");
    Object ship = null;
    for (Object p : on) if (p.getClass().getName().contains("ShipAttributes")) { ship = p; break; }
    Hashtable<String,Object> a = A(ship);
    System.out.println("SHIP: " + S(a.get("Label")) + "  ship_type=" + S(a.get("ship_type")));

    // --- system_box_status ---
    Object sbs = a.get("system_box_status");
    System.out.println("\n=== system_box_status : " + sbs.getClass().getName() + " len=" + Array.getLength(sbs));
    for (int i = 0; i < Math.min(6, Array.getLength(sbs)); i++) {
      Object row = Array.get(sbs, i);
      System.out.println("  [" + i + "] " + (row == null ? "null" : row.getClass().getName()));
      if (row != null) allFields(row, "        ");
    }

    // --- SSD boxes ---
    Object ssd = a.get("SSD");
    Object boxes = fld(ssd, "boxes");
    System.out.println("\n=== SSD.boxes : len=" + Array.getLength(boxes) + "  (element: " + Array.get(boxes, 0).getClass().getName() + ")");
    for (int i = 0; i < Math.min(5, Array.getLength(boxes)); i++) {
      System.out.println("  box[" + i + "]");
      allFields(Array.get(boxes, i), "        ");
    }
    // tally box kinds across the whole SSD
    Map<String,Integer> kinds = new TreeMap<>();
    for (int i = 0; i < Array.getLength(boxes); i++) {
      Object bx = Array.get(boxes, i);
      Object kind = fld(bx, "kind"); if (kind == null) kind = fld(bx, "type"); if (kind == null) kind = fld(bx, "category");
      kinds.merge(S(kind), 1, Integer::sum);
    }
    System.out.println("\n  box kind tally: " + kinds);
  }
}
