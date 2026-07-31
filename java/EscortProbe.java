import java.io.*; import java.nio.file.*; import java.util.*; import java.lang.reflect.*;
public class EscortProbe {
  static Object fld(Object o,String n) throws Exception { if(o==null)return null;
    for(Class<?> c=o.getClass(); c!=null; c=c.getSuperclass())
      try{Field f=c.getDeclaredField(n);f.setAccessible(true);return f.get(o);}catch(NoSuchFieldException e){} return null; }
  @SuppressWarnings("unchecked") static Hashtable<String,Object> A(Object el) throws Exception { return (Hashtable<String,Object>) fld(el,"attributes"); }
  static String S(Object o){ return o==null?"":String.valueOf(o); }
  public static void main(String[] a) throws Exception {
    byte[] raw=Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(a[0]))).trim());
    Object b; try(ObjectInputStream in=new ObjectInputStream(new ByteArrayInputStream(raw))){ b=in.readObject(); }
    for(String v:new String[]{"onBoardPieces","offBoardPieces"}){
      Vector<?> vec=(Vector<?>) fld(b,v); if(vec==null) continue;
      for(Object p:vec){
        if(!p.getClass().getName().contains("ShipAttributes")) continue;
        Hashtable<String,Object> h=A(p);
        String att=S(h.get("attached_to")), pri=S(h.get("attached_priority")),
               grd=S(h.get("guard_assignments")), shu=S(h.get("shuttle_assignments")),
               fpf=S(h.get("fighters_pfs")), grp=S(h.get("groups"));
        if(!att.isBlank()||!grd.isBlank()||!fpf.isBlank()||!grp.isBlank())
          System.out.println(String.format("%-24s type=%-6s attached_to=%-22s pri=%-3s guard=%-18s ftrs=%-20s grp=%s",
            S(h.get("Label")), S(h.get("ship_type")), att, pri, grd.length()>18?grd.substring(0,18):grd,
            fpf.length()>20?fpf.substring(0,20):fpf, grp));
      }
    }
  }
}
