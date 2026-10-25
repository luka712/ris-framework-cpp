#pragma once

#include <array>

#include "ris_export.h"

namespace ris {

/// Column-major 4x4 matrix (OpenGL layout), 16 contiguous floats.
struct Mat4 {
    std::array<float, 16> m{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    const float* data() const noexcept { return m.data(); }
    float* data() noexcept { return m.data(); }

    RIS_API static Mat4 identity() noexcept;
    RIS_API static Mat4 translation(float x, float y, float z) noexcept;
    RIS_API static Mat4 scale(float x, float y, float z) noexcept;
    RIS_API static Mat4 rotation_x(float radians) noexcept;
    RIS_API static Mat4 rotation_y(float radians) noexcept;
    RIS_API static Mat4 rotation_z(float radians) noexcept;
    RIS_API static Mat4 ortho(float left, float right, float bottom, float top, float near_z, float far_z) noexcept;
    RIS_API static Mat4 perspective(float fov_y_radians, float aspect, float near_z, float far_z) noexcept;
    RIS_API static Mat4 look_at(const std::array<float, 3>& eye, const std::array<float, 3>& target,
                                const std::array<float, 3>& up) noexcept;

    /// this * rhs
    RIS_API Mat4 operator*(const Mat4& rhs) const noexcept;
};

} // namespace ris
