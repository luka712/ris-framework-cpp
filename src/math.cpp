#include "ris/math.hpp"

#include <cmath>

namespace ris {

Mat4 Mat4::identity() noexcept { return Mat4{}; }

Mat4 Mat4::translation(float x, float y, float z) noexcept {
    Mat4 r;
    r.m[12] = x;
    r.m[13] = y;
    r.m[14] = z;
    return r;
}

Mat4 Mat4::scale(float x, float y, float z) noexcept {
    Mat4 r;
    r.m[0] = x;
    r.m[5] = y;
    r.m[10] = z;
    return r;
}

Mat4 Mat4::rotation_x(float a) noexcept {
    Mat4 r;
    const float c = std::cos(a), s = std::sin(a);
    r.m[5] = c;
    r.m[6] = s;
    r.m[9] = -s;
    r.m[10] = c;
    return r;
}

Mat4 Mat4::rotation_y(float a) noexcept {
    Mat4 r;
    const float c = std::cos(a), s = std::sin(a);
    r.m[0] = c;
    r.m[2] = -s;
    r.m[8] = s;
    r.m[10] = c;
    return r;
}

Mat4 Mat4::rotation_z(float a) noexcept {
    Mat4 r;
    const float c = std::cos(a), s = std::sin(a);
    r.m[0] = c;
    r.m[1] = s;
    r.m[4] = -s;
    r.m[5] = c;
    return r;
}

Mat4 Mat4::ortho(float l, float r_, float b, float t, float n, float f) noexcept {
    Mat4 r;
    r.m[0] = 2.0f / (r_ - l);
    r.m[5] = 2.0f / (t - b);
    r.m[10] = -2.0f / (f - n);
    r.m[12] = -(r_ + l) / (r_ - l);
    r.m[13] = -(t + b) / (t - b);
    r.m[14] = -(f + n) / (f - n);
    return r;
}

Mat4 Mat4::perspective(float fov_y, float aspect, float n, float f) noexcept {
    Mat4 r;
    const float t = 1.0f / std::tan(fov_y * 0.5f);
    r.m = {t / aspect, 0, 0, 0, 0, t, 0, 0, 0, 0, (f + n) / (n - f), -1, 0, 0, (2.0f * f * n) / (n - f), 0};
    return r;
}

Mat4 Mat4::look_at(const std::array<float, 3>& eye, const std::array<float, 3>& target,
                   const std::array<float, 3>& up) noexcept {
    auto normalize = [](std::array<float, 3> v) {
        const float len = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        if (len > 0.0f) {
            for (float& c : v) c /= len;
        }
        return v;
    };
    auto cross = [](const std::array<float, 3>& a, const std::array<float, 3>& b) {
        return std::array<float, 3>{a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
    };
    auto dot = [](const std::array<float, 3>& a, const std::array<float, 3>& b) {
        return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    };

    const auto f = normalize({target[0] - eye[0], target[1] - eye[1], target[2] - eye[2]});
    const auto s = normalize(cross(f, up));
    const auto u = cross(s, f);

    Mat4 r;
    r.m = {s[0], u[0], -f[0], 0, s[1], u[1], -f[1], 0, s[2], u[2], -f[2], 0, -dot(s, eye), -dot(u, eye), dot(f, eye), 1};
    return r;
}

Mat4 Mat4::operator*(const Mat4& rhs) const noexcept {
    Mat4 out;
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) {
                sum += m[static_cast<size_t>(k * 4 + row)] * rhs.m[static_cast<size_t>(col * 4 + k)];
            }
            out.m[static_cast<size_t>(col * 4 + row)] = sum;
        }
    }
    return out;
}

} // namespace ris
