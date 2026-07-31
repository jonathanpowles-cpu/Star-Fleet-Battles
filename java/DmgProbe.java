import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.lang.reflect.*;

public class DmgProbe {
  static Object fld(Object o, String n) throws Exception {
    if (o == null) return null;
    for (Class<?> c = o.getClass(); c != null; c = c.getSuperclass())
      try { Field f = c.getDeclaredField(n); f.setAccessible(true); return f.get(o); } catch (NoSuchFieldException e) {}
    return null;
  }
  @SuppressWarnings("unchecked")
  static Hashtable<String,Object> A(Object el) throws Exception { return (Hashtable<String,Object>) fld(el, "attributes"); }
  static String S(Object o) { return o == null ? "" : String.valueOf(o); }

  public static void main(String[] args) throws Exception {
    byte[] raw = Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(args[0]))).trim());
    Object board;
    try (ObjectInputStream in = new ObjectInputStream(new ByteArrayInputStream(raw))) { board = in.readObject(); }
    for (String vecName : new String[]{"onBoardPieces","offBoardPieces","discardedPieces"}) {
      Vector<?> v = (Vector<?>) fld(board, vecName);
      System.out.println("\n##### " + vecName + " = " + (v == null ? 0 : v.size()));
      if (v == null) continue;
      Map<String,Integer> kinds = new TreeMap<>();
      for (Object p : v) kinds.merge(p.getClass().getSimpleName(), 1, Integer::sum);
      System.out.println("   kinds: " + kinds);
      int shown = 0;
      for (Object p : v) {
        if (!p.getClass().getName().contains("ShipAttributes")) continue;
        Hashtable<String,Object> a = A(p);
        System.out.println("   " + S(a.get("Label")) + " | " + S(a.get("race")) + " | ship_type=" + S(a.get("ship_type"))
          + " | owner=" + S(a.get("owner")) + " | ID=" + S(a.get("ID")));
        Object sbs = a.get("system_box_status");
        Object ssd = a.get("SSD");
        Object cr  = a.get("f&e_command_rating");
        Object crew= a.get("crew_units");
        System.out.println("        f&e_command_rating=" + S(cr) + "  crew_units=" + S(crew)
          + "  bpv=" + S(a.get("bpv")) + "/" + S(a.get("base_bpv"))
          + "  shipDefFile=" + S(a.get("shipDefFile")) + "  ss_id=" + S(a.get("ss_id")));
        if (sbs != null) {
          String t = S(sbs);
          System.out.println("        system_box_status : " + sbs.getClass().getSimpleName()
            + (sbs.getClass().isArray() ? " len=" + Array.getLength(sbs) : "")
            + " -> " + t.substring(0, Math.min(150, t.length())));
        }
        if (ssd != null) {
          System.out.println("        SSD : " + ssd.getClass().getName());
          Object boxes = fld(ssd, "boxes");
          if (boxes == null) boxes = fld(ssd, "box");
          if (boxes != null) System.out.println("            boxes -> " + boxes.getClass().getSimpleName()
            + (boxes instanceof Collection ? " size=" + ((Collection<?>) boxes).size()
               : boxes.getClass().isArray() ? " len=" + Array.getLength(boxes) : ""));
          for (Class<?> c = ssd.getClass(); c != null && !c.equals(Object.class); c = c.getSuperclass())
            for (Field f : c.getDeclaredFields()) {
              if (Modifier.isStatic(f.getModifiers())) continue;
              f.setAccessible(true); Object val = f.get(ssd);
              String d = val == null ? "null" : (val instanceof Collection ? val.getClass().getSimpleName()+" size="+((Collection<?>)val).size()
                : val.getClass().isArray() ? val.getClass().getSimpleName()+" len="+Array.getLength(val)
                : val.getClass().getSimpleName()+"="+S(val).substring(0, Math.min(28, S(val).length())));
              System.out.println("            SSD." + f.getName() + " : " + d);
            }
        }
        if (++shown >= 2) break;
      }
    }
  }
}
