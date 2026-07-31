import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.lang.reflect.*;

/** Assign per-race ownership so the human (Skylark) owns Lyran and a bot (KzintiAI)
 *  owns Kzinti — to test whether the client applies a second player's live moves. */
public class SetOwner {
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
    String lyOwner = args[2], kzOwner = args[3];
    byte[] raw = Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(args[0]))).trim());
    Object board;
    try (ObjectInputStream in = new ObjectInputStream(new ByteArrayInputStream(raw))) { board = in.readObject(); }
    Vector<Object> on = (Vector<Object>) fld(board, "onBoardPieces");
    int ly = 0, kz = 0;
    for (Object p : on) {
      Hashtable<String,Object> h = A(p);
      String race = S(h.get("race"));
      String owner = race.equals("Lyran") ? lyOwner : kzOwner;
      h.put("owner", owner);
      h.put("player", owner);
      if (race.equals("Lyran")) ly++; else kz++;
    }
    System.out.println("set owner: Lyran(" + ly + ")=" + lyOwner + "  Kzinti(" + kz + ")=" + kzOwner);
    ByteArrayOutputStream b = new ByteArrayOutputStream();
    try (ObjectOutputStream out = new ObjectOutputStream(b)) { out.writeObject(board); }
    byte[] bytes = b.toByteArray();
    try (ObjectInputStream in = new ObjectInputStream(new ByteArrayInputStream(bytes))) { in.readObject(); }
    Files.write(Paths.get(args[1]), Base64.getEncoder().encode(bytes));
    System.out.println("WROTE " + args[1] + "  (reload-verified)");
  }
}
