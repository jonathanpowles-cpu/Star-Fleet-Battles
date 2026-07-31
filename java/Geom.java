import java.io.*; import java.nio.file.*; import java.util.*; import java.lang.reflect.*;
public class Geom {
  static Object fld(Object o,String n) throws Exception { if(o==null)return null;
    for(Class<?> c=o.getClass(); c!=null; c=c.getSuperclass())
      try{Field f=c.getDeclaredField(n);f.setAccessible(true);return f.get(o);}catch(NoSuchFieldException e){} return null; }
  @SuppressWarnings("unchecked") static Hashtable<String,Object> A(Object el) throws Exception { return (Hashtable<String,Object>) fld(el,"attributes"); }
  static String S(Object o){ return o==null?"":String.valueOf(o); }
  public static void main(String[] a) throws Exception {
    for(String p:a){
      byte[] raw=Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(p))).trim());
      Object b; try(ObjectInputStream in=new ObjectInputStream(new ByteArrayInputStream(raw))){ b=in.readObject(); }
      System.out.println("\n### "+p.substring(p.lastIndexOf('/')+1));
      System.out.println("   boardSize="+S(fld(b,"boardSize"))+"  turn="+S(fld(b,"currentTurn"))+" imp="+S(fld(b,"currentImpulse")));
      System.out.println("   shared="+S(fld(b,"sharedAttributes")));
      Vector<?> on=(Vector<?>) fld(b,"onBoardPieces");
      int n=0;
      for(Object q:on){ Hashtable<String,Object> h=A(q); if(h==null) continue;
        System.out.println("      "+String.format("%-20s",S(h.get("Label")))+" loc="+S(h.get("boardLocation"))
          +" facing="+S(h.get("facing"))+" spd="+S(h.get("current_speed"))+" owner="+S(h.get("owner")));
        if(++n>=5) break; }
    }
  }
}
