import java.io.*; import java.nio.file.*; import java.util.*; import java.lang.reflect.*;
/** Simulate battle damage in a tactical save: kill a fraction of each of the first N ships'
 *  boxes, and wipe one ship entirely. java Maul <in.SFB> <out.SFB> */
public class Maul {
  static Object fld(Object o,String n) throws Exception{ if(o==null)return null;
    for(Class<?> c=o.getClass();c!=null;c=c.getSuperclass())
      try{Field f=c.getDeclaredField(n);f.setAccessible(true);return f.get(o);}catch(NoSuchFieldException e){} return null;}
  static void setfld(Object o,String n,Object v) throws Exception{
    for(Class<?> c=o.getClass();c!=null;c=c.getSuperclass())
      try{Field f=c.getDeclaredField(n);f.setAccessible(true);f.set(o,v);return;}catch(NoSuchFieldException e){}}
  @SuppressWarnings("unchecked") static Hashtable<String,Object> A(Object e) throws Exception{return (Hashtable<String,Object>)fld(e,"attributes");}
  static String S(Object o){return o==null?"":String.valueOf(o);}
  public static void main(String[] a) throws Exception{
    byte[] raw=Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(a[0]))).trim());
    Object b; try(ObjectInputStream in=new ObjectInputStream(new ByteArrayInputStream(raw))){b=in.readObject();}
    int idx=0;
    for(Object p:(Vector<?>)fld(b,"onBoardPieces")){
      if(!p.getClass().getName().contains("ShipAttributes"))continue;
      Hashtable<String,Object> at=A(p); Object ssd=at.get("SSD"); if(ssd==null)continue;
      Object boxes=fld(ssd,"boxes"); if(boxes==null)continue;
      idx++;
      double frac = (idx==1)? 1.00 : (idx<=4)? 0.35 : (idx<=6)? 0.10 : 0.0;  // ship1 destroyed
      if(frac<=0) continue;
      int killed=0,total=0;
      for(int i=0;i<Array.getLength(boxes);i++){
        Object bx=Array.get(boxes,i); Object bs=fld(bx,"boxStatus");
        if(!(bs instanceof int[]))continue; int[] arr=(int[])bs;
        for(int k=0;k<arr.length;k++){ total++;
          if(((double)killed)/Math.max(1,total) < frac){ arr[k]=2; killed++; } }
        int intact=0; for(int v:arr) if(v!=2) intact++;
        setfld(bx,"numOfBoxes",Integer.valueOf(intact));
      }
      if(idx==1) at.put("boardPile","1");   // removed from the board = killed
      System.out.println("mauled "+S(at.get("Label"))+"  killed "+killed+"/"+total
                         +(idx==1?"  (REMOVED FROM BOARD)":""));
    }
    ByteArrayOutputStream bo=new ByteArrayOutputStream();
    try(ObjectOutputStream out=new ObjectOutputStream(bo)){out.writeObject(b);}
    Files.write(Paths.get(a[1]),Base64.getEncoder().encodeToString(bo.toByteArray()).getBytes());
    System.out.println("wrote "+a[1]);
  }
}
