#pragma once
#include "board.h"
#include "ship.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <vector>

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
enum class EafField { Speed, Shield, WeaponAlloc, WeaponArm, Commit };
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
    Renderer(SDL_Renderer* ren, TTF_Font* font, int screen_w, int screen_h, float hex_size);

    SDL_FPoint hex_to_pixel(Hex h) const;
    Hex        pixel_to_hex(SDL_FPoint p) const;

    // Board / ships
    void draw_board(const Board& board) const;
    void draw_ships(const std::vector<Ship>& ships, int selected_idx) const;
    void draw_move_options(const Ship& ship, const Board& board,
                           const std::vector<Ship>& all_ships) const;
    void draw_arc_overlay(const Ship& attacker, int weapon_idx,
                          const Board& board, const std::vector<Ship>& all_ships) const;
    void draw_highlight(Hex h, Color c) const;

    // HUD / panels
    void draw_hud(int turn, int impulse, int screen_w) const;
    void draw_sidebar(const Ship* ship, int selected_weapon,
                      int screen_w, int screen_h,
                      std::vector<EafButton>& weapon_buttons) const;
    void draw_eaf_modal(Ship& ship, int turn, int screen_w, int screen_h,
                        std::vector<EafButton>& out_buttons) const;
    void draw_text(const char* text, int x, int y, Color c) const;

    // Arc check (pixel-space angle method)
    bool hex_in_arc(Hex ship_pos, int facing, uint8_t arc_mask, Hex target) const;

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

    SDL_Renderer* ren_;
    TTF_Font*     font_;
    float hex_size_;
    SDL_FPoint origin_;
};
