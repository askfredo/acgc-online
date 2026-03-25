#pragma once

#include <vector>
#include <cstdint>
#include <glm/glm.hpp>
#include <span>

namespace nba {

// Type aliases
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using f32 = float;

// GX Primitive types
enum class GXPrimitive : u8 {
    Quads = 0x80,
    Triangles = 0x90,
    TriangleStrip = 0x98,
    TriangleFan = 0xA0,
};

// Vertex attribute format
enum class GXCompType : u8 {
    U8 = 0,
    S8 = 1,
    U16 = 2,
    S16 = 3,
    F32 = 4,
};

// VAT (Vertex Attribute Table) configuration
struct VATConfig {
    u8 vat_index = 0;
    
    // Position attribute
    bool has_position = true;
    u8 position_comp_count = 3;  // 2 or 3 components
    GXCompType position_type = GXCompType::F32;
    u8 position_shift = 0;  // for fixed-point formats
    
    // Normal attribute
    bool has_normal = false;
    u8 normal_comp_count = 3;
    GXCompType normal_type = GXCompType::F32;
    
    // Color0 attribute
    bool has_color0 = false;
    GXCompType color0_type = GXCompType::U8;  // RGB565, RGB8, RGBX8, RGBA4, RGBA6, RGBA8
    
    // TexCoord0 attribute
    bool has_texcoord0 = false;
    u8 texcoord0_comp_count = 2;
    GXCompType texcoord0_type = GXCompType::F32;
    u8 texcoord0_shift = 0;
    
    // Indexed mode (VAT 2 style)
    bool indexed = false;
    
    // Calculate stride for this VAT configuration
    [[nodiscard]] u32 calculateStride() const;
};

// Predefined VAT configurations for NBA Street Vol. 2
namespace VATPresets {
    // VAT 0: Court geometry (textured + lit) - 32 bytes
    [[nodiscard]] VATConfig CourtGeometryLit();
    
    // VAT 1: Court geometry (textured + vertex colored) - 24 bytes
    [[nodiscard]] VATConfig CourtGeometryColored();
    
    // VAT 2: Player models (indexed) - 6 bytes
    [[nodiscard]] VATConfig PlayerModelsIndexed();
    
    // VAT 3: UI/2D elements - 20 bytes
    [[nodiscard]] VATConfig UIElements();
}

// Output vertex format for OpenGL (48 bytes, interleaved)
struct GLVertex {
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f, 0.0f, 1.0f};
    glm::vec2 texcoord{0.0f};
    glm::vec4 color{1.0f};
    
    static constexpr u32 PositionOffset = 0;
    static constexpr u32 NormalOffset = 12;
    static constexpr u32 TexCoordOffset = 24;
    static constexpr u32 ColorOffset = 32;
    static constexpr u32 Stride = 48;
};

// Conversion statistics
struct ConversionStats {
    u32 input_vertex_count = 0;
    u32 output_vertex_count = 0;
    u32 triangle_count = 0;
    u32 degenerate_triangles_stripped = 0;
    u32 draw_calls = 0;
    
    void reset() {
        input_vertex_count = 0;
        output_vertex_count = 0;
        triangle_count = 0;
        degenerate_triangles_stripped = 0;
        draw_calls = 0;
    }
};

// Vertex attribute arrays for indexed mode (VAT 2)
struct IndexedVertexArrays {
    std::span<const glm::vec3> positions;
    std::span<const glm::vec3> normals;
    std::span<const glm::vec2> texcoords;
    std::span<const glm::vec4> colors;
};

// Result of conversion
struct ConversionResult {
    std::vector<GLVertex> vertices;
    std::vector<u32> indices;
    ConversionStats stats;
    bool success = false;
    const char* error_message = nullptr;
};

// Main converter class
class GXDisplayListConverter {
public:
    GXDisplayListConverter();
    ~GXDisplayListConverter() = default;
    
    // Non-copyable, movable
    GXDisplayListConverter(const GXDisplayListConverter&) = delete;
    GXDisplayListConverter& operator=(const GXDisplayListConverter&) = delete;
    GXDisplayListConverter(GXDisplayListConverter&&) noexcept = default;
    GXDisplayListConverter& operator=(GXDisplayListConverter&&) noexcept = default;
    
