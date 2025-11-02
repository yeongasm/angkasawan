module hana;

public typedef int8_t int8;
public typedef int16_t int16;
public typedef int int32;
public typedef uint64_t int64;
public typedef uint8_t uint8;
public typedef uint16_t uint16;
public typedef uint uint32;
public typedef uint64_t uint64;
public typedef half float16;
public typedef float float32;
public typedef double float64;
public typedef vector<float32, 2> vec2;
public typedef vector<float32, 3> vec3;
public typedef vector<float32, 4> vec4;
public typedef vector<int32, 2> ivec2;
public typedef vector<int32, 3> ivec3;
public typedef vector<int32, 4> ivec4;
public typedef vector<uint32, 2> uvec2;
public typedef vector<uint32, 3> uvec3;
public typedef vector<uint32, 4> uvec4;
public typedef vector<float64, 2> dvec2;
public typedef vector<float64, 3> dvec3;
public typedef vector<float64, 4> dvec4;
public typedef vector<int64, 2> i64vec2;
public typedef vector<int64, 3> i64vec3;
public typedef vector<int64, 4> i64vec4;
public typedef vector<uint64, 2> u64vec2;
public typedef vector<uint64, 3> u64vec3;
public typedef vector<uint64, 4> u64vec4;
public typedef matrix<float32, 2, 2> mat2;
public typedef matrix<float32, 2, 3> mat2x3;
public typedef matrix<float32, 2, 4> mat2x4;
public typedef matrix<float32, 3, 2> mat3x2;
public typedef matrix<float32, 3, 3> mat3;
public typedef matrix<float32, 3, 4> mat3x4;
public typedef matrix<float32, 4, 2> mat4x2;
public typedef matrix<float32, 4, 3> mat4x3;
public typedef matrix<float32, 4, 4> mat4;
public typedef matrix<float64, 2, 2> dmat2;
public typedef matrix<float64, 2, 3> dmat2x3;
public typedef matrix<float64, 2, 4> dmat2x4;
public typedef matrix<float64, 3, 2> dmat3x2;
public typedef matrix<float64, 3, 3> dmat3;
public typedef matrix<float64, 3, 4> dmat3x4;
public typedef matrix<float64, 4, 2> dmat4x2;
public typedef matrix<float64, 4, 3> dmat4x3;
public typedef matrix<float64, 4, 4> dmat4;
public typedef matrix<int32, 2, 2> imat2;
public typedef matrix<int32, 2, 3> imat2x3;
public typedef matrix<int32, 2, 4> imat2x4;
public typedef matrix<int32, 3, 2> imat3x2;
public typedef matrix<int32, 3, 3> imat3;
public typedef matrix<int32, 3, 4> imat3x4;
public typedef matrix<int32, 4, 2> imat4x2;
public typedef matrix<int32, 4, 3> imat4x3;
public typedef matrix<int32, 4, 4> imat4;
public typedef matrix<uint32, 2, 2> umat2;
public typedef matrix<uint32, 2, 3> umat2x3;
public typedef matrix<uint32, 2, 4> umat2x4;
public typedef matrix<uint32, 3, 2> umat3x2;
public typedef matrix<uint32, 3, 3> umat3;
public typedef matrix<uint32, 3, 4> umat3x4;
public typedef matrix<uint32, 4, 2> umat4x2;
public typedef matrix<uint32, 4, 3> umat4x3;
public typedef matrix<uint32, 4, 4> umat4;
public typedef matrix<int64, 2, 2> i64mat2;
public typedef matrix<int64, 2, 3> i64mat2x3;
public typedef matrix<int64, 2, 4> i64mat2x4;
public typedef matrix<int64, 3, 2> i64mat3x2;
public typedef matrix<int64, 3, 3> i64mat3;
public typedef matrix<int64, 3, 4> i64mat3x4;
public typedef matrix<int64, 4, 2> i64mat4x2;
public typedef matrix<int64, 4, 3> i64mat4x3;
public typedef matrix<int64, 4, 4> i64mat4;
public typedef matrix<uint64, 2, 2> u64mat2;
public typedef matrix<uint64, 2, 3> u64mat2x3;
public typedef matrix<uint64, 2, 4> u64mat2x4;
public typedef matrix<uint64, 3, 2> u64mat3x2;
public typedef matrix<uint64, 3, 3> u64mat3;
public typedef matrix<uint64, 3, 4> u64mat3x4;
public typedef matrix<uint64, 4, 2> u64mat4x2;
public typedef matrix<uint64, 4, 3> u64mat4x3;
public typedef matrix<uint64, 4, 4> u64mat4;

#ifndef STORAGE_BUFFER_BINDING
#define STORAGE_BUFFER_BINDING 0
#endif

#ifndef STORAGE_IMAGE_BINDING
#define STORAGE_IMAGE_BINDING 1
#endif

#ifndef SAMPLED_IMAGE_BINDING
#define SAMPLED_IMAGE_BINDING 2
#endif

#ifndef SAMPLER_BINDING
#define SAMPLER_BINDING 3
#endif

#ifndef BUFFER_DEVICE_ADDRESS_BINDING
#define BUFFER_DEVICE_ADDRESS_BINDING 4
#endif

