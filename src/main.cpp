#include "board.h"
#include "ship.h"
#include "renderer.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <vector>
#include <cstdio>
#include <algorithm>

enum class GamePhase { Planning, Impulse };

// ── Helper: determine which shield face of `target` is hit by `attacker` ────
static int hit_facing(const Ship& attacker, const Ship& target) {
    // The shield hit is the one facing toward the attacker
    // Compute direction from target to attacker in sextant space
    SDL_FPoint tp = {(float)target.position.q,  (float)target.position.r};
    SDL_FPoint ap = {(float)attacker.position.q, (float)attacker.position.r};
    // Use cube-coord angle
    // For simplicity: the hit facing is (target_facing + sextant_toward_attacker) % 6
    // We approximate by checking which of target's 6 directions is closest to attacker
    int best = 0, best_dist = 999;
    for (int d = 0; d < 6; ++d) {
        Hex nb = hex_neighbour(target.position, d);
        int dist = attacker.position.distance(nb);
        // Slightly weight the direction that lines up with the attacker
        // We want the facing whose adjacent hex is closest to the attacker
        if (dist < best_dist) { best_dist = dist; best = d; }
    }
    // Translate absolute direction `best` to shield index relative to target's facing
    int rel = (best - target.facing + 6) % 6;
    return rel;
}

