#include "renderer.h"
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <vector>

static constexpr float SFB_PI = 3.14159265358979f;

Renderer::Renderer(SDL_Renderer* ren, TTF_Font* font, int screen_w, int screen_h, float hex_size,
                   std::string assets_dir)
    : ren_(ren), font_(font), hex_size_(hex_size)
    , origin_{screen_w / 2.0f, screen_h / 2.0f}
    , assets_dir_(std::move(assets_dir))
{
    IMG_Init(IMG_INIT_PNG);
}

SDL_Texture* Renderer::load_ssd_texture(const std::string& ship_class, Faction faction_fallback) const {
    // Try exact ship-class PNG first, then faction fallback
    static const std::pair<const char*, const char*> CLASS_MAP[] = {
        // Federation
        {"fed_ca",           "fed_ca.png"},
        {"fed_ff",           "federation_frigate_ff.png"},
        {"fed_dd",           "federation_destroyer_leader_ddl.png"},
        {"fed_sc",           "federation_new_scout_cruiser_nsc.png"},
        {"fed_cl",           "federation_new_light_cruiser_ncl.png"},
        {"fed_cc",           "federation_commando_cruiser_cmc.png"},
        {"fed_dn",           "fed_ca.png"},           // no DN in Advanced Missions
        {"axcvl",            "federation_strike_carrier_cvs.png"},
        // Klingon
        {"klingon_d7",       "klingon_d7.png"},
        {"klingon_d6",       "klingon_d5_war_cruiser_d5.png"},  // closest available
        {"klingon_c8",       "klingon_b10_battleship_b10.png"},
        {"klingon_c9",       "klingon_b10_battleship_b10.png"},
        {"klingon_f5",       "klingon_f5c_frigate_leader_f5c.png"},
        {"klingon_e4",       "klingon_e3_escort_e3.png"},
        // Romulan
        {"romulan_kr",       "romulan_kr.png"},
        {"romulan_condor",   "romulan_condor_dreadnought_con.png"},
        {"romulan_ke",       "romulan_king_eagle_command_cruiser_ke.png"},
        {"romulan_we",       "romulan_k7r_battlecruiser_k7r.png"},
        {"romulan_sh",       "romulan_sparrowhawk_a_light_cruiser_spa.png"},
        {"romulan_sky",      "romulan_skyhawk_a_destroyer_ska.png"},
        {"romulan_snipe",    "romulan_snipe_a_frigate_sna.png"},
        {"romulan_wb",       "romulan_kr.png"},
        {"romulan_k5r",      "romulan_k5s_scout_k5s.png"},
        // Gorn
        {"gorn_ca",          "gorn_ca.png"},
        {"gorn_bc",          "gorn_command_cruiser_cc.png"},
        {"gorn_cl",          "gorn_large_scout_lsc.png"},
        {"gorn_dd",          "gorn_heavy_destroyer_hdd.png"},
        {"gorn_ddf",         "gorn_battle_destroyer_bdd.png"},
        {"gorn_sc",          "gorn_scout_sc.png"},
        {"gorn_ff",          "gorn_scout_sc.png"},  // no FF in book; use SC
        // Kzinti
        {"kzinti_cw",        "kzinti_cw.png"},
        {"kzinti_bc",        "kzinti_cw.png"},
        {"kzinti_cc",        "kzinti_medium_cruiser_cm.png"},
        {"kzinti_cl",        "kzinti_medium_cruiser_cm.png"},
        {"kzinti_cv",        "kzinti_light_carrier_cvl.png"},
        {"kzinti_cvs",       "kzinti_escort_carrier_cve.png"},
        {"kzinti_ff",        "kzinti_scout_frigate_sf.png"},
        // Tholian
        {"tholian_pc",       "tholian_pc.png"},
        {"tholian_pc_plus",  "tholian_cargo_patrol_corvette_cpc.png"},
        {"tholian_dd",       "tholian_destroyer_dd.png"},
        {"tholian_co",       "tholian_cruiser_c.png"},
        // Orion
        {"orion_cr",         "orion_ca.png"},
        {"orion_lr",         "orion_light_raider_lr.png"},
        {"orion_br",         "orion_battle_raider_br.png"},
        {"orion_ca",         "orion_heavy_cruiser_ca.png"},
        // Hydran and Lyran: no PNGs in book; faction fallback handles it
    };
    // Faction fallback filenames
    static const std::pair<Faction, const char*> FACTION_MAP[] = {
        {Faction::Federation, "fed_ca.png"},
        {Faction::Klingon,    "klingon_d7.png"},
        {Faction::Romulan,    "romulan_kr.png"},
        {Faction::Kzinti,     "kzinti_cw.png"},
        {Faction::Gorn,       "gorn_ca.png"},
        {Faction::Tholian,    "tholian_pc.png"},
        {Faction::Orion,      "orion_ca.png"},
    };

    // Check cache
    const std::string& key = ship_class;
    auto it = ssd_cache_.find(key);
    if (it != ssd_cache_.end()) return it->second;

    // Find filename
    const char* fname = nullptr;
    for (auto& [cls, fn] : CLASS_MAP) {
        if (ship_class == cls) { fname = fn; break; }
    }
    if (!fname) {
        for (auto& [fac, fn] : FACTION_MAP) {
            if (fac == faction_fallback) { fname = fn; break; }
        }
    }

    SDL_Texture* tex = nullptr;
    if (fname) {
        std::string path = assets_dir_ + "/" + fname;
        SDL_Surface* surf = IMG_Load(path.c_str());
        if (surf) {
            tex = SDL_CreateTextureFromSurface(ren_, surf);
            SDL_FreeSurface(surf);
        }
    }
    ssd_cache_[key.empty() ? std::string("__fallback__") + std::to_string((int)faction_fallback) : key] = tex;
    return tex;
}

SDL_FPoint Renderer::hex_corner(SDL_FPoint centre, int i) const {
    float angle = SFB_PI / 180.0f * (60.0f * i - 30.0f);
    return { centre.x + hex_size_ * std::cos(angle),
             centre.y + hex_size_ * std::sin(angle) };
}

void Renderer::set_screen_size(int board_w, int screen_h) {
    origin_ = {board_w / 2.0f, screen_h / 2.0f};
}

