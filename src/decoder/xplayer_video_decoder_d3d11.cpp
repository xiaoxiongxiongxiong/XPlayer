#include "xplayer_video_decoder_d3d11.h"

#include <d3d9.h>
#include <d3d11.h>
#include <dxva2api.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxva2.lib")

#include "xplayer_utils.h"

// H.264 Modes
static const GUID XPLAYER_DXVA2_ModeH264_A = { 0x1b81be64, 0xa0c7, 0x11d3, {0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5} };
static const GUID XPLAYER_DXVA2_ModeH264_B = { 0x1b81be65, 0xa0c7, 0x11d3, {0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5} };
static const GUID XPLAYER_DXVA2_ModeH264_C = { 0x1b81be66, 0xa0c7, 0x11d3, {0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5} };
static const GUID XPLAYER_DXVA2_ModeH264_D = { 0x1b81be67, 0xa0c7, 0x11d3, {0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5} };
static const GUID XPLAYER_DXVA2_ModeH264_E = { 0x1b81be68, 0xa0c7, 0x11d3, {0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5} };
static const GUID XPLAYER_DXVA2_ModeH264_F = { 0x1b81be69, 0xa0c7, 0x11d3, {0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5} };

// MPEG2 Modes
static const GUID XPLAYER_DXVA2_ModeMPEG2_MoComp = { 0xe6a9f44b, 0x61b0, 0x4563, {0x9e, 0xa4, 0x63, 0xd2, 0xa3, 0xc6, 0xfe, 0x66} };
static const GUID XPLAYER_DXVA2_ModeMPEG2_IDCT = { 0xbf22ad00, 0x03ea, 0x4690, {0x80, 0x77, 0x47, 0x33, 0x46, 0x20, 0x9b, 0x7e} };
static const GUID XPLAYER_DXVA2_ModeMPEG2_VLD = { 0xee27417f, 0x5e28, 0x4e65, {0xbe, 0xea, 0x1d, 0x26, 0xb5, 0x08, 0xad, 0xc9} };

// VC-1 / WMV Modes
static const GUID XPLAYER_DXVA2_ModeVC1_A = { 0x1b81beA0, 0xa0c7, 0x11d3, {0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5} };
static const GUID XPLAYER_DXVA2_ModeVC1_B = { 0x1b81beA1, 0xa0c7, 0x11d3, {0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5} };
static const GUID XPLAYER_DXVA2_ModeVC1_C = { 0x1b81beA2, 0xa0c7, 0x11d3, {0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5} };
static const GUID XPLAYER_DXVA2_ModeVC1_D = { 0x1b81beA3, 0xa0c7, 0x11d3, {0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5} };

// HEVC Modes
static const GUID XPLAYER_DXVA2_ModeHEVC_VLD_Main = { 0x5b11d51b, 0x2f4c, 0x4452, {0xbc, 0xc3, 0x09, 0xf2, 0xa1, 0x16, 0x0c, 0xc0} };
static const GUID XPLAYER_DXVA2_ModeHEVC_VLD_Main10 = { 0x107af0e0, 0xef1a, 0x4d19, {0xab, 0xa8, 0x67, 0xa1, 0x63, 0x07, 0x3d, 0x13} };

// 使用宏替换，这样你后续的代码无需修改
#define DXVA2_ModeH264_A XPLAYER_DXVA2_ModeH264_A
#define DXVA2_ModeH264_B XPLAYER_DXVA2_ModeH264_B
#define DXVA2_ModeH264_C XPLAYER_DXVA2_ModeH264_C
#define DXVA2_ModeH264_D XPLAYER_DXVA2_ModeH264_D
#define DXVA2_ModeH264_E XPLAYER_DXVA2_ModeH264_E
#define DXVA2_ModeH264_F XPLAYER_DXVA2_ModeH264_F
#define DXVA2_ModeMPEG2_MoComp XPLAYER_DXVA2_ModeMPEG2_MoComp
#define DXVA2_ModeMPEG2_IDCT XPLAYER_DXVA2_ModeMPEG2_IDCT
#define DXVA2_ModeMPEG2_VLD XPLAYER_DXVA2_ModeMPEG2_VLD
#define DXVA2_ModeVC1_A XPLAYER_DXVA2_ModeVC1_A
#define DXVA2_ModeVC1_B XPLAYER_DXVA2_ModeVC1_B
#define DXVA2_ModeVC1_C XPLAYER_DXVA2_ModeVC1_C
#define DXVA2_ModeVC1_D XPLAYER_DXVA2_ModeVC1_D
#define DXVA2_ModeHEVC_VLD_Main XPLAYER_DXVA2_ModeHEVC_VLD_Main
#define DXVA2_ModeHEVC_VLD_Main10 XPLAYER_DXVA2_ModeHEVC_VLD_Main10

