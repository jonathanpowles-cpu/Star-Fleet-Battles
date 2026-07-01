#include "renderer.h"
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <vector>

static constexpr float SFB_PI = 3.14159265358979f;

Renderer::Renderer(SDL_Renderer* ren, TTF_Font* font, int screen_w, int screen_h, float hex_size)
    : ren_(ren), font_(font), hex_size_(hex_size)
    , origin_{screen_w / 2.0f, screen_h / 2.0f}
{}

SDL_FPoint Renderer::hex_corner(SDL_FPoint centre, int i) const {
    float angle = SFB_PI / 180.0f * (60.0f * i - 30.0f);
    return { centre.x + hex_size_ * std::cos(angle),
             centre.y + hex_size_ * std::sin(angle) };
}

SDL_FPoint Renderer::hex_to_pixel(Hex h) const {
    float x = origin_.x + hex_size_ * (std::sqrt(3.0f) * h.q + std::sqrt(3.0f) / 2.0f * h.r);
    float y = origin_.y + hex_size_ * (3.0f / 2.0f * h.r);
    return {x, y};
}

Hex Renderer::pixel_to_hex(SDL_FPoint p) const {
    float px = (p.x - origin_.x) / hex_size_;
    float py = (p.y - origin_.y) / hex_size_;
    float fq = std::sqrt(3.0f) / 3.0f * px - 1.0f / 3.0f * py;
    float fr = 2.0f / 3.0f * py;
    float fs = -fq - fr;
    int q = (int)std::round(fq), r = (int)std::round(fr), s = (int)std::round(fs);
    float dq = std::abs(q - fq), dr = std::abs(r - fr), ds = std::abs(s - fs);
    if (dq > dr && dq > ds) q = -r - s;
    else if (dr > ds)        r = -q - s;
    return {q, r};
}

bool Renderer::hex_in_arc(Hex ship_pos, int facing, uint8_t arc_mask, Hex target) const {
    if (ship_pos.q == target.q && ship_pos.r == target.r) return false;
    SDL_FPoint sp = hex_to_pixel(ship_pos);
    SDL_FPoint tp = hex_to_pixel(target);
    float dx = tp.x - sp.x, dy = tp.y - sp.y;
    float a = std::atan2(dy, dx) * 180.0f / SFB_PI;
    float ship_a = 60.0f * facing - 30.0f;
    float rel = std::fmod(a - ship_a + 720.0f, 360.0f);
    int sextant = (int)std::floor(rel / 60.0f + 0.5f) % 6;
    return (arc_mask >> sextant) & 1;
}

// ── Primitive helpers ────────────────────────────────────────────────────────

void Renderer::set_color(Color c) const {
    SDL_SetRenderDrawColor(ren_, c.r, c.g, c.b, c.a);
}

void Renderer::fill_rect(int x, int y, int w, int h, Color c) const {
    set_color(c);
    SDL_Rect r{x, y, w, h};
    SDL_RenderFillRect(ren_, &r);
}

void Renderer::outline_rect(int x, int y, int w, int h, Color c) const {
    set_color(c);
    SDL_Rect r{x, y, w, h};
    SDL_RenderDrawRect(ren_, &r);
}

static void fill_polygon(SDL_Renderer* ren, const std::vector<SDL_FPoint>& pts, Color c) {
    if (pts.size() < 3) return;
    SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a);
    float min_y = pts[0].y, max_y = pts[0].y;
    for (auto& p : pts) { min_y = std::min(min_y, p.y); max_y = std::max(max_y, p.y); }
    int n = (int)pts.size();
    for (float y = std::floor(min_y); y <= max_y; y += 1.0f) {
        float x0 = 1e9f, x1 = -1e9f;
        for (int i = 0; i < n; ++i) {
            SDL_FPoint a = pts[i], b = pts[(i + 1) % n];
            if ((a.y <= y && b.y > y) || (b.y <= y && a.y > y)) {
                float t = (y - a.y) / (b.y - a.y);
                float x = a.x + t * (b.x - a.x);
                x0 = std::min(x0, x); x1 = std::max(x1, x);
            }
        }
        if (x1 >= x0) SDL_RenderDrawLineF(ren, x0, y, x1, y);
    }
}

