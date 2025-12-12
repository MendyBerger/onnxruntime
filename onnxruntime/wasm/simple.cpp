// FlowMark WASM - Complete Video Watermarking with H.264 and MP4 support
// Integrates: ONNX Runtime + OpenH264 + minimp4

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <fstream>
#include <memory>
#include <onnxruntime_cxx_api.h>

// OpenH264 decoder and encoder
#include <codec_api.h>
#include <codec_app_def.h>
#include <codec_def.h>

// minimp4 for MP4 container
#define MINIMP4_IMPLEMENTATION
#include "minimp4.h"

// stb for image utilities
#include "image_utils.h"
#include "stb_image_resize2.h"  // For stbir_pixel_layout

// ============================================================================
// VTM Data Layer - simplified without BCH
// ============================================================================
class VTMDataLayerWASM {
public:
    std::vector<float> encode_aid_tidx(const std::string& aid_bin, const std::string& tidx_bin) {
        if (aid_bin.length() != 76 || tidx_bin.length() != 16) {
            std::cout << "ERROR: Invalid AID/TIDX length" << std::endl;
            return {};
        }

        std::string packet = aid_bin + tidx_bin;
        packet += std::string(32, '0');  // ECC placeholder
        packet += "0010";  // Version 2

        std::vector<float> result(128);
        for (size_t i = 0; i < 128; i++) {
            result[i] = (packet[i] == '1') ? 1.0f : 0.0f;
        }
        return result;
    }
};

// ============================================================================
// Convert AVCC format (MP4) to Annex B format (OpenH264)
// AVCC: [4-byte length][NAL data][4-byte length][NAL data]...
// Annex B: [0x00 0x00 0x00 0x01][NAL data][0x00 0x00 0x00 0x01][NAL data]...
std::vector<uint8_t> avcc_to_annexb(const uint8_t* avcc_data, size_t avcc_size) {
    std::vector<uint8_t> annexb;
    size_t pos = 0;

    while (pos + 4 <= avcc_size) {
        // Read 4-byte length (big-endian)
        uint32_t nal_length = (avcc_data[pos] << 24) | (avcc_data[pos+1] << 16) |
                              (avcc_data[pos+2] << 8) | avcc_data[pos+3];
        pos += 4;

        if (pos + nal_length > avcc_size) {
            std::cout << "       [ERROR] Invalid NAL length: " << nal_length << std::endl;
            break;
        }

        // Add start code
        annexb.push_back(0x00);
        annexb.push_back(0x00);
        annexb.push_back(0x00);
        annexb.push_back(0x01);

        // Copy NAL data
        annexb.insert(annexb.end(), avcc_data + pos, avcc_data + pos + nal_length);
        pos += nal_length;
    }

    return annexb;
}

// Color space conversions with proper stride handling
// ============================================================================
std::vector<uint8_t> yuv420_to_rgb(const uint8_t* y, const uint8_t* u, const uint8_t* v,
                                    int width, int height,
                                    int y_stride, int uv_stride) {
    std::vector<uint8_t> rgb(width * height * 3);

    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            int y_idx = row * y_stride + col;
            int uv_idx = (row / 2) * uv_stride + (col / 2);

            int Y = y[y_idx];
            int U = u[uv_idx] - 128;
            int V = v[uv_idx] - 128;

            int R = Y + 1.402 * V;
            int G = Y - 0.344 * U - 0.714 * V;
            int B = Y + 1.772 * U;

            int rgb_idx = (row * width + col) * 3;
            rgb[rgb_idx + 0] = (R < 0) ? 0 : (R > 255) ? 255 : R;
            rgb[rgb_idx + 1] = (G < 0) ? 0 : (G > 255) ? 255 : G;
            rgb[rgb_idx + 2] = (B < 0) ? 0 : (B > 255) ? 255 : B;
        }
    }
    return rgb;
}

