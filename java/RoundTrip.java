import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.lang.reflect.*;

public class RoundTrip {
  static Object fld(Object o, String name) throws Exception {
    Class<?> c = o.getClass();
    while (c != null) {
      try { Field f = c.getDeclaredField(name); f.setAccessible(true); return f.get(o); }
      catch (NoSuchFieldException e) { c = c.getSuperclass(); }
    }
    throw new NoSuchFieldException(name);
  }
  public static void main(String[] args) throws Exception {
    byte[] raw = Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(args[0]))).trim());
    Object board;
    try (ObjectInputStream in = new ObjectInputStream(new ByteArrayInputStream(raw))) {
      board = in.readObject();
    }
    System.out.println("LOAD  ok: on=" + ((Vector<?>)fld(board,"onBoardPieces")).size()
                     + " discarded=" + ((Vector<?>)fld(board,"discardedPieces")).size());
    // re-serialize unmodified
    ByteArrayOutputStream bos = new ByteArrayOutputStream();
    try (ObjectOutputStream out = new ObjectOutputStream(bos)) { out.writeObject(board); }
    byte[] again = bos.toByteArray();
    System.out.println("WRITE ok: " + raw.length + " -> " + again.length + " bytes"
                     + (Arrays.equals(raw, again) ? "  (BYTE-IDENTICAL)" : "  (differs - re-encoded)"));
    // reload our own output to prove it is well-formed
    try (ObjectInputStream in2 = new ObjectInputStream(new ByteArrayInputStream(again))) {
      Object b2 = in2.readObject();
      System.out.println("RELOAD ok: on=" + ((Vector<?>)fld(b2,"onBoardPieces")).size()
                       + " discarded=" + ((Vector<?>)fld(b2,"discardedPieces")).size());
    }
    Files.write(Paths.get(args[1]), Base64.getEncoder().encode(again));
    System.out.println("wrote round-tripped save -> " + args[1]);
  }
}
