// Tests of the gles converter. The functions are pure, no window is needed. The expected values are the
// OpenGL ES 3 constants (https://registry.khronos.org/OpenGL/api/GLES3/gl3.h).
#include <catch2/catch_test_macros.hpp>

#include <ris/gles/gles.hpp>

namespace conv = ris::gles::converter;

TEST_CASE("converter: primitives and culling") {
    REQUIRE(conv::convert_primitive_type(ris::Topology::Points) == 0x0000);
    REQUIRE(conv::convert_primitive_type(ris::Topology::Lines) == 0x0001);
    REQUIRE(conv::convert_primitive_type(ris::Topology::LineStrip) == 0x0003);
    REQUIRE(conv::convert_primitive_type(ris::Topology::Triangles) == 0x0004);
    REQUIRE(conv::convert_primitive_type(ris::Topology::TriangleStrip) == 0x0005);
    REQUIRE(conv::convert_primitive_type(ris::Topology::TriangleFan) == 0x0006);

    REQUIRE(conv::convert_cull_face(ris::CullMode::None) == 0);
    REQUIRE(conv::convert_cull_face(ris::CullMode::Front) == 0x0404);
    REQUIRE(conv::convert_cull_face(ris::CullMode::Back) == 0x0405);
    REQUIRE(conv::convert_front_face(ris::FrontFace::Cw) == 0x0900);
    REQUIRE(conv::convert_front_face(ris::FrontFace::Ccw) == 0x0901);
}

TEST_CASE("converter: buffers") {
    REQUIRE(conv::convert_buffer_usage(ris::BufferUsageFlags::Vertex) == 0x88E4);                   // GL_STATIC_DRAW
    REQUIRE(conv::convert_buffer_usage(ris::BufferUsageFlags::Vertex | ris::BufferUsageFlags::CopyDst) == 0x88E8);      // GL_DYNAMIC_DRAW
    REQUIRE(conv::convert_buffer_usage(ris::BufferUsage::Static) == 0x88E4);
    REQUIRE(conv::convert_buffer_usage(ris::BufferUsage::Dynamic) == 0x88E8);
    REQUIRE(conv::convert_buffer_usage(ris::BufferUsage::Stream) == 0x88E0);

    REQUIRE(conv::convert_buffer_target(ris::BufferType::Vertex) == 0x8892);
    REQUIRE(conv::convert_buffer_target(ris::BufferType::Index) == 0x8893);
    REQUIRE(conv::convert_buffer_target(ris::BufferType::Uniform) == 0x8A11);

    REQUIRE(conv::convert_index_type(ris::IndexBufferType::UInt16) == 0x1403);
    REQUIRE(conv::convert_index_type(ris::IndexBufferType::UInt32) == 0x1405);
    REQUIRE(conv::convert_index_type(ris::IndexType::UInt16) == 0x1403);
    REQUIRE(conv::convert_index_type(ris::IndexType::UInt32) == 0x1405);
    REQUIRE_THROWS_AS(conv::convert_index_type(ris::IndexType::None), std::invalid_argument);
}

TEST_CASE("converter: vertex formats and attribute types") {
    REQUIRE(conv::convert_vertex_format(ris::VertexFormat::Float32) == 1);
    REQUIRE(conv::convert_vertex_format(ris::VertexFormat::Float32x2) == 2);
    REQUIRE(conv::convert_vertex_format(ris::VertexFormat::Float32x3) == 3);
    REQUIRE(conv::convert_vertex_format(ris::VertexFormat::Float32x4) == 4);
    REQUIRE(conv::convert_vertex_format(ris::VertexFormat::Float32x16) == 16);
    REQUIRE(conv::convert_vertex_format_type(ris::VertexFormat::Float32x3) == 0x1406);

    REQUIRE(conv::convert_attrib_type(ris::AttribType::Byte) == 0x1400);
    REQUIRE(conv::convert_attrib_type(ris::AttribType::UByte) == 0x1401);
    REQUIRE(conv::convert_attrib_type(ris::AttribType::Short) == 0x1402);
    REQUIRE(conv::convert_attrib_type(ris::AttribType::UShort) == 0x1403);
    REQUIRE(conv::convert_attrib_type(ris::AttribType::Int) == 0x1404);
    REQUIRE(conv::convert_attrib_type(ris::AttribType::UInt) == 0x1405);
    REQUIRE(conv::convert_attrib_type(ris::AttribType::Float) == 0x1406);
}