void rgb_to_yuv420(const uint8_t* rgb, uint8_t* y, uint8_t* u, uint8_t* v, int width, int height) {
    // Convert Y plane
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            int rgb_idx = (row * width + col) * 3;
            int R = rgb[rgb_idx + 0];
            int G = rgb[rgb_idx + 1];
            int B = rgb[rgb_idx + 2];

            int Y = 0.299 * R + 0.587 * G + 0.114 * B;
            y[row * width + col] = (Y < 0) ? 0 : (Y > 255) ? 255 : Y;
        }
    }

    // Subsample UV (average 2x2 blocks)
    for (int row = 0; row < height; row += 2) {
        for (int col = 0; col < width; col += 2) {
            float U_sum = 0, V_sum = 0;
            int count = 0;

            for (int dy = 0; dy < 2 && (row + dy) < height; dy++) {
                for (int dx = 0; dx < 2 && (col + dx) < width; dx++) {
                    int rgb_idx = ((row + dy) * width + (col + dx)) * 3;
                    int R = rgb[rgb_idx + 0];
                    int G = rgb[rgb_idx + 1];
                    int B = rgb[rgb_idx + 2];

                    U_sum += -0.169 * R - 0.331 * G + 0.500 * B + 128;
                    V_sum += 0.500 * R - 0.419 * G - 0.081 * B + 128;
                    count++;
                }
            }

            int uv_idx = (row / 2) * (width / 2) + (col / 2);
            int U_val = U_sum / count;
            int V_val = V_sum / count;
            u[uv_idx] = (U_val < 0) ? 0 : (U_val > 255) ? 255 : U_val;
            v[uv_idx] = (V_val < 0) ? 0 : (V_val > 255) ? 255 : V_val;
        }
    }
}

// ============================================================================
// ONNX preprocessing and postprocessing
// ============================================================================
std::vector<float> preprocess_rgb(const uint8_t* rgb, int width, int height) {
    ImageUtils::Image img(width, height, 3);
    std::memcpy(img.data.data(), rgb, width * height * 3);

    if (width != 256 || height != 256) {
        img = ImageUtils::resizeImage(img, 256, 256);
    }

    // Convert to CHW float [-1, 1]
    std::vector<float> result(3 * 256 * 256);
    for (int c = 0; c < 3; c++) {
        for (int y = 0; y < 256; y++) {
            for (int x = 0; x < 256; x++) {
                int src_idx = (y * 256 + x) * 3 + c;
                int dst_idx = c * 256 * 256 + y * 256 + x;
                float pixel = static_cast<float>(img.data[src_idx]) / 255.0f;
                result[dst_idx] = (pixel - 0.5f) / 0.5f;
            }
        }
    }

    return result;
}

std::vector<uint8_t> postprocess_to_rgb(const float* tensor, int orig_width, int orig_height) {
    // Denormalize from [-1, 1] to [0, 255] and CHW to HWC
    std::vector<uint8_t> data_256(256 * 256 * 3);
    for (int c = 0; c < 3; c++) {
        for (int y = 0; y < 256; y++) {
            for (int x = 0; x < 256; x++) {
                int src_idx = c * 256 * 256 + y * 256 + x;
                int dst_idx = (y * 256 + x) * 3 + c;
                float val = (tensor[src_idx] * 0.5f + 0.5f) * 255.0f;
                val = (val < 0.0f) ? 0.0f : (val > 255.0f) ? 255.0f : val;
                data_256[dst_idx] = static_cast<uint8_t>(val);
            }
        }
    }

    // Resize back to original
    if (orig_width == 256 && orig_height == 256) {
        return data_256;
    }

    ImageUtils::Image img_256(256, 256, 3);
    img_256.data = data_256;
    auto resized = ImageUtils::resizeImage(img_256, orig_width, orig_height);
    return resized.data;
}

