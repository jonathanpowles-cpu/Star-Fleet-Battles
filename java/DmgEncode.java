import java.io.*; import java.nio.file.*; import java.util.*; import java.lang.reflect.*;
public class DmgEncode {
  static Object fld(Object o,String n) throws Exception { if(o==null)return null;
    for(Class<?> c=o.getClass(); c!=null; c=c.getSuperclass())
      try{Field f=c.getDeclaredField(n);f.setAccessible(true);return f.get(o);}catch(NoSuchFieldException e){} return null; }
  @SuppressWarnings("unchecked") static Hashtable<String,Object> A(Object el) throws Exception { return (Hashtable<String,Object>) fld(el,"attributes"); }
  static String S(Object o){ return o==null?"":String.valueOf(o); }
  static Object find(Object b,String vec,String lbl) throws Exception {
    Vector<?> v=(Vector<?>) fld(b,vec); if(v==null) return null;
    for(Object p:v){ Hashtable<String,Object> h=A(p); if(h!=null && S(h.get("Label")).contains(lbl)) return p; } return null; }
  static Object load(String p) throws Exception {
    byte[] raw=Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(p))).trim());
    try(ObjectInputStream in=new ObjectInputStream(new ByteArrayInputStream(raw))){ return in.readObject(); } }
  public static void main(String[] a) throws Exception {
    // a damaged real ship (CL Mystic 53 destroyed in 0801)
    Object dmg=find(load(a[0]),"onBoardPieces","Mystic");
    Hashtable<String,Object> h=A(dmg);
    System.out.println("=== REAL DAMAGED: "+S(h.get("Label"))+" ===");
    for(String k:new String[]{"undo_damage","system_box_status","doubling_status","war_ship_status","breakdown","explosion_strength"}){
      Object v=h.get(k); String s=S(v);
      System.out.println("  "+k+" ("+(v==null?"null":v.getClass().getSimpleName())+") = "+s.substring(0,Math.min(120,s.length()))); }
    // the SSD internal damage count vs boxStatus
    Object ssd=h.get("SSD");
    System.out.println("  SSD.toString = "+S(ssd));
    // look at a Box's non-boxStatus damage fields
    Object boxes=fld(ssd,"boxes");
    for(int i=0;i<Array.getLength(boxes);i++){
      Object bx=Array.get(boxes,i); Object bs=fld(bx,"boxStatus");
      if(bs instanceof int[]){ int d=0; for(int v:(int[])bs) if(v==2) d++;
        if(d>0){ System.out.println("  box["+i+"] kind="+S(fld(bx,"kind"))+" state="+S(fld(bx,"state"))
          +" numOfBoxes="+S(fld(bx,"numOfBoxes"))+"/"+S(fld(bx,"maxNumOfBoxes"))+" destroyed(status2)="+d); } } }
  }
}