static void outline_polygon(SDL_Renderer* ren, const std::vector<SDL_FPoint>& pts, Color c) {
    SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a);
    int n = (int)pts.size();
    for (int i = 0; i < n; ++i) {
        SDL_FPoint a = pts[i], b = pts[(i + 1) % n];
        SDL_RenderDrawLineF(ren, a.x, a.y, b.x, b.y);
    }
}

static std::vector<SDL_FPoint> transform(
    const std::vector<std::pair<float,float>>& local,
    SDL_FPoint centre, float angle_rad, float scale)
{
    std::vector<SDL_FPoint> out;
    out.reserve(local.size());
    float ca = std::cos(angle_rad), sa = std::sin(angle_rad);
    for (auto [lx, ly] : local) {
        float x = lx * scale, y = ly * scale;
        out.push_back({ centre.x + x * ca - y * sa,
                        centre.y + x * sa + y * ca });
    }
    return out;
}

// ── Ship shape definitions ───────────────────────────────────────────────────

static const std::vector<std::pair<float,float>> FED_SAUCER = {
    { 0.48f,  0.00f}, { 0.38f,  0.26f}, { 0.14f,  0.38f},
    {-0.12f,  0.36f}, {-0.30f,  0.20f}, {-0.32f,  0.00f},
    {-0.30f, -0.20f}, {-0.12f, -0.36f}, { 0.14f, -0.38f},
    { 0.38f, -0.26f}
};
static const std::vector<std::pair<float,float>> FED_SEC_HULL = {
    {-0.18f,  0.10f}, {-0.65f,  0.07f}, {-0.65f, -0.07f}, {-0.18f, -0.10f}
};
static const std::vector<std::pair<float,float>> FED_NACELLE_P = {
    {-0.22f,  0.32f}, {-0.64f,  0.28f}, {-0.66f,  0.18f}, {-0.24f,  0.22f}
};
static const std::vector<std::pair<float,float>> FED_NACELLE_S = {
    {-0.22f, -0.32f}, {-0.64f, -0.28f}, {-0.66f, -0.18f}, {-0.24f, -0.22f}
};

static const std::vector<std::pair<float,float>> KLI_POD = {
    { 0.50f,  0.00f}, { 0.38f,  0.14f}, { 0.22f,  0.16f},
    { 0.10f,  0.08f}, { 0.10f, -0.08f}, { 0.22f, -0.16f},
    { 0.38f, -0.14f}
};
static const std::vector<std::pair<float,float>> KLI_BOOM = {
    { 0.12f,  0.06f}, {-0.28f,  0.06f}, {-0.28f, -0.06f}, { 0.12f, -0.06f}
};
static const std::vector<std::pair<float,float>> KLI_WING_P = {
    {-0.10f,  0.08f}, { 0.05f,  0.10f}, {-0.08f,  0.44f},
    {-0.38f,  0.46f}, {-0.50f,  0.30f}, {-0.34f,  0.10f}
};
static const std::vector<std::pair<float,float>> KLI_WING_S = {
    {-0.10f, -0.08f}, { 0.05f, -0.10f}, {-0.08f, -0.44f},
    {-0.38f, -0.46f}, {-0.50f, -0.30f}, {-0.34f, -0.10f}
};
static const std::vector<std::pair<float,float>> KLI_NAC_P = {
    {-0.30f,  0.44f}, {-0.60f,  0.44f}, {-0.62f,  0.34f}, {-0.32f,  0.34f}
};
static const std::vector<std::pair<float,float>> KLI_NAC_S = {
    {-0.30f, -0.44f}, {-0.60f, -0.44f}, {-0.62f, -0.34f}, {-0.32f, -0.34f}
};

