import java.lang.reflect.*;
import java.util.*;

public class Inspect {
  static void dump(String cn) {
    try {
      Class<?> c = Class.forName(cn);
      System.out.println("\n================ " + cn);
      System.out.print("  superclasses:");
      for (Class<?> s = c.getSuperclass(); s != null && !s.equals(Object.class); s = s.getSuperclass())
        System.out.print("  <- " + s.getName());
      System.out.println();
      System.out.println("  interfaces: " + Arrays.toString(c.getInterfaces()));
      System.out.println("  -- constructors --");
      for (Constructor<?> k : c.getDeclaredConstructors())
        System.out.println("     " + Modifier.toString(k.getModifiers()) + " (" +
          Arrays.stream(k.getParameterTypes()).map(Class::getSimpleName).reduce((a,b)->a+", "+b).orElse("") + ")");
      System.out.println("  -- declared fields --");
      for (Class<?> k = c; k != null && !k.equals(Object.class); k = k.getSuperclass())
        for (Field f : k.getDeclaredFields()) {
          if (Modifier.isStatic(f.getModifiers())) continue;
          System.out.println("     " + k.getSimpleName()+"."+f.getName() + " : " + f.getType().getSimpleName());
        }
      System.out.println("  -- key methods (set/add/create/init) --");
      for (Method m : c.getMethods()) {
        String n = m.getName();
        if (n.startsWith("set")||n.startsWith("add")||n.startsWith("create")||n.startsWith("init")||n.startsWith("newInstance")||n.equals("clone"))
          System.out.println("     " + m.getReturnType().getSimpleName() + " " + n + "(" +
            Arrays.stream(m.getParameterTypes()).map(Class::getSimpleName).reduce((a,b)->a+", "+b).orElse("") + ")");
      }
    } catch (Throwable t) { System.out.println("  ERR " + cn + " : " + t); }
  }
  public static void main(String[] a) {
    for (String cn : a) dump(cn);
  }
}
