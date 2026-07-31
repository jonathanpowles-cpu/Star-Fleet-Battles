import java.io.*; import java.nio.file.*; import java.util.*; import java.lang.reflect.*;

public class Inventory {
  static Object fld(Object o, String n) throws Exception {
    if (o==null) return null;
    for (Class<?> c=o.getClass(); c!=null; c=c.getSuperclass())
      try { Field f=c.getDeclaredField(n); f.setAccessible(true); return f.get(o); } catch (NoSuchFieldException e) {}
    return null;
  }
  @SuppressWarnings("unchecked")
  static Hashtable<String,Object> attrs(Object el) throws Exception {
    Object h = fld(el,"attributes");                 // element IS the attributes object
    if (h == null) h = fld(fld(el,"attribs"),"attributes");   // or a GamePiece wrapper
    return (Hashtable<String,Object>) h;
  }
  static String s(Object o){ return o==null?"":String.valueOf(o); }
  static String cut(Object o,int n){ String v=s(o); return v.length()>n?v.substring(0,n):v; }
  static String loc(Object bl) throws Exception {
    if (bl==null) return "-";
    Object x=fld(bl,"x"), y=fld(bl,"y");
    if (x==null||y==null) return "?";
    return String.format("%02d%02d", ((Number)x).intValue(), ((Number)y).intValue());
  }
  static void list(String tag, Vector<?> v) throws Exception {
    System.out.println("===== " + tag + "  (" + v.size() + ") =====");
    if (!v.isEmpty()) System.out.println("   [element class = " + v.get(0).getClass().getName() + "]");
    for (Object p : v) {
      Hashtable<String,Object> a = attrs(p);
      if (a == null) { System.out.println("   (no attributes: " + p.getClass().getName() + ")"); continue; }
      System.out.println(String.join("|",
        cut(a.get("Label"),28), cut(a.get("race"),6), cut(a.get("unit_type"),5),
        loc(a.get("boardLocation")), s(a.get("condition")),
        cut(a.get("Note"),40), cut(a.get("status"),6), cut(a.get("crew"),4), s(a.get("ID"))));
    }
  }
  public static void main(String[] args) throws Exception {
    byte[] raw = Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(args[0]))).trim());
    Object board;
    try (ObjectInputStream in = new ObjectInputStream(new ByteArrayInputStream(raw))) { board = in.readObject(); }
    System.out.println("boardId=" + fld(board,"boardId"));
    list("ONBOARD",  (Vector<?>) fld(board,"onBoardPieces"));
    list("DISCARDED",(Vector<?>) fld(board,"discardedPieces"));
    Object log = fld(board,"log");
    System.out.println("EventLog=" + (log==null?"null":log.getClass().getName()));
    if (log!=null) for (Class<?> c=log.getClass(); c!=null && !c.equals(Object.class); c=c.getSuperclass())
      for (Field f : c.getDeclaredFields()) {
        if (Modifier.isStatic(f.getModifiers())) continue;
        f.setAccessible(true); Object v=f.get(log);
        System.out.println("   EventLog."+f.getName()+" : "+(v==null?"null":
          (v instanceof Collection? v.getClass().getSimpleName()+" size="+((Collection<?>)v).size() : v.getClass().getSimpleName())));
      }
  }
}
