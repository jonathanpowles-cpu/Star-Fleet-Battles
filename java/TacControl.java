import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.lang.reflect.*;

/** Demonstrate the AI-referee control surface on a tactical .SFB via save-edit:
 *  apply damage (boxStatus->2), advance the impulse, and change a ship's speed. */
public class TacControl {
  static Object fld(Object o, String n) throws Exception {
    if (o == null) return null;
    for (Class<?> c = o.getClass(); c != null; c = c.getSuperclass())
      try { Field f = c.getDeclaredField(n); f.setAccessible(true); return f.get(o); } catch (NoSuchFieldException e) {}
    return null;
  }
  static void setfld(Object o, String n, Object v) throws Exception {
    for (Class<?> c = o.getClass(); c != null; c = c.getSuperclass())
      try { Field f = c.getDeclaredField(n); f.setAccessible(true); f.set(o, v); return; } catch (NoSuchFieldException e) {}
  }
  @SuppressWarnings("unchecked")
  static Hashtable<String,Object> A(Object el) throws Exception { return (Hashtable<String,Object>) fld(el, "attributes"); }
  static String S(Object o) { return o == null ? "" : String.valueOf(o); }
  static Object find(Vector<?> v, String lbl) throws Exception {
    for (Object p : v) { Hashtable<String,Object> h = A(p); if (h != null && S(h.get("Label")).contains(lbl)) return p; }
    return null;
  }
  static int damage(Object ship, int n) throws Exception {   // mark n boxes destroyed
    Object ssd = A(ship).get("SSD");
    Object boxes = fld(ssd, "boxes");
    int done = 0;
    for (int i = 0; i < Array.getLength(boxes) && done < n; i++) {
      Object box = Array.get(boxes, i);
      Object bs = fld(box, "boxStatus");
      if (!(bs instanceof int[])) continue;
      int[] arr = (int[]) bs;
      for (int k = 0; k < arr.length && done < n; k++) if (arr[k] == 1) { arr[k] = 2; done++; }
      // numOfBoxes = current intact count (what the SSD's damage count actually reads)
      int intact = 0; for (int v : arr) if (v != 2) intact++;
      setfld(box, "numOfBoxes", Integer.valueOf(intact));
    }
    return done;
  }
  static int[] tally(Object ship) throws Exception {
    Object boxes = fld(A(ship).get("SSD"), "boxes");
    int t = 0, d = 0;
    for (int i = 0; i < Array.getLength(boxes); i++) { Object bs = fld(Array.get(boxes, i), "boxStatus");
      if (bs instanceof int[]) for (int v : (int[]) bs) { t++; if (v == 2) d++; } }
    return new int[]{t, d};
  }

  @SuppressWarnings("unchecked")
  public static void main(String[] args) throws Exception {
    byte[] raw = Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(args[0]))).trim());
    Object board;
    try (ObjectInputStream in = new ObjectInputStream(new ByteArrayInputStream(raw))) { board = in.readObject(); }
    Vector<Object> on = (Vector<Object>) fld(board, "onBoardPieces");

    // 1) DAMAGE: BC Starfire takes 45 boxes
    Object star = find(on, "Starfire");
    int did = damage(star, 45);
    int[] t = tally(star);
    System.out.println("DAMAGE  BC Starfire: destroyed " + did + " boxes -> now " + t[1] + "/" + t[0] + " destroyed");

    // 2) ADVANCE the impulse: turn 1, impulse 8
    Map<String,Object> shared = (Map<String,Object>) fld(board, "sharedAttributes");
    Object ti = shared.get("current_turn_impulse");
    setfld(ti, "turn", Integer.valueOf(1));
    setfld(ti, "impulse", Integer.valueOf(8));
    setfld(board, "currentTurn", 1);
    setfld(board, "currentImpulse", 8);
    shared.put("first_impulse", Boolean.FALSE);
    System.out.println("IMPULSE advanced -> turn " + fld(ti, "turn") + " impulse " + fld(ti, "impulse"));

    // 3) SPEED: CL Mystic to speed 24
    Object myst = find(on, "Mystic");
    A(myst).put("current_speed", 24);
    A(myst).put("previous_speed", 24);
    A(myst).put("speed_plot", "1-32=24");
    System.out.println("SPEED   CL Mystic -> 24 (" + S(A(myst).get("speed_plot")) + ")");

    ByteArrayOutputStream b = new ByteArrayOutputStream();
    try (ObjectOutputStream out = new ObjectOutputStream(b)) { out.writeObject(board); }
    byte[] bytes = b.toByteArray();
    try (ObjectInputStream in = new ObjectInputStream(new ByteArrayInputStream(bytes))) { in.readObject(); }
    Files.write(Paths.get(args[1]), Base64.getEncoder().encode(bytes));
    System.out.println("WROTE " + args[1] + "  (reload-verified)");
  }
}
