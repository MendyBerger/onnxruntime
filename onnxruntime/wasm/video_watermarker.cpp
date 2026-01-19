#include "video_watermarker.h"
#include <iostream>
#include <iomanip>
#include <bitset>

namespace FlowMark {

VideoWatermarker::VideoWatermarker(const std::string& watermark_model_path,
                                 const std::string& mask_predictor_path,
                                 const std::string& decoder_model_path,
                                 bool use_cuda,
                                 float watermark_strength)
    : watermark_strength_(watermark_strength) {
    std::cout << "Initializing watermark session..." << std::endl;
    watermark_session_ = std::make_unique<ONNXSession>(watermark_model_path, use_cuda);
    
    std::cout << "Initializing mask session..." << std::endl;
    mask_session_ = std::make_unique<ONNXSession>(mask_predictor_path, use_cuda);
    
    std::cout << "Initializing decoder session..." << std::endl;
    decoder_session_ = std::make_unique<ONNXSession>(decoder_model_path, use_cuda);
    
    std::cout << "Initializing VTM data layer..." << std::endl;
    vtm_layer_ = std::make_unique<VTMDataLayer>();
    
    std::cout << "✓ VideoWatermarker initialized" << std::endl;
}

VideoWatermarker::~VideoWatermarker() = default;

std::vector<float> VideoWatermarker::preprocess_frame(const cv::Mat& frame) {
    // Resize to 256x256
    cv::Mat resized;
    cv::resize(frame, resized, cv::Size(frame_width_, frame_height_));
    
    // Convert BGR to RGB
    cv::Mat rgb;
    cv::cvtColor(resized, rgb, cv::COLOR_BGR2RGB);
    
    // Convert to float and normalize to [-1, 1]
    std::vector<float> input_tensor;
    input_tensor.reserve(3 * frame_width_ * frame_height_);
    
    // Convert to CHW format and normalize
    for (int c = 0; c < 3; c++) {
        for (int h = 0; h < frame_height_; h++) {
            for (int w = 0; w < frame_width_; w++) {
                float pixel = static_cast<float>(rgb.at<cv::Vec3b>(h, w)[c]) / 255.0f;
                pixel = (pixel - 0.5f) / 0.5f;  // Normalize to [-1, 1]
                input_tensor.push_back(pixel);
            }
        }
    }
    
    return input_tensor;
}

cv::Mat VideoWatermarker::denormalize_image(const std::vector<float>& tensor_data, 
                                           int height, int width) {
    cv::Mat result(height, width, CV_8UC3);
    
    // Convert from CHW to HWC and denormalize
    for (int h = 0; h < height; h++) {
        for (int w = 0; w < width; w++) {
            for (int c = 0; c < 3; c++) {
                int idx = c * height * width + h * width + w;
                float val = tensor_data[idx];
                val = (val * 0.5f + 0.5f) * 255.0f;
                val = std::max(0.0f, std::min(255.0f, val));
                result.at<cv::Vec3b>(h, w)[2 - c] = static_cast<uint8_t>(val);  // RGB to BGR
            }
        }
    }
    
    return result;
}

std::vector<float> VideoWatermarker::generate_message(const std::string& aid_bin, int frame_idx) {
    // Convert frame index to 16-bit binary string
    std::bitset<16> tidx_bits(frame_idx);
    std::string tidx_bin = tidx_bits.to_string();
    
    // Encode using VTM data layer
    return vtm_layer_->encode_aid_tidx(aid_bin, tidx_bin);
}

cv::Mat VideoWatermarker::encode_frame(const cv::Mat& frame, const std::vector<float>& message) {
    int orig_height = frame.rows;
    int orig_width = frame.cols;
    
    // Preprocess frame
    auto image_input = preprocess_frame(frame);
    
    // Run mask predictor
    std::vector<int64_t> image_shape = {1, 3, frame_height_, frame_width_};
    auto mask_output = mask_session_->run(
        {{"input_image", image_input}},
        {image_shape}
    );
    
    // Run watermark encoder
    std::vector<int64_t> message_shape = {1, message_len_};
    std::vector<int64_t> mask_shape = {1, 1, frame_height_, frame_width_};
    
    auto wm_output = watermark_session_->run(
        {{"image", image_input}, {"message", message}, {"mask", mask_output}},
        {image_shape, message_shape, mask_shape}
    );
    
    // Blend watermarked image with original using mask
    // fused = 0.1 * wm * mask + image * (1 - mask)
    std::vector<float> fused(wm_output.size());
    for (size_t i = 0; i < wm_output.size(); i++) {
        size_t mask_idx = i % (frame_height_ * frame_width_);
        fused[i] = 0.1f * wm_output[i] * mask_output[mask_idx] + 
                   image_input[i] * (1.0f - mask_output[mask_idx]);
    }
    
    // Calculate residual
    std::vector<float> residual(wm_output.size());
    for (size_t i = 0; i < wm_output.size(); i++) {
        residual[i] = fused[i] - image_input[i];
    }
    
    // Denormalize and resize residual back to original size
    cv::Mat residual_img = denormalize_image(residual, frame_height_, frame_width_);
    cv::Mat residual_up;
    cv::resize(residual_img, residual_up, cv::Size(orig_width, orig_height), 0, 0, cv::INTER_LINEAR);
    
    // Normalize original frame
    cv::Mat frame_norm;
    frame.convertTo(frame_norm, CV_32FC3, 1.0/255.0);
    frame_norm = (frame_norm - 0.5f) / 0.5f;
    
    // Normalize residual and apply strength
    cv::Mat residual_norm;
    residual_up.convertTo(residual_norm, CV_32FC3, 1.0/255.0);
    residual_norm = (residual_norm - 0.5f) / 0.5f;
    residual_norm = residual_norm * watermark_strength_;  // Apply strength factor
    
    // Add residual to original frame
    cv::Mat fused_full = frame_norm + residual_norm;
    
    // Clamp to [-1, 1]
    cv::Mat clamped;
    cv::max(fused_full, -1.0, clamped);
    cv::min(clamped, 1.0, fused_full);
    
    // Denormalize to [0, 255]
    cv::Mat result;
    fused_full = (fused_full * 0.5f + 0.5f) * 255.0f;
    fused_full.convertTo(result, CV_8UC3);
    
    return result;
}

std::vector<bool> VideoWatermarker::decode_frame(const cv::Mat& frame) {
    // Preprocess frame
    auto image_input = preprocess_frame(frame);
    
    // Run decoder
    std::vector<int64_t> image_shape = {1, 3, frame_height_, frame_height_};
    auto decoded_output = decoder_session_->run(
        {{"encoded_image", image_input}},
        {image_shape}
    );
    
    // Convert to binary (threshold at 0.5)
    std::vector<bool> bits;
    bits.reserve(decoded_output.size());
    for (float val : decoded_output) {
        bits.push_back(val > 0.5f);
    }
    
    return bits;
}

bool VideoWatermarker::encode_video(const std::string& input_video_path,
                                   const std::string& output_video_path,
                                   const std::string& aid_bin) {
    if (aid_bin.length() != 76) {
        std::cerr << "✗ AID must be 76 bits" << std::endl;
        return false;
    }
    
    std::cout << "📌 AID for Encoding: " << aid_bin << std::endl;
    
    // Open input video
    cv::VideoCapture cap(input_video_path);
    if (!cap.isOpened()) {
        std::cerr << "✗ Failed to open input video: " << input_video_path << std::endl;
        return false;
    }
    
    // Get video properties
    double fps = cap.get(cv::CAP_PROP_FPS);
    int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    int total_frames = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));
    
    std::cout << "📹 Input video: " << width << "x" << height << " @ " << fps << " fps, " 
              << total_frames << " frames" << std::endl;
    
    // Open output video
    int fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
    cv::VideoWriter writer(output_video_path, fourcc, fps, cv::Size(width, height));
    if (!writer.isOpened()) {
        std::cerr << "✗ Failed to open output video: " << output_video_path << std::endl;
        return false;
    }
    
    // Process frames
    int frame_count = 0;
    cv::Mat frame;
    
    std::cout << "🎬 Encoding video..." << std::endl;
    
    while (cap.read(frame)) {
        std::cout << "Processing frame " << frame_count << "/" << total_frames << std::endl;
        
        // Generate message for this frame
        std::cout << "  Generating message..." << std::endl;
        auto message = generate_message(aid_bin, frame_count);
        std::cout << "  Message size: " << message.size() << std::endl;
        
        // Encode frame
        std::cout << "  Encoding frame..." << std::endl;
        cv::Mat watermarked = encode_frame(frame, message);
        std::cout << "  Frame encoded" << std::endl;
        
        // Write to output
        writer.write(watermarked);
        
        frame_count++;
    }
    
    std::cout << std::endl << "✅ Watermarked video saved: " << output_video_path << std::endl;
    std::cout << "   Encoded " << frame_count << " frames" << std::endl;
    
    cap.release();
    writer.release();
    
    return true;
}