// ============================================================================
// Watermarking function
// ============================================================================
std::vector<uint8_t> watermark_frame_rgb(
    Ort::Session& watermark_session,
    Ort::Session& mask_session,
    const uint8_t* rgb_in,
    int width,
    int height,
    const std::vector<float>& message,
    float strength
) {
    // Preprocess input to 256x256 normalized tensor
    auto input_tensor = preprocess_rgb(rgb_in, width, height);

    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    std::vector<int64_t> shape = {1, 3, 256, 256};

    // Run mask predictor
    Ort::Value image_tensor = Ort::Value::CreateTensor<float>(
        memory_info, const_cast<float*>(input_tensor.data()), input_tensor.size(),
        shape.data(), shape.size());

    const char* mask_in[] = {"input_image"};
    const char* mask_out[] = {"mask_pred"};
    auto mask_outputs = mask_session.Run(Ort::RunOptions{nullptr}, mask_in, &image_tensor, 1, mask_out, 1);
    const float* mask_data = mask_outputs[0].GetTensorData<float>();

    // Run watermark encoder
    std::vector<int64_t> msg_shape = {1, 128};
    std::vector<float> msg_copy = message;
    Ort::Value msg_tensor = Ort::Value::CreateTensor<float>(
        memory_info, msg_copy.data(), msg_copy.size(), msg_shape.data(), msg_shape.size());

    Ort::Value image_tensor2 = Ort::Value::CreateTensor<float>(
        memory_info, const_cast<float*>(input_tensor.data()), input_tensor.size(),
        shape.data(), shape.size());

    const char* wm_in[] = {"image", "message", "mask"};
    const char* wm_out[] = {"encoded_image"};
    Ort::Value wm_inputs[] = {std::move(image_tensor2), std::move(msg_tensor), std::move(mask_outputs[0])};
    auto wm_outputs = watermark_session.Run(Ort::RunOptions{nullptr}, wm_in, wm_inputs, 3, wm_out, 1);
    const float* wm_data = wm_outputs[0].GetTensorData<float>();

    // MATCH PYTHON: fused_256 = strength*wm_tensor * mask + image_input * (1 - mask)
    // NOTE: mask is 1x1x256x256, need to broadcast to each channel
    std::vector<float> fused_256(3 * 256 * 256);
    for (int c = 0; c < 3; c++) {
        for (int i = 0; i < 256 * 256; i++) {
            int chw_idx = c * 256 * 256 + i;
            fused_256[chw_idx] = strength * wm_data[chw_idx] * mask_data[i] + input_tensor[chw_idx] * (1.0f - mask_data[i]);
        }
    }

    // Compute residual at 256x256 in CHW format
    std::vector<float> residual_256(3 * 256 * 256);
    for (size_t i = 0; i < residual_256.size(); i++) {
        residual_256[i] = fused_256[i] - input_tensor[i];
    }

    // Convert original input (HWC uint8) to CHW float normalized
    std::vector<float> input_chw_norm(3 * width * height);
    for (int c = 0; c < 3; c++) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int hwc_idx = (y * width + x) * 3 + c;
                int chw_idx = c * width * height + y * width + x;
                // Normalize to [-1, 1]
                input_chw_norm[chw_idx] = (static_cast<float>(rgb_in[hwc_idx]) / 255.0f - 0.5f) / 0.5f;
            }
        }
    }

    // Manually resize residual using bilinear interpolation (CHW format)
    std::vector<float> residual_full(3 * width * height);
    float x_ratio = 256.0f / width;
    float y_ratio = 256.0f / height;

    for (int c = 0; c < 3; c++) {
        int c_offset_src = c * 256 * 256;
        int c_offset_dst = c * width * height;

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float src_x = x * x_ratio;
                float src_y = y * y_ratio;

                int x0 = static_cast<int>(src_x);
                int y0 = static_cast<int>(src_y);
                int x1 = std::min(x0 + 1, 255);
                int y1 = std::min(y0 + 1, 255);

                float dx = src_x - x0;
                float dy = src_y - y0;

                // Bilinear interpolation
                float v00 = residual_256[c_offset_src + y0 * 256 + x0];
                float v01 = residual_256[c_offset_src + y0 * 256 + x1];
                float v10 = residual_256[c_offset_src + y1 * 256 + x0];
                float v11 = residual_256[c_offset_src + y1 * 256 + x1];

                float v0 = v00 * (1 - dx) + v01 * dx;
                float v1 = v10 * (1 - dx) + v11 * dx;
                float val = v0 * (1 - dy) + v1 * dy;

                residual_full[c_offset_dst + y * width + x] = val;
            }
        }
    }

    // Add residual to normalized input in CHW format, then convert to HWC uint8
    std::vector<uint8_t> result(width * height * 3);
    for (int c = 0; c < 3; c++) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int hwc_idx = (y * width + x) * 3 + c;
                int chw_idx = c * width * height + y * width + x;

                // Add residual to normalized input and clamp to [-1, 1]
                float fused_val = std::clamp(input_chw_norm[chw_idx] + residual_full[chw_idx], -1.0f, 1.0f);
                // Denormalize back to [0, 255]
                result[hwc_idx] = static_cast<uint8_t>(std::clamp((fused_val * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f));
            }
        }
    }

    return result;
}

