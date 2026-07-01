#include "renderer.h"
#include <cmath>
#include <raymath.h>

static constexpr float SFB_PI = 3.14159265358979f;

Renderer::Renderer(int screen_w, int screen_h, float hex_size)
    : hex_size_(hex_size)
    , origin_{screen_w / 2.0f, screen_h / 2.0f}
{}

Vector2 Renderer::hex_corner(Vector2 centre, int i) const {
    float angle = SFB_PI / 180.0f * (60.0f * i - 30.0f); // pointy-top
    return { centre.x + hex_size_ * std::cos(angle),
             centre.y + hex_size_ * std::sin(angle) };
}

Vector2 Renderer::hex_to_pixel(Hex h) const {
    float x = origin_.x + hex_size_ * (std::sqrt(3.0f) * h.q + std::sqrt(3.0f) / 2.0f * h.r);
    float y = origin_.y + hex_size_ * (3.0f / 2.0f * h.r);
    return {x, y};
}

Hex Renderer::pixel_to_hex(Vector2 p) const {
    float px = (p.x - origin_.x) / hex_size_;
    float py = (p.y - origin_.y) / hex_size_;
    float fq = (std::sqrt(3.0f) / 3.0f * px - 1.0f / 3.0f * py);
    float fr = (2.0f / 3.0f * py);
    // Cube-round
    float fs = -fq - fr;
    int q = (int)std::round(fq), r = (int)std::round(fr), s = (int)std::round(fs);
    float dq = std::abs(q - fq), dr = std::abs(r - fr), ds = std::abs(s - fs);
    if (dq > dr && dq > ds) q = -r - s;
    else if (dr > ds)        r = -q - s;
    return {q, r};
}

void Renderer::draw_hex_outline(Vector2 centre, Color c) const {
    for (int i = 0; i < 6; ++i) {
        Vector2 a = hex_corner(centre, i);
        Vector2 b = hex_corner(centre, (i + 1) % 6);
        DrawLineV(a, b, c);
    }
}

void Renderer::draw_board(const Board& board) const {
    board.for_each([&](Hex h, const Cell& cell) {
        Vector2 centre = hex_to_pixel(h);
        Color c = (cell.terrain == Terrain::Nebula)   ? PURPLE :
                  (cell.terrain == Terrain::Asteroid) ? GRAY   : DARKBLUE;
        // Fill
        Vector2 pts[6];
        for (int i = 0; i < 6; ++i) pts[i] = hex_corner(centre, i);
        DrawTriangleFan(pts, 6, c);
        draw_hex_outline(centre, SKYBLUE);
    });
}

void Renderer::draw_ships(const std::vector<Ship>& ships) const {
    for (auto& ship : ships) {
        Vector2 centre = hex_to_pixel(ship.position);
        Color c = (ship.faction == Faction::Federation) ? GOLD :
                  (ship.faction == Faction::Klingon)    ? RED  : GREEN;
        DrawCircleV(centre, hex_size_ * 0.35f, c);
        // Facing indicator
        float angle = SFB_PI / 180.0f * (60.0f * ship.facing - 30.0f);
        Vector2 tip = { centre.x + hex_size_ * 0.45f * std::cos(angle),
                        centre.y + hex_size_ * 0.45f * std::sin(angle) };
        DrawLineV(centre, tip, WHITE);
    }
}

void Renderer::draw_highlight(Hex h, Color c) const {
    draw_hex_outline(hex_to_pixel(h), c);
}

