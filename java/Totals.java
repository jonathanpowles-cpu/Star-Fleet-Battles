import java.io.*; import java.nio.file.*; import java.util.*; import java.lang.reflect.*;
public class Totals {
  static Object fld(Object o,String n) throws Exception {
    if(o==null) return null;
    for(Class<?> c=o.getClass(); c!=null; c=c.getSuperclass())
      try{ Field f=c.getDeclaredField(n); f.setAccessible(true); return f.get(o);}catch(NoSuchFieldException e){}
    return null; }
  @SuppressWarnings("unchecked")
  static Hashtable<String,Object> A(Object el) throws Exception { return (Hashtable<String,Object>) fld(el,"attributes"); }
  public static void main(String[] a) throws Exception {
    for(String f: a){
      byte[] raw=Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(f))).trim());
      Object b; try(ObjectInputStream in=new ObjectInputStream(new ByteArrayInputStream(raw))){ b=in.readObject(); }
      System.out.println("### "+f.substring(f.lastIndexOf('/')+1));
      for(String vn: new String[]{"onBoardPieces","discardedPieces"}){
        Vector<?> v=(Vector<?>) fld(b,vn); if(v==null) continue;
        for(Object p:v){
          if(!p.getClass().getName().contains("ShipAttributes")) continue;
          Object ssd=A(p).get("SSD"); if(ssd==null) continue;
          String t=String.valueOf(ssd);
          System.out.printf("  %-8s %-22s %-6s %s%n", vn.replace("Pieces",""),
            String.valueOf(A(p).get("Label")), String.valueOf(A(p).get("ship_type")), t);
        }
      }
    }
  }
}
