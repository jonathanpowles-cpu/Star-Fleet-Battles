import java.io.*; import java.nio.file.*; import java.util.*; import java.lang.reflect.*;
public class SeekProbe {
  static Object fld(Object o,String n) throws Exception { if(o==null)return null;
    for(Class<?> c=o.getClass(); c!=null; c=c.getSuperclass())
      try{Field f=c.getDeclaredField(n);f.setAccessible(true);return f.get(o);}catch(NoSuchFieldException e){} return null; }
  @SuppressWarnings("unchecked") static Hashtable<String,Object> A(Object el) throws Exception { return (Hashtable<String,Object>) fld(el,"attributes"); }
  static String S(Object o){ return o==null?"":String.valueOf(o); }
  static int shown=0;
  public static void main(String[] a) throws Exception {
    byte[] raw=Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(a[0]))).trim());
    Object b; try(ObjectInputStream in=new ObjectInputStream(new ByteArrayInputStream(raw))){ b=in.readObject(); }
    for(String v:new String[]{"onBoardPieces","offBoardPieces"}){
      Vector<?> vec=(Vector<?>) fld(b,v); if(vec==null) continue;
      for(Object p:vec){
        String cn=p.getClass().getSimpleName();
        if(!cn.contains("Seeking")) continue;
        Hashtable<String,Object> h=A(p);
        System.out.println("=== "+cn+" : "+S(h.get("Label"))+" ===");
        for(String k:new TreeSet<>(h.keySet())){
          String val=S(h.get(k));
          if(val.length()>60) val=val.substring(0,60);
          System.out.println("   "+String.format("%-24s",k)+"= "+val);
        }
        System.out.println();
        if(++shown>=2) return;
      }
    }
    System.out.println("no seeking-weapon pieces in this save");
  }
}
