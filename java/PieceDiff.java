import java.io.*; import java.nio.file.*; import java.util.*; import java.lang.reflect.*;
public class PieceDiff {
  static Object fld(Object o,String n) throws Exception { if(o==null)return null;
    for(Class<?> c=o.getClass(); c!=null; c=c.getSuperclass())
      try{Field f=c.getDeclaredField(n);f.setAccessible(true);return f.get(o);}catch(NoSuchFieldException e){} return null; }
  @SuppressWarnings("unchecked") static Hashtable<String,Object> A(Object el) throws Exception { return (Hashtable<String,Object>) fld(el,"attributes"); }
  static String S(Object o){ return o==null?"<null>":String.valueOf(o); }
  static Object load(String p) throws Exception {
    byte[] raw=Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(p))).trim());
    try(ObjectInputStream in=new ObjectInputStream(new ByteArrayInputStream(raw))){ return in.readObject(); } }
  static Object find(Object b,String vec,String lbl) throws Exception {
    Vector<?> v=(Vector<?>) fld(b,vec); if(v==null) return null;
    for(Object p:v){ Hashtable<String,Object> h=A(p); if(h!=null && S(h.get("Label")).contains(lbl)) return p; }
    return null; }
  public static void main(String[] a) throws Exception {
    Object good=find(load(a[0]),"onBoardPieces",a[1]);   // real battle ship
    Object mine=find(load(a[2]),"onBoardPieces",a[3]);   // my clone
    Hashtable<String,Object> g=A(good), m=A(mine);
    System.out.println("GOOD="+S(g.get("Label"))+"   MINE="+S(m.get("Label")));
    Set<String> keys=new TreeSet<>(); keys.addAll(g.keySet()); keys.addAll(m.keySet());
    System.out.println(String.format("%-24s %-34s | %s","attr","REAL-BATTLE","MINE"));
    for(String k:keys){
      Object gv=g.get(k), mv=m.get(k);
      String gs=S(gv), ms=S(mv);
      String gt=gv==null?"-":gv.getClass().getSimpleName(), mt=mv==null?"-":mv.getClass().getSimpleName();
      boolean diffVal=!Objects.equals(gs,ms), diffType=!gt.equals(mt);
      if(diffType || (diffVal && !Set.of("ID","Label","Name","boardLocation","saved_boardLocation","SSD",
          "system_box_status","Eaf","savedMoves","boardID","Note","facing","saved_facing","imageDefFile",
          "shipDefFile","ImageName","ImagePatternName","counter_file","ship_type","unit_type","race",
          "reference_number","ss_id","pot_id","pot_ss_prefix","refit","bpv","base_bpv","epv","base_epv",
          "crew_units","DamageDlg","eaColumnWidths").contains(k)))
        System.out.println(String.format("%-24s %-34s | %s%s", k, gs.substring(0,Math.min(34,gs.length())),
          ms.substring(0,Math.min(34,ms.length())), diffType?("   TYPE "+gt+" vs "+mt):""));
    }
  }
}
