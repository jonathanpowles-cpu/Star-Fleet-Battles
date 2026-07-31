import java.io.*; import java.nio.file.*; import java.util.*; import java.lang.reflect.*;
public class TmplIndex {
  static Object fld(Object o,String n) throws Exception { if(o==null)return null;
    for(Class<?> c=o.getClass(); c!=null; c=c.getSuperclass())
      try{Field f=c.getDeclaredField(n);f.setAccessible(true);return f.get(o);}catch(NoSuchFieldException e){} return null; }
  @SuppressWarnings("unchecked") static Hashtable<String,Object> A(Object el) throws Exception { return (Hashtable<String,Object>) fld(el,"attributes"); }
  static String S(Object o){ return o==null?"":String.valueOf(o); }
  public static void main(String[] a) throws Exception {
    for(String path:a){
      byte[] raw=Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(path))).trim());
      Object b; try(ObjectInputStream in=new ObjectInputStream(new ByteArrayInputStream(raw))){ b=in.readObject(); }
      System.out.println("\n##### "+path.substring(path.lastIndexOf('/')+1));
      for(String v:new String[]{"onBoardPieces","offBoardPieces","discardedPieces"}){
        Vector<?> vec=(Vector<?>) fld(b,v); if(vec==null||vec.isEmpty()) continue;
        Map<String,Integer> kinds=new TreeMap<>();
        for(Object p:vec) kinds.merge(p.getClass().getSimpleName(),1,Integer::sum);
        System.out.println("  ["+v+"] "+vec.size()+"  kinds="+kinds);
        for(Object p:vec){
          Hashtable<String,Object> h=A(p); if(h==null) continue;
          Object ssd=h.get("SSD");
          int boxes=0; String ttl="";
          if(ssd!=null){ Object bx=fld(ssd,"boxes"); if(bx!=null) boxes=Array.getLength(bx); ttl=S(fld(ssd,"title")); }
          System.out.println(String.format("      %-26s type=%-7s race=%-7s ssdBoxes=%-3d %s",
            S(h.get("Label")), S(h.get("ship_type")), S(h.get("race")), boxes, ttl));
        }
      }
    }
  }
}
