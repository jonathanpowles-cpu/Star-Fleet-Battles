import java.io.*; import java.nio.file.*; import java.util.*; import java.lang.reflect.*;
public class Coverage {
  static Object fld(Object o,String n) throws Exception { if(o==null)return null;
    for(Class<?> c=o.getClass(); c!=null; c=c.getSuperclass())
      try{Field f=c.getDeclaredField(n);f.setAccessible(true);return f.get(o);}catch(NoSuchFieldException e){} return null; }
  @SuppressWarnings("unchecked") static Hashtable<String,Object> A(Object el) throws Exception { return (Hashtable<String,Object>) fld(el,"attributes"); }
  static String S(Object o){ return o==null?"":String.valueOf(o); }
  static String base(String t){ return t.replaceAll("[+puB]+$",""); }  // CW+uB -> CW
  public static void main(String[] a) throws Exception {
    // tactical templates available (base ship_type -> example)
    Map<String,String> tmpl=new TreeMap<>();
    for(int i=1;i<a.length;i++){
      byte[] raw=Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(a[i]))).trim());
      Object b; try(ObjectInputStream in=new ObjectInputStream(new ByteArrayInputStream(raw))){ b=in.readObject(); }
      for(String v:new String[]{"onBoardPieces","offBoardPieces","discardedPieces"}){
        Vector<?> vec=(Vector<?>) fld(b,v); if(vec==null) continue;
        for(Object p:vec) if(p.getClass().getName().contains("ShipAttributes")){
          String st=S(A(p).get("ship_type")); tmpl.putIfAbsent(base(st), st+" ("+S(A(p).get("Label"))+")"); }
      }
    }
    System.out.println("=== TACTICAL TEMPLATES available (base type -> example) ===");
    for(Map.Entry<String,String> e:tmpl.entrySet()) System.out.println("   "+e.getKey()+" <- "+e.getValue());
    // campaign ship types needed at 0707 and 0703
    byte[] raw=Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(a[0]))).trim());
    Object b; try(ObjectInputStream in=new ObjectInputStream(new ByteArrayInputStream(raw))){ b=in.readObject(); }
    Vector<?> on=(Vector<?>) fld(b,"onBoardPieces");
    Set<String> need=new TreeSet<>();
    for(Object p:on){ Hashtable<String,Object> h=A(p);
      Object bl=h.get("boardLocation"); if(bl==null) continue;
      Object x=fld(bl,"x"),y=fld(bl,"y"); String hex=String.format("%02d%02d",((Number)x).intValue(),((Number)y).intValue());
      if((hex.equals("0707")||hex.equals("0703")||hex.equals("0701")) && !"Fleet".equals(S(h.get("unit_type"))))
        need.add(base(S(h.get("unit_type")))+" ("+S(h.get("unit_type"))+")"); }
    System.out.println("\n=== CAMPAIGN types needed at 0707/0703/0701 ===");
    Set<String> miss=new TreeSet<>();
    for(String n:need){ String bt=n.split(" ")[0]; boolean ok=tmpl.containsKey(bt);
      System.out.println("   "+(ok?"OK  ":"MISS")+" "+n); if(!ok) miss.add(bt); }
    System.out.println("\nMISSING templates: "+miss);
  }
}
