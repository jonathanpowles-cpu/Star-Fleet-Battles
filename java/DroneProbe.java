import java.io.*; import java.nio.file.*; import java.util.*; import java.lang.reflect.*;
public class DroneProbe {
  static Object fld(Object o,String n) throws Exception { if(o==null) return null;
    for(Class<?> c=o.getClass();c!=null;c=c.getSuperclass())
      try{Field f=c.getDeclaredField(n);f.setAccessible(true);return f.get(o);}catch(NoSuchFieldException e){}
    return null; }
  @SuppressWarnings("unchecked")
  public static void main(String[] a) throws Exception {
    byte[] raw=Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(a[0]))).trim());
    Object board; try(ObjectInputStream in=new ObjectInputStream(new ByteArrayInputStream(raw))){board=in.readObject();}
    for(Object p:(Vector<?>)fld(board,"onBoardPieces")){
      if(!p.getClass().getName().contains("ShipAttributes")) continue;
      Hashtable<String,Object> at=(Hashtable<String,Object>)fld(p,"attributes");
      String lbl=String.valueOf(at.get("Label"));
      if(!lbl.contains("Sabre")&&!lbl.contains("FF 9")) continue;
      System.out.println("\n=== "+lbl+" ===");
      for(Map.Entry<String,Object> e:new TreeMap<>(at).entrySet()){
        String k=e.getKey();
        if(!k.toLowerCase().matches(".*(drone|rack|expend|ammo|load|seek).*")) continue;
        Object v=e.getValue();
        System.out.println("  "+k+" ["+(v==null?"null":v.getClass().getSimpleName())+"]");
        if(v!=null&&v.getClass().isArray()){
          for(int i=0;i<Math.min(Array.getLength(v),4);i++){
            Object bx=Array.get(v,i);
            System.out.println("     ["+i+"] "+bx);
            if(bx!=null) for(Field f:bx.getClass().getDeclaredFields()){
              f.setAccessible(true);
              System.out.println("         ."+f.getName()+" = "+f.get(bx)); } } }
        else if(v instanceof Collection){ int i=0;
          for(Object o:(Collection<?>)v){ if(i++>2) break;
            System.out.println("     - "+o);
            if(o!=null) for(Class<?> c=o.getClass();c!=null&&c!=Object.class;c=c.getSuperclass())
              for(Field f:c.getDeclaredFields()){ f.setAccessible(true);
                if(java.lang.reflect.Modifier.isStatic(f.getModifiers())) continue;
                Object fv=f.get(o); String sv=String.valueOf(fv);
                if(sv.length()>70) sv=sv.substring(0,70)+"...";
                System.out.println("         ."+f.getName()+" = "+sv); } } }
        else System.out.println("     = "+v); } } }
}
