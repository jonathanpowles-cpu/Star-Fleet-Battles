import java.io.*; import java.nio.file.*; import java.util.*; import java.lang.reflect.*;
public class TIcheck {
  static Object fld(Object o,String n) throws Exception { if(o==null)return null;
    for(Class<?> c=o.getClass(); c!=null; c=c.getSuperclass())
      try{Field f=c.getDeclaredField(n);f.setAccessible(true);return f.get(o);}catch(NoSuchFieldException e){} return null; }
  public static void main(String[] a) throws Exception {
    byte[] raw=Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(a[0]))).trim());
    Object b; try(ObjectInputStream in=new ObjectInputStream(new ByteArrayInputStream(raw))){ b=in.readObject(); }
    Object sh=fld(b,"sharedAttributes");
    Object ti=((Map<?,?>)sh).get("current_turn_impulse");
    System.out.println("current_turn_impulse class = "+(ti==null?"null":ti.getClass().getName())+"  value="+ti);
  }
}
