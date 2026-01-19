#include "vtm_datalayer.h"
#include <iostream>
#include <bitset>
#include <stdexcept>
#include <sstream>
#include <iomanip>

namespace FlowMark {

VTMDataLayer::VTMDataLayer() : version_(2) {
    std::cout << "  VTM: Initialized (BCH disabled for now)" << std::endl;
    // TODO: Fix BCH build_cyclic infinite loop
    // bch_ = std::make_unique<BCH>(4, BCH_POLYNOMIAL);
}

VTMDataLayer::~VTMDataLayer() = default;

std::string VTMDataLayer::pad_to_byte(const std::string& bitstr) {
    int pad_len = (8 - bitstr.length() % 8) % 8;
    return bitstr + std::string(pad_len, '0');
}

std::vector<float> VTMDataLayer::encode_aid_tidx(const std::string& aid_bin, const std::string& tidx_bin) {
    if (aid_bin.length() != 76) {
        throw std::invalid_argument("AID must be 76 bits");
    }
    if (tidx_bin.length() != 16) {
        throw std::invalid_argument("TIDX must be 16 bits");
    }
    
    // Combine AID and TIDX (92 bits)
    std::string data_bits = aid_bin + tidx_bin;
    
    // Add 32 zero bits for ECC placeholder (TODO: implement BCH)
    std::string ecc_bits(32, '0');
    
    // Add 4-bit version
    std::bitset<VERSION_BITS> version_bits(version_);
    std::string version_str = version_bits.to_string();
    
    // Combine into final packet (128 bits total)
    std::string packet = data_bits + ecc_bits + version_str;
    
    if (packet.length() != TOTAL_BITS) {
        throw std::runtime_error("Packet length mismatch: expected " + 
                                std::to_string(TOTAL_BITS) + ", got " + 
                                std::to_string(packet.length()));
    }
    
    // Convert to float vector
    std::vector<float> result;
    result.reserve(TOTAL_BITS);
    for (char c : packet) {
        result.push_back(c == '1' ? 1.0f : 0.0f);
    }
    
    return result;
}

std::tuple<std::string, int, bool, int> VTMDataLayer::decode_binary(const std::vector<bool>& bits) {
    auto [recovered_bin, success, version] = decode_bitstring(bits);
    
    if (!success) {
        return {"", -1, false, version};
    }
    
    // Extract AID and TIDX
    std::string aid = recovered_bin.substr(0, 76);
    std::string tidx_bin = recovered_bin.substr(76);
    
    // Convert TIDX from binary string to int
    int tidx = std::bitset<16>(tidx_bin).to_ulong();
    
    return {aid, tidx, true, version};
}

std::tuple<std::string, bool, int> VTMDataLayer::decode_bitstring(const std::vector<bool>& packet) {
    // Convert packet to string
    std::string packet_str;
    for (bool b : packet) {
        packet_str += b ? '1' : '0';
    }
    
    auto [bitflips, packet_d, packet_e, bch_decoder, version] = raw_payload_split(packet_str);
    
    if (bch_decoder == nullptr) {
        return {"", false, version};
    }
    
    // Pad bits to byte boundary
    auto pad_bits = [](const std::string& bits) -> std::string {
        int pad = (8 - bits.length() % 8) % 8;
        return bits + std::string(pad, '0');
    };
    
    // Convert data bits to bytes
    std::string padded_data = pad_bits(packet_d);
    std::vector<uint8_t> data_bytes;
    for (size_t i = 0; i < padded_data.length(); i += 8) {
        std::string byte_str = padded_data.substr(i, 8);
        uint8_t byte_val = static_cast<uint8_t>(std::bitset<8>(byte_str).to_ulong());
        data_bytes.push_back(byte_val);
    }
    
    // Convert ECC bits to bytes
    std::string padded_ecc = pad_bits(packet_e);
    std::vector<uint8_t> ecc_bytes;
    for (size_t i = 0; i < padded_ecc.length(); i += 8) {
        std::string byte_str = padded_ecc.substr(i, 8);
        uint8_t byte_val = static_cast<uint8_t>(std::bitset<8>(byte_str).to_ulong());
        ecc_bytes.push_back(byte_val);
    }
    
    // Decode with BCH
    int flips = -1;
    if (ecc_bytes.size() == static_cast<size_t>(bch_decoder->get_ecc_bytes())) {
        flips = bch_decoder->decode(data_bytes, ecc_bytes);
    }
    
    if (flips == -1) {
        return {"", false, version};
    }
    
    // Convert corrected data bytes back to bit string
    std::string bin_str;
    for (auto byte : data_bytes) {
        std::bitset<8> bits(byte);
        bin_str += bits.to_string();
    }
    
    // Return only the payload bits
    return {bin_str.substr(0, PAYLOAD_BITS), true, version};
}

std::tuple<int, std::string, std::string, BCH*, int> 
VTMDataLayer::raw_payload_split(const std::string& packet) {
    // Extract version from last VERSION_BITS
    std::string version_str = packet.substr(packet.length() - VERSION_BITS);
    int version = std::bitset<VERSION_BITS>(version_str).to_ulong();
    
    if (version != 2) {
        return {-1, "", "", nullptr, version};
    }
    
    // Extract data and ECC
    std::string data = packet.substr(0, PAYLOAD_BITS);
    std::string ecc = packet.substr(PAYLOAD_BITS, ECC_BITS);
    
    return {0, data, ecc, bch_.get(), version};
}

} // namespace FlowMark
