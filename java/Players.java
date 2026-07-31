import java.io.*; import java.nio.file.*; import java.util.*; import java.lang.reflect.*;
public class Players {
  static Object fld(Object o,String n) throws Exception { if(o==null)return null;
    for(Class<?> c=o.getClass(); c!=null; c=c.getSuperclass())
      try{Field f=c.getDeclaredField(n);f.setAccessible(true);return f.get(o);}catch(NoSuchFieldException e){} return null; }
  @SuppressWarnings("unchecked") static Hashtable<String,Object> A(Object el) throws Exception { return (Hashtable<String,Object>) fld(el,"attributes"); }
  static String S(Object o){ return o==null?"":String.valueOf(o); }
  static Object load(String p) throws Exception {
    byte[] raw=Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(p))).trim());
    try(ObjectInputStream in=new ObjectInputStream(new ByteArrayInputStream(raw))){ return in.readObject(); } }
  public static void main(String[] a) throws Exception {
    Object b=load(a[0]);
    System.out.println("### "+a[0].substring(a[0].lastIndexOf('/')+1));
    // board-level player/owner-ish fields + sharedAttributes
    Object sh=fld(b,"sharedAttributes");
    System.out.println("sharedAttributes keys = "+(sh instanceof Map? ((Map<?,?>)sh).keySet() : sh));
    for(String f:new String[]{"players","playerList","owner","host","boardId"}){
      Object v=fld(b,f); if(v!=null) System.out.println("board."+f+" = "+S(v)); }
    // per-ship ownership fields
    Vector<?> on=(Vector<?>) fld(b,"onBoardPieces");
    System.out.println("\nper-ship ownership fields:");
    int n=0;
    for(Object p:on){ Hashtable<String,Object> h=A(p);
      System.out.println("  "+String.format("%-20s",S(h.get("Label")))+" race="+S(h.get("race"))
        +" owner=["+S(h.get("owner"))+"] player=["+S(h.get("player"))+"] visible_to=["+S(h.get("visible_to"))
        +"] war_ship_status=["+S(h.get("war_ship_status"))+"]");
      if(++n>=4) break; }
  }
}
