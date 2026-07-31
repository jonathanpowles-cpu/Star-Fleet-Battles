import java.io.*; import java.nio.file.*; import java.util.*; import java.lang.reflect.*;
public class HexScan {
  static Object fld(Object o,String n) throws Exception{ if(o==null)return null;
    for(Class<?> c=o.getClass();c!=null;c=c.getSuperclass())
      try{Field f=c.getDeclaredField(n);f.setAccessible(true);return f.get(o);}catch(NoSuchFieldException e){} return null;}
  @SuppressWarnings("unchecked") static Hashtable<String,Object> A(Object e) throws Exception{return (Hashtable<String,Object>)fld(e,"attributes");}
  static String S(Object o){return o==null?"":String.valueOf(o);}
  public static void main(String[] a) throws Exception{
    byte[] raw=Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(a[0]))).trim());
    Object b; try(ObjectInputStream in=new ObjectInputStream(new ByteArrayInputStream(raw))){b=in.readObject();}
    Map<String,List<String>> byHex=new TreeMap<>();
    for(Object p:(Vector<?>)fld(b,"onBoardPieces")){ Hashtable<String,Object> at=A(p);
      if(at==null||S(at.get("Label")).isEmpty())continue;
      if(!"0".equals(S(at.get("boardPile"))))continue;
      Object bl=at.get("boardLocation"); if(bl==null)continue;
      String hex=String.format("%02d%02d",((Number)fld(bl,"x")).intValue(),((Number)fld(bl,"y")).intValue());
      byHex.computeIfAbsent(hex,k->new ArrayList<>()).add(S(at.get("race")).charAt(0)+" "+S(at.get("Label")));}
    byHex.entrySet().stream().filter(e->e.getValue().size()>=2)
      .sorted((x,y)->y.getValue().size()-x.getValue().size()).limit(40)
      .forEach(e->{System.out.println("hex "+e.getKey()+"  ("+e.getValue().size()+" ships)");
        e.getValue().forEach(s->System.out.println("    "+s));});
  }
}
