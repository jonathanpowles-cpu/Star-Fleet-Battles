import java.io.*; import java.nio.file.*; import java.util.*; import java.lang.reflect.*;
public class TypeManifest {
  static Object fld(Object o,String n) throws Exception { if(o==null)return null;
    for(Class<?> c=o.getClass(); c!=null; c=c.getSuperclass())
      try{Field f=c.getDeclaredField(n);f.setAccessible(true);return f.get(o);}catch(NoSuchFieldException e){} return null; }
  @SuppressWarnings("unchecked") static Hashtable<String,Object> A(Object el) throws Exception { return (Hashtable<String,Object>) fld(el,"attributes"); }
  static String S(Object o){ return o==null?"":String.valueOf(o); }
  public static void main(String[] a) throws Exception {
    byte[] raw=Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(a[0]))).trim());
    Object b; try(ObjectInputStream in=new ObjectInputStream(new ByteArrayInputStream(raw))){ b=in.readObject(); }
    // distinct race + unit_type + refit, with a sample ship and count
    Map<String,int[]> count=new TreeMap<>();
    Map<String,String> sample=new HashMap<>();
    for(String v:new String[]{"onBoardPieces","discardedPieces"}){
      Vector<?> vec=(Vector<?>) fld(b,v); if(vec==null) continue;
      for(Object p:vec){ Hashtable<String,Object> h=A(p);
        if("Fleet".equals(S(h.get("unit_type")))) continue;
        String race=S(h.get("race")); if(race.isEmpty()) continue;
        String ut=S(h.get("unit_type")), refit=S(h.get("refit"));
        String key=race+" | "+ut+(refit.isBlank()?"":"  (refit: "+refit+")");
        count.computeIfAbsent(key,k->new int[1])[0]++;
        sample.putIfAbsent(key,S(h.get("Label"))); }
    }
    String lastRace="";
    int distinct=0, total=0;
    for(Map.Entry<String,int[]> e:count.entrySet()){
      String race=e.getKey().substring(0, e.getKey().indexOf(" | "));
      if(!race.equals(lastRace)){ System.out.println("\n=== "+race+" ==="); lastRace=race; }
      System.out.println(String.format("   %-34s x%-2d   e.g. %s", e.getKey().substring(race.length()+3), e.getValue()[0], sample.get(e.getKey())));
      distinct++; total+=e.getValue()[0];
    }
    System.out.println("\nDISTINCT TYPES = "+distinct+"   (covering "+total+" ships)");
  }
}