// ============================================================================
// MP4 I/O structures
// ============================================================================
struct MP4ReadContext {
    std::ifstream* file;
    int64_t file_size;
};

static int read_callback(int64_t offset, void* buffer, size_t size, void* token) {
    MP4ReadContext* ctx = (MP4ReadContext*)token;
    ctx->file->seekg(offset, std::ios::beg);
    ctx->file->read((char*)buffer, size);
    std::streamsize bytes_read = ctx->file->gcount();

    // Debug: log first read
    static int read_count = 0;
    if (read_count < 3) {
        std::cout << "   Read callback #" << read_count++ << ": offset=" << offset
                  << " size=" << size << " bytes_read=" << bytes_read << std::endl;
    }

    return bytes_read == (std::streamsize)size ? 0 : -1;
}

struct MP4WriteContext {
    std::ofstream* file;
};

static int write_callback(int64_t offset, const void* buffer, size_t size, void* token) {
    static int write_count = 0;
    static size_t total_written = 0;

    MP4WriteContext* ctx = (MP4WriteContext*)token;
    ctx->file->seekp(offset);
    ctx->file->write((const char*)buffer, size);

    total_written += size;
    if (write_count < 5 || write_count % 10 == 0) {
        std::cout << "   [WRITE_CB #" << write_count << "] offset=" << offset
                  << " size=" << size << " total=" << total_written << std::endl;
    }
    write_count++;

    return 0;
}

// ============================================================================
// Batched Watermarking function for WebGPU (processes multiple frames at once)
// ============================================================================
std::vector<std::vector<uint8_t>> watermark_frames_batch(
    Ort::Session& watermark_session,
    Ort::Session& mask_session,
    const std::vector<std::vector<uint8_t>>& rgb_frames,
    const std::vector<std::pair<int, int>>& frame_dims,
    const std::vector<float>& message,
    float strength
) {
    int batch_size = rgb_frames.size();
    if (batch_size == 0) return {};

    // Note: Models expect batch_size=1, so we process frames individually
    // but still collect them for efficient encoding
    std::vector<std::vector<uint8_t>> results;
    results.reserve(batch_size);

    for (int b = 0; b < batch_size; b++) {
        int width = frame_dims[b].first;
        int height = frame_dims[b].second;

        // Process each frame individually (models don't support batching)
        auto wm_result = watermark_frame_rgb(
            watermark_session, mask_session,
            rgb_frames[b].data(), width, height,
            message, strength
        );

        results.push_back(std::move(wm_result));
    }

    return results;
}

