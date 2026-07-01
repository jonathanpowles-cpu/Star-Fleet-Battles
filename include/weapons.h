#pragma once
#include <cstdint>
#include <string>

// Arc bitmask: bit i means sextant i (relative to ship facing) is covered.
// Sextant 0=fwd, 1=fwd-right, 2=aft-right, 3=aft, 4=aft-left, 5=fwd-left
static constexpr uint8_t ARC_ALL   = 0b00111111; // 360°
static constexpr uint8_t ARC_FWD   = 0b00100011; // 180° forward  (sextants 5,0,1)
static constexpr uint8_t ARC_BROAD = 0b00110111; // 240° forward  (sextants 4,5,0,1,2)
static constexpr uint8_t ARC_REAR  = 0b00011100; // 180° aft      (sextants 2,3,4)

enum class WeaponType { Phaser1, Phaser2, Phaser3, PhotonTorpedo, Disruptor };

inline const char* weapon_name(WeaponType t) {
    switch (t) {
        case WeaponType::Phaser1:       return "Ph-1";
        case WeaponType::Phaser2:       return "Ph-2";
        case WeaponType::Phaser3:       return "Ph-3";
        case WeaponType::PhotonTorpedo: return "Photon";
        case WeaponType::Disruptor:     return "Disruptor";
    }
    return "";
}

struct Weapon {
    std::string label;
    WeaponType  type;
    uint8_t     arc;

    // For phasers: max_power is the max fire input; arming_turns = 0 (instant)
    // For photon/disruptor: max_power = 0 (fixed cost); arming_turns = 2 / 1
    int max_power;     // max power allocatable per turn (phasers)
    int arming_turns;  // turns needed to arm (0 = instant)
    int arming_cost;   // power per arming turn

    int  allocated    = 0;  // power set in EAF this turn
    int  charge       = 0;  // arming turns completed so far
    bool armed        = false;
    bool fired        = false;

    bool is_instant() const { return arming_turns == 0; }
    bool is_ready()   const { return armed || is_instant(); }
    bool can_fire()   const { return is_ready() && !fired && (allocated > 0 || !is_instant()); }

    // Phaser damage table (simplified SFB)
    int phaser_damage(int range) const {
        float d = (float)allocated;
        if (range <= 3)       return (int)d;
        if (range <= 8)       return (int)(d * 0.5f);
        if (range <= 15)      return (int)(d * 0.25f);
        return 0;
    }

    int damage_at(int range) const {
        switch (type) {
            case WeaponType::Phaser1:
            case WeaponType::Phaser2:
            case WeaponType::Phaser3:     return phaser_damage(range);
            case WeaponType::PhotonTorpedo: return range <= 15 ? 8 : 0;
            case WeaponType::Disruptor:   return range <= 8 ? 6 : (range <= 15 ? 3 : 0);
        }
        return 0;
    }

    // Effective power cost for EAF budget
    int eaf_cost() const {
        if (is_instant()) return allocated;          // phasers: pay what you use
        if (armed)        return 0;                  // armed weapon: no holding cost
        return (charge < arming_turns) ? arming_cost : 0; // actively arming
    }
};

inline Weapon make_ph1(std::string lbl) {
    return {std::move(lbl), WeaponType::Phaser1, ARC_BROAD, 10, 0, 0};
}
inline Weapon make_ph2(std::string lbl) {
    return {std::move(lbl), WeaponType::Phaser2, ARC_BROAD, 5, 0, 0};
}
inline Weapon make_ph3(std::string lbl) {
    return {std::move(lbl), WeaponType::Phaser3, ARC_ALL, 2, 0, 0};
}
inline Weapon make_photon(std::string lbl) {
    return {std::move(lbl), WeaponType::PhotonTorpedo, ARC_FWD, 0, 2, 4};
}
inline Weapon make_disruptor(std::string lbl) {
    return {std::move(lbl), WeaponType::Disruptor, ARC_BROAD, 0, 1, 3};
}