TEST_CASE("converter: blending and depth") {
    using ris::BlendOperation;
    REQUIRE(conv::convert_blend_operation(BlendOperation::Add) == 0x8006);
    REQUIRE(conv::convert_blend_operation(BlendOperation::Min) == 0x8007);
    REQUIRE(conv::convert_blend_operation(BlendOperation::Max) == 0x8008);
    REQUIRE(conv::convert_blend_operation(BlendOperation::Subtract) == 0x800A);
    REQUIRE(conv::convert_blend_operation(BlendOperation::ReverseSubtract) == 0x800B);

    using ris::BlendFactor;
    REQUIRE(conv::convert_blend_factor(BlendFactor::Zero) == 0);
    REQUIRE(conv::convert_blend_factor(BlendFactor::One) == 1);
    REQUIRE(conv::convert_blend_factor(BlendFactor::SrcColor) == 0x0300);
    REQUIRE(conv::convert_blend_factor(BlendFactor::OneMinusSrcColor) == 0x0301);
    REQUIRE(conv::convert_blend_factor(BlendFactor::SrcAlpha) == 0x0302);
    REQUIRE(conv::convert_blend_factor(BlendFactor::OneMinusSrcAlpha) == 0x0303);
    REQUIRE(conv::convert_blend_factor(BlendFactor::DstAlpha) == 0x0304);
    REQUIRE(conv::convert_blend_factor(BlendFactor::OneMinusDstAlpha) == 0x0305);
    REQUIRE(conv::convert_blend_factor(BlendFactor::DstColor) == 0x0306);
    REQUIRE(conv::convert_blend_factor(BlendFactor::OneMinusDstColor) == 0x0307);

    using ris::CompareFunc;
    REQUIRE(conv::convert_compare_func(CompareFunc::Never) == 0x0200);
    REQUIRE(conv::convert_compare_func(CompareFunc::Less) == 0x0201);
    REQUIRE(conv::convert_compare_func(CompareFunc::Equal) == 0x0202);
    REQUIRE(conv::convert_compare_func(CompareFunc::LessEqual) == 0x0203);
    REQUIRE(conv::convert_compare_func(CompareFunc::Greater) == 0x0204);
    REQUIRE(conv::convert_compare_func(CompareFunc::NotEqual) == 0x0205);
    REQUIRE(conv::convert_compare_func(CompareFunc::GreaterEqual) == 0x0206);
    REQUIRE(conv::convert_compare_func(CompareFunc::Always) == 0x0207);
    REQUIRE(conv::convert_compare_function(ris::SamplerCompareFunction::Never) == 0x0200);
    REQUIRE(conv::convert_compare_function(ris::SamplerCompareFunction::LessEqual) == 0x0203);
}

TEST_CASE("converter: texture formats") {
    using ris::TextureFormat;
    REQUIRE(conv::convertToInternalFormat(TextureFormat::Rgba8Unorm) == 0x8058);
    REQUIRE(conv::convertToInternalFormat(TextureFormat::Rgba8UnormSrgb) == 0x8C43);
    REQUIRE(conv::convertToInternalFormat(TextureFormat::Depth32Float) == 0x8CAC);
    REQUIRE(conv::convertToInternalFormat(TextureFormat::Depth24Stencil8) == 0x88F0);
    REQUIRE(conv::convertToInternalFormat(TextureFormat::Bc3RgbaUnorm) == 0x83F3);
    REQUIRE(conv::convertToInternalFormat(TextureFormat::Bc7RgbaUnorm) == 0x8E8C);
    REQUIRE(conv::convertToInternalFormat(TextureFormat::Etc2Rgba8Unorm) == 0x9278);
    REQUIRE(conv::convertToInternalFormat(TextureFormat::Astc4x4Rgba) == 0x93B0);
    REQUIRE_THROWS_AS(conv::convertToInternalFormat(TextureFormat::Undefined), std::domain_error);
    REQUIRE_THROWS_AS(conv::convertToInternalFormat(TextureFormat::Red32Float), std::domain_error);

    REQUIRE(conv::convertToPixelFormat(TextureFormat::Rgba8Unorm) == 0x1908);
    REQUIRE(conv::convertToPixelFormat(TextureFormat::Rgba8UnormSrgb) == 0x1908);
    REQUIRE(conv::convertToPixelFormat(TextureFormat::Depth32Float) == 0x1902);
    REQUIRE(conv::convertToPixelFormat(TextureFormat::Depth24Stencil8) == 0x84F9);
    REQUIRE_THROWS_AS(conv::convertToPixelFormat(TextureFormat::Astc4x4Rgba), std::domain_error);

    REQUIRE(conv::convertToTextureType(TextureFormat::Rgba8Unorm) == 0x1401);
    REQUIRE(conv::convertToTextureType(TextureFormat::Depth32Float) == 0x1406);
    REQUIRE(conv::convertToTextureType(TextureFormat::Depth24Stencil8) == 0x84FA);
    REQUIRE_THROWS_AS(conv::convertToTextureType(TextureFormat::Etc2Rgba8Unorm), std::domain_error);

    REQUIRE(conv::is_compressed_format(TextureFormat::Bc3RgbaUnorm));
    REQUIRE(conv::is_compressed_format(TextureFormat::Bc7RgbaUnorm));
    REQUIRE(conv::is_compressed_format(TextureFormat::Etc2Rgba8Unorm));
    REQUIRE(conv::is_compressed_format(TextureFormat::Astc4x4Rgba));
    REQUIRE_FALSE(conv::is_compressed_format(TextureFormat::Rgba8Unorm));
    REQUIRE_FALSE(conv::is_compressed_format(TextureFormat::Depth32Float));
}

