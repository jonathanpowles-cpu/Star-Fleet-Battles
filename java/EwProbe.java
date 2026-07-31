import java.io.*; import java.nio.file.*; import java.util.*; import java.lang.reflect.*;
public class EwProbe {
  static Object fld(Object o,String n) throws Exception { if(o==null) return null;
    for(Class<?> c=o.getClass();c!=null;c=c.getSuperclass())
      try{Field f=c.getDeclaredField(n);f.setAccessible(true);return f.get(o);}catch(NoSuchFieldException e){}
    return null; }
  @SuppressWarnings("unchecked")
  public static void main(String[] a) throws Exception {
    byte[] raw=Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(a[0]))).trim());
    Object board; try(ObjectInputStream in=new ObjectInputStream(new ByteArrayInputStream(raw))){board=in.readObject();}
    for(Object p:(Vector<?>)fld(board,"onBoardPieces")){
      if(!p.getClass().getName().contains("ShipAttributes")) continue;
      Hashtable<String,Object> at=(Hashtable<String,Object>)fld(p,"attributes");
      System.out.println("\n=== "+at.get("Label")+" ===");
      for(Map.Entry<String,Object> e:new TreeMap<>(at).entrySet()){
        String k=e.getKey().toLowerCase();
        if(!(k.contains("ew")||k.contains("ecm")||k.contains("eccm")||k.contains("scout")
             ||k.contains("lend")||k.contains("loan")||k.contains("sensor")||k.contains("lock"))) continue;
        String v=String.valueOf(e.getValue()); if(v.length()>110) v=v.substring(0,110)+"...";
        System.out.println("   "+e.getKey()+" = ["+v+"]"); } } }
}
