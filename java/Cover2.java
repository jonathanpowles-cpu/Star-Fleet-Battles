import java.io.*; import java.nio.file.*; import java.util.*; import java.lang.reflect.*;
public class Cover2 {
  static Object fld(Object o,String n) throws Exception { if(o==null)return null;
    for(Class<?> c=o.getClass(); c!=null; c=c.getSuperclass())
      try{Field f=c.getDeclaredField(n);f.setAccessible(true);return f.get(o);}catch(NoSuchFieldException e){} return null; }
  @SuppressWarnings("unchecked") static Hashtable<String,Object> A(Object el) throws Exception { return (Hashtable<String,Object>) fld(el,"attributes"); }
  static String S(Object o){ return o==null?"":String.valueOf(o); }
  static String cls(String ut){ return ut.replaceAll("[+pP]+$",""); }
  static void collect(String path, Map<String,String> out) throws Exception {
    byte[] raw=Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(path))).trim());
    Object b; try(ObjectInputStream in=new ObjectInputStream(new ByteArrayInputStream(raw))){ b=in.readObject(); }
    for(String v:new String[]{"onBoardPieces","offBoardPieces","discardedPieces"}){
      Vector<?> vec=(Vector<?>) fld(b,v); if(vec==null) continue;
      for(Object p:vec){ if(!p.getClass().getName().contains("ShipAttributes")) continue;
        Hashtable<String,Object> h=A(p);
        String race=S(h.get("race")), st=S(h.get("ship_type"));
        Object ssd=h.get("SSD"); String ttl = ssd==null?"":S(fld(ssd,"title"));
        // key by LABEL (user labelled them by class) and by ship_type
        out.put(race+"|"+cls(S(h.get("Label"))), st+" :: "+ttl);
        out.putIfAbsent(race+"|"+cls(st), st+" :: "+ttl); }
    }
  }
  public static void main(String[] a) throws Exception {
    Map<String,String> tmpl=new TreeMap<>();
    collect(a[1],tmpl); collect(a[2],tmpl);
    // campaign classes needed
    byte[] raw=Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(a[0]))).trim());
    Object b; try(ObjectInputStream in=new ObjectInputStream(new ByteArrayInputStream(raw))){ b=in.readObject(); }
    Map<String,String> need=new TreeMap<>();
    for(String v:new String[]{"onBoardPieces","discardedPieces"}){
      Vector<?> vec=(Vector<?>) fld(b,v); if(vec==null) continue;
      for(Object p:vec){ Hashtable<String,Object> h=A(p);
        if("Fleet".equals(S(h.get("unit_type")))) continue;
        String race=S(h.get("race")); if(race.isEmpty()) continue;
        need.putIfAbsent(race+"|"+cls(S(h.get("unit_type"))), S(h.get("Label"))); }
    }
    System.out.println("=== COVERAGE: campaign class -> template ===");
    List<String> miss=new ArrayList<>();
    for(Map.Entry<String,String> e:need.entrySet()){
      String hit=tmpl.get(e.getKey());
      // try race-agnostic for generics (BATS/FRD)
      String bare = e.getKey().substring(e.getKey().indexOf('|')+1);
      if(hit==null) for(Map.Entry<String,String> t:tmpl.entrySet())
        if(t.getKey().endsWith("|"+bare)){ hit=t.getValue()+"   (from "+t.getKey().substring(0,t.getKey().indexOf('|'))+")"; break; }
      System.out.println(String.format("  %-14s %-28s %s", e.getKey(), e.getValue(), hit==null?"*** MISSING ***":"-> "+hit));
      if(hit==null) miss.add(e.getKey());
    }
    System.out.println("\nMISSING: "+(miss.isEmpty()?"none - full coverage":miss));
  }
}
