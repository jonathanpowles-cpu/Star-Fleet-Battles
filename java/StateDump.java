import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.lang.reflect.*;

/** Dump full tactical state as JSON for the order engine, using the client's own
 *  box-kind names (abbrev_boxtypes.names) to categorize power, weapons, systems. */
public class StateDump {
  static Object fld(Object o, String n) throws Exception {
    if (o == null) return null;
    for (Class<?> c = o.getClass(); c != null; c = c.getSuperclass())
      try { Field f = c.getDeclaredField(n); f.setAccessible(true); return f.get(o); } catch (NoSuchFieldException e) {}
    return null;
  }
  @SuppressWarnings("unchecked")
  static Hashtable<String,Object> A(Object el) throws Exception { return (Hashtable<String,Object>) fld(el, "attributes"); }
  static String S(Object o) { return o == null ? "" : String.valueOf(o); }
  static int I(Object o) { try { return ((Number) o).intValue(); } catch (Exception e) { return 0; } }
  static String jesc(String s) { return s.replace("\\", "\\\\").replace("\"", "\\\""); }

  // The ACTUAL submitted EAF allocation, per turn, straight from the client's
  // Eaf object (turns[] of EafCol.row[], labelled by the engine's eafLines).
  // This is the real thing the player entered - not our advice - so the console
  // can show turn-by-turn allocation history. Emitted as
  //   "eaf": [ {"row label": "value", ...}, ... ]   (index = turn-1)
  @SuppressWarnings("unchecked")
  static String eafJson(Hashtable<String,Object> a) {
    try {
      Object eaf = a.get("Eaf");
      if (eaf == null) return "[]";
      Object eng = fld(eaf, "engine");
      Vector<?> lines = (Vector<?>) fld(eng, "eafLines");   // row definitions (labels)
      Vector<?> turns = (Vector<?>) fld(eaf, "turns");
      if (turns == null) return "[]";
      String[] labels = new String[lines == null ? 0 : lines.size()];
      for (int i = 0; i < labels.length; i++) labels[i] = S(fld(lines.get(i), "name"));
      StringBuilder sb = new StringBuilder("[");
      for (int t = 0; t < turns.size(); t++) {
        if (t > 0) sb.append(", ");
        String[] row = (String[]) fld(turns.get(t), "row");
        sb.append("{");
        boolean first = true;
        for (int i = 0; row != null && i < row.length; i++) {
          String v = row[i];
          if (v == null || v.isEmpty() || v.equals("0") || v.equals("0.0")) continue;
          String lbl = (i < labels.length && labels[i] != null) ? labels[i] : ("row" + i);
          if (lbl.equals("Notes")) continue;
          if (!first) sb.append(", ");
          first = false;
          sb.append("\"").append(jesc(lbl)).append("\": \"").append(jesc(v)).append("\"");
        }
        sb.append("}");
      }
      return sb.append("]").toString();
    } catch (Exception e) {
      return "[]";
    }
  }

