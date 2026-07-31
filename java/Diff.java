import java.io.*; import java.nio.file.*; import java.util.*; import java.lang.reflect.*;
public class Diff {
  static Object fld(Object o,String n) throws Exception {
    if(o==null) return null;
    for(Class<?> c=o.getClass(); c!=null; c=c.getSuperclass())
      try{ Field f=c.getDeclaredField(n); f.setAccessible(true); return f.get(o);}catch(NoSuchFieldException e){}
    return null; }
  @SuppressWarnings("unchecked")
  static Hashtable<String,Object> A(Object el) throws Exception { return (Hashtable<String,Object>) fld(el,"attributes"); }
  static String S(Object o){ return o==null?"":String.valueOf(o); }
  public static void main(String[] a) throws Exception {
    byte[] raw=Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(a[0]))).trim());
    Object b; try(ObjectInputStream in=new ObjectInputStream(new ByteArrayInputStream(raw))){ b=in.readObject(); }
    Vector<?> on=(Vector<?>) fld(b,"onBoardPieces");
    // any attribute whose value differs between Kzinti ships => candidate fleet marker
    Map<String,Set<String>> kz=new TreeMap<>(), ly=new TreeMap<>();
    for(Object p:on){
      Hashtable<String,Object> h=A(p);
      Map<String,Set<String>> t = "Kzinti".equals(S(h.get("race"))) ? kz : ("Lyran".equals(S(h.get("race")))? ly : null);
      if(t==null) continue;
      for(Map.Entry<String,Object> e:h.entrySet()){
        String v=S(e.getValue()); if(v.length()>40) v=v.substring(0,40);
        t.computeIfAbsent(e.getKey(),k->new TreeSet<>()).add(v);
      }
    }
    System.out.println("=== attributes that VARY across Kzinti ships (excluding obvious per-ship ones) ===");
    Set<String> skip=new HashSet<>(Arrays.asList("ID","Label","Name","boardLocation","saved_boardLocation",
      "savedMoves","ImageName","ImagePatternName","counter_file","bpv","epv","unit_type","Depth","boardID","ss_id"));
    for(Map.Entry<String,Set<String>> e: kz.entrySet()){
      if(skip.contains(e.getKey())) continue;
      if(e.getValue().size()>1 && e.getValue().size()<=8)
        System.out.println("   "+e.getKey()+" -> "+e.getValue());
    }
    System.out.println("\n=== same for Lyran (to compare) ===");
    for(Map.Entry<String,Set<String>> e: ly.entrySet()){
      if(skip.contains(e.getKey())) continue;
      if(e.getValue().size()>1 && e.getValue().size()<=8)
        System.out.println("   "+e.getKey()+" -> "+e.getValue());
    }
  }
}
