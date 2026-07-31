import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.lang.reflect.*;

public class PowerBoxes {
  static Object fld(Object o, String n) throws Exception {
    if (o == null) return null;
    for (Class<?> c = o.getClass(); c != null; c = c.getSuperclass())
      try { Field f = c.getDeclaredField(n); f.setAccessible(true); return f.get(o); } catch (NoSuchFieldException e) {}
    return null;
  }
  @SuppressWarnings("unchecked")
  static Hashtable<String,Object> A(Object el) throws Exception { return (Hashtable<String,Object>) fld(el, "attributes"); }
  static String S(Object o) { return o == null ? "" : String.valueOf(o); }
  static int I(Object o) { try { return ((Number) o).intValue(); } catch (Exception e) { return 0; } }

  static final Map<Integer,String> NAMES = new HashMap<>();
  static { String[][] n = {
    {"6","L Warp"},{"12","R Warp"},{"17","C Warp"},{"16","Imp"},{"20","APR"},{"32","AWR"},
    {"18","Batt"},{"26","Shield"},{"5","R Hull"},{"11","F Hull"},{"57","C Hull"},
    {"29","Disr"},{"33","Ph-1"},{"35","Ph-3"},{"14","Drn"},{"62","Drn-A"},{"22","Sen"},
    {"3","Scan"},{"8","Trac"},{"7","Trans"},{"4","Dam Con"},{"1","Brdg"},{"2","Flag"},
    {"9","Shut"},{"10","Lab"},{"13","Ex Dam"},{"23","Aux"},{"24","Emer"},{"21","Probe"},
    {"40","Emer Imp"},{"41","Sec"},{"59","Repr"}
  }; for (String[] p : n) NAMES.put(Integer.parseInt(p[0]), p[1]); }

  public static void main(String[] args) throws Exception {
    byte[] raw = Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(args[0]))).trim());
    Object b;
    try (ObjectInputStream in = new ObjectInputStream(new ByteArrayInputStream(raw))) { b = in.readObject(); }
    Vector<?> on = (Vector<?>) fld(b, "onBoardPieces");
    for (Object p : on) {
      Hashtable<String,Object> h = A(p);
      if (!S(h.get("Label")).contains(args[1])) continue;
      System.out.println("=== " + S(h.get("Label")) + " box kinds (kind name: cur/max) ===");
      Object boxes = fld(h.get("SSD"), "boxes");
      Map<Integer,int[]> byKind = new TreeMap<>();
      for (int i = 0; i < Array.getLength(boxes); i++) {
        Object bx = Array.get(boxes, i);
        int kind = I(fld(bx, "kind")), num = I(fld(bx, "numOfBoxes")), max = I(fld(bx, "maxNumOfBoxes"));
        byKind.computeIfAbsent(kind, z -> new int[2]);
        byKind.get(kind)[0] += num; byKind.get(kind)[1] += max;
      }
      for (Map.Entry<Integer,int[]> e : byKind.entrySet())
        System.out.println("  kind " + e.getKey() + " (" + NAMES.getOrDefault(e.getKey(), "?") + "): "
          + e.getValue()[0] + "/" + e.getValue()[1]);
    }
  }
}
