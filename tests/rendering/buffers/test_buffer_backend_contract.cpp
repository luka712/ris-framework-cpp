// A tiny in-memory backend written only against the graphics API neutral headers of ris/rendering/buffers/. It
// shows that a future backend (Vulkan, Metal, D3D, ...) can implement ABufferFactory and the abstract buffers
// without any OpenGL type, and reuses the shared validation. No display is needed.
#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <ris/rendering/buffers/buffers.hpp>

namespace {

namespace bv = ris::buffer_validation;
using ris::BufferUsageFlags;

/// Storage of the fake backend: plain bytes on the CPU.
struct Memory {
    std::vector<uint8_t> bytes;
    BufferUsageFlags usage = ris::BufferUsageFlags::None;
    std::string label;

    void write(size_t offset, std::span<const float> data, const char* what) {
        bv::check_update_allowed(usage, what);
        bv::check_update_range(bytes.size(), offset, data.data(), data.size_bytes(), what);
        std::memcpy(bytes.data() + offset, data.data(), data.size_bytes());
    }
};

class FakeVertexBuffer final : public ris::AVertexBuffer {
public:
    FakeVertexBuffer(size_t byte_size, uint32_t stride, BufferUsageFlags usage, std::string label)
        : stride_(stride) {
        mem_.usage = bv::resolve_vertex_buffer(byte_size, stride, usage, ris::BufferUsageFlags::Storage | ris::BufferUsageFlags::MapRead, "Fake");
        mem_.bytes.resize(byte_size);
        mem_.label = std::move(label);
    }
    const std::string& label() const noexcept override { return mem_.label; }
    size_t byte_size() const noexcept override { return mem_.bytes.size(); }
    BufferUsageFlags usage() const noexcept override { return mem_.usage; }
    uint32_t byte_stride() const noexcept override { return stride_; }
    void update(std::span<const float> data, size_t byte_offset = 0) override {
        mem_.write(byte_offset, data, "FakeVertexBuffer::update");
    }

private:
    Memory mem_;
    uint32_t stride_;
};

class FakeIndexBuffer final : public ris::AIndexBuffer {
public:
    FakeIndexBuffer(std::span<const uint32_t> indices, std::string label) {
        type_ = bv::select_index_type(indices);
        mem_.usage = bv::resolve_index_buffer(indices.size_bytes());
        mem_.bytes.resize(indices.size() * (type_ == ris::IndexBufferType::UInt16 ? 2u : 4u));
        mem_.label = std::move(label);
    }
    const std::string& label() const noexcept override { return mem_.label; }
    size_t byte_size() const noexcept override { return mem_.bytes.size(); }
    BufferUsageFlags usage() const noexcept override { return mem_.usage; }
    ris::IndexBufferType type() const noexcept override { return type_; }

private:
    Memory mem_;
    ris::IndexBufferType type_;
};

class FakeUniformBuffer final : public ris::AUniformBuffer {
public:
    FakeUniformBuffer(std::span<const float> data, BufferUsageFlags usage, std::string label) {
        const std::vector<float> padded = bv::pad_uniform_data(data);
        mem_.usage = bv::resolve_usage(usage, ris::BufferUsageFlags::Uniform, "Fake", ris::BufferUsageFlags::Storage | ris::BufferUsageFlags::MapRead, "Fake");
        mem_.bytes.resize(padded.size() * sizeof(float));
        std::memcpy(mem_.bytes.data(), padded.data(), mem_.bytes.size());
        mem_.label = std::move(label);
    }
    const std::string& label() const noexcept override { return mem_.label; }
    size_t byte_size() const noexcept override { return mem_.bytes.size(); }
    BufferUsageFlags usage() const noexcept override { return mem_.usage; }
    void update(std::span<const float> data, size_t byte_offset = 0) override {
        mem_.write(byte_offset, data, "FakeUniformBuffer::update");
    }
    using AUniformBuffer::update;
    float float_at(size_t i) const {
        float v;
        std::memcpy(&v, mem_.bytes.data() + i * sizeof(float), sizeof(float));
        return v;
    }

private:
    Memory mem_;
};

class FakeBufferFactory final : public ris::ABufferFactory {
public:
    std::unique_ptr<ris::AIndexBuffer> create_index_buffer(std::span<const uint32_t> indices,
                                                           std::string label = {}) const override {
        return std::make_unique<FakeIndexBuffer>(indices, std::move(label));
    }
    std::unique_ptr<ris::AIndexBuffer> create_index_buffer(std::span<const uint16_t> indices,
                                                           std::string label = {}) const override {
        const std::vector<uint32_t> wide(indices.begin(), indices.end());
        return std::make_unique<FakeIndexBuffer>(wide, std::move(label));
    }
    std::unique_ptr<ris::AIndexBuffer> create_index_buffer_uint32(std::span<const uint32_t> indices,
                                                                  std::string label = {}) const override {
        return create_index_buffer(indices, std::move(label));   // the fake always narrows
    }
    std::unique_ptr<ris::AVertexBuffer> create_vertex_buffer(std::span<const float> data, uint32_t byte_stride,
                                                             BufferUsageFlags usage = ris::BufferUsageFlags::Vertex,
                                                             std::string label = {}) const override {
        return std::make_unique<FakeVertexBuffer>(data.size_bytes(), byte_stride, usage, std::move(label));
    }
    std::unique_ptr<ris::AVertexBuffer> create_vertex_buffer_with_byte_size(
        size_t byte_size, uint32_t byte_stride, BufferUsageFlags usage = ris::BufferUsageFlags::Vertex | ris::BufferUsageFlags::CopyDst,
        std::string label = {}) const override {
        return std::make_unique<FakeVertexBuffer>(byte_size, byte_stride, usage, std::move(label));
    }
    std::unique_ptr<ris::AUniformBuffer> create_uniform_buffer(std::span<const float> data,
                                                               BufferUsageFlags usage = ris::BufferUsageFlags::Uniform | ris::BufferUsageFlags::CopyDst,
                                                               std::string label = {}) const override {
        return std::make_unique<FakeUniformBuffer>(data, usage, std::move(label));
    }
    std::unique_ptr<ris::AUniformBuffer> create_uniform_buffer_with_byte_size(
        size_t byte_size, BufferUsageFlags usage = ris::BufferUsageFlags::Uniform | ris::BufferUsageFlags::CopyDst, std::string label = {}) const override {
        const std::vector<float> zeros(bv::padded_uniform_byte_size(byte_size) / sizeof(float), 0.0f);
        return std::make_unique<FakeUniformBuffer>(zeros, usage, std::move(label));
    }
};

} // namespace