void Renderer::set_hex_size(float s) {
    hex_size_ = std::clamp(s, 18.0f, 90.0f);
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
    float ship_a = -60.0f * facing;
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

// Romulan Bird-of-Prey: swept forward wings + central spine
static const std::vector<std::pair<float,float>> ROM_BODY = {
    { 0.42f,  0.00f}, { 0.28f,  0.10f}, {-0.20f,  0.10f},
    {-0.32f,  0.00f}, {-0.20f, -0.10f}, { 0.28f, -0.10f}
};
static const std::vector<std::pair<float,float>> ROM_WING_P = {
    { 0.20f,  0.10f}, {-0.10f,  0.12f}, {-0.50f,  0.48f},
    {-0.62f,  0.38f}, {-0.28f,  0.12f}, { 0.10f,  0.10f}
};
static const std::vector<std::pair<float,float>> ROM_WING_S = {
    { 0.20f, -0.10f}, {-0.10f, -0.12f}, {-0.50f, -0.48f},
    {-0.62f, -0.38f}, {-0.28f, -0.12f}, { 0.10f, -0.10f}
};
static const std::vector<std::pair<float,float>> ROM_NACELLE_P = {
    {-0.46f,  0.42f}, {-0.68f,  0.40f}, {-0.68f,  0.32f}, {-0.46f,  0.34f}
};
static const std::vector<std::pair<float,float>> ROM_NACELLE_S = {
    {-0.46f, -0.42f}, {-0.68f, -0.40f}, {-0.68f, -0.32f}, {-0.46f, -0.34f}
};

// Gorn CA: heavy rounded body + two stubby swept-back wings; no nacelles
static const std::vector<std::pair<float,float>> GOR_BODY = {
    { 0.38f,  0.00f}, { 0.26f,  0.18f}, { 0.00f,  0.22f},
    {-0.22f,  0.18f}, {-0.36f,  0.00f}, {-0.22f, -0.18f},
    { 0.00f, -0.22f}, { 0.26f, -0.18f}
};
static const std::vector<std::pair<float,float>> GOR_WING_P = {
    {-0.06f,  0.20f}, {-0.42f,  0.46f}, {-0.56f,  0.36f}, {-0.18f,  0.16f}
};
static const std::vector<std::pair<float,float>> GOR_WING_S = {
    {-0.06f, -0.20f}, {-0.42f, -0.46f}, {-0.56f, -0.36f}, {-0.18f, -0.16f}
};

// Kzinti CW: sharp delta-wing attack ship
static const std::vector<std::pair<float,float>> KZI_BODY = {
    { 0.50f,  0.00f}, { 0.22f,  0.08f}, {-0.36f,  0.08f}, {-0.36f, -0.08f}, { 0.22f, -0.08f}
};
static const std::vector<std::pair<float,float>> KZI_WING_P = {
    { 0.22f,  0.08f}, {-0.12f,  0.10f}, {-0.60f,  0.44f}, {-0.52f,  0.52f}, {-0.08f,  0.14f}
};
static const std::vector<std::pair<float,float>> KZI_WING_S = {
    { 0.22f, -0.08f}, {-0.12f, -0.10f}, {-0.60f, -0.44f}, {-0.52f, -0.52f}, {-0.08f, -0.14f}
};
static const std::vector<std::pair<float,float>> KZI_FIN = {
    { 0.46f,  0.00f}, { 0.30f,  0.06f}, { 0.10f,  0.06f}, { 0.10f, -0.06f}, { 0.30f, -0.06f}
};

static void draw_ship_icon(SDL_Renderer* ren, const Ship& ship, SDL_FPoint centre, float hex_size) {
    float angle = SFB_PI / 180.0f * (-60.0f * ship.facing);
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
    else if (ship.faction == Faction::Romulan) {
        Color body = {165, 170, 180, 255};
        Color dark = {110, 115, 125, 255};
        Color edge = {200,  20,  20, 255};
        Color tip  = {240, 100,  10, 255};
        fill_polygon   (ren, transform(ROM_WING_P,    centre, angle, s), dark);
        fill_polygon   (ren, transform(ROM_WING_S,    centre, angle, s), dark);
        outline_polygon(ren, transform(ROM_WING_P,    centre, angle, s), edge);
        outline_polygon(ren, transform(ROM_WING_S,    centre, angle, s), edge);
        fill_polygon   (ren, transform(ROM_NACELLE_P, centre, angle, s), body);
        fill_polygon   (ren, transform(ROM_NACELLE_S, centre, angle, s), body);
        outline_polygon(ren, transform(ROM_NACELLE_P, centre, angle, s), tip);
        outline_polygon(ren, transform(ROM_NACELLE_S, centre, angle, s), tip);
        fill_polygon   (ren, transform(ROM_BODY,      centre, angle, s), body);
        outline_polygon(ren, transform(ROM_BODY,      centre, angle, s), edge);
    }
    else if (ship.faction == Faction::Gorn) {
        // Dark olive-green armoured hull; bright green border; amber tips
        Color body = { 55, 90,  45, 255};
        Color dark = { 35, 65,  30, 255};
        Color edge = {100, 200,  60, 255};
        Color tip  = {210, 160,  20, 255};
        fill_polygon   (ren, transform(GOR_WING_P, centre, angle, s), dark);
        fill_polygon   (ren, transform(GOR_WING_S, centre, angle, s), dark);
        outline_polygon(ren, transform(GOR_WING_P, centre, angle, s), tip);
        outline_polygon(ren, transform(GOR_WING_S, centre, angle, s), tip);
        fill_polygon   (ren, transform(GOR_BODY,   centre, angle, s), body);
        outline_polygon(ren, transform(GOR_BODY,   centre, angle, s), edge);
    }
    else if (ship.faction == Faction::Kzinti) {
        // Red-orange delta wing; white-orange highlights
        Color body = {170,  45,  20, 255};
        Color dark = {120,  30,  10, 255};
        Color edge = {255, 140,  30, 255};
        Color tip  = {255, 220, 100, 255};
        fill_polygon   (ren, transform(KZI_WING_P, centre, angle, s), dark);
        fill_polygon   (ren, transform(KZI_WING_S, centre, angle, s), dark);
        outline_polygon(ren, transform(KZI_WING_P, centre, angle, s), edge);
        outline_polygon(ren, transform(KZI_WING_S, centre, angle, s), edge);
        fill_polygon   (ren, transform(KZI_BODY,   centre, angle, s), body);
        outline_polygon(ren, transform(KZI_BODY,   centre, angle, s), tip);
        fill_polygon   (ren, transform(KZI_FIN,    centre, angle, s), body);
        outline_polygon(ren, transform(KZI_FIN,    centre, angle, s), edge);
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
        Color fill = (cell.terrain == Terrain::Nebula)    ? Color{40,0,60,255} :
                     (cell.terrain == Terrain::Asteroid)  ? Color{50,40,30,255} :
                     (cell.terrain == Terrain::Planet)    ? Color{20,60,20,255} :
                     (cell.terrain == Terrain::BlackHole) ? Color{0,0,0,255} :
                     (cell.terrain == Terrain::DustCloud) ? Color{55,45,30,255} :
                     (cell.terrain == Terrain::IonStorm)  ? Color{0,30,60,255} :
                     (cell.terrain == Terrain::GravityRift)? Color{0,20,50,255} :
                     (cell.terrain == Terrain::TholianWeb)? Color{0,50,50,255} :
                                                            BLACK;
        fill_hex(centre, fill);
        if (cell.terrain == Terrain::Asteroid)
            draw_text("*", (int)centre.x - 3, (int)centre.y - 7, Color{140,120,80,255});
        else if (cell.terrain == Terrain::Nebula)
            draw_text("~", (int)centre.x - 3, (int)centre.y - 7, Color{160,80,220,255});
        else if (cell.terrain == Terrain::Planet)
            draw_text("O", (int)centre.x - 4, (int)centre.y - 7, Color{60,180,60,255});
        else if (cell.terrain == Terrain::BlackHole) {
            draw_text("BH", (int)centre.x - 7, (int)centre.y - 7, Color{180,0,220,255});
            draw_hex_outline(centre, {100,0,140,255});
        }
        else if (cell.terrain == Terrain::DustCloud)
            draw_text("DC", (int)centre.x - 7, (int)centre.y - 7, Color{180,150,80,255});
        else if (cell.terrain == Terrain::IonStorm)
            draw_text("IS", (int)centre.x - 7, (int)centre.y - 7, Color{60,160,255,255});
        else if (cell.terrain == Terrain::GravityRift) {
            static const char* dir_arrows[] = {"E","NE","NW","W","SW","SE"};
            char rbuf[8]; std::snprintf(rbuf,sizeof(rbuf),"GR%s",dir_arrows[cell.rift_dir % 6]);
            draw_text(rbuf, (int)centre.x - 10, (int)centre.y - 7, Color{0,200,255,255});
        }
        else if (cell.terrain == Terrain::TholianWeb)
            draw_text("WEB", (int)centre.x - 10, (int)centre.y - 7, Color{0,220,200,255});
        draw_hex_outline(centre, {25, 40, 60, 255});
    });
}

