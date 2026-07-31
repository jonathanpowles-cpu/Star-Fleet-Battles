import java.util.*; import java.lang.reflect.*;
public class KindMap {
  public static void main(String[] a) throws Exception {
    Class<?> ssd = Class.forName("gub.plugins.game.sfb.AbbrevSSD");
    // try to load the kind names (method takes a String - maybe a resource path)
    try {
      Method load = ssd.getDeclaredMethod("loadAbbrevKindNames", String.class);
      load.setAccessible(true);
      for (String arg : new String[]{"", "abbrev", "data/sfbol/abbrevKindNames.txt", null}) {
        try { load.invoke(null, arg); break; } catch (Throwable t) {}
      }
    } catch (Throwable t) { System.out.println("load skipped: "+t); }
    Method get = ssd.getDeclaredMethod("getAbbrevKindMap");
    get.setAccessible(true);
    Object m = get.invoke(null);
    if (m instanceof Map && !((Map<?,?>)m).isEmpty()) {
      System.out.println("KIND MAP ("+((Map<?,?>)m).size()+"):");
      new TreeMap<Integer,Object>((Map)m).forEach((k,v)->System.out.println("  "+k+" = "+v));
    } else {
      System.out.println("map empty - trying getAbbrevKindName per int");
      Method gn = ssd.getDeclaredMethod("getAbbrevKindName", int.class); gn.setAccessible(true);
      for (int i=0;i<80;i++){ Object n=gn.invoke(null,i); if(n!=null && !String.valueOf(n).isBlank()) System.out.println("  "+i+" = "+n); }
    }
  }
}
