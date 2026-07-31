import java.io.*; import java.nio.file.*; import java.util.*; import java.lang.reflect.*;
public class ShieldBox {
  static Object fld(Object o,String n) throws Exception { if(o==null)return null;
    for(Class<?> c=o.getClass(); c!=null; c=c.getSuperclass())
      try{Field f=c.getDeclaredField(n);f.setAccessible(true);return f.get(o);}catch(NoSuchFieldException e){} return null; }
  @SuppressWarnings("unchecked") static Hashtable<String,Object> A(Object el) throws Exception { return (Hashtable<String,Object>) fld(el,"attributes"); }
  static String S(Object o){ return o==null?"":String.valueOf(o); }
  static int I(Object o){ try{return((Number)o).intValue();}catch(Exception e){return 0;} }
  public static void main(String[] a) throws Exception {
    byte[] raw=Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(a[0]))).trim());
    Object b; try(ObjectInputStream in=new ObjectInputStream(new ByteArrayInputStream(raw))){ b=in.readObject(); }
    Vector<?> on=(Vector<?>) fld(b,"onBoardPieces");
    Object ship=null; for(Object p:on) if(p.getClass().getName().contains("ShipAttributes")){ship=p;break;}
    Object ssd=A(ship).get("SSD"); Object boxes=fld(ssd,"boxes");
    System.out.println("ship="+S(A(ship).get("Label")));
    for(int i=0;i<Array.getLength(boxes);i++){ Object bx=Array.get(boxes,i);
      if(I(fld(bx,"kind"))!=26) continue;
      Object designation=fld(bx,"designation"), designations=fld(bx,"designations"), designationList=fld(bx,"designationList");
      System.out.println("  shield box["+i+"] num="+I(fld(bx,"numOfBoxes"))+"/"+I(fld(bx,"maxNumOfBoxes"))
        +" designation="+S(designation)+" designations="+S(designations)+" designationList="+S(designationList)+" arcPos="+S(fld(bx,"arcPosition"))+" section="+S(fld(bx,"section"))); }
  }
}