static void draw_ship_icon(SDL_Renderer* ren, const Ship& ship, SDL_FPoint centre, float hex_size) {
    float angle = SFB_PI / 180.0f * (60.0f * ship.facing - 30.0f);
    float s = hex_size;

    if (ship.faction == Faction::Federation) {
        Color body  = {175, 185, 200, 255};
        Color dark  = {110, 120, 140, 255};
        Color edge  = { 25, 100, 200, 255};
        Color tip   = {210,  30,  30, 255};
        fill_polygon   (ren, transform(FED_SAUCER,    centre, angle, s), body);
        outline_polygon(ren, transform(FED_SAUCER,    centre, angle, s), edge);
        fill_polygon   (ren, transform(FED_SEC_HULL,  centre, angle, s), dark);
        outline_polygon(ren, transform(FED_SEC_HULL,  centre, angle, s), edge);
        fill_polygon   (ren, transform(FED_NACELLE_P, centre, angle, s), dark);
        outline_polygon(ren, transform(FED_NACELLE_P, centre, angle, s), tip);
        fill_polygon   (ren, transform(FED_NACELLE_S, centre, angle, s), dark);
        outline_polygon(ren, transform(FED_NACELLE_S, centre, angle, s), tip);
    }
    else if (ship.faction == Faction::Klingon) {
        Color body  = { 80,  95, 115, 255};
        Color light = { 55,  75,  95, 255};
        Color edge  = {220, 180,  20, 255};
        Color tip   = {210,  30,  30, 255};
        fill_polygon   (ren, transform(KLI_WING_P, centre, angle, s), light);
        fill_polygon   (ren, transform(KLI_WING_S, centre, angle, s), light);
        outline_polygon(ren, transform(KLI_WING_P, centre, angle, s), edge);
        outline_polygon(ren, transform(KLI_WING_S, centre, angle, s), edge);
        fill_polygon   (ren, transform(KLI_NAC_P,  centre, angle, s), body);
        fill_polygon   (ren, transform(KLI_NAC_S,  centre, angle, s), body);
        outline_polygon(ren, transform(KLI_NAC_P,  centre, angle, s), tip);
        outline_polygon(ren, transform(KLI_NAC_S,  centre, angle, s), tip);
        fill_polygon   (ren, transform(KLI_BOOM,   centre, angle, s), body);
        outline_polygon(ren, transform(KLI_BOOM,   centre, angle, s), edge);
        fill_polygon   (ren, transform(KLI_POD,    centre, angle, s), body);
        outline_polygon(ren, transform(KLI_POD,    centre, angle, s), edge);
    }
    else {
        Color body = {175, 180, 190, 255};
        Color edge = {200,  20,  20, 255};
        float r = hex_size * 0.35f;
        std::vector<SDL_FPoint> circle;
        for (int i = 0; i < 12; ++i) {
            float a = SFB_PI / 6.0f * i;
            circle.push_back({centre.x + r * std::cos(a), centre.y + r * std::sin(a)});
        }
        fill_polygon(ren, circle, body);
        outline_polygon(ren, circle, edge);
    }
}

// ── Hex drawing ──────────────────────────────────────────────────────────────

void Renderer::draw_hex_outline(SDL_FPoint centre, Color c) const {
    set_color(c);
    for (int i = 0; i < 6; ++i) {
        SDL_FPoint a = hex_corner(centre, i);
        SDL_FPoint b = hex_corner(centre, (i + 1) % 6);
        SDL_RenderDrawLineF(ren_, a.x, a.y, b.x, b.y);
    }
}

void Renderer::fill_hex(SDL_FPoint centre, Color c) const {
    std::vector<SDL_FPoint> pts;
    for (int i = 0; i < 6; ++i) pts.push_back(hex_corner(centre, i));
    fill_polygon(ren_, pts, c);
}

// ── Board / ships ────────────────────────────────────────────────────────────

void Renderer::draw_board(const Board& board) const {
    board.for_each([&](Hex h, const Cell& cell) {
        SDL_FPoint centre = hex_to_pixel(h);
        Color fill = (cell.terrain == Terrain::Nebula)   ? Color{40,0,60,255} :
                     (cell.terrain == Terrain::Asteroid) ? Color{50,40,30,255} :
                                                           BLACK;
        fill_hex(centre, fill);
        draw_hex_outline(centre, {25, 40, 60, 255});
    });
}

void Renderer::draw_move_options(const Ship& ship, const Board& board,
                                  const std::vector<Ship>& all_ships) const {
    for (int d = 0; d < 6; ++d) {
        Hex nb = hex_neighbour(ship.position, d);
        if (!board.contains(nb)) continue;
        bool occupied = false;
        for (auto& s : all_ships)
            if (s.position.q == nb.q && s.position.r == nb.r) { occupied = true; break; }
        SDL_FPoint ctr = hex_to_pixel(nb);
        if (occupied) {
            fill_hex(ctr, {80, 10, 10, 120});
            draw_hex_outline(ctr, {200, 50, 50, 200});
        } else {
            fill_hex(ctr, {0, 70, 70, 100});
            draw_hex_outline(ctr, {0, 180, 180, 180});
        }
    }
}

