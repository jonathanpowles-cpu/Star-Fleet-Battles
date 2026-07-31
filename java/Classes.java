import java.io.*; import java.nio.file.*; import java.util.*; import java.lang.reflect.*;
public class Classes {
  static Object fld(Object o,String n) throws Exception { if(o==null)return null;
    for(Class<?> c=o.getClass(); c!=null; c=c.getSuperclass())
      try{Field f=c.getDeclaredField(n);f.setAccessible(true);return f.get(o);}catch(NoSuchFieldException e){} return null; }
  @SuppressWarnings("unchecked") static Hashtable<String,Object> A(Object el) throws Exception { return (Hashtable<String,Object>) fld(el,"attributes"); }
  static String S(Object o){ return o==null?"":String.valueOf(o); }
  static String cls(String ut){ return ut.replaceAll("[+pP]+$",""); } // CA+ -> CA, CL+p -> CL
  public static void main(String[] a) throws Exception {
    byte[] raw=Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(a[0]))).trim());
    Object b; try(ObjectInputStream in=new ObjectInputStream(new ByteArrayInputStream(raw))){ b=in.readObject(); }
    Map<String,TreeMap<String,String>> byRace=new TreeMap<>(); // race -> class -> sample
    for(String v:new String[]{"onBoardPieces","discardedPieces"}){
      Vector<?> vec=(Vector<?>) fld(b,v); if(vec==null) continue;
      for(Object p:vec){ Hashtable<String,Object> h=A(p);
        if("Fleet".equals(S(h.get("unit_type")))) continue;
        String race=S(h.get("race")); if(race.isEmpty()) continue;
        byRace.computeIfAbsent(race,k->new TreeMap<>()).putIfAbsent(cls(S(h.get("unit_type"))), S(h.get("Label"))); }
    }
    for(Map.Entry<String,TreeMap<String,String>> e:byRace.entrySet()){
      System.out.println("\n=== "+e.getKey()+" : "+e.getValue().size()+" classes ===");
      List<String> row=new ArrayList<>();
      for(Map.Entry<String,String> c:e.getValue().entrySet()) row.add(c.getKey());
      System.out.println("   "+String.join(", ", row));
    }
  }
}
