import java.io.*; import java.nio.file.*; import java.util.*; import java.lang.reflect.*;
public class Groups {
  static Object fld(Object o,String n) throws Exception {
    if(o==null) return null;
    for(Class<?> c=o.getClass(); c!=null; c=c.getSuperclass())
      try{ Field f=c.getDeclaredField(n); f.setAccessible(true); return f.get(o);}catch(NoSuchFieldException e){}
    return null; }
  @SuppressWarnings("unchecked")
  static Hashtable<String,Object> A(Object el) throws Exception { return (Hashtable<String,Object>) fld(el,"attributes"); }
  static String S(Object o){ return o==null?"":String.valueOf(o); }
  static String hex(Object p) throws Exception {
    Object bl=A(p).get("boardLocation"); if(bl==null) return "-";
    Object x=fld(bl,"x"), y=fld(bl,"y"); if(x==null||y==null) return "-";
    return String.format("%02d%02d",((Number)x).intValue(),((Number)y).intValue()); }
  public static void main(String[] a) throws Exception {
    byte[] raw=Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(a[0]))).trim());
    Object b; try(ObjectInputStream in=new ObjectInputStream(new ByteArrayInputStream(raw))){ b=in.readObject(); }
    Vector<?> on=(Vector<?>) fld(b,"onBoardPieces");
    System.out.println("=== STACK PIECES (IsStackPiece=true) ===");
    for(Object p:on){ Hashtable<String,Object> h=A(p);
      if("true".equals(S(h.get("IsStackPiece"))))
        System.out.println("   "+String.format("%-22s",S(h.get("Label")))+" race="+S(h.get("race"))
          +" hex="+hex(p)+" groups=["+S(h.get("groups"))+"] id="+S(h.get("ID"))
          +"\n        PiecesInStack="+S(h.get("PiecesInStack")));
    }
    System.out.println("\n=== ships WITH a groups value ===");
    Map<String,List<String>> g=new TreeMap<>();
    for(Object p:on){ Hashtable<String,Object> h=A(p); String gg=S(h.get("groups"));
      if(!gg.isBlank()) g.computeIfAbsent(S(h.get("race"))+" / "+gg,k->new ArrayList<>()).add(S(h.get("Label"))+"@"+hex(p)); }
    for(Map.Entry<String,List<String>> e:g.entrySet())
      System.out.println("   ["+e.getKey()+"] "+e.getValue().size()+": "+e.getValue());
    System.out.println("\n=== Kzinti ships by hex (current) ===");
    Map<String,List<String>> byhex=new TreeMap<>();
    for(Object p:on){ Hashtable<String,Object> h=A(p);
      if(!"Kzinti".equals(S(h.get("race")))) continue;
      byhex.computeIfAbsent(hex(p),k->new ArrayList<>()).add(S(h.get("Label"))); }
    for(Map.Entry<String,List<String>> e:byhex.entrySet())
      System.out.println("   "+e.getKey()+" ("+e.getValue().size()+"): "+e.getValue());
  }
}