void Renderer::draw_arc_overlay(const Ship& attacker, int weapon_idx,
                                  const Board& board, const std::vector<Ship>& all_ships) const {
    if (weapon_idx < 0 || weapon_idx >= (int)attacker.weapons.size()) return;
    const Weapon& w = attacker.weapons[weapon_idx];

    board.for_each([&](Hex h, const Cell& /*cell*/) {
        if (h.q == attacker.position.q && h.r == attacker.position.r) return;
        if (!hex_in_arc(attacker.position, attacker.facing, w.arc, h)) return;

        int range = attacker.position.distance(h);
        SDL_FPoint ctr = hex_to_pixel(h);

        // Check if an enemy is on this hex
        bool enemy_here = false;
        for (auto& s : all_ships)
            if (s.faction != attacker.faction &&
                s.position.q == h.q && s.position.r == h.r) { enemy_here = true; break; }

        if (enemy_here) {
            fill_hex(ctr,      {180, 60,  0, 160});
            draw_hex_outline(ctr, {255,140,  0, 255});
        } else if (range <= 8) {
            fill_hex(ctr,      { 60,  0,  0,  80});
            draw_hex_outline(ctr, {160, 30, 30, 140});
        } else {
            fill_hex(ctr,      { 40,  0,  0,  40});
            draw_hex_outline(ctr, { 80, 20, 20,  80});
        }
    });
}

void Renderer::draw_ships(const std::vector<Ship>& ships, int selected_idx) const {
    for (int i = 0; i < (int)ships.size(); ++i) {
        SDL_FPoint ctr = hex_to_pixel(ships[i].position);
        if (i == selected_idx) {
            fill_hex(ctr, {0, 60, 80, 180});
            draw_hex_outline(ctr, {0, 230, 230, 255});
        }
        draw_ship_icon(ren_, ships[i], ctr, hex_size_);
    }
}

void Renderer::draw_highlight(Hex h, Color c) const {
    draw_hex_outline(hex_to_pixel(h), c);
}

// ── HUD ─────────────────────────────────────────────────────────────────────

void Renderer::draw_hud(int turn, int impulse, int screen_w) const {
    fill_rect(0, 0, screen_w - SIDEBAR_W, 28, {15, 18, 25, 240});
    char buf[80];
    std::snprintf(buf, sizeof(buf), "TURN %d   IMPULSE %d / 32", turn, impulse);
    draw_text(buf, 10, 7, SKYBLUE);
    draw_text("[Q/E] rotate   [Click] move   [F] fire mode   [Space] next impulse",
              260, 7, GRAY);
}

// ── Sidebar ──────────────────────────────────────────────────────────────────

static const char* faction_name(Faction f) {
    switch (f) {
        case Faction::Federation: return "FEDERATION";
        case Faction::Klingon:    return "KLINGON";
        case Faction::Romulan:    return "ROMULAN";
    }
    return "";
}

static const char* SHIELD_LABELS[6] = {
    "#1 Fwd","#2 FwdR","#3 AftR","#4 Aft","#5 AftL","#6 FwdL"
};

static Color shield_color(int cur, int max) {
    if (max == 0) return GRAY;
    float pct = (float)cur / max;
    if (pct > 0.5f)  return {30, 180, 30, 255};
    if (pct > 0.25f) return {220, 180, 20, 255};
    return {200, 40, 40, 255};
}