bool CXPlayerVideoDecoderD3D11::create(int width, int height, int profile)
{
    if (!initialize())
        return false;

    D3D11_VIDEO_DECODER_DESC desc = {};
    desc.SampleWidth = width;
    desc.SampleHeight = height;
    desc.Guid = DXVA2_ModeH264_VLD_NoFGT;
    desc.OutputFormat = DXGI_FORMAT_NV12;

    UINT cnt = 0;
    auto hr = m_ptrVideoDevice->GetVideoDecoderConfigCount(&desc, &cnt);
    if (FAILED(hr))
    {
        xpu_format_string(m_strError, "GetVideoDecoderConfigCount failed");
        return false;
    }
    if (0 == cnt)
    {
        xpu_format_string(m_strError, "No available video decoder config");
        return false;
    }

    D3D11_VIDEO_DECODER_CONFIG config = {};
    for (int i = 0; i < cnt; i++)
    {
        hr = m_ptrVideoDevice->GetVideoDecoderConfig(&desc, i, &config);
        if (FAILED(hr))
        {
            xpu_format_string(m_strError, "GetVideoDecoderConfig failed");
            return false;
        }

        if (1 == config.ConfigBitstreamRaw)
            break;
    }

    hr = m_ptrVideoDevice->CreateVideoDecoder(&desc, &config, &m_ptrVideoDecoder);
    if (FAILED(hr))
    { 
        xpu_format_string(m_strError, "CreateVideoDecoder failed");
        return false; 
    }

    D3D11_TEXTURE2D_DESC text_desc = {};
    text_desc.Width = width;
    text_desc.Height = height;
    text_desc.MipLevels = 1;
    text_desc.ArraySize = 1;
    text_desc.Format = DXGI_FORMAT_NV12; // 硬解标准输出格式
    text_desc.SampleDesc.Count = 1;
    text_desc.Usage = D3D11_USAGE_DEFAULT;
    text_desc.BindFlags = D3D11_BIND_DECODER | D3D11_BIND_SHADER_RESOURCE; // 关键
    text_desc.CPUAccessFlags = 0;

     //通常分配 16-20 个表面足够 H.264 Level 5.1 使用
    for (int i = 0; i < 20; ++i)
    {
        ID3D11Texture2D * tex = nullptr;
        hr = m_ptrDevice->CreateTexture2D(&text_desc, nullptr, &tex);
        if (SUCCEEDED(hr)) 
            m_ptrTextures.push_back(tex);

        D3D11_VIDEO_DECODER_OUTPUT_VIEW_DESC viewDesc = {};
        viewDesc.DecodeProfile = DXVA2_ModeH264_VLD_NoFGT;
        viewDesc.ViewDimension = D3D11_VDOV_DIMENSION_TEXTURE2D;
        viewDesc.Texture2D.ArraySlice = 0;

        ID3D11VideoDecoderOutputView * view = nullptr;
        if (SUCCEEDED(m_ptrVideoDevice->CreateVideoDecoderOutputView(tex, &viewDesc, &view)))
        {
            m_ptrOutputViews.push_back(view);
        }

    }

    return !m_ptrTextures.empty();
}

