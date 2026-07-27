#pragma once

namespace gottvergessen
{
    class textures
    {
    public:
        textures() = default;
        ~textures() noexcept = default;
        
        static bool load_from_file(const char* filename, ID3D11Device* d3dDevice, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height);
		static bool load_from_memory(const unsigned char* buffer, int buffer_size, ID3D11Device* d3dDevice, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height);
		static void destroy_texture(ID3D11ShaderResourceView** tex_resources);
    };
}