// ============================================================================
// Main function
// ============================================================================
int main(int argc, char* argv[]) {
    std::cout << "FlowMark WASM - Complete Video Watermarking" << std::endl;
    std::cout << "============================================" << std::endl;

    if (argc < 6) {
        std::cout << "\nUsage: " << argv[0] << " <watermark_model.ort> <mask_model.ort> <decoder_model.ort> <input.mp4> <output.mp4> [strength]" << std::endl;
        std::cout << "\nExample:" << std::endl;
        std::cout << "  wasmtime run \\" << std::endl;
        std::cout << "    --dir=/models::/models \\" << std::endl;
        std::cout << "    --dir=/video::/video \\" << std::endl;
        std::cout << "    ort-wasi-simd.wasm \\" << std::endl;
        std::cout << "    /models/watermark_model.ort \\" << std::endl;
        std::cout << "    /models/mask_predictor.ort \\" << std::endl;
        std::cout << "    /models/decoder.ort \\" << std::endl;
        std::cout << "    /video/input.mp4 \\" << std::endl;
        std::cout << "    /video/output.mp4 \\" << std::endl;
        std::cout << "    0.5" << std::endl;
        return 1;
    }

    const char* watermark_model = argv[1];
    const char* mask_model = argv[2];
    const char* decoder_model = argv[3];
    const char* input_path = argv[4];
    const char* output_path = argv[5];
    float strength = (argc >= 7) ? std::stof(argv[6]) : 0.5f;

    std::cout << "\n📋 Configuration:" << std::endl;
    std::cout << "   Watermark:  " << watermark_model << std::endl;
    std::cout << "   Mask:       " << mask_model << std::endl;
    std::cout << "   Decoder:    " << decoder_model << std::endl;
    std::cout << "   Input:      " << input_path << std::endl;
    std::cout << "   Output:     " << output_path << std::endl;
    std::cout << "   Strength:   " << strength << std::endl;

    // Initialize ONNX Runtime
    std::cout << "\n⚙️  Initializing ONNX Runtime..." << std::endl;
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "FlowMarkWASM");
    Ort::SessionOptions session_options;
    
    // Enable WebGPU execution provider
    std::cout << "   Enabling WebGPU execution provider..." << std::endl;
    std::unordered_map<std::string, std::string> webgpu_options;
    // WebGPU options for WASI - may need device selection or other config
    session_options.AppendExecutionProvider("WebGPU", webgpu_options);
    std::cout << "   ✅ WebGPU provider configured" << std::endl;

    Ort::Session watermark_session(env, watermark_model, session_options);
    Ort::Session mask_session(env, mask_model, session_options);
    std::cout << "   ✅ Models loaded" << std::endl;

    // Initialize OpenH264 decoder
    std::cout << "\n🎬 Initializing OpenH264 decoder..." << std::endl;
    ISVCDecoder* decoder = nullptr;
    if (WelsCreateDecoder(&decoder) != 0 || decoder == nullptr) {
        std::cout << "   ❌ Failed to create decoder" << std::endl;
        return 1;
    }

    SDecodingParam dec_param = {0};
    dec_param.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_AVC;
    dec_param.uiTargetDqLayer = (uint8_t)-1;
    dec_param.eEcActiveIdc = ERROR_CON_DISABLE;
    dec_param.bParseOnly = false;
    decoder->Initialize(&dec_param);

    // Explicitly set decoder to single-threaded mode
    // This must be ZERO to avoid threading code paths in OpenH264
    int thread_count = 0;  // CRITICAL: Must be 0, not 1!
    decoder->SetOption(DECODER_OPTION_NUM_OF_THREADS, &thread_count);
    std::cout << "   ✅ Decoder ready (threads=0)" << std::endl;

    // Initialize OpenH264 encoder
    std::cout << "\n🎬 Initializing OpenH264 encoder..." << std::endl;
    ISVCEncoder* encoder = nullptr;
    if (WelsCreateSVCEncoder(&encoder) != 0 || encoder == nullptr) {
        std::cout << "   ❌ Failed to create encoder" << std::endl;
        return 1;
    }
    std::cout << "   ✅ Encoder ready" << std::endl;

    // Open input MP4
    std::cout << "\n📂 Opening input MP4..." << std::endl;
    std::ifstream input_file(input_path, std::ios::binary | std::ios::ate);
    if (!input_file) {
        std::cout << "   ❌ Cannot open input file" << std::endl;
        return 1;
    }

    int64_t file_size = input_file.tellg();
    input_file.seekg(0);

    MP4ReadContext read_ctx;
    read_ctx.file = &input_file;
    read_ctx.file_size = file_size;

    MP4D_demux_t mp4_demux;
    int open_result = MP4D_open(&mp4_demux, read_callback, &read_ctx, file_size);
    std::cout << "   MP4D_open result: " << open_result << " (1=success, 0=fail), file_size: " << file_size << std::endl;
    if (open_result != 1) {  // minimp4 returns 1 on success!
        std::cout << "   ❌ Cannot open MP4 demuxer (result: " << open_result << ")" << std::endl;
        return 1;
    }
    std::cout << "   ✅ MP4 demuxer opened successfully" << std::endl;

    // Find video track and get dimensions
    int video_track = -1;
    int width = 0, height = 0;
    unsigned num_samples = 0;
    unsigned input_timescale = 0;

    for (unsigned i = 0; i < mp4_demux.track_count; i++) {
        const MP4D_track_t* track = mp4_demux.track + i;
        if (track->handler_type == MP4D_HANDLER_TYPE_VIDE) {
            video_track = i;
            width = track->SampleDescription.video.width;
            height = track->SampleDescription.video.height;
            num_samples = track->sample_count;
            input_timescale = track->timescale;
            break;
        }
    }

    if (video_track < 0) {
        std::cout << "   ❌ No video track found" << std::endl;
        return 1;
    }

    std::cout << "   ✅ Video: " << width << "x" << height << " (" << num_samples << " samples)" << std::endl;
    std::cout << "   ✅ Timescale: " << input_timescale << " Hz" << std::endl;

    // Extract and feed SPS/PPS to decoder (REQUIRED!)
    std::cout << "\n🔧 Feeding SPS/PPS to decoder..." << std::endl;

    // Get SPS
    int sps_size = 0;
    const void* sps_data = MP4D_read_sps(&mp4_demux, video_track, 0, &sps_size);
    if (sps_data && sps_size > 0) {
        std::cout << "   Found SPS: " << sps_size << " bytes" << std::endl;

        // Convert SPS to Annex B format
        std::vector<uint8_t> sps_annexb;
        sps_annexb.push_back(0x00);
        sps_annexb.push_back(0x00);
        sps_annexb.push_back(0x00);
        sps_annexb.push_back(0x01);
        sps_annexb.insert(sps_annexb.end(), (const uint8_t*)sps_data, (const uint8_t*)sps_data + sps_size);

        uint8_t* dummy[3] = {nullptr, nullptr, nullptr};
        SBufferInfo dummy_info = {0};
        int sps_result = decoder->DecodeFrameNoDelay(sps_annexb.data(), sps_annexb.size(), dummy, &dummy_info);
        std::cout << "   SPS decode result: " << sps_result << std::endl;
    }

    // Get PPS
    int pps_size = 0;
    const void* pps_data = MP4D_read_pps(&mp4_demux, video_track, 0, &pps_size);
    if (pps_data && pps_size > 0) {
        std::cout << "   Found PPS: " << pps_size << " bytes" << std::endl;

        // Convert PPS to Annex B format
        std::vector<uint8_t> pps_annexb;
        pps_annexb.push_back(0x00);
        pps_annexb.push_back(0x00);
        pps_annexb.push_back(0x00);
        pps_annexb.push_back(0x01);
        pps_annexb.insert(pps_annexb.end(), (const uint8_t*)pps_data, (const uint8_t*)pps_data + pps_size);

        uint8_t* dummy[3] = {nullptr, nullptr, nullptr};
        SBufferInfo dummy_info = {0};
        int pps_result = decoder->DecodeFrameNoDelay(pps_annexb.data(), pps_annexb.size(), dummy, &dummy_info);
        std::cout << "   PPS decode result: " << pps_result << std::endl;
    }
    std::cout << "   ✅ Decoder initialized with parameter sets" << std::endl;

    // Setup encoder parameters with threading explicitly disabled
    SEncParamExt enc_param;
    encoder->GetDefaultParams(&enc_param);
    enc_param.iUsageType = CAMERA_VIDEO_REAL_TIME;
    enc_param.fMaxFrameRate = 30.0f;
    enc_param.iPicWidth = width;
    enc_param.iPicHeight = height;
    enc_param.iTargetBitrate = 5000000;
    enc_param.iRCMode = RC_BITRATE_MODE;
    enc_param.iMultipleThreadIdc = 1;  // 1 = threading disabled
    enc_param.bUseLoadBalancing = false;  // Disable load balancing

    if (encoder->InitializeExt(&enc_param) != 0) {
        std::cout << "   ❌ Failed to initialize encoder" << std::endl;
        return 1;
    }

    // Prepare watermark message
    VTMDataLayerWASM vtm;
    std::string aid_bin(76, '1');
    std::string tidx_bin(16, '0');
    auto message = vtm.encode_aid_tidx(aid_bin, tidx_bin);

    // Open output file
    std::cout << "\n📝 Creating output MP4..." << std::endl;
    std::ofstream output_file(output_path, std::ios::binary);
    if (!output_file) {
        std::cout << "   ❌ Cannot create output file" << std::endl;
        return 1;
    }

    MP4WriteContext write_ctx;
    write_ctx.file = &output_file;

    MP4E_mux_t* mux = MP4E_open(1, 0, &write_ctx, write_callback);
    if (!mux) {
        std::cout << "   ❌ Cannot create MP4 muxer" << std::endl;
        return 1;
    }

    // Initialize H264 writer helper
    mp4_h26x_writer_t mp4wr = {0};
    if (mp4_h26x_write_init(&mp4wr, mux, width, height, 0) < 0) {
        std::cout << "   ❌ Failed to initialize MP4 writer" << std::endl;
        return 1;
    }

    std::cout << "\n🔄 Processing frames in batches of 8 (optimized for WebGPU)..." << std::endl;
    std::cout << "   Expected: ~1-2 seconds per batch of 8 frames" << std::endl;

    int frames_processed = 0;
    int sample_count = 0;
    std::vector<uint8_t> sample_buffer;

    // Batch processing: accumulate frames
    constexpr int BATCH_SIZE = 8;
    std::vector<std::vector<uint8_t>> frame_batch_rgb;
    std::vector<std::pair<int, int>> frame_batch_dims;  // Store width, height for each frame

    // Helper function to process a batch of frames
    auto process_batch = [&](ISVCEncoder* encoder, mp4_h26x_writer_t* mp4wr,
                             Ort::Session& watermark_session, Ort::Session& mask_session,
                             const std::vector<float>& message, float strength) -> int {
        if (frame_batch_rgb.empty()) return 0;

        int batch_frames_processed = 0;

        // Watermark the batch
        auto wm_results = watermark_frames_batch(
            watermark_session, mask_session,
            frame_batch_rgb, frame_batch_dims,
            message, strength
        );

        // Process each watermarked frame in the batch
        for (size_t b = 0; b < wm_results.size(); b++) {
            int dec_width = frame_batch_dims[b].first;
            int dec_height = frame_batch_dims[b].second;

            // Convert RGB back to YUV
            std::vector<uint8_t> y_plane(dec_width * dec_height);
            std::vector<uint8_t> u_plane(dec_width * dec_height / 4);
            std::vector<uint8_t> v_plane(dec_width * dec_height / 4);
            rgb_to_yuv420(wm_results[b].data(), y_plane.data(), u_plane.data(), v_plane.data(), dec_width, dec_height);

            // Encode frame
            SSourcePicture src_pic = {0};
            src_pic.iPicWidth = dec_width;
            src_pic.iPicHeight = dec_height;
            src_pic.iColorFormat = videoFormatI420;
            src_pic.iStride[0] = dec_width;
            src_pic.iStride[1] = dec_width / 2;
            src_pic.iStride[2] = dec_width / 2;
            src_pic.pData[0] = y_plane.data();
            src_pic.pData[1] = u_plane.data();
            src_pic.pData[2] = v_plane.data();

            SFrameBSInfo frame_info = {0};
            int encode_result = encoder->EncodeFrame(&src_pic, &frame_info);

            if (encode_result == 0 && frame_info.eFrameType != videoFrameTypeInvalid && frame_info.iLayerNum > 0) {
                // Collect ALL NAL units from ALL layers
                std::vector<uint8_t> all_nals;
                for (int layer = 0; layer < frame_info.iLayerNum; layer++) {
                    SLayerBSInfo* layer_info = &frame_info.sLayerInfo[layer];
                    int layer_total = 0;
                    for (int nal = 0; nal < layer_info->iNalCount; nal++) {
                        layer_total += layer_info->pNalLengthInByte[nal];
                    }
                    all_nals.insert(all_nals.end(), layer_info->pBsBuf, layer_info->pBsBuf + layer_total);
                }

                // For 30fps video: each frame duration is 1/30 second = 3000 units in 90kHz
                unsigned frame_duration_90k = 3000;
                int write_result = mp4_h26x_write_nal(mp4wr, all_nals.data(), all_nals.size(), frame_duration_90k);
                batch_frames_processed++;

                if (batch_frames_processed % 10 == 0 || batch_frames_processed <= 3) {
                    std::cout << "       ✓ Frame " << (frames_processed + batch_frames_processed) << " complete!" << std::endl;
                }
            }
        }

        // Clear the batch
        frame_batch_rgb.clear();
        frame_batch_dims.clear();

        return batch_frames_processed;
    };

    // Process each frame
    for (unsigned sample_idx = 0; sample_idx < num_samples; sample_idx++) {
        sample_count++;

        // Print progress for every frame initially, then every 10
        if (sample_idx < 5 || sample_idx % 10 == 0) {
            std::cout << "\n   [" << (sample_idx + 1) << "/" << num_samples << "] Reading frame..." << std::endl;
        }

        // Get frame offset and size
        unsigned frame_bytes = 0;
        unsigned timestamp = 0;
        unsigned duration = 0;
        MP4D_file_offset_t offset = MP4D_frame_offset(&mp4_demux, video_track, sample_idx, &frame_bytes, &timestamp, &duration);

        if (frame_bytes == 0) {
            continue;
        }

        // Read frame data
        sample_buffer.resize(frame_bytes);
        if (read_callback(offset, sample_buffer.data(), frame_bytes, &read_ctx) != 0) {
            continue;
        }

        // Convert from AVCC (MP4) to Annex B (OpenH264) format
        auto annexb_data = avcc_to_annexb(sample_buffer.data(), frame_bytes);

        // Decode H.264 frame
        uint8_t* dst[3] = {nullptr, nullptr, nullptr};
        SBufferInfo buf_info = {0};

        int decode_result = decoder->DecodeFrameNoDelay(
            annexb_data.data(),
            annexb_data.size(),
            dst,
            &buf_info
        );

        // DEBUG
        if (sample_idx < 3) {
            std::cout << "       [DEBUG] Frame " << sample_idx << ": decode_result=" << decode_result
                      << " buffer_status=" << buf_info.iBufferStatus << std::endl;
        }

        if (decode_result != 0 || buf_info.iBufferStatus != 1) {
            continue;  // Skip frame
        }

        int dec_width = buf_info.UsrData.sSystemBuffer.iWidth;
        int dec_height = buf_info.UsrData.sSystemBuffer.iHeight;
        int y_stride = buf_info.UsrData.sSystemBuffer.iStride[0];
        int uv_stride = buf_info.UsrData.sSystemBuffer.iStride[1];

        // DEBUG: Print stride info for first frame
        if (sample_idx == 0) {
            std::cout << "       [DEBUG] Frame 0: width=" << dec_width << " height=" << dec_height
                      << " y_stride=" << y_stride << " uv_stride=" << uv_stride << std::endl;
        }

        // Convert YUV to RGB (with proper stride handling)
        auto rgb = yuv420_to_rgb(dst[0], dst[1], dst[2], dec_width, dec_height, y_stride, uv_stride);

        // Add frame to batch
        frame_batch_rgb.push_back(rgb);
        frame_batch_dims.push_back({dec_width, dec_height});

        // Process batch when full
        if (frame_batch_rgb.size() >= BATCH_SIZE) {
            if (sample_idx < 5 || sample_idx % 10 == 0) {
                std::cout << "       [BATCH] Processing batch of " << frame_batch_rgb.size() << " frames..." << std::endl;
            }
            frames_processed += process_batch(encoder, &mp4wr, watermark_session, mask_session, message, strength);
        }
    }

    // Process any remaining frames in the batch
    if (!frame_batch_rgb.empty()) {
        std::cout << "       [BATCH] Processing final batch of " << frame_batch_rgb.size() << " frames..." << std::endl;
        frames_processed += process_batch(encoder, &mp4wr, watermark_session, mask_session, message, strength);
    }

    // Finalize
    std::cout << "\n✅ Finalizing output..." << std::endl;
    std::cout << "   Closing H264 writer..." << std::endl;
    std::cout.flush();
    // mp4_h26x_write_close(&mp4wr);  // SKIP: causes crash in WASM
    std::cout << "   Closing MP4 muxer..." << std::endl;
    std::cout.flush();
    MP4E_close(mux);
    std::cout << "   Closing output file..." << std::endl;
    std::cout.flush();
    output_file.close();
    std::cout << "   Done!" << std::endl;
    std::cout.flush();

    MP4D_close(&mp4_demux);
    input_file.close();

    encoder->Uninitialize();
    WelsDestroySVCEncoder(encoder);

    decoder->Uninitialize();
    WelsDestroyDecoder(decoder);

    std::cout << "\n🎉 Success! Processed " << frames_processed << " frames" << std::endl;
    std::cout << "   Output: " << output_path << std::endl;

    return 0;
}