void Renderer::draw_sidebar(const Ship* ship, int selected_weapon,
                             int screen_w, int screen_h,
                             std::vector<EafButton>& weapon_buttons) const {
    weapon_buttons.clear();
    int x0 = screen_w - SIDEBAR_W;
    fill_rect(x0, 0, SIDEBAR_W, screen_h, DARKGRAY);
    set_color(MIDGRAY);
    SDL_RenderDrawLine(ren_, x0, 0, x0, screen_h);

    if (!ship) {
        draw_text("No ship selected", x0 + 10, screen_h / 2 - 8, GRAY);
        draw_text("Click a ship", x0 + 10, screen_h / 2 + 10, GRAY);
        return;
    }

    int y = 12;
    const int lh = 19;
    const int bw = SIDEBAR_W - 20;

    draw_text(ship->name.c_str(), x0 + 10, y, WHITE); y += lh + 2;
    draw_text(faction_name(ship->faction), x0 + 10, y, LIGHTGRAY); y += lh - 2;

    set_color(MIDGRAY);
    SDL_RenderDrawLine(ren_, x0 + 8, y + 4, screen_w - 8, y + 4); y += 12;

    // Shields
    draw_text("─ SHIELDS ─", x0 + 10, y, SKYBLUE); y += lh;
    for (int i = 0; i < 6; ++i) {
        int cur = ship->sys.shields[i], max = ship->sys.shields_max[i];
        Color sc = shield_color(cur, max);
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%s", SHIELD_LABELS[i]);
        draw_text(buf, x0 + 10, y, LIGHTGRAY);
        std::snprintf(buf, sizeof(buf), "%d/%d", cur, max);
        draw_text(buf, x0 + 108, y, sc);
        y += lh - 3;
        int filled = max > 0 ? (bw * cur / max) : 0;
        fill_rect(x0 + 10, y, bw, 5, MIDGRAY);
        fill_rect(x0 + 10, y, filled, 5, sc);
        y += 8;
    }

    set_color(MIDGRAY);
    SDL_RenderDrawLine(ren_, x0 + 8, y + 2, screen_w - 8, y + 2); y += 10;

    // Systems
    draw_text("─ SYSTEMS ─", x0 + 10, y, SKYBLUE); y += lh;
    auto sys_row = [&](const char* lbl, int cur, int max) {
        draw_text(lbl, x0 + 10, y, LIGHTGRAY);
        char buf[24];
        if (max > 0) std::snprintf(buf, sizeof(buf), "%d/%d", cur, max);
        else         std::snprintf(buf, sizeof(buf), "%d", cur);
        draw_text(buf, x0 + 115, y, max > 0 ? shield_color(cur, max) : LIGHTGRAY);
        y += lh;
    };
    sys_row("Hull",  ship->sys.hull, ship->sys.hull_max);
    sys_row("Power", ship->sys.total_power, 0);
    {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "Speed: %d", ship->eaf.speed);
        draw_text(buf, x0 + 10, y, LIGHTGRAY); y += lh;
    }

    set_color(MIDGRAY);
    SDL_RenderDrawLine(ren_, x0 + 8, y + 2, screen_w - 8, y + 2); y += 10;

    // Weapons
    draw_text("─ WEAPONS ─", x0 + 10, y, SKYBLUE); y += lh;
    for (int i = 0; i < (int)ship->weapons.size(); ++i) {
        const Weapon& w = ship->weapons[i];
        bool sel = (i == selected_weapon);

        Color bg   = sel ? Color{60,40,0,255} : DARKGRAY;
        Color lc   = w.can_fire() ? GOLD :
                     w.is_instant() ? LIGHTGRAY :
                     w.armed ? GREEN : GRAY;

        fill_rect(x0 + 8, y - 1, bw + 4, lh + 1, bg);
        if (sel) outline_rect(x0 + 8, y - 1, bw + 4, lh + 1, GOLD);

        char buf[48];
        // Status suffix
        const char* status = w.armed  ? " [RDY]" :
                             w.is_instant() ? (w.allocated > 0 ? " [ON]" : " [--]") :
                             w.charge > 0   ? " [CHG]" : " [---]";
        std::snprintf(buf, sizeof(buf), "%s%s", w.label.c_str(), status);
        draw_text(buf, x0 + 12, y, lc);

        // "FIRE" button
        SDL_Rect fire_r{x0 + bw - 22, y - 1, 36, lh + 1};
        if (w.can_fire()) {
            fill_rect(fire_r.x, fire_r.y, fire_r.w, fire_r.h, {120, 30, 0, 200});
            outline_rect(fire_r.x, fire_r.y, fire_r.w, fire_r.h, RED);
            draw_text("FIRE", fire_r.x + 3, fire_r.y + 2, RED);
            // Register as a selectable weapon button (selecting it shows arc)
            EafButton b;
            b.rect      = {x0 + 8, y - 1, bw + 4, lh + 1};
            b.field     = EafField::WeaponArm;
            b.index     = i;
            b.delta     = 0;
            weapon_buttons.push_back(b);
        }
        y += lh + 2;
    }

    // EAF committed state
    y += 4;
    set_color(MIDGRAY);
    SDL_RenderDrawLine(ren_, x0 + 8, y, screen_w - 8, y); y += 8;
    char pos_buf[48];
    std::snprintf(pos_buf, sizeof(pos_buf), "Hex %d,%d  Facing %d",
                  ship->position.q, ship->position.r, ship->facing);
    draw_text(pos_buf, x0 + 10, y, GRAY);
}