namespace hana
{
[[vk::binding(STORAGE_BUFFER_BINDING, 0)]] __DynamicResource storage_buffers[];
[[vk::binding(STORAGE_BUFFER_BINDING, 0)]] coherent __DynamicResource storage_buffers_coherent[];
[[vk::binding(STORAGE_IMAGE_BINDING, 0)]] __DynamicResource storage_images[];
[[vk::binding(STORAGE_IMAGE_BINDING, 0)]] coherent __DynamicResource storage_images_coherent[];
[[vk::binding(SAMPLED_IMAGE_BINDING, 0)]] __DynamicResource sampled_images[];
[[vk::binding(SAMPLED_IMAGE_BINDING, 0)]] coherent __DynamicResource sampled_images_coherent[];
[[vk::binding(SAMPLER_BINDING, 0)]] SamplerState samplers[];
[[vk::binding(BUFFER_DEVICE_ADDRESS_BINDING, 0)]] StructuredBuffer<uint64_t> bda;

public struct Image
{
    private uint64 value;

    public func id() -> uint32 
    { 
        return uint32(value & 0xFFFFFFFF);
    };
};

public struct Sampler
{
    private uint64 value;

    public func id() -> uint32
    {
        return uint32(value & 0xFFFFFFFF);
    }

    public static func get(Sampler sampler) -> SamplerState
    {
        return samplers[sampler.id()];
    }
};

public struct Buffer
{
    private uint64 value;

    public func id() -> uint32
    {
        return uint32(value & 0xFFFFFFFF);
    }
};
}

public __generic<T : ITexelElement> extension RWTexture1D<T>
{
    public static func get(hana::Image image) -> RWTexture1D<T> 
    { 
        return hana::storage_images[image.id()]; 
    }

    public static func get_coherent(hana::Image image) -> RWTexture1D<T> 
    { 
        return hana::storage_images_coherent[image.id()]; 
    }
};

public __generic<T : ITexelElement> extension RWTexture2D<T>
{
    public static func get(hana::Image image) -> RWTexture2D<T> 
    { 
        return hana::storage_images[image.id()]; 
    }

    public static func get_coherent(hana::Image image) -> RWTexture2D<T> 
    { 
        return hana::storage_images_coherent[image.id()]; 
    }
};

public __generic<T : ITexelElement> extension RWTexture3D<T>
{
    public static func get(hana::Image image) -> RWTexture3D<T> 
    { 
        return hana::storage_images[image.id()]; 
    }

    public static func get_coherent(hana::Image image) -> RWTexture3D<T> 
    { 
        return hana::storage_images_coherent[image.id()]; 
    }
};

public __generic<T : ITexelElement> extension RWTexture1DArray<T>
{
    public static func get(hana::Image image) -> RWTexture1DArray<T> 
    { 
        return hana::storage_images[image.id()]; 
    }

    public static func get_coherent(hana::Image image) -> RWTexture1DArray<T> 
    { 
        return hana::storage_images_coherent[image.id()]; 
    }
};

public __generic<T : ITexelElement> extension RWTexture2DArray<T>
{
    public static func get(hana::Image image) -> RWTexture2DArray<T> 
    { 
        return hana::storage_images[image.id()]; 
    }

    public static func get_coherent(hana::Image image) -> RWTexture2DArray<T> 
    { 
        return hana::storage_images_coherent[image.id()]; 
    }
};

public __generic<T : ITexelElement> extension Texture1D<T>
{
    public static func get(hana::Image image) -> Texture1D<T> 
    { 
        return hana::sampled_images[image.id()]; 
    }

    public static func get_coherent(hana::Image image) -> Texture1D<T> 
    { 
        return hana::sampled_images_coherent[image.id()]; 
    }
};

public __generic<T : ITexelElement> extension Texture2D<T>
{
    public static func get(hana::Image image) -> Texture2D<T> 
    { 
        return hana::sampled_images[image.id()]; 
    }

    public static func get_coherent(hana::Image image) -> Texture2D<T> 
    { 
        return hana::sampled_images_coherent[image.id()]; 
    }
};

public __generic<T : ITexelElement> extension Texture3D<T>
{
    public static func get(hana::Image image) -> Texture3D<T> 
    { 
        return hana::sampled_images[image.id()]; 
    }

    public static func get_coherent(hana::Image image) -> Texture3D<T> 
    { 
        return hana::sampled_images_coherent[image.id()]; 
    }
};

public __generic<T : ITexelElement> extension Texture1DArray<T>
{
    public static func get(hana::Image image) -> Texture1DArray<T> 
    { 
        return hana::sampled_images[image.id()]; 
    }

    public static func get_coherent(hana::Image image) -> Texture1DArray<T> 
    { 
        return hana::sampled_images_coherent[image.id()]; 
    }
};

public __generic<T : ITexelElement> extension Texture2DArray<T>
{
    public static func get(hana::Image image) -> Texture2DArray<T> 
    { 
        return hana::sampled_images[image.id()]; 
    }

    public static func get_coherent(hana::Image image) -> Texture2DArray<T> 
    { 
        return hana::sampled_images_coherent[image.id()]; 
    }
};

public __generic<T : ITexelElement> extension TextureCube<T>
{
    public static func get(hana::Image image) -> TextureCube<T> 
    { 
        return hana::sampled_images[image.id()]; 
    }

    public static func get_coherent(hana::Image image) -> TextureCube<T> 
    { 
        return hana::sampled_images_coherent[image.id()]; 
    }
};

public __generic<T : ITexelElement> extension TextureCubeArray<T>
{
    public static func get(hana::Image image) -> TextureCubeArray<T> 
    { 
        return hana::sampled_images[image.id()]; 
    }

    public static func get_coherent(hana::Image image) -> TextureCubeArray<T> 
    { 
        return hana::sampled_images_coherent[image.id()]; 
    }
};

public __generic<T : ITexelElement> extension Texture2DMS<T>
{
    public static func get(hana::Image image) -> Texture2DMS<T> 
    { 
        return hana::sampled_images[image.id()]; 
    }

    public static func get_coherent(hana::Image image) -> Texture2DMS<T> 
    { 
        return hana::sampled_images_coherent[image.id()]; 
    }
};