TEST_CASE("converter: samplers") {
    using ris::MipMapSamplerFilter;
    using ris::SamplerFilter;
    REQUIRE(conv::convert_mag_filter(SamplerFilter::Nearest) == 0x2600);
    REQUIRE(conv::convert_mag_filter(SamplerFilter::Linear) == 0x2601);

    // without a mip map filter the minification filter is the plain one
    REQUIRE(conv::convert_min_filter(SamplerFilter::Nearest, MipMapSamplerFilter::None) == 0x2600);
    REQUIRE(conv::convert_min_filter(SamplerFilter::Linear, MipMapSamplerFilter::None) == 0x2601);
    REQUIRE(conv::convert_min_filter(SamplerFilter::Nearest, MipMapSamplerFilter::Nearest) == 0x2700);
    REQUIRE(conv::convert_min_filter(SamplerFilter::Linear, MipMapSamplerFilter::Nearest) == 0x2701);
    REQUIRE(conv::convert_min_filter(SamplerFilter::Nearest, MipMapSamplerFilter::Linear) == 0x2702);
    REQUIRE(conv::convert_min_filter(SamplerFilter::Linear, MipMapSamplerFilter::Linear) == 0x2703);

    using ris::SamplerAddressMode;
    REQUIRE(conv::convert_address_mode(SamplerAddressMode::Repeat) == 0x2901);
    REQUIRE(conv::convert_address_mode(SamplerAddressMode::MirrorRepeat) == 0x8370);
    REQUIRE(conv::convert_address_mode(SamplerAddressMode::ClampToEdge) == 0x812F);
    REQUIRE(conv::convert_address_mode(SamplerAddressMode::ClampToBorder) == 0x812F);   // no border colour in ES 3.0

    REQUIRE(conv::convert_filter(ris::Filter::Nearest) == 0x2600);
    REQUIRE(conv::convert_filter(ris::Filter::Linear) == 0x2601);
    REQUIRE(conv::convert_filter(ris::Filter::NearestMipmapNearest) == 0x2700);
    REQUIRE(conv::convert_filter(ris::Filter::LinearMipmapNearest) == 0x2701);
    REQUIRE(conv::convert_filter(ris::Filter::NearestMipmapLinear) == 0x2702);
    REQUIRE(conv::convert_filter(ris::Filter::LinearMipmapLinear) == 0x2703);
    REQUIRE(conv::convert_wrap(ris::Wrap::Repeat) == 0x2901);
    REQUIRE(conv::convert_wrap(ris::Wrap::Clamp) == 0x812F);
    REQUIRE(conv::convert_wrap(ris::Wrap::Mirror) == 0x8370);
}

TEST_CASE("converter: values outside an enum throw") {
    REQUIRE_THROWS_AS(conv::convert_blend_factor(static_cast<ris::BlendFactor>(99)), std::domain_error);
    REQUIRE_THROWS_AS(conv::convert_cull_face(static_cast<ris::CullMode>(99)), std::domain_error);
    REQUIRE_THROWS_AS(conv::convert_wrap(static_cast<ris::Wrap>(99)), std::domain_error);
}
