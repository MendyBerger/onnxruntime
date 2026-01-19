#pragma once

#include "bch_ecc.h"
#include <string>
#include <vector>
#include <tuple>
#include <memory>

namespace FlowMark {

// Constants
constexpr int BCH_POLYNOMIAL = 137;
constexpr int PAYLOAD_BITS = 92;  // 76-bit AID + 16-bit TIDX
constexpr int ECC_BITS = 32;
constexpr int VERSION_BITS = 4;
constexpr int TOTAL_BITS = PAYLOAD_BITS + ECC_BITS + VERSION_BITS;  // 128 bits

// VTM Data Layer for encoding/decoding watermark messages
class VTMDataLayer {
public:
    // Constructor
    VTMDataLayer();
    
    // Destructor
    ~VTMDataLayer();
    
    // Encode AID and TIDX into a 128-bit message
    // @param aid_bin: 76-bit AID as binary string "0101..."
    // @param tidx_bin: 16-bit TIDX as binary string "0000000000000001"
    // @return: 128-bit encoded message as float vector
    std::vector<float> encode_aid_tidx(const std::string& aid_bin, const std::string& tidx_bin);
    
    // Decode a 128-bit message back to AID and TIDX
    // @param bits: 128-bit message as bool vector
    // @return: tuple of (aid, tidx, success, version)
    std::tuple<std::string, int, bool, int> decode_binary(const std::vector<bool>& bits);
    
    // Get version
    int get_version() const { return version_; }

private:
    // Pad bits to byte boundary
    std::string pad_to_byte(const std::string& bitstr);
    
    // Decode a single bitstring
    std::tuple<std::string, bool, int> decode_bitstring(const std::vector<bool>& packet);
    
    // Split packet into data, ECC, and version
    std::tuple<int, std::string, std::string, BCH*, int> raw_payload_split(const std::string& packet);
    
    // BCH codec
    std::unique_ptr<BCH> bch_;
    
    // Version
    int version_;
};

} // namespace FlowMark
