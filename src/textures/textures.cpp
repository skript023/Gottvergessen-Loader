#include "textures.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace gottvergessen
{
    bool textures::load_from_file(const char* filename, ID3D11Device* d3dDevice, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
	{
		int image_width = 0;
		int image_height = 0;
		unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
		if (image_data == NULL)
		{
			LOG(WARNING) << "Failed to load image: " << stbi_failure_reason();

			return false;
		}

		// Create texture
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Width = image_width;
		desc.Height = image_height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;

		ID3D11Texture2D* pTexture = nullptr;
		D3D11_SUBRESOURCE_DATA subResource;
		subResource.pSysMem = image_data;
		subResource.SysMemPitch = desc.Width * 4;
		subResource.SysMemSlicePitch = 0;

		HRESULT hr = d3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);
		if (FAILED(hr))
		{
			LOG(WARNING) << "Failed to create texture. HRESULT: " << hr;
			stbi_image_free(image_data);

			return false;
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = desc.MipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;

		hr = d3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
		if (FAILED(hr))
		{
			LOG(WARNING) << "Failed to create shader resource view. HRESULT: " << hr;
			pTexture->Release();
			stbi_image_free(image_data);

			return false;
		}

		pTexture->Release();
		*out_width = image_width;
		*out_height = image_height;
		stbi_image_free(image_data);

		return true;
	}

	bool textures::load_from_memory(const unsigned char* buffer, int buffer_size, ID3D11Device* d3dDevice, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
	{
		int image_width = 0;
		int image_height = 0;
		unsigned char* image_data = stbi_load_from_memory(buffer, buffer_size, &image_width, &image_height, NULL, 4);
		if (image_data == NULL)
		{
			LOG(WARNING) << "Failed to load image: " << stbi_failure_reason();

			return false;
		}

		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Width = image_width;
		desc.Height = image_height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;

		ID3D11Texture2D* pTexture = nullptr;
		D3D11_SUBRESOURCE_DATA subResource;
		subResource.pSysMem = image_data;
		subResource.SysMemPitch = desc.Width * 4;
		subResource.SysMemSlicePitch = 0;

		HRESULT hr = d3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);
		if (FAILED(hr))
		{
			LOG(WARNING) << "Failed to create texture. HRESULT: " << hr;
			stbi_image_free(image_data);

			return false;
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = desc.MipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;

		hr = d3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
		if (FAILED(hr))
		{
			LOG(WARNING) << "Failed to create shader resource view. HRESULT: " << hr;
			pTexture->Release();
			stbi_image_free(image_data);

			return false;
		}

		pTexture->Release();
		*out_width = image_width;
		*out_height = image_height;
		stbi_image_free(image_data);

		return true;
	}

	void textures::destroy_texture(ID3D11ShaderResourceView** tex_resources)
	{
		(*tex_resources)->Release();
		*tex_resources = NULL;
	}
}