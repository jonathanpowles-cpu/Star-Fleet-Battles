import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.lang.reflect.*;

/** Build Lyran fleet stack markers mirroring the user's Kzinti convention:
 *  a piece with unit_type="Fleet", IsStackPiece=true, PiecesInStack=<csv of member IDs>,
 *  and each member's StackPiece pointing back at it. */
public class LyranStacks {
  static boolean APPLY = false;
  static Object fld(Object o, String n) throws Exception {
    if (o == null) return null;
    for (Class<?> c = o.getClass(); c != null; c = c.getSuperclass())
      try { Field f = c.getDeclaredField(n); f.setAccessible(true); return f.get(o); } catch (NoSuchFieldException e) {}
    return null;
  }
  @SuppressWarnings("unchecked")
  static Hashtable<String,Object> A(Object el) throws Exception { return (Hashtable<String,Object>) fld(el, "attributes"); }
  static String S(Object o) { return o == null ? "" : String.valueOf(o); }
  static String hex(Object p) throws Exception {
    Object bl = A(p).get("boardLocation");
    if (bl == null) return "-";
    Object x = fld(bl, "x"), y = fld(bl, "y");
    if (x == null || y == null) return "-";
    return String.format("%02d%02d", ((Number) x).intValue(), ((Number) y).intValue());
  }
  static Object deepCopy(Object o) throws Exception {
    ByteArrayOutputStream b = new ByteArrayOutputStream();
    try (ObjectOutputStream out = new ObjectOutputStream(b)) { out.writeObject(o); }
    try (ObjectInputStream in = new ObjectInputStream(new ByteArrayInputStream(b.toByteArray()))) { return in.readObject(); }
  }

  @SuppressWarnings("unchecked")
  public static void main(String[] args) throws Exception {
    APPLY = args.length > 1 && args[1].equals("--apply");
    byte[] raw = Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(args[0]))).trim());
    Object board;
    try (ObjectInputStream in = new ObjectInputStream(new ByteArrayInputStream(raw))) { board = in.readObject(); }
    Vector<Object> on = (Vector<Object>) fld(board, "onBoardPieces");

    // hex -> stack label
    Map<String,String> names = new LinkedHashMap<>();
    names.put("0504", "Red Claw Fleet");
    names.put("0502", "Red Claw Fleet - Predator");
    names.put("0604", "Red Claw Fleet - Golden Fang");
    names.put("0408", "Home Fleet");
    names.put("0608", "Home Fleet - Foremost");
    names.put("0707", "Home Fleet - Silver Moon");

    // Prefer an existing LYRAN stack marker as template (correct race + the generic
    // counter the user already gets); fall back to any stack marker.
    Object tmpl = null;
    for (Object p : on)
      if ("true".equals(S(A(p).get("IsStackPiece"))) && "Lyran".equals(S(A(p).get("race")))) { tmpl = p; break; }
    if (tmpl == null)
      for (Object p : on) if ("true".equals(S(A(p).get("IsStackPiece")))) { tmpl = p; break; }
    if (tmpl == null) { System.out.println("no existing stack marker to model on - abort"); return; }
    System.out.println("modelling on: " + S(A(tmpl).get("Label")) + " (" + S(A(tmpl).get("race"))
      + ", counter=" + S(A(tmpl).get("counter_file")) + ")");

    // highest existing numeric ID -> allocate new ones after it
    long maxId = 0;
    for (Object p : on) {
      String id = S(A(p).get("ID"));
      int c = id.lastIndexOf(':');
      if (c > 0) try { maxId = Math.max(maxId, Long.parseLong(id.substring(c + 1))); } catch (Exception e) {}
    }
    System.out.println("next free ID: Skylark:" + (maxId + 1));

    // clear the legacy groups tag
    for (Object p : on) {
      Hashtable<String,Object> h = A(p);
      if (!S(h.get("groups")).isBlank()) {
        System.out.println("  CLEAR groups=[" + S(h.get("groups")) + "] on " + S(h.get("Label")));
        h.put("groups", "");
      }
    }

    List<Object> created = new ArrayList<>();
    for (Map.Entry<String,String> e : names.entrySet()) {
      String hx = e.getKey(), label = e.getValue();
      // Lyran ships at this hex that are not themselves stack markers
      List<Object> members = new ArrayList<>();
      for (Object p : on) {
        Hashtable<String,Object> h = A(p);
        if (!"Lyran".equals(S(h.get("race")))) continue;
        if ("true".equals(S(h.get("IsStackPiece")))) continue;
        if ("Fleet".equals(S(h.get("unit_type")))) continue;
        if (!hex(p).equals(hx)) continue;
        members.add(p);
      }
      if (members.isEmpty()) { System.out.println("  (no Lyran ships at " + hx + " - skip)"); continue; }
      // is there already a Lyran stack marker here? (user made one at 0502)
      Object existing = null;
      for (Object p : on) {
        Hashtable<String,Object> h = A(p);
        if ("true".equals(S(h.get("IsStackPiece"))) && "Lyran".equals(S(h.get("race"))) && hex(p).equals(hx)) { existing = p; break; }
      }
      Object marker;
      if (existing != null) {
        marker = existing;
        System.out.println("  REUSE  " + hx + " existing marker '" + S(A(marker).get("Label")) + "' -> relabel '" + label + "'");
      } else {
        marker = deepCopy(tmpl);
        maxId++;
        Hashtable<String,Object> m = A(marker);
        m.put("ID", "Skylark:" + maxId);
        m.put("race", "Lyran");
        m.put("Name", "Lyran Fleet");
        // no Lyran fleet counter exists server-side; keep the template's art reference
        m.put("boardLocation", A(members.get(0)).get("boardLocation"));
        m.put("saved_boardLocation", A(members.get(0)).get("boardLocation"));
        on.add(marker);
        created.add(marker);
        System.out.println("  CREATE " + hx + " '" + label + "' id=Skylark:" + maxId);
      }
      Hashtable<String,Object> m = A(marker);
      m.put("Label", label);
      m.put("IsStackPiece", true);
      m.put("condition", 100);
      StringBuilder ids = new StringBuilder();
      for (Object mem : members) {
        if (ids.length() > 0) ids.append(",");
        ids.append(S(A(mem).get("ID")));
        A(mem).put("StackPiece", S(m.get("ID")));
      }
      m.put("PiecesInStack", ids.toString());
      System.out.println("         " + members.size() + " ships stacked");
    }

    System.out.println("\ncreated " + created.size() + " new marker(s); onBoard now " + on.size());
    if (APPLY) {
      ByteArrayOutputStream b = new ByteArrayOutputStream();
      try (ObjectOutputStream out = new ObjectOutputStream(b)) { out.writeObject(board); }
      byte[] bytes = b.toByteArray();
      try (ObjectInputStream in = new ObjectInputStream(new ByteArrayInputStream(bytes))) { in.readObject(); }
      Files.write(Paths.get(args[0] + ".new"), Base64.getEncoder().encode(bytes));
      System.out.println("WROTE " + args[0] + ".new  (" + bytes.length + " raw bytes, reload-verified)");
    }
  }
}