// ── EAF Modal ────────────────────────────────────────────────────────────────

void Renderer::eaf_row(const char* label, int value, int max_val, int min_val,
                        EafField field, int idx,
                        int x, int& y, int w,
                        std::vector<EafButton>& out) const {
    const int BW = 22, BH = 18;
    int val_x = x + 180, dec_x = x + 230, inc_x = x + 270, cost_x = x + 310;

    draw_text(label, x + 8, y + 2, LIGHTGRAY);

    // Value
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%d", value);
    draw_text(buf, val_x, y + 2, WHITE);

    // [─] button
    fill_rect(dec_x, y, BW, BH, {60, 30, 30, 200});
    outline_rect(dec_x, y, BW, BH, {120, 60, 60, 255});
    draw_text("-", dec_x + 7, y + 2, WHITE);
    if (value > min_val)
        out.push_back({{dec_x, y, BW, BH}, field, idx, -1});

    // [+] button
    fill_rect(inc_x, y, BW, BH, {30, 60, 30, 200});
    outline_rect(inc_x, y, BW, BH, {60, 120, 60, 255});
    draw_text("+", inc_x + 7, y + 2, WHITE);
    if (value < max_val)
        out.push_back({{inc_x, y, BW, BH}, field, idx, +1});

    // Cost label
    std::snprintf(buf, sizeof(buf), "=%dPW", value);
    draw_text(buf, cost_x, y + 2, GRAY);

    y += BH + 4;
}