std::vector<std::tuple<std::string, int, bool, int>> 
VideoWatermarker::decode_video(const std::string& input_video_path) {
    // Open input video
    cv::VideoCapture cap(input_video_path);
    if (!cap.isOpened()) {
        std::cerr << "✗ Failed to open input video: " << input_video_path << std::endl;
        return {};
    }
    
    // Get video properties
    int total_frames = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));
    
    std::cout << "🔍 Decoding video: " << total_frames << " frames" << std::endl;
    
    // Decode frames
    std::vector<std::tuple<std::string, int, bool, int>> results;
    int frame_count = 0;
    int success_count = 0;
    cv::Mat frame;
    
    while (cap.read(frame)) {
        if (frame_count % 10 == 0) {
            std::cout << "\rDecoding frame " << frame_count << "/" << total_frames << std::flush;
        }
        
        // Decode frame
        auto bits = decode_frame(frame);
        
        // Parse message
        auto [aid, tidx, success, version] = vtm_layer_->decode_binary(bits);
        
        results.push_back({aid, tidx, success, version});
        
        if (success) {
            success_count++;
        }
        
        frame_count++;
    }
    
    std::cout << std::endl << "✅ Decoding complete" << std::endl;
    std::cout << "   Success rate: " << success_count << "/" << frame_count 
              << " (" << (100.0 * success_count / frame_count) << "%)" << std::endl;
    
    cap.release();
    
    return results;
}

} // namespace FlowMark