    // Convert a single display list
    [[nodiscard]] ConversionResult convert(
        std::span<const u8> display_list_data,
        const VATConfig& vat_config,
        const IndexedVertexArrays* indexed_arrays = nullptr
    );
    
    // Convert multiple display lists (batched)
    [[nodiscard]] ConversionResult convertBatch(
        std::span<const std::span<const u8>> display_lists,
        const VATConfig& vat_config,
        const IndexedVertexArrays* indexed_arrays = nullptr
    );
    
    // Get last conversion stats
    [[nodiscard]] const ConversionStats& getLastStats() const { return last_stats_; }
    
    // Enable/disable degenerate triangle stripping
    void setStripDegenerates(bool strip) { strip_degenerates_ = strip; }
    [[nodiscard]] bool getStripDegenerates() const { return strip_degenerates_; }
    
    // Reserve output buffers for expected vertex/index counts
    void reserve(u32 expected_vertices, u32 expected_indices);
    
    // Clear internal buffers
    void clear();

private:
    // Endian conversion helpers
    [[nodiscard]] static inline u16 swap16(u16 value) noexcept;
    [[nodiscard]] static inline u32 swap32(u32 value) noexcept;
    [[nodiscard]] static inline f32 swapF32(f32 value) noexcept;
    
    // Read functions that handle big-endian to little-endian conversion
    [[nodiscard]] static inline u16 readU16BE(const u8* ptr) noexcept;
    [[nodiscard]] static inline u32 readU32BE(const u8* ptr) noexcept;
    [[nodiscard]] static inline f32 readF32BE(const u8* ptr) noexcept;
    
    // Attribute reading functions
    [[nodiscard]] static glm::vec3 readPosition(const u8* ptr, const VATConfig& vat);
    [[nodiscard]] static glm::vec3 readNormal(const u8* ptr, const VATConfig& vat);
    [[nodiscard]] static glm::vec2 readTexCoord(const u8* ptr, const VATConfig& vat);
    [[nodiscard]] static glm::vec4 readColor(const u8* ptr, const VATConfig& vat);
    
    // Indexed mode helpers
    [[nodiscard]] static GLVertex dereferenceIndexedVertex(
        const u8* ptr,
        const VATConfig& vat,
        const IndexedVertexArrays& arrays
    );
    
    // Primitive conversion functions
    void convertTriangles(
        const u8* vertex_data,
        u32 vertex_count,
        const VATConfig& vat,
        const IndexedVertexArrays* indexed_arrays
    );
    
    void convertTriangleStrip(
        const u8* vertex_data,
        u32 vertex_count,
        const VATConfig& vat,
        const IndexedVertexArrays* indexed_arrays
    );
    
    void convertTriangleFan(
        const u8* vertex_data,
        u32 vertex_count,
        const VATConfig& vat,
        const IndexedVertexArrays* indexed_arrays
    );
    
    void convertQuads(
        const u8* vertex_data,
        u32 vertex_count,
        const VATConfig& vat,
        const IndexedVertexArrays* indexed_arrays
    );
    
    // Degenerate triangle check
    [[nodiscard]] static bool isDegenerate(
        const glm::vec3& v0,
        const glm::vec3& v1,
        const glm::vec3& v2
    ) noexcept;
    
    // Emit a triangle (adds 3 indices)
    void emitTriangle(u32 i0, u32 i1, u32 i2, const GLVertex* v0, const GLVertex* v1, const GLVertex* v2);
    
    // Precompute output vertex count for buffer reservation
    [[nodiscard]] static u32 estimateOutputVertices(u32 input_count, GXPrimitive prim);
    [[nodiscard]] static u32 estimateOutputIndices(u32 input_count, GXPrimitive prim);
    
    // Output buffers
    std::vector<GLVertex> output_vertices_;
    std::vector<u32> output_indices_;
    
    // Current write position (for indexed emission)
    u32 current_vertex_base_ = 0;
    
    // Stats
    ConversionStats last_stats_;
    ConversionStats accumulated_stats_;
    
    // Options
    bool strip_degenerates_ = true;
};

} // namespace nba