void Renderer::draw_move_options(const Ship& ship, const Board& board,
                                  const std::vector<Ship>& all_ships) const {
    if (!ship.move_available) return;  // no highlight when move not due this impulse
    // SFB C2.0: ships only move into their forward hex
    for (int d = 0; d < 1; ++d) {  // single iteration — forward hex only
        Hex nb = hex_neighbour(ship.position, ship.facing);
        if (!board.contains(nb)) continue;
        Terrain nb_t = board.at(nb).terrain;
        if (nb_t == Terrain::Asteroid || nb_t == Terrain::Planet ||
            nb_t == Terrain::TholianWeb || nb_t == Terrain::BlackHole) continue;
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
        const Ship& s = ships[i];
        SDL_FPoint ctr = hex_to_pixel(s.position);

        if (s.destroyed) {
            // Draw as darkened wreck — faint red hex with an X
            fill_hex(ctr, {60, 10, 10, 140});
            draw_hex_outline(ctr, {100, 20, 20, 200});
            float r = hex_size_ * 0.35f;
            set_color({140, 30, 30, 220});
            SDL_RenderDrawLineF(ren_, ctr.x - r, ctr.y - r, ctr.x + r, ctr.y + r);
            SDL_RenderDrawLineF(ren_, ctr.x + r, ctr.y - r, ctr.x - r, ctr.y + r);
            continue;
        }

        if (s.move_available) {
            fill_hex(ctr, {60, 55, 0, 120});
            draw_hex_outline(ctr, {255, 230, 0, 255});
        }
        if (i == selected_idx) {
            fill_hex(ctr, {0, 60, 80, 160});
            draw_hex_outline(ctr, {0, 230, 230, 255});
        }
        draw_ship_icon(ren_, s, ctr, hex_size_);

        // Cloaking device: overlay a dark translucent fill to ghost the ship
        if (s.sys.cloak_active) {
            SDL_SetRenderDrawBlendMode(ren_, SDL_BLENDMODE_BLEND);
            std::vector<SDL_FPoint> hex_pts;
            for (int h = 0; h < 6; ++h) hex_pts.push_back(hex_corner(ctr, h));
            fill_polygon(ren_, hex_pts, {0, 0, 20, 185});
            SDL_SetRenderDrawBlendMode(ren_, SDL_BLENDMODE_NONE);
            draw_hex_outline(ctr, {80, 0, 130, 255}); // ghostly purple ring
        }
    }
}

void Renderer::draw_highlight(Hex h, Color c) const {
    draw_hex_outline(hex_to_pixel(h), c);
}

// ── HUD ─────────────────────────────────────────────────────────────────────

void Renderer::draw_hud(int turn, int impulse, int max_turns, int screen_w,
                         const std::vector<Ship>& ships, bool llm_on) const {
    fill_rect(0, 0, screen_w - SIDEBAR_W, 28, {15, 18, 25, 240});
    char buf[80];
    std::snprintf(buf, sizeof(buf), "TURN %d/%d   IMPULSE %d/32", turn, max_turns, impulse);
    draw_text(buf, 10, 7, SKYBLUE);
    draw_text(llm_on ? "[AI]" : "[det]", 220, 7, llm_on ? Color{80,200,80,255} : GRAY);

    // Show which ships have unspent moves this impulse
    std::string pending;
    for (auto& s : ships) {
        if (!s.destroyed && s.move_available) {
            if (!pending.empty()) pending += ", ";
            pending += s.name;
        }
    }
    if (!pending.empty()) {
        std::string msg = "MOVE: " + pending;
        draw_text(msg.c_str(), 270, 7, YELLOW);
    } else {
        draw_text("[Q/E] turn  [Click] move  [F] fire  [Space] next  [F1] SSD",
                  270, 7, GRAY);
    }
    // Always-visible menu hint at top-right of board area
    draw_text("[Esc] Menu", screen_w - SIDEBAR_W - 90, 7, Color{110, 110, 140, 255});
}

// ── Sidebar ──────────────────────────────────────────────────────────────────

