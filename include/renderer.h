#pragma once
#include "board.h"
#include "ship.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

// Forward-declare SeekingWeapon so renderer can draw them
// (full definition is in main.cpp — pass as void* or define a minimal view)
struct SeekerView { Hex pos; int type_id; }; // type_id: 0=drone, 1=plasma

struct Color { Uint8 r, g, b, a; };

inline constexpr Color BLACK    = {  0,  0,  0,255};
inline constexpr Color DARKBLUE = {  0, 20, 60,255};
inline constexpr Color SKYBLUE  = {100,180,230,255};
inline constexpr Color YELLOW   = {255,220,  0,255};
inline constexpr Color GOLD     = {255,180,  0,255};
inline constexpr Color RED      = {200, 30, 30,255};
inline constexpr Color GREEN    = { 30,180, 30,255};
inline constexpr Color WHITE    = {255,255,255,255};
inline constexpr Color LIGHTGRAY= {180,180,180,255};
inline constexpr Color PURPLE   = {100,  0,150,255};
inline constexpr Color GRAY     = {120,120,120,255};
inline constexpr Color DARKGRAY = { 35, 38, 45,255};
inline constexpr Color MIDGRAY  = { 55, 60, 70,255};
inline constexpr Color ORANGE   = {230,120, 20,255};

static constexpr int SIDEBAR_W = 270;

// ── EAF button descriptor (built each frame during draw_eaf_modal) ────────────
enum class EafField {
    WarpPower, ImpulsePower,           // power sources
    AprOutput, BatteryTap,            // extra power sources
    Speed,                             // movement
    Shield,                            // shield reinforce (index 0-5)
    GeneralReinforce,                  // D3.341 omnidirectional reinforce
    WeaponAlloc, WeaponArm,            // weapons
    Ecm, Eccm,                         // electronic warfare
    Repair, Lab, Tractors, Transporters, // support
    Cloak,                             // cloak toggle (Romulan/Klingon)
    EngineDouble,                      // Orion emergency overdrive (G15.2)
    Commit
};
struct EafButton {
    SDL_Rect rect;
    EafField field;
    int      index; // shield 0-5, weapon 0-N
    int      delta; // +1 or -1 (unused for Commit/WeaponArm)
};

// ── Combat event result ───────────────────────────────────────────────────────
struct HitResult {
    int   damage;
    int   shield_hit;   // 0-5
    bool  hull_hit;
    std::string desc;
};

class Renderer {
public:
    Renderer(SDL_Renderer* ren, TTF_Font* font, int screen_w, int screen_h, float hex_size,
             std::string assets_dir = "assets/ssd");

    SDL_FPoint hex_to_pixel(Hex h) const;
    Hex        pixel_to_hex(SDL_FPoint p) const;
    void       set_screen_size(int board_w, int screen_h);
    void       set_hex_size(float s);

    // Board / ships
    void draw_board(const Board& board) const;
    void draw_ships(const std::vector<Ship>& ships, int selected_idx) const;
    void draw_move_options(const Ship& ship, const Board& board,
                           const std::vector<Ship>& all_ships) const;
    void draw_arc_overlay(const Ship& attacker, int weapon_idx,
                          const Board& board, const std::vector<Ship>& all_ships) const;
    void draw_highlight(Hex h, Color c) const;

    // HUD / panels
    void draw_hud(int turn, int impulse, int max_turns, int screen_w,
                  const std::vector<Ship>& ships, bool llm_on) const;
    void draw_combat_log(const std::deque<std::string>& log,
                         int screen_w, int screen_h) const;
    void draw_shield_hex(const Ship& ship, int cx, int cy, int radius) const;
    void draw_sidebar(const Ship* ship, int selected_weapon,
                      int screen_w, int screen_h,
                      std::vector<EafButton>& weapon_buttons) const;
    void draw_eaf_modal(Ship& ship, int turn, int screen_w, int screen_h,
                        std::vector<EafButton>& out_buttons, int scroll_y = 0) const;
    void draw_crew_advice(const CrewAdvice& advice,
                          int screen_w, int screen_h,
                          int edit_role = -1,
                          std::vector<SDL_Rect>* out_rects = nullptr) const;
    void draw_game_over(const std::string& winner, int screen_w, int screen_h) const;
    void draw_explosion(Hex pos, int frame) const;
    void draw_seekers(const std::vector<SeekerView>& seekers) const;
    void draw_ssd_panel(const Ship* ship, int screen_w, int screen_h) const;

    // Setup screen — returns list of clicked button rects by index
    // action: 0=include toggle, 1=ctrl cycle, 2=start, 3=open/close class dropdown, 4=select class
    struct SetupButton { SDL_Rect rect; int ship_idx; int action; int value = 0; };
    void draw_setup_screen(
        const std::vector<std::string>& names,
        const std::vector<std::string>& class_labels,
        const std::vector<Faction>& factions,
        const std::vector<bool>& included,
        const std::vector<ShipController>& controllers,
        int screen_w, int screen_h,
        std::vector<SetupButton>& out,
        int dropdown_row = -1,
        int dropdown_scroll = 0,
        const std::vector<std::pair<std::string,std::string>>* dd_items = nullptr) const;

    void draw_text(const char* text, int x, int y, Color c) const;

    // Pause / game menu overlay
    // action: 0=resume, 1=save, 2=load, 3=return-to-setup, 4=quit
    struct PauseButton { SDL_Rect rect; int action; };
    void draw_pause_menu(int screen_w, int screen_h,
                         std::vector<PauseButton>& out) const;

    // Arc check (pixel-space angle method)
    bool hex_in_arc(Hex ship_pos, int facing, uint8_t arc_mask, Hex target) const;
    // Map panning
    void pan(float dx, float dy) { origin_.x += dx; origin_.y += dy; }
    SDL_FPoint origin() const { return origin_; }

private:
    void set_color(Color c) const;
    void fill_rect(int x, int y, int w, int h, Color c) const;
    void outline_rect(int x, int y, int w, int h, Color c) const;
    void draw_hex_outline(SDL_FPoint centre, Color c) const;
    void fill_hex(SDL_FPoint centre, Color c) const;
    SDL_FPoint hex_corner(SDL_FPoint centre, int i) const;

    // EAF helper: draw a +/- row; pushes two buttons into out
    void eaf_row(const char* label, int value, int max_val, int min_val,
                 EafField field, int idx,
                 int x, int& y, int w,
                 std::vector<EafButton>& out) const;

    SDL_Texture* load_ssd_texture(const std::string& ship_class, Faction faction_fallback) const;

    SDL_Renderer* ren_;
    TTF_Font*     font_;
    float hex_size_;
    SDL_FPoint origin_;

    mutable std::unordered_map<std::string, SDL_Texture*> ssd_cache_;
    std::string assets_dir_;
};
