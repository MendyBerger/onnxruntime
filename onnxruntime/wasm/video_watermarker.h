#pragma once

#include "onnx_session.h"
#include "vtm_datalayer.h"
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <memory>

namespace FlowMark {

class VideoWatermarker {
public:
    // Constructor
    VideoWatermarker(const std::string& watermark_model_path,
                    const std::string& mask_predictor_path,
                    const std::string& decoder_model_path,
                    bool use_cuda = false,
                    float watermark_strength = 0.5f);
    
    // Destructor
    ~VideoWatermarker();
    
    // Encode a video with watermark
    // @param input_video_path: path to input video
    // @param output_video_path: path to output watermarked video
    // @param aid_bin: 76-bit AID as binary string
    // @return: true if successful
    bool encode_video(const std::string& input_video_path,
                     const std::string& output_video_path,
                     const std::string& aid_bin);
    
    // Decode watermark from a video
    // @param input_video_path: path to watermarked video
    // @return: vector of decoded messages (AID, TIDX, success, version) for each frame
    std::vector<std::tuple<std::string, int, bool, int>> 
    decode_video(const std::string& input_video_path);
    
    // Set watermark strength (0.0 = no watermark, 1.0 = full strength)
    void set_watermark_strength(float strength) { watermark_strength_ = strength; }
    float get_watermark_strength() const { return watermark_strength_; }

private:
    // Preprocess a frame for model input
    std::vector<float> preprocess_frame(const cv::Mat& frame);
    
    // Encode a single frame
    cv::Mat encode_frame(const cv::Mat& frame, const std::vector<float>& message);
    
    // Decode a single frame
    std::vector<bool> decode_frame(const cv::Mat& frame);
    
    // Denormalize model output back to image
    cv::Mat denormalize_image(const std::vector<float>& tensor_data, int height, int width);
    
    // Generate message for frame
    std::vector<float> generate_message(const std::string& aid_bin, int frame_idx);
    
    // ONNX sessions
    std::unique_ptr<ONNXSession> watermark_session_;
    std::unique_ptr<ONNXSession> mask_session_;
    std::unique_ptr<ONNXSession> decoder_session_;
    
    // VTM data layer
    std::unique_ptr<VTMDataLayer> vtm_layer_;
    
    // Frame size
    const int frame_width_ = 256;
    const int frame_height_ = 256;
    const int message_len_ = 128;
    
    // Watermark strength (0.0 = no watermark, 1.0 = full strength)
    float watermark_strength_;
};

} // namespace FlowMark