void Renderer::draw_eaf_modal(Ship& ship, int turn, int screen_w, int screen_h,
                               std::vector<EafButton>& out) const {
    out.clear();
    const int MW = 520, MH = 560;
    const int MX = (screen_w - SIDEBAR_W - MW) / 2;
    const int MY = (screen_h - MH) / 2;

    // Backdrop
    fill_rect(0, 0, screen_w - SIDEBAR_W, screen_h, {0, 0, 0, 160});

    // Modal box
    fill_rect(MX, MY, MW, MH, {22, 25, 35, 255});
    outline_rect(MX, MY, MW, MH, SKYBLUE);
    // Title bar
    fill_rect(MX, MY, MW, 30, {10, 40, 80, 255});
    char title[80];
    std::snprintf(title, sizeof(title),
                  "ENERGY ALLOCATION — TURN %d — %s", turn, ship.name.c_str());
    draw_text(title, MX + 10, MY + 7, WHITE);

    int y = MY + 38;

    // Power bar
    int used = ship.power_used(), avail = ship.sys.total_power;
    int remaining = avail - used;
    Color pw_col = remaining >= 0 ? GREEN : RED;
    char pw_buf[64];
    std::snprintf(pw_buf, sizeof(pw_buf),
                  "Power: %d / %d used   Remaining: %d", used, avail, remaining);
    draw_text(pw_buf, MX + 8, y, pw_col);
    y += 18;
    // Bar
    int bar_w = MW - 16;
    fill_rect(MX + 8, y, bar_w, 8, MIDGRAY);
    int filled_w = avail > 0 ? std::min(bar_w, bar_w * used / avail) : 0;
    fill_rect(MX + 8, y, filled_w, 8, remaining >= 0 ? Color{30,160,30,255} : RED);
    y += 14;

    set_color({50, 60, 80, 255});
    SDL_RenderDrawLine(ren_, MX + 4, y, MX + MW - 4, y); y += 6;

    // ── Speed ────────────────────────────────────────────────────────────────
    draw_text("MOVEMENT", MX + 8, y, SKYBLUE); y += 18;
    eaf_row("Speed setting", ship.eaf.speed, 31, 0,
            EafField::Speed, 0, MX, y, MW, out);

    set_color({50, 60, 80, 255});
    SDL_RenderDrawLine(ren_, MX + 4, y, MX + MW - 4, y); y += 6;

    // ── Shield reinforce ─────────────────────────────────────────────────────
    draw_text("SHIELD REINFORCEMENT", MX + 8, y, SKYBLUE); y += 18;
    for (int i = 0; i < 6; ++i) {
        char lbl[32];
        std::snprintf(lbl, sizeof(lbl), "Shield %s", SHIELD_LABELS[i]);
        eaf_row(lbl, ship.eaf.reinforce[i], 8, 0,
                EafField::Shield, i, MX, y, MW, out);
    }

    set_color({50, 60, 80, 255});
    SDL_RenderDrawLine(ren_, MX + 4, y, MX + MW - 4, y); y += 6;

    // ── Weapons ──────────────────────────────────────────────────────────────
    draw_text("WEAPONS", MX + 8, y, SKYBLUE); y += 18;
    for (int i = 0; i < (int)ship.weapons.size(); ++i) {
        Weapon& w = ship.weapons[i];
        if (w.is_instant()) {
            // Phaser: allocate firing power
            char lbl[48];
            std::snprintf(lbl, sizeof(lbl), "%s  (max %d PW)",
                          w.label.c_str(), w.max_power);
            eaf_row(lbl, w.allocated, w.max_power, 0,
                    EafField::WeaponAlloc, i, MX, y, MW, out);
        } else {
            // Torpedo / disruptor: toggle arming
            const char* arm_status = w.armed    ? " [ARMED]"  :
                                     w.allocated ? " [ARMING]" : " [IDLE]";
            char lbl[64];
            std::snprintf(lbl, sizeof(lbl), "%s%s  cost %dPW/turn",
                          w.label.c_str(), arm_status, w.arming_cost);
            draw_text(lbl, MX + 8, y + 2,
                      w.armed ? GREEN : w.allocated ? GOLD : GRAY);

            // [ARM] / [DISARM] toggle button
            const char* btn_lbl = (w.allocated || w.armed) ? "DISARM" : " ARM  ";
            bool arm_on = w.allocated > 0 || w.armed;
            fill_rect(MX + 340, y, 80, 18, arm_on ? Color{60,30,0,200} : Color{0,60,30,200});
            outline_rect(MX + 340, y, 80, 18, arm_on ? ORANGE : GREEN);
            draw_text(btn_lbl, MX + 350, y + 2, arm_on ? ORANGE : GREEN);
            out.push_back({{MX + 340, y, 80, 18}, EafField::WeaponArm, i, 0});
            y += 22;
        }
    }

    set_color({50, 60, 80, 255});
    SDL_RenderDrawLine(ren_, MX + 4, y, MX + MW - 4, y); y += 8;

    // ── Commit button ─────────────────────────────────────────────────────────
    bool can_commit = remaining >= 0;
    Color btn_col  = can_commit ? Color{20, 90, 20, 255} : Color{60, 20, 20, 255};
    Color btn_edge = can_commit ? GREEN : RED;
    fill_rect(MX + 80, y, 360, 30, btn_col);
    outline_rect(MX + 80, y, 360, 30, btn_edge);
    const char* commit_lbl = can_commit ? "COMMIT ALLOCATION"
                                        : "OVER BUDGET — REDUCE ALLOCATION";
    draw_text(commit_lbl, MX + 88, y + 8, can_commit ? GREEN : RED);
    if (can_commit)
        out.push_back({{MX + 80, y, 360, 30}, EafField::Commit, 0, 0});
}

// ── Text ─────────────────────────────────────────────────────────────────────

void Renderer::draw_text(const char* text, int x, int y, Color c) const {
    if (!font_) return;
    SDL_Color col = {c.r, c.g, c.b, c.a};
    SDL_Surface* surf = TTF_RenderText_Blended(font_, text, col);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(ren_, surf);
    SDL_Rect dst = {x, y, surf->w, surf->h};
    SDL_FreeSurface(surf);
    if (tex) { SDL_RenderCopy(ren_, tex, nullptr, &dst); SDL_DestroyTexture(tex); }
}