  // kind -> category (from abbrev_boxtypes.names)
  static final Set<Integer> POWER_WARP = new HashSet<>(Arrays.asList(6, 12, 17));  // L/R/C Warp
  static final int K_IMP = 16, K_APR = 20, K_AWR = 32, K_BATT = 18, K_SHIELD = 26;
  static String weaponFamily(int k) {
    switch (k) {
      case 29: return "disruptor";
      case 28: return "photon";
      case 46: return "hellbore";
      case 47: return "fusion";
      case 49: return "esg";
      case 48: return "ppd";
      // Phaser TYPES, per the client's own data/sfbol/boxtypes.names. Collapsing
      // these into one "phaser" bucket lost two things that matter: ph-3 costs
      // half a point of capacitor (H6.21), and each type has its own damage
      // table with quite different range behaviour (a ph-3 is not a ph-1).
      case 33: return "phaser-1";
      case 34: return "phaser-2";
      case 35: return "phaser-3";
      case 36: return "phaser-4";
      case 37: return "phaser-G";      // fires on the ph-3 table (E2.152)
      case 15: return "phaser";        // generic/unspecified
      case 42: case 43: case 44: case 45: return "plasma";
      case 14: case 38: case 39: case 62: case 63: case 64: case 65: case 66: case 67: return "drone";
      default: return null;
    }
  }
  static String systemName(int k) {
    switch (k) {
      case 8: return "tractor";
      case 7: return "transporter";
      case 22: return "sensor";
      case 3: return "scanner";
      case 61: return "fighter";
      // Shuttle boxes (kind 9) are NOT fighters - they are the wild-weasel /
      // suicide-shuttle / scatter-pack platforms, a separate resource.
      case 9: return "shuttle";
      case 5: case 11: case 57: return "hull";
      // Box types the DAC can land on. Without these the damage model cannot
      // tell whether a ship HAS the system a chart entry names - which matters,
      // because a ship lacking that box type takes the hit elsewhere (an LDR DN
      // with no Armor boxes took its "Armor" result on Cargo instead).
      case 1:  return "bridge";
      case 2:  return "flag_bridge";
      case 4:  return "damage_control";
      case 10: return "lab";
      case 18: return "battery";
      case 21: return "probe";
      case 23: return "aux_control";
      case 24: return "emergency_bridge";
      case 25: return "cargo";
      case 27: return "armor";
      default: return null;
    }
  }

