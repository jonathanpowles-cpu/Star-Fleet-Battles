import java.io.*; import java.nio.file.*; import java.util.*; import java.lang.reflect.*;
public class BoxKinds {
  static Object fld(Object o,String n) throws Exception{ if(o==null)return null;
    for(Class<?> c=o.getClass();c!=null;c=c.getSuperclass())
      try{Field f=c.getDeclaredField(n);f.setAccessible(true);return f.get(o);}catch(NoSuchFieldException e){} return null;}
  @SuppressWarnings("unchecked") static Hashtable<String,Object> A(Object e) throws Exception{return (Hashtable<String,Object>)fld(e,"attributes");}
  static String S(Object o){return o==null?"":String.valueOf(o);}
  public static void main(String[] a) throws Exception{
    byte[] raw=Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(a[0]))).trim());
    Object b; try(ObjectInputStream in=new ObjectInputStream(new ByteArrayInputStream(raw))){b=in.readObject();}
    for(Object p:(Vector<?>)fld(b,"onBoardPieces")){
      if(!p.getClass().getName().contains("ShipAttributes"))continue;
      Hashtable<String,Object> at=A(p);
      if(!S(at.get("Label")).contains(a[1]))continue;
      System.out.println("=== "+S(at.get("Label"))+" ===");
      Object ssd=at.get("SSD"); Object boxes=fld(ssd,"boxes");
      for(int i=0;i<Array.getLength(boxes);i++){
        Object bx=Array.get(boxes,i);
        int kind=((Number)fld(bx,"kind")).intValue();
        int num=((Number)fld(bx,"numOfBoxes")).intValue();
        if(kind==9||kind==61||kind==71||kind==150||kind==195||kind==156)
          System.out.println("   kind "+kind+"  count "+num+"  desigs "+S(fld(bx,"designations")));
      }
    }
  }
}
