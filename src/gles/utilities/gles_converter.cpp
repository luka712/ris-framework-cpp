#include "ris/gles/utilities/gles_converter.hpp"

#include "ris/rendering/texture/texture_utilities.hpp"
#include "internal.hpp"

using namespace ris::detail;

namespace ris::gles::converter {

namespace {

// Compressed formats that <GLES3/gl3.h> does not define. https://registry.khronos.org/OpenGL/api/GLES3/gl3.h
constexpr GLenum kCompressedRgbaS3tcDxt5 = 0x83F3;   // BC3, GL_EXT_texture_compression_s3tc
constexpr GLenum kCompressedRgbaBptcUnorm = 0x8E8C;  // BC7, GL_EXT_texture_compression_bptc
constexpr GLenum kCompressedRgbaAstc4x4 = 0x93B0;    // GL_KHR_texture_compression_astc_ldr

[[noreturn]] void unsupported(const char* what, uint32_t value) {
    throw std::domain_error(std::string(what) + ": value " + std::to_string(value) + " has no OpenGL ES 3 mapping");
}

} // namespace

// ---- primitives, culling ----------------------------------------------------------------------

GlEnum convert_primitive_type(Topology topology) {
    switch (topology) {
        case Topology::Triangles: return GL_TRIANGLES;
        case Topology::TriangleStrip: return GL_TRIANGLE_STRIP;
        case Topology::TriangleFan: return GL_TRIANGLE_FAN;
        case Topology::Lines: return GL_LINES;
        case Topology::LineStrip: return GL_LINE_STRIP;
        case Topology::Points: return GL_POINTS;
    }
    unsupported("convert_primitive_type", static_cast<uint32_t>(topology));
}

GlEnum convert_primitive_topology(PrimitiveTopology topology) {
    switch (topology) {
        case PrimitiveTopology::LineList: return GL_LINES;
        case PrimitiveTopology::LineStrip: return GL_LINE_STRIP;
        case PrimitiveTopology::PointList: return GL_POINTS;
        case PrimitiveTopology::TriangleList: return GL_TRIANGLES;
        case PrimitiveTopology::TriangleStrip: return GL_TRIANGLE_STRIP;
    }
    unsupported("convert_primitive_topology", static_cast<uint32_t>(topology));
}

GlEnum convert_cull_face(CullMode cull_mode) {
    switch (cull_mode) {
        case CullMode::None: return GL_NONE;
        case CullMode::Back: return GL_BACK;
        case CullMode::Front: return GL_FRONT;
    }
    unsupported("convert_cull_face", static_cast<uint32_t>(cull_mode));
}

GlEnum convert_front_face(FrontFace front_face) {
    switch (front_face) {
        case FrontFace::Cw: return GL_CW;
        case FrontFace::Ccw: return GL_CCW;
    }
    unsupported("convert_front_face", static_cast<uint32_t>(front_face));
}

// ---- buffers ----------------------------------------------------------------------------------

GlEnum convert_buffer_usage(BufferUsageFlags usage) {
    return has_flag(usage, BufferUsageFlags::CopyDst) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
}

GlEnum convert_buffer_usage(BufferUsage usage) {
    switch (usage) {
        case BufferUsage::Static: return GL_STATIC_DRAW;
        case BufferUsage::Dynamic: return GL_DYNAMIC_DRAW;
        case BufferUsage::Stream: return GL_STREAM_DRAW;
    }
    unsupported("convert_buffer_usage", static_cast<uint32_t>(usage));
}

GlEnum convert_buffer_target(BufferType type) {
    switch (type) {
        case BufferType::Vertex: return GL_ARRAY_BUFFER;
        case BufferType::Index: return GL_ELEMENT_ARRAY_BUFFER;
        case BufferType::Uniform: return GL_UNIFORM_BUFFER;
    }
    unsupported("convert_buffer_target", static_cast<uint32_t>(type));
}

GlEnum convert_index_type(IndexBufferType type) {
    switch (type) {
        case IndexBufferType::UInt16: return GL_UNSIGNED_SHORT;
        case IndexBufferType::UInt32: return GL_UNSIGNED_INT;
    }
    unsupported("convert_index_type", static_cast<uint32_t>(type));
}

GlEnum convert_index_type(IndexType type) {
    switch (type) {
        case IndexType::UInt16: return GL_UNSIGNED_SHORT;
        case IndexType::UInt32: return GL_UNSIGNED_INT;
        case IndexType::None: break;
    }
    throw std::invalid_argument("convert_index_type: IndexType::None has no OpenGL ES type");
}

// ---- vertex attributes ------------------------------------------------------------------------

uint32_t convert_vertex_format(VertexFormat format) {
    switch (format) {
        case VertexFormat::Float32: return 1;
        case VertexFormat::Float32x2: return 2;
        case VertexFormat::Float32x3: return 3;
        case VertexFormat::Float32x4: return 4;
        case VertexFormat::Float32x16: return 16;
    }
    unsupported("convert_vertex_format", static_cast<uint32_t>(format));
}

GlEnum convert_vertex_format_type(VertexFormat) noexcept { return GL_FLOAT; }

GlEnum convert_attrib_type(AttribType type) {
    switch (type) {
        case AttribType::Float: return GL_FLOAT;
        case AttribType::UByte: return GL_UNSIGNED_BYTE;
        case AttribType::Byte: return GL_BYTE;
        case AttribType::UShort: return GL_UNSIGNED_SHORT;
        case AttribType::Short: return GL_SHORT;
        case AttribType::UInt: return GL_UNSIGNED_INT;
        case AttribType::Int: return GL_INT;
    }
    unsupported("convert_attrib_type", static_cast<uint32_t>(type));
}

// ---- blending, depth --------------------------------------------------------------------------

GlEnum convert_blend_operation(BlendOperation operation) {
    switch (operation) {
        case BlendOperation::Add: return GL_FUNC_ADD;
        case BlendOperation::Subtract: return GL_FUNC_SUBTRACT;
        case BlendOperation::ReverseSubtract: return GL_FUNC_REVERSE_SUBTRACT;
        case BlendOperation::Min: return GL_MIN;
        case BlendOperation::Max: return GL_MAX;
    }
    unsupported("convert_blend_operation", static_cast<uint32_t>(operation));
}

GlEnum convert_blend_factor(BlendFactor factor) {
    switch (factor) {
        case BlendFactor::One: return GL_ONE;
        case BlendFactor::SrcAlpha: return GL_SRC_ALPHA;
        case BlendFactor::OneMinusSrcAlpha: return GL_ONE_MINUS_SRC_ALPHA;
        case BlendFactor::Zero: return GL_ZERO;
        case BlendFactor::SrcColor: return GL_SRC_COLOR;
        case BlendFactor::OneMinusSrcColor: return GL_ONE_MINUS_SRC_COLOR;
        case BlendFactor::DstColor: return GL_DST_COLOR;
        case BlendFactor::OneMinusDstColor: return GL_ONE_MINUS_DST_COLOR;
        case BlendFactor::DstAlpha: return GL_DST_ALPHA;
        case BlendFactor::OneMinusDstAlpha: return GL_ONE_MINUS_DST_ALPHA;
    }
    unsupported("convert_blend_factor", static_cast<uint32_t>(factor));
}

GlEnum convert_compare_func(CompareFunc func) {
    switch (func) {
        case CompareFunc::Never: return GL_NEVER;
        case CompareFunc::Less: return GL_LESS;
        case CompareFunc::Equal: return GL_EQUAL;
        case CompareFunc::LessEqual: return GL_LEQUAL;
        case CompareFunc::Greater: return GL_GREATER;
        case CompareFunc::NotEqual: return GL_NOTEQUAL;
        case CompareFunc::GreaterEqual: return GL_GEQUAL;
        case CompareFunc::Always: return GL_ALWAYS;
    }
    unsupported("convert_compare_func", static_cast<uint32_t>(func));
}

GlEnum convert_compare_function(SamplerCompareFunction func) {
    switch (func) {
        case SamplerCompareFunction::Never: return GL_NEVER;
        case SamplerCompareFunction::LessEqual: return GL_LEQUAL;
    }
    unsupported("convert_compare_function", static_cast<uint32_t>(func));
}

// ---- textures, samplers -----------------------------------------------------------------------

GlEnum convertToInternalFormat(TextureFormat format) {
    switch (format) {
        case TextureFormat::Rgba8Unorm: return GL_RGBA8;
        case TextureFormat::Rgba8UnormSrgb: return GL_SRGB8_ALPHA8;
        case TextureFormat::Depth32Float: return GL_DEPTH_COMPONENT32F;
        case TextureFormat::Depth24Stencil8: return GL_DEPTH24_STENCIL8;
        case TextureFormat::Bc3RgbaUnorm: return kCompressedRgbaS3tcDxt5;
        case TextureFormat::Etc2Rgba8Unorm: return GL_COMPRESSED_RGBA8_ETC2_EAC;
        case TextureFormat::Bc7RgbaUnorm: return kCompressedRgbaBptcUnorm;
        case TextureFormat::Astc4x4Rgba: return kCompressedRgbaAstc4x4;
        case TextureFormat::Undefined:
        case TextureFormat::Red32Float:
        case TextureFormat::Red32Typeless: break;
    }
    unsupported("convert_internal_format", static_cast<uint32_t>(format));
}

GlEnum convertToPixelFormat(TextureFormat format) {
    switch (format) {
        case TextureFormat::Rgba8Unorm:
        case TextureFormat::Rgba8UnormSrgb: return GL_RGBA;
        case TextureFormat::Depth32Float: return GL_DEPTH_COMPONENT;
        case TextureFormat::Depth24Stencil8: return GL_DEPTH_STENCIL;
        default: break;
    }
    unsupported("convert_to_pixel_format", static_cast<uint32_t>(format));
}

GlEnum convertToTextureType(TextureFormat format) {
    switch (format) {
        case TextureFormat::Rgba8Unorm:
        case TextureFormat::Rgba8UnormSrgb: return GL_UNSIGNED_BYTE;
        case TextureFormat::Depth32Float: return GL_FLOAT;
        case TextureFormat::Depth24Stencil8: return GL_UNSIGNED_INT_24_8;
        default: break;
    }
    unsupported("convert_to_texture_type", static_cast<uint32_t>(format));
}

bool is_compressed_format(TextureFormat format) noexcept {
    return TextureUtilities::isCompressedTextureFormat(format);
}

GlEnum convert_mag_filter(SamplerFilter filter) {
    switch (filter) {
        case SamplerFilter::Nearest: return GL_NEAREST;
        case SamplerFilter::Linear: return GL_LINEAR;
    }
    unsupported("convert_mag_filter", static_cast<uint32_t>(filter));
}

GlEnum convert_min_filter(SamplerFilter filter, MipMapSamplerFilter mip_map_filter) {
    if (mip_map_filter == MipMapSamplerFilter::None) return convert_mag_filter(filter);
    switch (filter) {
        case SamplerFilter::Nearest:
            switch (mip_map_filter) {
                case MipMapSamplerFilter::Nearest: return GL_NEAREST_MIPMAP_NEAREST;
                case MipMapSamplerFilter::Linear: return GL_NEAREST_MIPMAP_LINEAR;
                default: break;
            }
            break;
        case SamplerFilter::Linear:
            switch (mip_map_filter) {
                case MipMapSamplerFilter::Nearest: return GL_LINEAR_MIPMAP_NEAREST;
                case MipMapSamplerFilter::Linear: return GL_LINEAR_MIPMAP_LINEAR;
                default: break;
            }
            break;
    }
    unsupported("convert_min_filter", static_cast<uint32_t>(mip_map_filter));
}

GlEnum convert_address_mode(SamplerAddressMode mode) {
    switch (mode) {
        case SamplerAddressMode::Repeat: return GL_REPEAT;
        case SamplerAddressMode::MirrorRepeat: return GL_MIRRORED_REPEAT;
        case SamplerAddressMode::ClampToEdge: return GL_CLAMP_TO_EDGE;
        case SamplerAddressMode::ClampToBorder: return GL_CLAMP_TO_EDGE;   // no border colour in ES 3.0
    }
    unsupported("convert_address_mode", static_cast<uint32_t>(mode));
}

GlEnum convert_filter(Filter filter) {
    switch (filter) {
        case Filter::Nearest: return GL_NEAREST;
        case Filter::Linear: return GL_LINEAR;
        case Filter::NearestMipmapNearest: return GL_NEAREST_MIPMAP_NEAREST;
        case Filter::LinearMipmapNearest: return GL_LINEAR_MIPMAP_NEAREST;
        case Filter::NearestMipmapLinear: return GL_NEAREST_MIPMAP_LINEAR;
        case Filter::LinearMipmapLinear: return GL_LINEAR_MIPMAP_LINEAR;
    }
    unsupported("convert_filter", static_cast<uint32_t>(filter));
}

GlEnum convert_wrap(Wrap wrap) {
    switch (wrap) {
        case Wrap::Repeat: return GL_REPEAT;
        case Wrap::Clamp: return GL_CLAMP_TO_EDGE;
        case Wrap::Mirror: return GL_MIRRORED_REPEAT;
    }
    unsupported("convert_wrap", static_cast<uint32_t>(wrap));
}

} // namespace ris::gles::converter