  @SuppressWarnings("unchecked")
  public static void main(String[] args) throws Exception {
    byte[] raw = Base64.getDecoder().decode(new String(Files.readAllBytes(Paths.get(args[0]))).trim());
    Object board;
    try (ObjectInputStream in = new ObjectInputStream(new ByteArrayInputStream(raw))) { board = in.readObject(); }
    Map<String,Object> shared = (Map<String,Object>) fld(board, "sharedAttributes");
    Object ti = shared.get("current_turn_impulse");
    int turn = I(fld(ti, "turn")) + 1, imp = I(fld(ti, "impulse")) + 1;

    StringBuilder sb = new StringBuilder();
    sb.append("{\n  \"turn\": ").append(turn).append(", \"impulse\": ").append(imp).append(",\n  \"ships\": [\n");
    Vector<?> on = (Vector<?>) fld(board, "onBoardPieces");
    boolean first = true;
    for (Object p : on) {
      if (!p.getClass().getName().contains("ShipAttributes")) continue;
      Hashtable<String,Object> a = A(p);
      Object bl = a.get("boardLocation");
      int x = I(fld(bl, "x")), y = I(fld(bl, "y"));
      int[] shield = new int[6], shieldMax = new int[6];
      int warp = 0, impB = 0, apr = 0, awr = 0, batt = 0, hull = 0, hullMax = 0;
      // Maximums too, so engine/power DAMAGE is visible (current < max). Without
      // these the SSD panel could not show a burned-out warp engine at all.
      int warpMax = 0, impMax = 0, aprMax = 0, awrMax = 0;
      Map<String,int[]> weap = new TreeMap<>();
      Map<String,int[]> sys = new TreeMap<>();
      Object ssd = a.get("SSD");
      if (ssd != null) {
        Object boxes = fld(ssd, "boxes");
        for (int i = 0; i < Array.getLength(boxes); i++) {
          Object bx = Array.get(boxes, i);
          int kind = I(fld(bx, "kind"));
          int num = I(fld(bx, "numOfBoxes")), max = I(fld(bx, "maxNumOfBoxes"));
          if (kind == K_SHIELD) {
            Object desigs = fld(bx, "designations");
            Integer face = null;
            if (desigs instanceof Collection && !((Collection<?>) desigs).isEmpty())
              try { face = Integer.parseInt(S(((Collection<?>) desigs).iterator().next()).trim()) - 1; } catch (Exception e) {}
            if (face != null && face >= 0 && face < 6) { shield[face] += num; shieldMax[face] += max; }
            continue;
          }
          if (POWER_WARP.contains(kind)) { warp += num; warpMax += max; }
          else if (kind == K_IMP || kind == 40) { impB += num; impMax += max; }  // 40 = Emergency Impulse
          else if (kind == K_APR) { apr += num; aprMax += max; }
          else if (kind == K_AWR) { awr += num; awrMax += max; }
          else if (kind == K_BATT) batt += num;
          String wf = weaponFamily(kind);
          if (wf != null) { weap.computeIfAbsent(wf, z -> new int[2]); weap.get(wf)[0] += num; weap.get(wf)[1] += max; }
          String sn = systemName(kind);
          if (sn != null) {
            if (sn.equals("hull")) { hull += num; hullMax += max; }
            else { sys.computeIfAbsent(sn, z -> new int[2]); sys.get(sn)[0] += num; sys.get(sn)[1] += max; }
          }
        }
      }
      int power = warp + impB + apr + awr;   // continuous power available this turn
      if (!first) sb.append(",\n");
      first = false;
      sb.append("    {\"id\": \"").append(jesc(S(a.get("ID"))))
        .append("\", \"label\": \"").append(jesc(S(a.get("Label"))))
        .append("\", \"race\": \"").append(jesc(S(a.get("race"))))
        .append("\", \"type\": \"").append(jesc(S(a.get("ship_type"))))
        .append("\", \"x\": ").append(x).append(", \"y\": ").append(y)
        .append(", \"facing\": ").append(I(a.get("facing")))
        .append(", \"speed\": ").append(I(a.get("current_speed")))
        .append(", \"size_class\": ").append(I(a.get("size_class")))
        .append(", \"move_cost\": \"").append(jesc(S(a.get("movement_cost"))))
        .append("\", \"turn_mode\": \"").append(jesc(S(a.get("turn_mode"))))
        // ew_status is the live "ECM/ECCM" pair the client tracks per ship, and
        // loaned_ew is scout lending (D6.3). Without these the engine had no
        // enemy ECM at all: ECCM was pinned at zero all game and every EW shift
        // read +0, so the fire advice silently assumed an unjammed battlefield.
        .append("\", \"ew_status\": \"").append(jesc(S(a.get("ew_status"))))
        .append("\", \"loaned_ew\": \"").append(jesc(S(a.get("loaned_ew"))))
        .append("\", \"deck_crews_reported\": ").append(I(a.get("deck_crews")))
        .append(", \"crew\": ").append(I(a.get("crew_units")))
        .append(", \"power\": {\"warp\": ").append(warp).append(", \"impulse\": ").append(impB)
        .append(", \"apr\": ").append(apr).append(", \"awr\": ").append(awr)
        .append(", \"warp_max\": ").append(warpMax).append(", \"impulse_max\": ").append(impMax)
        .append(", \"apr_max\": ").append(aprMax).append(", \"awr_max\": ").append(awrMax)
        .append(", \"total\": ").append(power).append(", \"battery\": ").append(batt).append("}")
        .append(", \"eaf\": ").append(eafJson(a))
        .append(", \"hull\": [").append(hull).append(",").append(hullMax).append("]")
        .append(", \"shields\": ").append(Arrays.toString(shield))
        .append(", \"shields_max\": ").append(Arrays.toString(shieldMax))
        .append(", \"weapons\": {");
      boolean wf2 = true;
      for (Map.Entry<String,int[]> e : weap.entrySet()) { if (!wf2) sb.append(", "); wf2 = false;
        sb.append("\"").append(e.getKey()).append("\": [").append(e.getValue()[0]).append(",").append(e.getValue()[1]).append("]"); }
      // Per-rack drone ammunition. Each ExpendableBox carries a designator and an
      // ammo list of "Type-<container>-<package>" strings, decodable against the
      // client's own alphadrones.expendable. This is the only source for what a
      // rack will ACTUALLY launch: the advice previously had to guess a generic
      // speed-8 standard drone, and could not see that a rack was loaded with
      // heavy Type-IVs or that it was empty.
      sb.append("}, \"drone_racks\": [");
      Object racks = a.get("drone_racks");
      if (racks instanceof Collection) {
        boolean rf = true;
        for (Object bx : (Collection<?>) racks) {
          if (!rf) sb.append(", ");
          rf = false;
          Object ammo = fld(bx, "ammo");
          // designator is not necessarily a Number (I() would silently yield 0
          // for every rack, collapsing them all into "rack 0"), so keep it as text.
          // boxType identifies the RACK TYPE (Drone-A=62, B=39, C=63, D=64,
          // E=65, F=66, G=67...), and rack type sets the rate of fire: type-A
          // fires one drone per turn, type-C fires TWO (12 impulses apart),
          // type-E four. Collapsing them all to "drone" made every rack look
          // alike, so the advice could not say how many drones a ship can
          // actually put in the air this turn.
          sb.append("{\"box_type\": ").append(I(fld(bx, "boxType")))
            .append(", \"designator\": \"").append(jesc(S(fld(bx, "designator"))))
            .append("\", \"ammo\": [");
          if (ammo instanceof Collection) {
            boolean af = true;
            for (Object rd : (Collection<?>) ammo) {
              if (!af) sb.append(", ");
              af = false;
              sb.append("\"").append(jesc(S(rd))).append("\"");
            }
          }
          sb.append("]}");
        }
      }
      sb.append("], \"systems\": {");
      boolean sf = true;
      for (Map.Entry<String,int[]> e : sys.entrySet()) { if (!sf) sb.append(", "); sf = false;
        sb.append("\"").append(e.getKey()).append("\": [").append(e.getValue()[0]).append(",").append(e.getValue()[1]).append("]"); }
      sb.append("}");
      // The client's OWN SSD layout: every box group with its position, size,
      // kind, and per-box damage state. This is the vector record sheet - the
      // bridge draws the authentic SSD from it, with exact box-level damage,
      // for every hull, no scans needed.
      sb.append(", \"ssd_boxes\": [");
      Object ssdObj = a.get("SSD");
      Object boxesArr = fld(ssdObj, "boxes");
      if (boxesArr != null) {
        boolean bf = true;
        for (int bi = 0; bi < java.lang.reflect.Array.getLength(boxesArr); bi++) {
          Object bx = java.lang.reflect.Array.get(boxesArr, bi);
          if (bx == null) continue;
          if (!bf) sb.append(", ");
          bf = false;
          Object st = fld(bx, "boxStatus");
          StringBuilder stj = new StringBuilder("[");
          if (st != null) {
            int n = java.lang.reflect.Array.getLength(st);
            for (int si = 0; si < n; si++) {
              if (si > 0) stj.append(",");
              stj.append(java.lang.reflect.Array.getInt(st, si));
            }
          }
          stj.append("]");
          // designation may be a single string or a Collection of them
          String des = S(fld(bx, "designation"));
          Object desL = fld(bx, "designations");
          if (des.isEmpty() && desL instanceof Collection) {
            StringBuilder dj = new StringBuilder();
            for (Object d : (Collection<?>) desL) {
              if (dj.length() > 0) dj.append(" ");
              dj.append(S(d));
            }
            des = dj.toString();
          }
          sb.append("{\"x\": ").append(I(fld(bx, "x")))
            .append(", \"y\": ").append(I(fld(bx, "y")))
            .append(", \"w\": ").append(I(fld(bx, "width")))
            .append(", \"h\": ").append(I(fld(bx, "height")))
            .append(", \"kind\": ").append(I(fld(bx, "kind")))
            .append(", \"n\": ").append(I(fld(bx, "numOfBoxes")))
            .append(", \"max\": ").append(I(fld(bx, "maxNumOfBoxes")))
            .append(", \"arc\": ").append(I(fld(bx, "firingArc")))
            .append(", \"section\": ").append(I(fld(bx, "section")))
            .append(", \"des\": \"").append(jesc(des))
            .append("\", \"status\": ").append(stj).append("}");
        }
      }
      sb.append("]}");
    }
    sb.append("\n  ],\n  \"shuttles\": [\n");
    // Airborne shuttles and fighters are real on-board pieces, so what is actually
    // flying is GROUND TRUTH straight from the save. Reconstructing it by tallying
    // launches and landings out of the play-by-play log drifts silently the moment
    // one event shape is missed - which is exactly how "fighters remaining" came to
    // be wrong. launch_unit_id attributes each craft to the ship that launched it;
    // `mission` separates a fighter from a wild weasel / suicide shuttle /
    // scatter-pack, all of which compete for the same shuttle boxes.
    boolean shf = true;
    for (Object p : on) {
      if (!p.getClass().getName().contains("Shuttle")) continue;
      Hashtable<String,Object> a = A(p);
      if (a == null) continue;
      Object bl = a.get("boardLocation");
      if (!shf) sb.append(",\n");
      shf = false;
      sb.append("    {\"label\": \"").append(jesc(S(a.get("Label"))))
        .append("\", \"launched_by\": \"").append(jesc(S(a.get("launch_unit_id"))))
        .append("\", \"mission\": \"").append(jesc(S(a.get("mission"))))
        .append("\", \"shuttle_type\": \"").append(jesc(S(a.get("shuttle_type"))))
        .append("\", \"race\": \"").append(jesc(S(a.get("race"))))
        .append("\", \"x\": ").append(I(fld(bl, "x"))).append(", \"y\": ").append(I(fld(bl, "y")))
        .append(", \"facing\": ").append(I(a.get("facing")))
        .append(", \"speed\": ").append(I(a.get("current_speed")))
        .append(", \"max_speed\": ").append(I(a.get("max_speed")))
        .append(", \"damage_taken\": ").append(I(a.get("damage_taken")))
        // A fighter's own armament, so a flight can be given a FIRE order and
        // not merely a move: df_rating is its direct-fire strength, and it
        // carries drones on launch rails (J4.0) with their own state. The
        // string fields come first so this block closes its own quote before
        // the numeric field that follows.
        .append(", \"launched_drones\": \"").append(jesc(S(a.get("launched_drones"))))
        .append("\", \"drone_load\": \"").append(jesc(S(a.get("drone_load"))))
        .append("\", \"special_orders\": \"").append(jesc(S(a.get("special_orders"))))
        .append("\", \"df_rating\": ").append(I(a.get("df_rating")))
        .append(", \"bay\": ").append(I(a.get("shuttle_bay")))
        .append(", \"launched\": \"").append(jesc(S(a.get("impulse_launched"))))
        .append("\"}");
    }
    sb.append("\n  ],\n  \"seeking\": [\n");
    // seeking weapons on the board (drones, plasma) - the EVADE inputs
    boolean sf2 = true;
    for (Object p : on) {
      if (!p.getClass().getName().contains("Seeking")) continue;
      Hashtable<String,Object> a = A(p);
      Object bl = a.get("boardLocation");
      if (!sf2) sb.append(",\n");
      sf2 = false;
      sb.append("    {\"label\": \"").append(jesc(S(a.get("Label"))))
        .append("\", \"kind\": \"").append(jesc(S(a.get("seeking_type"))))
        .append("\", \"name\": \"").append(jesc(S(a.get("Name"))))
        .append("\", \"x\": ").append(I(fld(bl, "x"))).append(", \"y\": ").append(I(fld(bl, "y")))
        .append(", \"speed\": ").append(I(a.get("current_speed")))
        .append(", \"max_speed\": ").append(I(a.get("max_speed")))
        .append(", \"target\": \"").append(jesc(S(a.get("target"))))
        .append("\", \"launched\": \"").append(jesc(S(a.get("impulse_launched"))))
        .append("\", \"damage_taken\": ").append(I(a.get("damage_taken")))
        .append(", \"loadout\": \"").append(jesc(S(a.get("complete_loadout")).replace("\n", " / ")))
        .append("\"}");
    }
    sb.append("\n  ]\n}\n");
    if (args.length > 1) Files.write(Paths.get(args[1]), sb.toString().getBytes());
    else System.out.print(sb);
  }
}
