import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.lang.reflect.*;

public class Probe {
  public static void main(String[] args) throws Exception {
    String txt = new String(Files.readAllBytes(Paths.get(args[0]))).trim();
    byte[] raw = Base64.getDecoder().decode(txt);
    System.out.println("decoded bytes: " + raw.length);
    ObjectInputStream in = new ObjectInputStream(new ByteArrayInputStream(raw));
    Object o = in.readObject();
    System.out.println("TOP-LEVEL: " + o.getClass().getName());
    Class<?> c = o.getClass();
    while (c != null && !c.equals(Object.class)) {
      for (Field f : c.getDeclaredFields()) {
        if (Modifier.isStatic(f.getModifiers())) continue;
        f.setAccessible(true);
        Object v = f.get(o);
        String d;
        if (v == null) d = "null";
        else if (v instanceof Collection) d = v.getClass().getSimpleName()+" size="+((Collection<?>)v).size();
        else if (v instanceof Map) d = v.getClass().getSimpleName()+" size="+((Map<?,?>)v).size();
        else d = v.getClass().getSimpleName()+" = "+String.valueOf(v).substring(0, Math.min(40, String.valueOf(v).length()));
        System.out.println("   " + c.getSimpleName()+"."+f.getName() + " : " + d);
      }
      c = c.getSuperclass();
    }
  }
}
