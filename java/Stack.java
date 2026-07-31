import java.io.*; import java.nio.file.*; import java.util.*; import java.lang.reflect.*;
public class Stack {
  static Object fld(Object o,String n) throws Exception {
    if(o==null) return null;
    for(Class<?> c=o.getClass(); c!=null; c=c.getSuperclass())
      try{ Field f=c.getDeclaredField(n); f.setAccessible(true); return f.get(o);}catch(NoSuchFieldException e){}
    return null; }
  @SuppressWarnings("unchecked")
  static Hashtable<String,Object> A(Object el) throws Exception { return (Hashtable<String,Object>) fld(el,"attributes"); }
  static String S(Object o){ return o==null?"":String.valueOf(o); }
  public static void main(String[] a) throws Exception {
    byte[] raw=Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(a[0]))).trim());
    Object b; try(ObjectInputStream in=new ObjectInputStream(new ByteArrayInputStream(raw))){ b=in.readObject(); }
    Vector<?> on=(Vector<?>) fld(b,"onBoardPieces");
    Object stack=null, member=null, plain=null;
    for(Object p:on){ Hashtable<String,Object> h=A(p);
      if(stack==null && "true".equals(S(h.get("IsStackPiece")))) stack=p;
      if(member==null && !S(h.get("StackPiece")).isBlank()) member=p;
      if(plain==null && "Lyran".equals(S(h.get("race"))) && S(h.get("StackPiece")).isBlank()) plain=p; }
    System.out.println("### STACK PIECE class = "+stack.getClass().getName());
    Hashtable<String,Object> sh=A(stack);
    for(String k: new TreeSet<>(sh.keySet())){
      String v=S(sh.get(k)); if(v.length()>60) v=v.substring(0,60)+"...";
      if(!v.isBlank()) System.out.println("     "+String.format("%-20s",k)+"= "+v);
    }
    System.out.println("\n### A STACKED MEMBER ship: "+S(A(member).get("Label")));
    for(String k: new String[]{"StackPiece","IsStackPiece","PiecesInStack","StackPieceId","groups","boardLocation","Depth","hidden"})
      System.out.println("     "+String.format("%-16s",k)+"= "+S(A(member).get(k)));
    System.out.println("\n### An UNSTACKED Lyran ship: "+S(A(plain).get("Label")));
    for(String k: new String[]{"StackPiece","IsStackPiece","PiecesInStack","StackPieceId","groups","boardLocation","Depth","hidden"})
      System.out.println("     "+String.format("%-16s",k)+"= "+S(A(plain).get(k)));
  }
}
