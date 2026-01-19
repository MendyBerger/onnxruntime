#pragma once

#include <vector>
#include <cstdint>
#include <memory>

namespace FlowMark {

// BCH Error Correction Code implementation
// Based on the Python implementation in bchecc.py
class BCH {
public:
    // Constructor
    // @param t: error correction capability (number of errors that can be corrected)
    // @param poly: generator polynomial
    BCH(int t, int poly);
    
    // Destructor
    ~BCH();
    
    // Encode data with BCH error correction
    // @param data: input data bytes
    // @return: ECC bytes
    std::vector<uint8_t> encode(const std::vector<uint8_t>& data);
    
    // Decode data with BCH error correction
    // @param data: input data bytes (will be modified in place if errors are corrected)
    // @param recvecc: received ECC bytes
    // @return: number of bit flips corrected, or -1 if decoding failed
    int decode(std::vector<uint8_t>& data, const std::vector<uint8_t>& recvecc);
    
    // Get ECC size in bits
    int get_ecc_bits() const { return ecc_bits_; }
    
    // Get ECC size in bytes
    int get_ecc_bytes() const;

private:
    // Galois field operations
    int g_inv(int a);
    int g_sqrt(int a);
    int g_mul(int a, int b);
    int g_div(int a, int b);
    int g_log(int x);
    int a_ilog(int x);
    int g_pow(int i);
    
    // Modular arithmetic
    int mod(int v);
    int modn(int v);
    
    // Utility functions
    int deg(int x) const;
    int ceilop(int a, int b) const;
    uint32_t load4bytes(const uint8_t* data) const;
    
    // Polynomial operations
    struct Polynomial {
        int deg;
        std::vector<int> c;
        
        Polynomial() : deg(0) {}
    };
    
    int getroots(int k, Polynomial& poly);
    void build_cyclic(const std::vector<uint32_t>& g);
    
    // BCH state
    struct ECCState {
        int m;              // Galois field degree
        int t;              // Error correction capability
        int poly;           // Generator polynomial
        int n;              // 2^m - 1
        int ecc_bytes;      // ECC size in bytes
        int ecc_bits;       // ECC size in bits
        
        std::vector<int> exponents;     // Exponential lookup table
        std::vector<int> logarithms;    // Logarithm lookup table
        std::vector<int> elp_pre;       // Error locator polynomial precomputation
        std::vector<uint32_t> cyclic_tab; // Cyclic encoding table
        std::vector<uint32_t> ecc_buf;   // ECC buffer
        std::vector<int> errloc;        // Error locations
        Polynomial elp;                  // Error locator polynomial
    };
    
    std::unique_ptr<ECCState> state_;
    int ecc_bits_;
    
    // Initialization
    bool initialize();
};

} // namespace FlowMark