void CXPlayerVideoDecoderD3D11::destroy()
{
    for (auto & tex : m_ptrTextures)
        tex->Release();
    m_ptrTextures.clear();

    if (m_ptrVideoDecoder)
    {
        m_ptrVideoDecoder->Release();
        m_ptrVideoDecoder = nullptr;
    }
    if (m_ptrVideoContext)
    {
        m_ptrVideoContext->Release();
        m_ptrVideoContext = nullptr;
    }
    if (m_ptrVideoDevice)
    {
        m_ptrVideoDevice->Release();
        m_ptrVideoDevice = nullptr;
    }
    if (m_ptrContext)
    {
        m_ptrContext->Release();
        m_ptrContext = nullptr;
    }
    if (m_ptrDevice)
    {
        m_ptrDevice->Release();
        m_ptrDevice = nullptr;
    }
}

bool CXPlayerVideoDecoderD3D11::send(const uint8_t * data, int size)
{
    if (nullptr == m_ptrVideoDecoder || nullptr == m_ptrVideoContext)
    {
        xpu_format_string(m_strError, "Not opened yet");
        return false;
    }

    int index = m_iFrameCount.load() % m_ptrTextures.size();
    ID3D11Texture2D * tex = m_ptrTextures[index];
    ID3D11VideoDecoderOutputView * out_view = m_ptrOutputViews[index];

    auto hr = m_ptrVideoContext->DecoderBeginFrame(m_ptrVideoDecoder, out_view, 0, nullptr);
    if (FAILED(hr))
    {
        xpu_format_string(m_strError, "DecoderBeginFrame failed");
        return false;
    }

    void * pBitstreamData = nullptr;
    UINT bitstreamSize = 0;

    // 请求一个 BITSTREAM 类型的缓冲区
    hr = m_ptrVideoContext->GetDecoderBuffer(m_ptrVideoDecoder, D3D11_VIDEO_DECODER_BUFFER_BITSTREAM, &bitstreamSize, &pBitstreamData);
    if (FAILED(hr) || !pBitstreamData)
    {
        //xpu_format_string(m_strError, "DecoderBeginFrame failed");
        m_ptrVideoContext->DecoderEndFrame(m_ptrVideoDecoder);
        return false;
    }

    ZeroMemory(pBitstreamData, bitstreamSize);
    memcpy(pBitstreamData, data, size);
    hr = m_ptrVideoContext->ReleaseDecoderBuffer(m_ptrVideoDecoder, D3D11_VIDEO_DECODER_BUFFER_BITSTREAM);


    D3D11_VIDEO_DECODER_BUFFER_DESC bufferDesc = {};
    bufferDesc.BufferType = D3D11_VIDEO_DECODER_BUFFER_BITSTREAM;
    bufferDesc.BufferIndex = 0; // 必须为 0
    bufferDesc.DataOffset = 0;
    bufferDesc.DataSize = static_cast<UINT>(size);
    hr = m_ptrVideoContext->SubmitDecoderBuffers(m_ptrVideoDecoder, 1, &bufferDesc);
    if (FAILED(hr))
    {
        xpu_format_string(m_strError, "SubmitDecoderBuffers failed");
        return false;
    }

    // 4. 结束帧解码
    hr = m_ptrVideoContext->DecoderEndFrame(m_ptrVideoDecoder);
    if (FAILED(hr))
    {
        xpu_format_string(m_strError, "DecoderEndFrame failed");
        return false;
    }

    m_iFrameCount++;

    return true;
}

bool CXPlayerVideoDecoderD3D11::initialize()
{
    D3D_FEATURE_LEVEL level;
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_VIDEO_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
                                   nullptr, 0, D3D11_SDK_VERSION, &m_ptrDevice, &level, &m_ptrContext);
    if (FAILED(hr))
    {
        xpu_format_string(m_strError, "CreateDevice failed");
        return false;
    }

    m_ptrDevice->QueryInterface(__uuidof(ID3D11VideoDevice), (void **)&m_ptrVideoDevice);
    m_ptrContext->QueryInterface(__uuidof(ID3D11VideoContext), (void **)&m_ptrVideoContext);
    if (nullptr == m_ptrVideoDevice || nullptr == m_ptrVideoContext)
    {
        xpu_format_string(m_strError, "QueryInterface failed");
        return false;
    }

    return true;
}
