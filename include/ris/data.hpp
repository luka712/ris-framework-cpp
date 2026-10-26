#pragma once

#include "data.h"
#include "math.hpp"

namespace ris {

/// Linear RGBA colour, components in [0, 1]. Mirrors `Color` of ris-framework-ts, including its presets.
struct Color {
    float r = 0, g = 0, b = 0, a = 1;

    constexpr Color() = default;
    constexpr Color(float r_, float g_, float b_, float a_ = 1.0f) : r(r_), g(g_), b(b_), a(a_) {}

    glm::vec3 rgb() const { return {r, g, b}; }
    glm::vec4 rgba() const { return {r, g, b, a}; }
    constexpr bool operator==(const Color&) const = default;

    static constexpr Color white() { return {1, 1, 1, 1}; }
    static constexpr Color black() { return {0, 0, 0, 1}; }
    static constexpr Color transparent() { return {0, 0, 0, 0}; }
    static constexpr Color red() { return {1, 0, 0, 1}; }
    static constexpr Color green() { return {0, 1, 0, 1}; }
    static constexpr Color blue() { return {0, 0, 1, 1}; }
    static constexpr Color yellow() { return {1, 1, 0, 1}; }
    static constexpr Color orange() { return {1, 0.647f, 0, 1}; }
    static constexpr Color gray() { return {0.5f, 0.5f, 0.5f, 1}; }
    static constexpr Color lightPink() { return {1.0f, 0.71f, 0.76f, 1}; }
    static constexpr Color wheat() { return {0.96f, 0.87f, 0.70f, 1}; }
    static constexpr Color whiteSmoke() { return {0.96f, 0.96f, 0.96f, 1}; }
    static constexpr Color slateGray() { return {0.44f, 0.50f, 0.56f, 1}; }
    static constexpr Color cornerFlowerBlue() { return {0.39f, 0.58f, 0.93f, 1}; }
};

/// Axis aligned rectangle, y pointing down (screen space).
struct Rect {
    float x = 0, y = 0, width = 0, height = 0;

    constexpr Rect() = default;
    constexpr Rect(float x_, float y_, float w, float h) : x(x_), y(y_), width(w), height(h) {}

    constexpr float left() const { return x; }
    constexpr float top() const { return y; }
    constexpr float right() const { return x + width; }
    constexpr float bottom() const { return y + height; }

    constexpr bool contains(float px, float py) const {
        return px >= left() && px <= right() && py >= top() && py <= bottom();
    }
    constexpr bool intersects(const Rect& o) const {
        return left() < o.right() && right() > o.left() && top() < o.bottom() && bottom() > o.top();
    }
    constexpr bool operator==(const Rect&) const = default;
};

} // namespace ris
