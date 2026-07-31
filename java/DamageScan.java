import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.lang.reflect.*;

public class DamageScan {
  static Object fld(Object o, String n) throws Exception {
    if (o == null) return null;
    for (Class<?> c = o.getClass(); c != null; c = c.getSuperclass())
      try { Field f = c.getDeclaredField(n); f.setAccessible(true); return f.get(o); } catch (NoSuchFieldException e) {}
    return null;
  }
  @SuppressWarnings("unchecked")
  static Hashtable<String,Object> A(Object el) throws Exception { return (Hashtable<String,Object>) fld(el, "attributes"); }
  static String S(Object o) { return o == null ? "" : String.valueOf(o); }

  static int[] boxStatus(Object box) throws Exception {
    Object bs = fld(box, "boxStatus");
    return bs instanceof int[] ? (int[]) bs : null;
  }
  static void scanShip(Object ship, String file) throws Exception {
    Hashtable<String,Object> a = A(ship);
    Object ssd = a.get("SSD");
    if (ssd == null) return;
    Object boxes = fld(ssd, "boxes");
    int total = 0, destroyed = 0;
    Map<Integer,int[]> byState = new TreeMap<>(); // value -> [count]
    for (int i = 0; i < Array.getLength(boxes); i++) {
      int[] bs = boxStatus(Array.get(boxes, i));
      if (bs == null) continue;
      for (int v : bs) {
        total++;
        byState.computeIfAbsent(v, k -> new int[1])[0]++;
        if (v != 0) destroyed++;   // hypothesis: 0 = intact
      }
    }
    System.out.println(String.format("  %-22s %-5s ssd=[%s]  boxes total=%d destroyed(non-0)=%d  valueHistogram=%s",
      S(a.get("Label")), S(a.get("ship_type")), S(ssd), total, destroyed, hist(byState)));
  }
  static String hist(Map<Integer,int[]> m) { StringBuilder b = new StringBuilder(); for (Map.Entry<Integer,int[]> e : m.entrySet()) b.append(e.getKey()).append(":").append(e.getValue()[0]).append(" "); return b.toString().trim(); }

  public static void main(String[] args) throws Exception {
    for (String path : args) {
      byte[] raw = Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(path))).trim());
      Object board;
      try (ObjectInputStream in = new ObjectInputStream(new ByteArrayInputStream(raw))) { board = in.readObject(); }
      System.out.println("\n##### " + path.substring(path.lastIndexOf('/') + 1));
      for (String v : new String[]{"onBoardPieces","offBoardPieces","discardedPieces"}) {
        Vector<?> vec = (Vector<?>) fld(board, v);
        if (vec == null) continue;
        boolean any = false;
        for (Object p : vec) if (p.getClass().getName().contains("ShipAttributes")) { any = true; break; }
        if (!any) continue;
        System.out.println(" [" + v + "]");
        for (Object p : vec) if (p.getClass().getName().contains("ShipAttributes")) scanShip(p, path);
      }
    }
  }
}
