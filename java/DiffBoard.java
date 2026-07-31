import java.io.*; import java.nio.file.*; import java.util.*; import java.lang.reflect.*;
public class DiffBoard {
  static Object fld(Object o,String n) throws Exception { if(o==null)return null;
    for(Class<?> c=o.getClass(); c!=null; c=c.getSuperclass())
      try{Field f=c.getDeclaredField(n);f.setAccessible(true);return f.get(o);}catch(NoSuchFieldException e){} return null; }
  @SuppressWarnings("unchecked") static Hashtable<String,Object> A(Object el) throws Exception { return (Hashtable<String,Object>) fld(el,"attributes"); }
  static String S(Object o){ return o==null?"":String.valueOf(o); }
  static Object load(String p) throws Exception {
    byte[] raw=Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(p))).trim());
    try(ObjectInputStream in=new ObjectInputStream(new ByteArrayInputStream(raw))){ return in.readObject(); } }
  static Map<String,String> boardFields(Object b) throws Exception {
    Map<String,String> m=new TreeMap<>();
    for(Class<?> c=b.getClass(); c!=null && !c.equals(Object.class); c=c.getSuperclass())
      for(Field f:c.getDeclaredFields()){
        if(Modifier.isStatic(f.getModifiers())) continue;
        f.setAccessible(true); Object v=f.get(b);
        String d = v==null?"null" : (v instanceof Collection? v.getClass().getSimpleName()+"("+((Collection<?>)v).size()+")" : S(v));
        if(d.length()>60) d=d.substring(0,60);
        m.put(c.getSimpleName()+"."+f.getName(), d); }
    return m; }
  public static void main(String[] a) throws Exception {
    Object good=load(a[0]), mine=load(a[1]);
    Map<String,String> g=boardFields(good), m=boardFields(mine);
    System.out.println("=== BOARD FIELD DIFFS (template-that-loads  vs  my export) ===");
    for(String k:new TreeSet<>(g.keySet())){
      String gv=g.get(k), mv=m.getOrDefault(k,"<absent>");
      if(!Objects.equals(gv,mv)) System.out.println(String.format("  %-46s %-30s | %s",k,gv,mv)); }
    // piece-level: boardID vs board's boardId
    System.out.println("\n=== boardId / piece boardID ===");
    for(Object[] pair : new Object[][]{{"TEMPLATE",good},{"MINE",mine}}){
      Object b=pair[1];
      System.out.println("  "+pair[0]+" board.boardId = "+S(fld(b,"boardId")));
      Vector<?> on=(Vector<?>) fld(b,"onBoardPieces");
      Set<String> ids=new TreeSet<>();
      for(Object p:on){ Hashtable<String,Object> h=A(p); if(h!=null) ids.add(S(h.get("boardID"))); }
      System.out.println("     piece boardID values: "+ids);
      Set<String> pid=new TreeSet<>();
      for(Object p:on){ Hashtable<String,Object> h=A(p); if(h!=null) pid.add(S(h.get("pluginId"))); }
      System.out.println("     piece pluginId: "+pid);
    }
  }
}