int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return 1;
    if (TTF_Init() != 0) { SDL_Quit(); return 1; }

    const int W = 1280, H = 800;
    SDL_Window* win = SDL_CreateWindow(
        "Star Fleet Battles",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        W, H, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!win) { TTF_Quit(); SDL_Quit(); return 1; }

    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    if (!ren) { SDL_DestroyWindow(win); TTF_Quit(); SDL_Quit(); return 1; }

    TTF_Font* font = TTF_OpenFont("C:/Windows/Fonts/arial.ttf", 14);

    Board board(8);
    const int board_area_w = W - SIDEBAR_W;
    Renderer rend(ren, font, board_area_w, H, 40.0f);

    std::vector<Ship> ships = {
        make_federation_ca("Enterprise",  {0, -3}, 3),
        make_klingon_d7   ("IKS Klothos", {0,  3}, 0),
    };

    // ── Game state ────────────────────────────────────────────────────────────
    GamePhase phase    = GamePhase::Planning;
    int eaf_player     = 0;   // which ship is currently filling EAF (Planning phase)
    int turn           = 1;
    int impulse        = 1;

    int selected       = -1;  // selected ship index (Impulse phase)
    int selected_weapon= -1;  // weapon index for arc/fire mode (-1 = none)
    bool fire_mode     = false;

    // Damage text pop-up
    char damage_text[64] = "";
    int  damage_text_ttl = 0; // frames to show

    // Button lists rebuilt each frame
    std::vector<EafButton> eaf_buttons;
    std::vector<EafButton> weapon_buttons;

    bool running = true;
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = false;

            // ── Planning phase ─────────────────────────────────────────────
            if (phase == GamePhase::Planning) {
                if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
                    int mx = ev.button.x, my = ev.button.y;
                    // Hit-test EAF buttons
                    for (auto& btn : eaf_buttons) {
                        if (mx < btn.rect.x || mx > btn.rect.x + btn.rect.w) continue;
                        if (my < btn.rect.y || my > btn.rect.y + btn.rect.h) continue;

                        Ship& s = ships[eaf_player];
                        switch (btn.field) {
                        case EafField::Speed:
                            s.eaf.speed = std::clamp(s.eaf.speed + btn.delta, 0, 31);
                            break;
                        case EafField::Shield:
                            s.eaf.reinforce[btn.index] =
                                std::clamp(s.eaf.reinforce[btn.index] + btn.delta, 0, 8);
                            break;
                        case EafField::WeaponAlloc: {
                            Weapon& w = s.weapons[btn.index];
                            w.allocated = std::clamp(w.allocated + btn.delta, 0, w.max_power);
                            break;
                        }
                        case EafField::WeaponArm: {
                            Weapon& w = s.weapons[btn.index];
                            if (w.armed) {
                                // Disarm a ready torpedo/disruptor
                                w.armed = false; w.charge = 0; w.allocated = 0;
                            } else if (w.allocated) {
                                // Cancel arming
                                w.allocated = 0;
                            } else {
                                // Start arming (mark allocation; cost checked via eaf_cost)
                                w.allocated = w.arming_cost;
                            }
                            break;
                        }
                        case EafField::Commit: {
                            s.eaf_committed = true;
                            // Advance arming state for non-instant weapons
                            for (auto& w : s.weapons) {
                                if (!w.is_instant() && w.allocated > 0 && !w.armed) {
                                    w.charge++;
                                    if (w.charge >= w.arming_turns) w.armed = true;
                                } else if (!w.is_instant() && w.allocated == 0 && !w.armed) {
                                    w.charge = 0; // interrupted
                                }
                            }
                            // Next player or start impulse phase
                            eaf_player++;
                            if (eaf_player >= (int)ships.size()) {
                                phase      = GamePhase::Impulse;
                                eaf_player = 0;
                                selected   = -1;
                            }
                            break;
                        }
                        }
                        break; // handle only first hit
                    }
                }
            }

            // ── Impulse phase ──────────────────────────────────────────────
            else {
                if (ev.type == SDL_KEYDOWN) {
                    switch (ev.key.keysym.sym) {
                    case SDLK_q:
                        if (selected >= 0) {
                            ships[selected].facing = (ships[selected].facing + 5) % 6;
                            selected_weapon = -1;
                        }
                        break;
                    case SDLK_e:
                        if (selected >= 0) {
                            ships[selected].facing = (ships[selected].facing + 1) % 6;
                            selected_weapon = -1;
                        }
                        break;
                    case SDLK_f:
                        fire_mode = (selected >= 0) ? !fire_mode : false;
                        if (!fire_mode) selected_weapon = -1;
                        break;
                    case SDLK_ESCAPE:
                        if (fire_mode) { fire_mode = false; selected_weapon = -1; }
                        else           { selected = -1; }
                        break;
                    case SDLK_SPACE:
                        // Advance impulse
                        selected_weapon = -1;
                        fire_mode       = false;
                        if (impulse < 32) {
                            ++impulse;
                        } else {
                            // End of turn: reset for next planning phase
                            impulse    = 1;
                            ++turn;
                            phase      = GamePhase::Planning;
                            eaf_player = 0;
                            selected   = -1;
                            for (auto& ship : ships) {
                                ship.eaf_committed = false;
                                ship.eaf.speed = 0;
                                for (int i = 0; i < 6; ++i) ship.eaf.reinforce[i] = 0;
                                for (auto& w : ship.weapons) {
                                    if (w.is_instant()) { w.allocated = 0; w.fired = false; }
                                    else if (!w.armed)  { w.fired = false; }
                                    else                { w.fired = false; }
                                }
                            }
                        }
                        break;
                    }
                }

                if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
                    int mx = ev.button.x, my = ev.button.y;

                    // ── Click in sidebar weapon buttons ────────────────────
                    bool sidebar_hit = false;
                    for (auto& btn : weapon_buttons) {
                        if (mx < btn.rect.x || mx > btn.rect.x + btn.rect.w) continue;
                        if (my < btn.rect.y || my > btn.rect.y + btn.rect.h) continue;
                        selected_weapon = (btn.index == selected_weapon) ? -1 : btn.index;
                        fire_mode = (selected_weapon >= 0);
                        sidebar_hit = true;
                        break;
                    }
                    if (sidebar_hit) break;

                    // ── Click on board ─────────────────────────────────────
                    if (mx >= board_area_w) break;
                    SDL_FPoint mp = {(float)mx, (float)my};
                    Hex clicked = rend.pixel_to_hex(mp);
                    if (!board.contains(clicked)) break;

                    // Find what's on the clicked hex
                    int hit = -1;
                    for (int i = 0; i < (int)ships.size(); ++i)
                        if (ships[i].position.q == clicked.q &&
                            ships[i].position.r == clicked.r) { hit = i; break; }

                    if (fire_mode && selected >= 0 && selected_weapon >= 0) {
                        // ── Fire at target ─────────────────────────────────
                        if (hit >= 0 && hit != selected) {
                            Ship& atk = ships[selected];
                            Ship& tgt = ships[hit];
                            Weapon& w = atk.weapons[selected_weapon];

                            if (w.can_fire() &&
                                rend.hex_in_arc(atk.position, atk.facing, w.arc, tgt.position)) {
                                int range  = atk.position.distance(tgt.position);
                                int dmg    = w.damage_at(range);
                                int shield = hit_facing(atk, tgt);
                                tgt.apply_damage(dmg, shield);
                                w.fired    = true;
                                if (!w.is_instant()) { w.armed = false; w.charge = 0; }

                                std::snprintf(damage_text, sizeof(damage_text),
                                    "%s fires %s: %d damage (shield #%d, range %d)",
                                    atk.name.c_str(), w.label.c_str(), dmg, shield + 1, range);
                                damage_text_ttl = 180;
                                selected_weapon = -1;
                                fire_mode = false;
                            }
                        }
                    } else if (hit >= 0) {
                        // ── Select / deselect ship ─────────────────────────
                        selected        = (hit == selected) ? -1 : hit;
                        selected_weapon = -1;
                        fire_mode       = false;
                    } else if (selected >= 0 && !fire_mode) {
                        // ── Move selected ship ─────────────────────────────
                        Ship& sel = ships[selected];
                        if (sel.position.distance(clicked) == 1)
                            sel.position = clicked;
                        else
                            selected = -1;
                    }
                }
            }
        }

        // ── Render ────────────────────────────────────────────────────────────
        int mx, my;
        SDL_GetMouseState(&mx, &my);
        Hex hovered = rend.pixel_to_hex({(float)mx, (float)my});

        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);

        rend.draw_board(board);

        if (phase == GamePhase::Impulse) {
            if (fire_mode && selected >= 0 && selected_weapon >= 0)
                rend.draw_arc_overlay(ships[selected], selected_weapon, board, ships);
            else if (selected >= 0 && !fire_mode)
                rend.draw_move_options(ships[selected], board, ships);
        }

        rend.draw_ships(ships, selected);

        if (board.contains(hovered) && mx < board_area_w && phase == GamePhase::Impulse)
            rend.draw_highlight(hovered, YELLOW);

        // HUD
        rend.draw_hud(turn, impulse, W);

        // Sidebar
        Ship* sel_ship = (selected >= 0) ? &ships[selected] : nullptr;
        rend.draw_sidebar(sel_ship, selected_weapon, W, H, weapon_buttons);

        // EAF modal overlay (Planning phase)
        if (phase == GamePhase::Planning) {
            rend.draw_eaf_modal(ships[eaf_player], turn, W, H, eaf_buttons);
            // Show whose turn it is
            char whose[64];
            std::snprintf(whose, sizeof(whose), "Allocating: %s",
                          ships[eaf_player].name.c_str());
            rend.draw_text(whose, 10, 35, GOLD);
        }

        // Damage text pop-up
        if (damage_text_ttl > 0) {
            rend.draw_text(damage_text, 10, H - 30, ORANGE);
            --damage_text_ttl;
        }

        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }

    if (font) TTF_CloseFont(font);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
