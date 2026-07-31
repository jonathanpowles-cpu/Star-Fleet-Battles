import java.io.*; import java.nio.file.*; import java.util.*; import java.lang.reflect.*;
public class PieceKinds {
  static Object fld(Object o,String n) throws Exception { if(o==null)return null;
    for(Class<?> c=o.getClass(); c!=null; c=c.getSuperclass())
      try{Field f=c.getDeclaredField(n);f.setAccessible(true);return f.get(o);}catch(NoSuchFieldException e){} return null; }
  @SuppressWarnings("unchecked") static Hashtable<String,Object> A(Object el) throws Exception { return (Hashtable<String,Object>) fld(el,"attributes"); }
  static String S(Object o){ return o==null?"":String.valueOf(o); }
  public static void main(String[] a) throws Exception {
    Map<String,TreeSet<String>> byClass=new TreeMap<>();
    for(String path:a){
      byte[] raw=Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(path))).trim());
      Object b; try(ObjectInputStream in=new ObjectInputStream(new ByteArrayInputStream(raw))){ b=in.readObject(); }
      for(String v:new String[]{"onBoardPieces","offBoardPieces","discardedPieces"}){
        Vector<?> vec=(Vector<?>) fld(b,v); if(vec==null) continue;
        for(Object p:vec){ Hashtable<String,Object> h=A(p);
          String cn=p.getClass().getSimpleName();
          String lbl=h==null?"?":S(h.get("Label"))+(S(h.get("unit_type")).isBlank()?"":" ["+S(h.get("unit_type"))+"]");
          byClass.computeIfAbsent(cn,k->new TreeSet<>()).add(lbl.isBlank()?"(no label)":lbl); }
      }
    }
    System.out.println("=== piece classes used across REAL battles ===");
    for(Map.Entry<String,TreeSet<String>> e:byClass.entrySet()){
      System.out.println("\n"+e.getKey()+"  ("+e.getValue().size()+" distinct labels)");
      int n=0; for(String s:e.getValue()){ System.out.println("     "+s); if(++n>=14){System.out.println("     ...");break;} }
    }
  }
}