TEST_CASE("a backend can implement the abstract buffers without a graphics API") {
    const std::unique_ptr<ris::ABufferFactory> factory = std::make_unique<FakeBufferFactory>();
    const std::vector<float> data(10, 0.0f);

    auto vb = factory->create_vertex_buffer(data, 20, ris::BufferUsageFlags::Vertex | ris::BufferUsageFlags::CopyDst, "vb");
    REQUIRE(vb->label() == "vb");
    REQUIRE(vb->vertex_count() == 2);
    REQUIRE(vb->byte_stride() == 20);
    REQUIRE(vb->byte_size() == 40);
    REQUIRE(ris::has_flag(vb->usage(), ris::BufferUsageFlags::Vertex));
    const std::vector<float> one(5, 1.0f);
    REQUIRE_NOTHROW(vb->update(one, 20));
    REQUIRE_THROWS_AS(vb->update(one, 24), std::out_of_range);
    REQUIRE_THROWS_AS(factory->create_vertex_buffer(data, 20, ris::BufferUsageFlags::Vertex | ris::BufferUsageFlags::Index), std::invalid_argument);
    REQUIRE_THROWS_AS(factory->create_vertex_buffer(data, 16), std::invalid_argument);

    auto fixed = factory->create_vertex_buffer(data, 20);   // no CopyDst
    REQUIRE_THROWS_AS(fixed->update(one), std::invalid_argument);

    const std::vector<uint32_t> idx = {0, 1, 70000};
    auto ib = factory->create_index_buffer(std::span<const uint32_t>(idx), "ib");
    REQUIRE(ib->type() == ris::IndexBufferType::UInt32);
    REQUIRE(ib->indices_count() == 3);
    REQUIRE(ib->element_byte_size() == 4);
    REQUIRE(factory->create_index_buffer(std::span<const uint32_t>(idx.data(), 2))->type() ==
            ris::IndexBufferType::UInt16);

    auto ub = factory->create_uniform_buffer(one);
    auto* fake_ub = dynamic_cast<FakeUniformBuffer*>(ub.get());
    REQUIRE(fake_ub != nullptr);
    REQUIRE(ub->byte_size() == 32);
    ub->update(glm::vec4(1, 2, 3, 4), 16);   // convenience overloads of the abstract class
    REQUIRE(fake_ub->float_at(5) == 2.0f);
    REQUIRE_THROWS_AS(ub->update(glm::mat4(1.0f)), std::out_of_range);   // 64 bytes do not fit into 32
}