static const char* faction_name(Faction f) {
    switch (f) {
        case Faction::Federation: return "FEDERATION";
        case Faction::Klingon:    return "KLINGON";
        case Faction::Romulan:    return "ROMULAN";
        case Faction::Gorn:       return "GORN";
        case Faction::Kzinti:     return "KZINTI";
        case Faction::Tholian:    return "THOLIAN";
        case Faction::Orion:      return "ORION";
        case Faction::Hydran:     return "HYDRAN";
        case Faction::Lyran:      return "LYRAN";
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

    // Shield hex diagram
    draw_shield_hex(*ship, x0 + SIDEBAR_W / 2, y + 44, 28);
    y += 96;

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
    sys_row("Power", ship->total_power(), 0);
    {
        char buf[48];
        std::snprintf(buf, sizeof(buf), "Speed: %d", ship->eaf.speed);
        draw_text(buf, x0 + 10, y, LIGHTGRAY); y += lh;
        if (ship->sys.battery_cap > 0) {
            std::snprintf(buf, sizeof(buf), "Battery: %d/%d", ship->sys.battery_charge, ship->sys.battery_cap);
            draw_text(buf, x0 + 10, y, ship->sys.battery_charge >= 4 ? GOLD : GRAY); y += lh;
        }
        if (ship->sys.apr_rated > 0) {
            std::snprintf(buf, sizeof(buf), "APR: %d/%d", ship->sys.apr_current, ship->sys.apr_rated);
            draw_text(buf, x0 + 10, y, ship->sys.apr_current > 0 ? GREEN : GRAY); y += lh;
        }
        if (ship->sys.cloak_installed) {
            draw_text(ship->sys.cloak_active ? "CLOAKED  [EAF to uncloak]" : "Cloak: standby",
                      x0 + 10, y, ship->sys.cloak_active ? Color{160,0,220,255} : GRAY); y += lh;
        }
        if (!ship->sys.bridge_ok) {
            draw_text("BRIDGE DAMAGED", x0 + 10, y, RED); y += lh;
        }
        if (ship->het_used) {
            draw_text("HET used this turn", x0 + 10, y, ORANGE); y += lh;
        } else if (ship->sys.battery_charge >= 4) {
            draw_text("[Shift+Q/E] HET available", x0 + 10, y, {100,100,255,255}); y += lh;
        }
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
                        int x, int& y, int /*w*/,
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
                               std::vector<EafButton>& out, int scroll_y) const {
    out.clear();

    const int MW = 530;
    const int MH = std::min(screen_h - 40, 860);  // taller modal, fills most of screen
    const int MX = (screen_w - SIDEBAR_W - MW) / 2;
    const int MY = (screen_h - MH) / 2;

    // Dim backdrop
    fill_rect(0, 0, screen_w - SIDEBAR_W, screen_h, {0, 0, 0, 170});

    // Modal
    fill_rect(MX, MY, MW, MH, {20, 24, 32, 255});
    outline_rect(MX, MY, MW, MH, SKYBLUE);

    // Title bar
    fill_rect(MX, MY, MW, 30, {8, 38, 78, 255});
    char title[96];
    std::snprintf(title, sizeof(title),
                  "ENERGY ALLOCATION — TURN %d — %s", turn, ship.name.c_str());
    draw_text(title, MX + 10, MY + 7, WHITE);

    // Commit button pinned below title bar (not scrollable)
    const int COMMIT_H = 34;
    {
        int avail0 = ship.total_power(), used0 = ship.power_used();
        bool can_commit0 = (avail0 - used0) >= 0;
        Color bc0 = can_commit0 ? Color{18,85,18,255} : Color{70,18,18,255};
        Color be0 = can_commit0 ? GREEN : RED;
        fill_rect(MX + 70, MY + 30, MW - 140, COMMIT_H - 4, bc0);
        outline_rect(MX + 70, MY + 30, MW - 140, COMMIT_H - 4, be0);
        const char* clbl0 = can_commit0 ? "COMMIT ALLOCATION"
                                        : "OVER BUDGET -- REDUCE ALLOCATIONS";
        draw_text(clbl0, MX + 80, MY + 36, can_commit0 ? GREEN : RED);
        if (can_commit0)
            out.push_back({{MX + 70, MY + 30, MW - 140, COMMIT_H - 4}, EafField::Commit, 0, 0});
    }

    // Clip scrollable body content below commit button
    SDL_Rect clip_area = {MX + 1, MY + 30 + COMMIT_H, MW - 2, MH - 31 - COMMIT_H};
    SDL_RenderSetClipRect(ren_, &clip_area);

    int y = MY + 30 + COMMIT_H + 6 - scroll_y;

    // ── Dynamic power bar ────────────────────────────────────────────────────
    int avail     = ship.total_power();
    int used      = ship.power_used();
    int remaining = avail - used;
    Color pc = remaining >= 0 ? GREEN : RED;
    {
        char buf[80];
        std::snprintf(buf, sizeof(buf),
            "Available: %d   Used: %d   Remaining: %d", avail, used, remaining);
        draw_text(buf, MX + 8, y, pc);
        y += 16;
        // A2.0/A5.0: show mandatory cost breakdown
        {
            char mbuf[80];
            std::snprintf(mbuf, sizeof(mbuf),
                "  Mandatory: Life Support 2 + Fire Control 2 = %d (required)", ship.mandatory_power());
            draw_text(mbuf, MX + 8, y, Color{150, 150, 150, 255});
            y += 14;
        }
        int bw = MW - 16;
        fill_rect(MX + 8, y, bw, 7, MIDGRAY);
        if (avail > 0) {
            int fw = std::min(bw, bw * used / avail);
            fill_rect(MX + 8, y, fw, 7, remaining >= 0 ? Color{30,160,30,255} : RED);
        }
        y += 12;
    }

    auto divider = [&]() {
        set_color({45, 55, 75, 255});
        SDL_RenderDrawLine(ren_, MX + 6, y + 3, MX + MW - 6, y + 3);
        y += 10;
    };

    // ── 1. POWER GENERATION ──────────────────────────────────────────────────
    divider();
    draw_text("POWER GENERATION", MX + 8, y, SKYBLUE); y += 17;
    {
        char lbl[64];
        std::snprintf(lbl, sizeof(lbl), "Warp engines   (max %d)", ship.sys.max_warp_power);
        eaf_row(lbl, ship.eaf.warp_power, ship.sys.max_warp_power, 0,
                EafField::WarpPower, 0, MX, y, MW, out);
        std::snprintf(lbl, sizeof(lbl), "Impulse engines (max %d)", ship.sys.max_impulse_power);
        eaf_row(lbl, ship.eaf.impulse_power, ship.sys.max_impulse_power, 0,
                EafField::ImpulsePower, 0, MX, y, MW, out);
        if (ship.sys.apr_current > 0) {
            std::snprintf(lbl, sizeof(lbl), "APR output  (max %d)", ship.sys.apr_current);
            eaf_row(lbl, ship.eaf.apr_output, ship.sys.apr_current, 0,
                    EafField::AprOutput, 0, MX, y, MW, out);
        }
        if (ship.sys.battery_cap > 0) {
            std::snprintf(lbl, sizeof(lbl), "Battery tap  (stored %d/%d)", ship.sys.battery_charge, ship.sys.battery_cap);
            eaf_row(lbl, ship.eaf.battery_tap, ship.sys.battery_charge, 0,
                    EafField::BatteryTap, 0, MX, y, MW, out);
        }
    }

    // ── 2. MOVEMENT ──────────────────────────────────────────────────────────
    divider();
    draw_text("MOVEMENT", MX + 8, y, SKYBLUE); y += 17;
    {
        char lbl[96];
        int mv_budget = ship.movement_power();
        int accel_cap = std::min(31, ship.last_speed + std::max(ship.last_speed, 10));
        int spd_max   = std::min(mv_budget, accel_cap);
        std::snprintf(lbl, sizeof(lbl),
            "Speed  (movement budget %d, accel cap %d)", mv_budget, accel_cap);
        eaf_row(lbl, ship.eaf.speed, spd_max, 0,
                EafField::Speed, 0, MX, y, MW, out);
        // Budget note (TM shown in HUD when ship selected)
        {
            char note[80];
            std::snprintf(note, sizeof(note),
                "  Warp %d + Impulse %d = movement budget %d (turn mode shown in HUD)",
                ship.eaf.warp_power, ship.eaf.impulse_power, mv_budget);
            draw_text(note, MX + 8, y, Color{140, 150, 170, 255});
            y += 14;
        }
    }

    // ── 3. DEFENSE ───────────────────────────────────────────────────────────
    divider();
    draw_text("DEFENSE", MX + 8, y, SKYBLUE); y += 17;
    for (int i = 0; i < 6; ++i) {
        char lbl[40];
        std::snprintf(lbl, sizeof(lbl), "Reinforce shield %s", SHIELD_LABELS[i]);
        eaf_row(lbl, ship.eaf.reinforce[i], ship.sys.shields[i], 0,
                EafField::Shield, i, MX, y, MW, out);
    }
    eaf_row("General reinforce (D3.341)  ",  ship.eaf.general_reinforce, 8, 0,
            EafField::GeneralReinforce, 0, MX, y, MW, out);
    eaf_row("ECM  (electronic countermeasures)",  ship.eaf.ecm,  4, 0,
            EafField::Ecm,  0, MX, y, MW, out);
    eaf_row("ECCM (counter-countermeasures)",      ship.eaf.eccm, 4, 0,
            EafField::Eccm, 0, MX, y, MW, out);

    // ── 4. WEAPONS ───────────────────────────────────────────────────────────
    divider();
    draw_text("WEAPONS", MX + 8, y, SKYBLUE); y += 17;
    for (int i = 0; i < (int)ship.weapons.size(); ++i) {
        Weapon& w = ship.weapons[i];
        if (w.is_instant()) {
            char lbl[56];
            std::snprintf(lbl, sizeof(lbl), "%s  (0–%d PW to fire)", w.label.c_str(), w.max_power);
            eaf_row(lbl, w.allocated, w.max_power, 0,
                    EafField::WeaponAlloc, i, MX, y, MW, out);
        } else {
            // Arm/disarm toggle for torpedoes and disruptors
            const char* status = w.armed    ? " ARMED"   :
                                 w.allocated ? " ARMING…" : " IDLE";
            Color sc           = w.armed ? GREEN : w.allocated ? GOLD : GRAY;
            char lbl[64];
            std::snprintf(lbl, sizeof(lbl), "%s —%s  %dPW/turn, %d turn%s",
                w.label.c_str(), status,
                w.arming_cost, w.arming_turns,
                w.arming_turns == 1 ? "" : "s");
            draw_text(lbl, MX + 8, y + 2, sc);

            // Charge progress dots
            if (!w.is_instant() && !w.armed) {
                for (int t = 0; t < w.arming_turns; ++t) {
                    Color dc = t < w.charge ? GREEN : MIDGRAY;
                    fill_rect(MX + 410 + t * 10, y + 4, 7, 10, dc);
                    outline_rect(MX + 410 + t * 10, y + 4, 7, 10, GRAY);
                }
            }

            bool arm_on = w.allocated > 0 || w.armed;
            const char* btn = arm_on ? "DISARM" : " ARM  ";
            Color bc = arm_on ? ORANGE : GREEN;
            fill_rect(MX + MW - 76, y, 70, 18, {30,30,30,200});
            outline_rect(MX + MW - 76, y, 70, 18, bc);
            draw_text(btn, MX + MW - 68, y + 2, bc);
            out.push_back({{MX + MW - 76, y, 70, 18}, EafField::WeaponArm, i, 0});
            y += 22;
        }
    }

    // ── 5. SUPPORT ───────────────────────────────────────────────────────────
    divider();
    draw_text("SUPPORT", MX + 8, y, SKYBLUE); y += 17;
    eaf_row("Damage control  (2 PW = 1 shield)",   ship.eaf.repair,     16, 0,
            EafField::Repair,      0, MX, y, MW, out);
    eaf_row("Laboratory / sensors",                 ship.eaf.lab,         4, 0,
            EafField::Lab,         0, MX, y, MW, out);
    eaf_row("Tractor beam",                         ship.eaf.tractors,    4, 0,
            EafField::Tractors,    0, MX, y, MW, out);
    eaf_row("Transporters",                         ship.eaf.transporters,2, 0,
            EafField::Transporters,0, MX, y, MW, out);

    // -- 6. SPECIAL SYSTEMS (cloak, Orion engine double) -----------------------
    {
        bool has_special = ship.sys.cloak_installed || ship.faction == Faction::Orion;
        if (has_special) { divider(); draw_text("SPECIAL SYSTEMS", MX + 8, y, SKYBLUE); y += 17; }
    }
    if (ship.sys.cloak_installed) {
        bool cloaked = (ship.eaf.cloak > 0);
        Color cbc = cloaked ? Color{80, 0, 130, 255} : Color{30, 30, 50, 220};
        Color cbe = cloaked ? Color{160, 60, 220, 255} : GRAY;
        fill_rect(MX + 8, y, 130, 22, cbc);
        outline_rect(MX + 8, y, 130, 22, cbe);
        draw_text(cloaked ? " CLOAK: ACTIVE" : " CLOAK: STANDBY", MX + 10, y + 4,
                  cloaked ? WHITE : GRAY);
        char cp[48];
        std::snprintf(cp, sizeof(cp), "  costs %dPW/turn -- cannot fire while cloaked", ship.sys.cloak_cost);
        draw_text(cp, MX + 148, y + 4, GRAY);
        out.push_back({{MX + 8, y, 130, 22}, EafField::Cloak, 0, 0});
        y += 28;
    }
    if (ship.faction == Faction::Orion) {
        bool eng_dbl = ship.eaf.engine_double;
        Color ebc = eng_dbl ? Color{160, 80, 0, 255} : Color{30, 30, 50, 220};
        Color ebe = eng_dbl ? Color{255, 160, 0, 255} : GRAY;
        fill_rect(MX + 8, y, 160, 22, ebc);
        outline_rect(MX + 8, y, 160, 22, ebe);
        draw_text(eng_dbl ? " ENGINE DOUBLE: ON" : " ENGINE DOUBLE: OFF", MX + 10, y + 4,
                  eng_dbl ? Color{255, 200, 0, 255} : GRAY);
        draw_text("  x2 warp power; 33% DAC risk (G15.2)", MX + 174, y + 4, GRAY);
        out.push_back({{MX + 8, y, 160, 22}, EafField::EngineDouble, 0, 0});
        y += 28;
    }
        // ── End of scrollable content — restore full draw region ─────────────────
    SDL_RenderSetClipRect(ren_, nullptr);

    // Scroll indicators
    if (scroll_y > 0)
        draw_text("^ more", MX + MW - 64, MY + 30 + 34 + 4, GRAY);

   
}

// ── SSD Panel (System Status Display — [TAB] toggle) ─────────────────────────

void Renderer::draw_ssd_panel(const Ship* ship, int screen_w, int screen_h) const {
    // Backdrop over board area
    fill_rect(0, 0, screen_w - SIDEBAR_W, screen_h, {0, 0, 0, 210});

    if (!ship) {
        draw_text("Select a ship (click) then press [TAB] for SSD", 20, screen_h / 2, GRAY);
        return;
    }

    const int PW = std::min(780, screen_w - SIDEBAR_W - 40);
    const int PH = std::min(720, screen_h - 40);
    const int PX = ((screen_w - SIDEBAR_W) - PW) / 2;
    const int PY = (screen_h - PH) / 2;

    fill_rect(PX, PY, PW, PH, {14, 18, 28, 255});
    outline_rect(PX, PY, PW, PH, SKYBLUE);

    // Title bar
    fill_rect(PX, PY, PW, 26, {8, 28, 58, 255});
    char title[128];
    std::snprintf(title, sizeof(title), "SSD — %s  (%s)    [TAB] close",
                  ship->name.c_str(), faction_name(ship->faction));
    draw_text(title, PX + 10, PY + 6, WHITE);

    const int lh   = 18;
    const int BOX  = 9;
    const int BPAD = 2;
    const int HALF = PW / 2 - 8;
    const int X1   = PX + 8;
    const int X2   = PX + PW / 2 + 4;
    const int BAR_START = 140;
    const int BAR_W = HALF - BAR_START - 10;

    // Helper: labelled progress bar
    auto pbar = [&](int lx, int& ly, const char* label, int cur, int mx, Color c) {
        draw_text(label, lx, ly, LIGHTGRAY);
        char buf[20]; std::snprintf(buf, sizeof(buf), "%d/%d", cur, mx);
        draw_text(buf, lx + BAR_START - 36, ly, c);
        fill_rect(lx + BAR_START, ly + 2, BAR_W, 7, MIDGRAY);
        if (mx > 0) fill_rect(lx + BAR_START, ly + 2,
                               BAR_W * std::min(cur, mx) / mx, 7, c);
        ly += lh;
    };

    // Helper: engine group as filled boxes
    auto eng_row = [&](int lx, int& ly, const char* label, int rated, int current) {
        draw_text(label, lx, ly + 1, LIGHTGRAY);
        int bx = lx + 90;
        for (int b = 0; b < rated; ++b) {
            bool op = (b < current);
            fill_rect(bx + b * (BOX + BPAD), ly, BOX, BOX,
                      op ? Color{40,170,40,255} : Color{40,40,40,255});
            outline_rect(bx + b * (BOX + BPAD), ly, BOX, BOX,
                         op ? Color{70,220,70,255} : Color{70,70,70,255});
        }
        char pw[12]; std::snprintf(pw, sizeof(pw), " %d PW", current);
        draw_text(pw, bx + rated * (BOX + BPAD) + 2, ly + 1,
                  current > 0 ? GREEN : GRAY);
        ly += lh;
    };

    // ── LEFT COLUMN: power + shields ──────────────────────────────────────────
    int y = PY + 32;

    draw_text("POWER SYSTEMS", X1, y, SKYBLUE); y += lh;
    pbar(X1, y, "Hull", ship->sys.hull, ship->sys.hull_max,
         shield_color(ship->sys.hull, ship->sys.hull_max));

    if (!ship->sys.warp_groups.empty()) {
        draw_text("Warp engines:", X1, y, LIGHTGRAY); y += lh - 2;
        for (auto& g : ship->sys.warp_groups)
            eng_row(X1 + 8, y, g.label.c_str(), g.rated, g.current);
    }
    if (!ship->sys.impulse_groups.empty()) {
        draw_text("Impulse:", X1, y, LIGHTGRAY); y += lh - 2;
        for (auto& g : ship->sys.impulse_groups)
            eng_row(X1 + 8, y, g.label.c_str(), g.rated, g.current);
    }
    if (ship->sys.apr_rated > 0)
        eng_row(X1, y, "APR", ship->sys.apr_rated, ship->sys.apr_current);
    if (ship->sys.battery_cap > 0)
        pbar(X1, y, "Battery", ship->sys.battery_charge, ship->sys.battery_cap, GOLD);

    y += 4;
    set_color(MIDGRAY);
    SDL_RenderDrawLine(ren_, X1, y, X1 + HALF, y); y += 6;
    draw_text("SHIELDS", X1, y, SKYBLUE); y += lh;
    for (int i = 0; i < 6; ++i) {
        int reinf    = ship->eaf.reinforce[i];
        int y_before = y;
        pbar(X1, y, SHIELD_LABELS[i],
             ship->sys.shields[i], ship->sys.shields_max[i],
             shield_color(ship->sys.shields[i], ship->sys.shields_max[i]));
        if (reinf > 0) {
            char rb[12]; std::snprintf(rb, sizeof(rb), "+%d", reinf);
            draw_text(rb, X1 + BAR_START + BAR_W + 4, y_before, Color{80,180,255,255});
        }
    }

    // ── RIGHT COLUMN: crew / special / weapons ─────────────────────────────
    int y2 = PY + 32;

    draw_text("CREW & SYSTEMS", X2, y2, SKYBLUE); y2 += lh;

    if (ship->sys.crew_total > 0) {
        char cl[32];
        std::snprintf(cl, sizeof(cl), "Crew (%d cas.)", ship->sys.crew_casualties);
        pbar(X2, y2, cl, ship->sys.crew_available(), ship->sys.crew_total,
             ship->sys.crew_casualties > 0 ? ORANGE : GREEN);
    }

    {
        draw_text("Bridge:", X2, y2, LIGHTGRAY);
        draw_text(ship->sys.bridge_ok ? "OK" : "DAMAGED", X2 + 80, y2,
                  ship->sys.bridge_ok ? GREEN : RED);
        y2 += lh;
    }

    if (ship->sys.cloak_installed) {
        draw_text("Cloak:", X2, y2, LIGHTGRAY);
        bool act = ship->sys.cloak_active;
        draw_text(act ? "ACTIVE" : "STANDBY", X2 + 80, y2,
                  act ? Color{180,60,255,255} : GRAY);
        char cc[20]; std::snprintf(cc, sizeof(cc), " (%dPW)", ship->sys.cloak_cost);
        draw_text(cc, X2 + 150, y2, GRAY);
        y2 += lh;
    }

    if (ship->sys.shuttles > 0) {
        char sh[32]; std::snprintf(sh, sizeof(sh), "Shuttles: %d", ship->sys.shuttles);
        draw_text(sh, X2, y2, LIGHTGRAY); y2 += lh;
    }

    if (ship->sys.pcap_rated > 0) {
        pbar(X2, y2, "Ph.Cap", ship->sys.pcap_charge, ship->sys.pcap_rated, SKYBLUE);
    }

    {
        char batt[48];
        std::snprintf(batt, sizeof(batt), "HET: %s  (needs 4 battery, Shift+Q/E)",
                      ship->het_used ? "used" : ship->sys.battery_charge >= 4 ? "ready" : "no battery");
        draw_text(batt, X2, y2, ship->het_used ? GRAY : ship->sys.battery_charge >= 4 ? Color{100,120,255,255} : GRAY);
        y2 += lh;
    }

    y2 += 4;
    set_color(MIDGRAY);
    SDL_RenderDrawLine(ren_, X2, y2, X2 + HALF, y2); y2 += 6;
    draw_text("WEAPONS", X2, y2, SKYBLUE); y2 += lh;

    for (auto& w : ship->weapons) {
        if (y2 > PY + PH - 14) break;
        Color wc = w.disabled ? RED :
                   w.fired    ? LIGHTGRAY :
                   w.armed    ? GREEN :
                   w.allocated ? GOLD : GRAY;
        const char* ws = w.disabled    ? "[DEST] " :
                         w.fired       ? "[FIRED]" :
                         w.armed       ? "[ARMED]" :
                         w.is_instant() ? (w.allocated > 0 ? " [ON]  " : " [-- ] ") :
                         w.allocated    ? "[ARMG] " : "[IDLE] ";
        char wb[64];
        std::snprintf(wb, sizeof(wb), "%-14s %s", w.label.c_str(), ws);
        draw_text(wb, X2, y2, wc);
        if (!w.is_instant() && !w.armed && !w.disabled && w.arming_turns > 0) {
            int bx = X2 + 210;
            for (int t = 0; t < w.arming_turns; ++t) {
                fill_rect(bx + t * 10, y2 + 2, 7, 10,
                          t < w.charge ? Color{40,180,40,255} : Color{45,45,55,255});
                outline_rect(bx + t * 10, y2 + 2, 7, 10, MIDGRAY);
            }
        }
        y2 += lh;
    }
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

// ── Explosion animation (call each frame while frame < 30) ───────────────────

void Renderer::draw_explosion(Hex pos, int frame) const {
    SDL_FPoint ctr = hex_to_pixel(pos);
    // Three expanding rings that fade out
    for (int ring = 0; ring < 3; ++ring) {
        float progress = std::min(1.0f, (frame - ring * 5) / 20.0f);
        if (progress <= 0.0f) continue;
        float radius = hex_size_ * progress * (0.5f + ring * 0.35f);
        Uint8 alpha  = (Uint8)(255 * (1.0f - progress));
        // Outer ring: orange-red; inner rings: yellow-white
        Color c = ring == 0 ? Color{255, 80, 10, alpha}
                : ring == 1 ? Color{255, 200, 30, alpha}
                            : Color{255, 255, 200, alpha};
        // Draw approximate circle as polygon points
        set_color(c);
        const int STEPS = 16;
        for (int s = 0; s < STEPS; ++s) {
            float a0 = 2.0f * 3.14159f * s / STEPS;
            float a1 = 2.0f * 3.14159f * (s + 1) / STEPS;
            SDL_RenderDrawLineF(ren_,
                ctr.x + radius * std::cos(a0), ctr.y + radius * std::sin(a0),
                ctr.x + radius * std::cos(a1), ctr.y + radius * std::sin(a1));
        }
    }
    // Central flash
    if (frame < 8) {
        Uint8 fa = (Uint8)(255 * (1.0f - frame / 8.0f));
        fill_hex(ctr, {255, 240, 180, (Uint8)(fa / 2)});
    }
}

// ── Seeking weapon markers ────────────────────────────────────────────────────

void Renderer::draw_seekers(const std::vector<SeekerView>& seekers) const {
    for (auto& sk : seekers) {
        SDL_FPoint ctr = hex_to_pixel(sk.pos);
        // Drone = small cyan diamond; Plasma = small orange circle
        if (sk.type_id == 0) {
            // Diamond
            set_color({0, 220, 220, 255});
            float r = hex_size_ * 0.18f;
            SDL_RenderDrawLineF(ren_, ctr.x,     ctr.y - r, ctr.x + r, ctr.y    );
            SDL_RenderDrawLineF(ren_, ctr.x + r, ctr.y,     ctr.x,     ctr.y + r);
            SDL_RenderDrawLineF(ren_, ctr.x,     ctr.y + r, ctr.x - r, ctr.y    );
            SDL_RenderDrawLineF(ren_, ctr.x - r, ctr.y,     ctr.x,     ctr.y - r);
        } else {
            // Small circle
            set_color({255, 100, 20, 255});
            float r = hex_size_ * 0.15f;
            const int STEPS = 8;
            for (int s = 0; s < STEPS; ++s) {
                float a0 = 2.0f * 3.14159f * s       / STEPS;
                float a1 = 2.0f * 3.14159f * (s + 1) / STEPS;
                SDL_RenderDrawLineF(ren_,
                    ctr.x + r * std::cos(a0), ctr.y + r * std::sin(a0),
                    ctr.x + r * std::cos(a1), ctr.y + r * std::sin(a1));
            }
        }
    }
}

// ── Game over overlay ─────────────────────────────────────────────────────────

// ── Setup screen ─────────────────────────────────────────────────────────────

static const char* faction_label(Faction f) {
    switch (f) {
        case Faction::Federation: return "FEDERATION";
        case Faction::Klingon:    return "KLINGON EMPIRE";
        case Faction::Romulan:    return "ROMULAN STAR EMPIRE";
        case Faction::Gorn:       return "GORN HEGEMONY";
        case Faction::Kzinti:     return "KZINTI HEGEMONY";
        case Faction::Tholian:    return "THOLIAN ASSEMBLY";
        case Faction::Orion:      return "ORION PIRATES";
        case Faction::Hydran:     return "HYDRAN KINGDOMS";
        case Faction::Lyran:      return "LYRAN STAR EMPIRE";
    }
    return "";
}
static Color faction_color(Faction f) {
    switch (f) {
        case Faction::Federation: return {100, 160, 255, 255};
        case Faction::Klingon:    return {220, 180,  20, 255};
        case Faction::Romulan:    return {210,  40,  40, 255};
        case Faction::Gorn:       return {100, 200,  60, 255};
        case Faction::Kzinti:     return {255, 140,  30, 255};
        case Faction::Tholian:    return {200, 140, 255, 255};
        case Faction::Orion:      return { 80, 200,  80, 255};
        case Faction::Hydran:     return { 60, 200, 220, 255};
        case Faction::Lyran:      return {255, 100, 150, 255};
    }
    return WHITE;
}
static const char* ctrl_label(ShipController c) {
    switch (c) {
        case ShipController::Player1: return " PLAYER 1 ";
        case ShipController::Player2: return " PLAYER 2 ";
        case ShipController::AI:      return "    AI    ";
    }
    return "";
}

void Renderer::draw_setup_screen(
    const std::vector<std::string>& names,
    const std::vector<std::string>& class_labels,
    const std::vector<Faction>& factions,
    const std::vector<bool>& included,
    const std::vector<ShipController>& controllers,
    int screen_w, int screen_h,
    std::vector<SetupButton>& out,
    int dropdown_row, int dropdown_scroll,
    const std::vector<std::pair<std::string,std::string>>* dd_items) const
{
    out.clear();
    fill_rect(0, 0, screen_w, screen_h, {8, 10, 18, 255});

    // Title bar
    fill_rect(0, 0, screen_w, 50, {10, 30, 70, 255});
    draw_text("STAR FLEET BATTLES — SCENARIO SETUP", screen_w/2 - 200, 14, WHITE);
    draw_text("Click [Class] to pick ship class. Assign controller. Then START BATTLE.",
              screen_w/2 - 270, 30, GRAY);

    const int BOX_W = 780, ROW_H = 50;
    const int BX = (screen_w - BOX_W) / 2;
    int y = 80;

    // Column headers
    draw_text("Ship",       BX + 44,  y + 6, LIGHTGRAY);
    draw_text("Faction",    BX + 200, y + 6, LIGHTGRAY);
    draw_text("Class",      BX + 370, y + 6, LIGHTGRAY);
    draw_text("Controller", BX + 560, y + 6, LIGHTGRAY);
    y += 28;

    // Collect per-row Y so the dropdown overlay can anchor
    std::vector<int> row_y;

    int n = (int)names.size();
    for (int i = 0; i < n; ++i) {
        row_y.push_back(y);
        Color row_bg = included[i] ? Color{20, 30, 45, 255} : Color{12, 14, 20, 255};
        fill_rect(BX, y, BOX_W, ROW_H - 4, row_bg);
        outline_rect(BX, y, BOX_W, ROW_H - 4,
                     included[i] ? faction_color(factions[i]) : MIDGRAY);

        // Include toggle
        Color tog = included[i] ? faction_color(factions[i]) : MIDGRAY;
        fill_rect(BX + 6, y + 10, 28, 28, included[i] ? Color{20,80,20,255} : Color{30,15,15,255});
        outline_rect(BX + 6, y + 10, 28, 28, tog);
        draw_text(included[i] ? "ON" : "  ", BX + 10, y + 15, tog);
        out.push_back({{BX + 6, y + 10, 28, 28}, i, 0});

        // Ship name
        draw_text(names[i].c_str(), BX + 44, y + 15, included[i] ? WHITE : GRAY);

        // Faction label
        draw_text(faction_label(factions[i]), BX + 200, y + 15,
                  included[i] ? faction_color(factions[i]) : GRAY);

        // Class dropdown button (shows full display label + arrow)
        bool dd_open_row = (dropdown_row == i);
        Color clbg = dd_open_row ? Color{50,50,110,255}
                   : (included[i]  ? Color{30,30, 70,255} : Color{18,18,28,255});
        fill_rect   (BX + 360, y + 9, 140, 28, clbg);
        outline_rect(BX + 360, y + 9, 140, 28,
                     dd_open_row ? WHITE : (included[i] ? faction_color(factions[i]) : MIDGRAY));
        draw_text(class_labels[i].c_str(), BX + 368, y + 15,
                  included[i] ? YELLOW : GRAY);
        draw_text(dd_open_row ? " v" : " >", BX + 480, y + 15,
                  included[i] ? LIGHTGRAY : GRAY);
        out.push_back({{BX + 360, y + 9, 140, 28}, i, 3});

        // Controller toggle button
        Color cbg = (controllers[i] == ShipController::AI)     ? Color{30,30,60,255}
                  : (controllers[i] == ShipController::Player1) ? Color{20,60,20,255}
                                                                 : Color{60,40,10,255};
        fill_rect   (BX + 510, y + 9, 130, 28, cbg);
        outline_rect(BX + 510, y + 9, 130, 28,
                     included[i] ? faction_color(factions[i]) : MIDGRAY);
        draw_text(ctrl_label(controllers[i]), BX + 516, y + 15,
                  included[i] ? WHITE : GRAY);
        out.push_back({{BX + 510, y + 9, 130, 28}, i, 1});

        y += ROW_H;
    }

    // Start Battle button
    y += 14;
    fill_rect   (BX + 200, y, 360, 40, {18, 80, 18, 255});
    outline_rect(BX + 200, y, 360, 40, GREEN);
    draw_text("START BATTLE", BX + 290, y + 12, GREEN);
    out.push_back({{BX + 200, y, 360, 40}, -1, 2});

    // Utility row: Save / Load / Quit
    y += 56;
    fill_rect   (BX + 10,  y, 120, 32, {22, 30, 52, 255});
    outline_rect(BX + 10,  y, 120, 32, {60, 100, 160, 255});
    draw_text("[S] Save", BX + 22, y + 8, LIGHTGRAY);
    out.push_back({{BX + 10, y, 120, 32}, -1, 5});

    fill_rect   (BX + 142, y, 120, 32, {22, 30, 52, 255});
    outline_rect(BX + 142, y, 120, 32, {60, 100, 160, 255});
    draw_text("[L] Load", BX + 154, y + 8, LIGHTGRAY);
    out.push_back({{BX + 142, y, 120, 32}, -1, 6});

    fill_rect   (BX + 650, y, 120, 32, {52, 16, 16, 255});
    outline_rect(BX + 650, y, 120, 32, RED);
    draw_text("[Q] Quit", BX + 662, y + 8, RED);
    out.push_back({{BX + 650, y, 120, 32}, -1, 7});

    // Dropdown overlay (drawn last so it floats above all rows)
    if (dropdown_row >= 0 && dropdown_row < n && dd_items && !dd_items->empty()) {
        const int DD_ITEM_H  = 22;
        const int DD_VISIBLE = 14;
        const int DD_X = BX + 360;
        const int DD_W = 220;
        int total   = (int)dd_items->size();
        int visible = (total < DD_VISIBLE) ? total : DD_VISIBLE;
        int DD_H    = visible * DD_ITEM_H + 4;

        // Open below the button, flip above if near bottom of screen
        int btn_bottom = row_y[dropdown_row] + 37;
        int DD_Y = (btn_bottom + DD_H <= screen_h - 60) ? btn_bottom
                                                         : row_y[dropdown_row] - DD_H;

        fill_rect   (DD_X, DD_Y, DD_W, DD_H, {12, 18, 36, 252});
        outline_rect(DD_X, DD_Y, DD_W, DD_H, {80, 140, 220, 255});

        int dy = DD_Y + 2;
        for (int k = dropdown_scroll; k < total && k < dropdown_scroll + DD_VISIBLE; ++k) {
            const auto& item = (*dd_items)[k];
            fill_rect(DD_X + 2, dy, DD_W - 4, DD_ITEM_H - 2, {22, 32, 58, 255});
            draw_text(item.first.c_str(),  DD_X + 6,  dy + 3, GRAY);
            draw_text(item.second.c_str(), DD_X + 70, dy + 3, YELLOW);
            out.push_back({{DD_X + 2, dy, DD_W - 4, DD_ITEM_H - 2},
                            dropdown_row, 4, k});
            dy += DD_ITEM_H;
        }

        // Scroll cues
        if (dropdown_scroll > 0)
            draw_text("^ more", DD_X + DD_W - 52, DD_Y + 3, MIDGRAY);
        if (dropdown_scroll + DD_VISIBLE < total)
            draw_text("v more", DD_X + DD_W - 52, DD_Y + DD_H - 14, MIDGRAY);
    }
}

// Needed a compile-time constant — but we don't have H here. Use a raw value.
// (The subtitle line is drawn via main.cpp instead.)

void Renderer::draw_pause_menu(int screen_w, int screen_h,
                                std::vector<PauseButton>& out) const
{
    out.clear();
    // Full-screen dim
    fill_rect(0, 0, screen_w, screen_h, {0, 0, 0, 175});

    const int MW = 300, MH = 310;
    const int MX = (screen_w - MW) / 2;
    const int MY = (screen_h - MH) / 2;

    fill_rect   (MX, MY, MW, MH, {16, 20, 32, 255});
    outline_rect(MX, MY, MW, MH, {80, 140, 220, 255});

    fill_rect(MX, MY, MW, 32, {8, 30, 68, 255});
    draw_text("GAME MENU", MX + MW / 2 - 44, MY + 8, WHITE);

    struct MI { const char* label; int action; bool danger; };
    const MI items[] = {
        { "Resume",           0, false },
        { "Save Scenario",    1, false },
        { "Load Scenario",    2, false },
        { "Return to Setup",  3, false },
        { "Quit Game",        4, true  },
    };

    int iy = MY + 44;
    for (const auto& m : items) {
        Color bg  = m.danger ? Color{55, 16, 16, 255} : Color{24, 34, 54, 255};
        Color brd = m.danger ? RED : Color{55, 95, 155, 255};
        Color fg  = m.danger ? RED : WHITE;
        fill_rect   (MX + 24, iy, MW - 48, 36, bg);
        outline_rect(MX + 24, iy, MW - 48, 36, brd);
        draw_text(m.label, MX + 40, iy + 10, fg);
        out.push_back({{MX + 24, iy, MW - 48, 36}, m.action});
        iy += 46;
    }
}

void Renderer::draw_crew_advice(const CrewAdvice& advice,
                                 int screen_w, int screen_h,
                                 int edit_role,
                                 std::vector<SDL_Rect>* out_rects) const {
    if (!advice.valid) return;
    int W = 520, H = 88;
    int X = 8, Y = screen_h - H - 8;
    fill_rect(X, Y, W, H, Color{10, 20, 40, 220});
    outline_rect(X, Y, W, H, Color{60, 120, 200, 255});
    draw_text("BRIDGE  (click name to edit)", X + 8, Y + 4, Color{80, 180, 255, 255});
    int y = Y + 18;
    const std::string* lines[4] = {&advice.helm, &advice.weapons, &advice.engineering, &advice.science};
    for (int i = 0; i < 4; ++i) {
        SDL_Rect row{X, y - 1, W, 15};
        if (out_rects) out_rects->push_back(row);
        bool editing = (edit_role == i);
        if (editing)
            fill_rect(row.x, row.y, row.w, row.h, Color{30, 60, 120, 200});
        Color c = editing ? Color{255, 255, 100, 255} : Color{200, 220, 255, 255};
        draw_text(lines[i]->c_str(), X + 8, y, c);
        if (editing)
            draw_text("_", X + 8 + (int)lines[i]->size() * 7, y, Color{255, 255, 0, 255});
        y += 15;
    }
}
void Renderer::draw_game_over(const std::string& winner, int screen_w, int screen_h) const {
    // Dim the whole screen
    fill_rect(0, 0, screen_w, screen_h, {0, 0, 0, 180});

    const int BW = 500, BH = 200;
    const int BX = (screen_w - BW) / 2;
    const int BY = (screen_h - BH) / 2;

    fill_rect(BX, BY, BW, BH, {15, 18, 28, 255});
    outline_rect(BX, BY, BW, BH, GOLD);
    outline_rect(BX + 2, BY + 2, BW - 4, BH - 4, {100, 80, 0, 200});

    draw_text("** BATTLE CONCLUDED **", BX + 140, BY + 30, GOLD);

    char buf[128];
    std::snprintf(buf, sizeof(buf), "%s VICTORIOUS", winner.c_str());
    draw_text(buf, BX + 80, BY + 80, WHITE);

    draw_text("Press [R] to restart or [Esc] to quit", BX + 60, BY + 140, GRAY);
}

// ── Combat log (last N events, bottom of board area) ─────────────────────────
void Renderer::draw_combat_log(const std::deque<std::string>& log,
                                int screen_w, int screen_h) const {
    if (log.empty()) return;
    const int board_w = screen_w - SIDEBAR_W;
    const int MAX_LINES = 7;
    const int LH = 16;
    const int PH = MAX_LINES * LH + 10;
    const int PY = screen_h - PH - 32; // above the message bar

    SDL_SetRenderDrawBlendMode(ren_, SDL_BLENDMODE_BLEND);
    fill_rect(0, PY, board_w, PH, {10, 14, 22, 200});
    SDL_SetRenderDrawBlendMode(ren_, SDL_BLENDMODE_NONE);

    int start = (int)log.size() > MAX_LINES ? (int)log.size() - MAX_LINES : 0;
    int y = PY + 5;
    for (int i = start; i < (int)log.size(); ++i) {
        float age = (float)(i - start) / MAX_LINES;
        Uint8 alpha = (Uint8)(120 + (int)(age * 135));
        draw_text(log[i].c_str(), 6, y, {alpha, alpha, alpha, 255});
        y += LH;
    }
}

// ── Shield hex diagram (drawn inside sidebar) ─────────────────────────────────
void Renderer::draw_shield_hex(const Ship& ship, int cx, int cy, int radius) const {
    // Fixed orientation: shield 1 (Fwd) at top, clockwise
    // Screen angles for flat-top hex faces (pointing toward face centre):
    // Fwd=270°, FwdR=330°, AftR=30°, Aft=90°, AftL=150°, FwdL=210°
    static const float FACE_DEG[6] = {270.f, 330.f, 30.f, 90.f, 150.f, 210.f};
    static const char* S_LBL[6]   = {"Fwd","FwdR","AftR","Aft","AftL","FwdL"};

    // Draw hex outline
    SDL_SetRenderDrawBlendMode(ren_, SDL_BLENDMODE_NONE);
    for (int i = 0; i < 6; ++i) {
        float a0 = (SFB_PI / 180.f) * (60.f * i - 30.f);
        float a1 = (SFB_PI / 180.f) * (60.f * (i + 1) - 30.f);
        int x0 = cx + (int)(radius * std::cos(a0));
        int y0 = cy + (int)(radius * std::sin(a0));
        int x1 = cx + (int)(radius * std::cos(a1));
        int y1 = cy + (int)(radius * std::sin(a1));
        set_color(MIDGRAY);
        SDL_RenderDrawLine(ren_, x0, y0, x1, y1);
    }

    // Draw shield value at each face
    for (int i = 0; i < 6; ++i) {
        float ang = (SFB_PI / 180.f) * FACE_DEG[i];
        int tx = cx + (int)((radius + 18) * std::cos(ang));
        int ty = cy + (int)((radius + 18) * std::sin(ang));

        int cur = ship.sys.shields[i], max = ship.sys.shields_max[i];
        int reinf = ship.eaf.reinforce[i];
        Color c = shield_color(cur, max);

        char buf[16];
        if (reinf > 0)
            std::snprintf(buf, sizeof(buf), "%d+%d", cur, reinf);
        else
            std::snprintf(buf, sizeof(buf), "%d", cur);

        // Offset text to centre it on the label point
        draw_text(buf, tx - 10, ty - 6, c);
        draw_text(S_LBL[i], tx - 12, ty + 4, GRAY);
    }

    draw_text("SHD", cx - 10, cy - 7, {60, 60, 80, 255});
}
