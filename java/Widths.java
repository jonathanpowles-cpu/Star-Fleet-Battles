import java.io.*; import java.nio.file.*; import java.util.*; import java.lang.reflect.*;
public class Widths {
  static Object fld(Object o,String n) throws Exception { if(o==null)return null;
    for(Class<?> c=o.getClass(); c!=null; c=c.getSuperclass())
      try{Field f=c.getDeclaredField(n);f.setAccessible(true);return f.get(o);}catch(NoSuchFieldException e){} return null; }
  static Object load(String p) throws Exception {
    byte[] raw=Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(p))).trim());
    try(ObjectInputStream in=new ObjectInputStream(new ByteArrayInputStream(raw))){ return in.readObject(); } }
  static String S(Object o){ return o==null?"":String.valueOf(o); }
  public static void main(String[] a) throws Exception {
    for(String p:a){
      Object b=load(p);
      System.out.println("\n### "+p.substring(p.lastIndexOf('/')+1));
      for(String f:new String[]{"onBoardWidths","offBoardWidths","discardedWidths"}){
        Object v=fld(b,f);
        System.out.println("   "+f+" = "+(v==null?"null":"int["+Array.getLength(v)+"] "+Arrays.toString((int[])v)));
      }
      for(String f:new String[]{"props","rtracker","windowBounds"}){
        Object v=fld(b,f);
        System.out.println("   "+f+" = "+(v==null?"null":v.getClass().getSimpleName()+" -> "+S(v).substring(0,Math.min(80,S(v).length()))));
      }
      Vector<?> on=(Vector<?>) fld(b,"onBoardPieces");
      System.out.println("   onBoardPieces="+(on==null?0:on.size()));
    }
  }
}
