#include "bch_ecc.h"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <stdexcept>

namespace FlowMark {

BCH::BCH(int t, int poly) : ecc_bits_(0) {
    std::cout << "  BCH: Creating state..." << std::endl;
    state_ = std::make_unique<ECCState>();
    state_->t = t;
    state_->poly = poly;
    
    std::cout << "  BCH: Initializing..." << std::endl;
    if (!initialize()) {
        throw std::runtime_error("Failed to initialize BCH codec");
    }
    std::cout << "  BCH: Initialized successfully" << std::endl;
}

BCH::~BCH() = default;

bool BCH::initialize() {
    std::cout << "    BCH: Step 1 - Calculate m..." << std::endl;
    // Calculate m (Galois field degree)
    int tmp = state_->poly;
    int m = 0;
    while (tmp >> 1) {
        tmp = tmp >> 1;
        m++;
    }
    state_->m = m;
    state_->n = (1 << m) - 1;  // 2^m - 1
    std::cout << "    BCH: m=" << m << ", n=" << state_->n << std::endl;
    
    std::cout << "    BCH: Step 2 - Allocate tables..." << std::endl;
    int words = ceilop(m * state_->t, 32);
    state_->ecc_bytes = ceilop(m * state_->t, 8);
    
    // Initialize lookup tables
    state_->exponents.resize(1 + state_->n);
    state_->logarithms.resize(1 + state_->n);
    state_->elp_pre.resize(1 + m);
    state_->cyclic_tab.resize(words * 1024);
    state_->ecc_buf.resize(words);
    state_->errloc.resize(state_->t);
    std::cout << "    BCH: Tables allocated" << std::endl;
    
    std::cout << "    BCH: Step 3 - Build exp/log tables..." << std::endl;
    // Build exponential and logarithm tables
    int x = 1;
    int k = 1 << deg(state_->poly);
    if (k != (1 << m)) {
        std::cout << "    BCH: ERROR - k != 2^m" << std::endl;
        return false;
    }
    
    for (int i = 0; i < state_->n; i++) {
        state_->exponents[i] = x;
        state_->logarithms[x] = i;
        if (i && x == 1) {
            std::cout << "    BCH: ERROR - x==1 too early" << std::endl;
            return false;
        }
        x *= 2;
        if (x & k) {
            x = x ^ state_->poly;
        }
    }
    
    state_->logarithms[0] = 0;
    state_->exponents[state_->n] = 1;
    std::cout << "    BCH: Exp/log tables built" << std::endl;
    
    std::cout << "    BCH: Step 4 - Enumerate roots..." << std::endl;
    // Build generator polynomial g(x)
    std::vector<int> roots(state_->n + 1, 0);
    
    // Enumerate all roots
    for (int i = 0; i < state_->t; i++) {
        int r = 2 * i + 1;
        for (int j = 0; j < m; j++) {
            roots[r] = 1;
            r = mod(2 * r);
        }
    }
    std::cout << "    BCH: Roots enumerated" << std::endl;
    
    std::cout << "    BCH: Step 5 - Build generator polynomial..." << std::endl;
    // Build g(x)
    Polynomial g;
    g.deg = 0;
    g.c.resize(m * state_->t + 1, 0);
    g.c[0] = 1;
    
    int root_count = 0;
    for (int i = 0; i < state_->n; i++) {
        if (roots[i]) {
            root_count++;
            int r = state_->exponents[i];
            g.c[g.deg + 1] = 1;
            for (int j = g.deg; j > 0; j--) {
                g.c[j] = g_mul(g.c[j], r) ^ g.c[j - 1];
            }
            g.c[0] = g_mul(g.c[0], r);
            g.deg++;
        }
    }
    std::cout << "    BCH: Generator polynomial built, deg=" << g.deg << ", roots=" << root_count << std::endl;
    
    std::cout << "    BCH: Step 6 - Store generator polynomial..." << std::endl;
    // Store generator polynomial
    int n = g.deg + 1;
    int i = 0;
    std::vector<uint32_t> genpoly(ceilop(m * state_->t + 1, 32), 0);
    
    while (n > 0) {
        int nbits = (n > 32) ? 32 : n;
        uint32_t word = 0;
        for (int j = 0; j < nbits; j++) {
            if (g.c[n - 1 - j]) {
                word = word | (1U << (31 - j));
            }
        }
        genpoly[i] = word;
        i++;
        n -= nbits;
    }
    
    ecc_bits_ = g.deg;
    state_->ecc_bits = g.deg;
    std::cout << "    BCH: Generator polynomial stored, ecc_bits=" << ecc_bits_ << std::endl;
    
    std::cout << "    BCH: Step 7 - Build cyclic encoding table..." << std::endl;
    // Build cyclic encoding table
    build_cyclic(genpoly);
    std::cout << "    BCH: Cyclic table built" << std::endl;
    
    std::cout << "    BCH: Step 8 - Precompute sqrt lookup..." << std::endl;
    // Precompute sqrt lookup table
    int sum = 0;
    int aexp = 0;
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < m; j++) {
            sum = sum ^ g_pow(i * (1 << j));
        }
        if (sum) {
            aexp = state_->exponents[i];
            break;
        }
    }
    
    x = 0;
    std::vector<int> precomp(31, 0);
    int remaining = m;
    
    while (x <= state_->n && remaining) {
        int y = g_sqrt(x) ^ x;
        for (int i = 0; i < 2; i++) {
            int r = g_log(y);
            if (y && (r < m) && !precomp[r]) {
                state_->elp_pre[r] = x;
                precomp[r] = 1;
                remaining--;
                break;
            }
            y = y ^ aexp;
        }
        x++;
    }
    
    return true;
}

std::vector<uint8_t> BCH::encode(const std::vector<uint8_t>& data) {
    int datalen = data.size();
    int l = ceilop(state_->m * state_->t, 32) - 1;
    
    std::vector<uint8_t> ecc(state_->ecc_bytes);
    
    const int ecc_max_words = ceilop(31 * 64, 32);
    std::vector<uint32_t> r(ecc_max_words, 0);
    
    int tab0idx = 0;
    int tab1idx = tab0idx + 256 * (l + 1);
    int tab2idx = tab1idx + 256 * (l + 1);
    int tab3idx = tab2idx + 256 * (l + 1);
    
    int mlen = datalen / 4;
    int offset = 0;
    
    while (mlen > 0) {
        uint32_t w = load4bytes(&data[offset]);
        w = w ^ r[0];
        int p0 = tab0idx + (l + 1) * ((w >> 0) & 0xff);
        int p1 = tab1idx + (l + 1) * ((w >> 8) & 0xff);
        int p2 = tab2idx + (l + 1) * ((w >> 16) & 0xff);
        int p3 = tab3idx + (l + 1) * ((w >> 24) & 0xff);
        
        for (int i = 0; i < l; i++) {
            r[i] = r[i + 1] ^ state_->cyclic_tab[p0 + i] ^ 
                   state_->cyclic_tab[p1 + i] ^ state_->cyclic_tab[p2 + i] ^ 
                   state_->cyclic_tab[p3 + i];
        }
        
        r[l] = state_->cyclic_tab[p0 + l] ^ state_->cyclic_tab[p1 + l] ^ 
               state_->cyclic_tab[p2 + l] ^ state_->cyclic_tab[p3 + l];
        mlen--;
        offset += 4;
    }
    
    // Process remaining bytes
    int leftdata = datalen - offset;
    int posn = offset;
    
    while (leftdata) {
        uint8_t tmp = data[posn];
        posn++;
        int pidx = (l + 1) * (((r[0] >> 24) ^ (tmp)) & 0xff);
        for (int i = 0; i < l; i++) {
            r[i] = ((r[i] << 8) | (r[i + 1] >> 24)) ^ state_->cyclic_tab[pidx];
            pidx++;
        }
        r[l] = (r[l] << 8) ^ state_->cyclic_tab[pidx];
        leftdata--;
    }
    
    state_->ecc_buf = r;
    
    // Convert to bytes
    std::vector<uint8_t> eccout;
    for (auto e : r) {
        eccout.push_back((e >> 24) & 0xff);
        eccout.push_back((e >> 16) & 0xff);
        eccout.push_back((e >> 8) & 0xff);
        eccout.push_back((e >> 0) & 0xff);
    }
    
    eccout.resize(state_->ecc_bytes);
    return eccout;
}

int BCH::decode(std::vector<uint8_t>& data, const std::vector<uint8_t>& recvecc) {
    // Calculate expected ECC
    auto calc_ecc = encode(data);
    
    state_->errloc.clear();
    
    // Load received ECC into buffer
    int ecclen = recvecc.size();
    int mlen = ecclen / 4;
    std::vector<uint32_t> eccbuf;
    int offset = 0;
    
    while (mlen > 0) {
        uint32_t w = load4bytes(&recvecc[offset]);
        eccbuf.push_back(w);
        offset += 4;
        mlen--;
    }
    
    int leftdata = ecclen - offset;
    if (leftdata > 0) {
        std::vector<uint8_t> padded(4, 0);
        std::copy(recvecc.begin() + offset, recvecc.end(), padded.begin());
        uint32_t w = load4bytes(padded.data());
        eccbuf.push_back(w);
    }
    
    int eccwords = ceilop(state_->ecc_bits, 32);
    
    // XOR with calculated ECC
    uint32_t sum = 0;
    for (int i = 0; i < eccwords; i++) {
        state_->ecc_buf[i] = state_->ecc_buf[i] ^ eccbuf[i];
        sum = sum | state_->ecc_buf[i];
    }
    
    if (sum == 0) {
        return 0; // No errors
    }
    
    // Note: Full syndrome computation and error locator polynomial calculation
    // would go here. For brevity, we return -1 to indicate correction is needed.
    // A production implementation would include the complete Berlekamp-Massey algorithm.
    
    return -1; // Error correction not fully implemented
}

int BCH::get_ecc_bytes() const {
    return ceilop(state_->m * state_->t, 8);
}

// Galois field operations
int BCH::g_inv(int a) {
    return state_->exponents[state_->n - state_->logarithms[a]];
}

int BCH::g_sqrt(int a) {
    if (a) {
        return state_->exponents[mod(2 * state_->logarithms[a])];
    }
    return 0;
}

int BCH::mod(int v) {
    if (v < state_->n) {
        return v;
    }
    return v - state_->n;
}

int BCH::g_mul(int a, int b) {
    if (a > 0 && b > 0) {
        int res = mod(state_->logarithms[a] + state_->logarithms[b]);
        return state_->exponents[res];
    }
    return 0;
}

int BCH::g_div(int a, int b) {
    if (a) {
        return state_->exponents[mod(state_->logarithms[a] + state_->n - state_->logarithms[b])];
    }
    return 0;
}

int BCH::modn(int v) {
    int n = state_->n;
    while (v >= n) {
        v -= n;
        v = (v & n) + (v >> state_->m);
    }
    return v;
}

int BCH::g_log(int x) {
    return state_->logarithms[x];
}

int BCH::a_ilog(int x) {
    return mod(state_->n - state_->logarithms[x]);
}

int BCH::g_pow(int i) {
    return state_->exponents[modn(i)];
}

int BCH::deg(int x) const {
    int count = 0;
    while (x >> 1) {
        x = x >> 1;
        count++;
    }
    return count;
}

int BCH::ceilop(int a, int b) const {
    return (a + b - 1) / b;
}

uint32_t BCH::load4bytes(const uint8_t* data) const {
    uint32_t w = 0;
    w += static_cast<uint32_t>(data[0]) << 24;
    w += static_cast<uint32_t>(data[1]) << 16;
    w += static_cast<uint32_t>(data[2]) << 8;
    w += static_cast<uint32_t>(data[3]) << 0;
    return w;
}

void BCH::build_cyclic(const std::vector<uint32_t>& g) {
    int l = ceilop(state_->m * state_->t, 32);
    int plen = ceilop(state_->ecc_bits + 1, 32);
    int ecclen = ceilop(state_->ecc_bits, 32);
    
    std::cout << "      build_cyclic: l=" << l << ", plen=" << plen << ", ecclen=" << ecclen << std::endl;
    std::cout << "      build_cyclic: table size=" << (4 * 256 * l) << std::endl;
    
    state_->cyclic_tab.resize(4 * 256 * l, 0);
    
    for (int i = 0; i < 256; i++) {
        if (i % 64 == 0) {
            std::cout << "      build_cyclic: processing byte " << i << "/256" << std::endl;
        }
        for (int b = 0; b < 4; b++) {
            int offset = (b * 256 + i) * l;
            uint32_t data = static_cast<uint32_t>(i) << (8 * b);
            
            int safety_counter = 0;
            const int MAX_ITERATIONS = 1000;
            while (data && safety_counter < MAX_ITERATIONS) {
                int d = deg(data);
                data = data ^ (g[0] >> (31 - d));
                
                for (int j = 0; j < ecclen; j++) {
                    uint32_t hi = (d < 31) ? (g[j] << (d + 1)) : 0;
                    uint32_t lo = (j + 1 < plen) ? (g[j + 1] >> (31 - d)) : 0;
                    state_->cyclic_tab[j + offset] = state_->cyclic_tab[j + offset] ^ (hi | lo);
                }
                safety_counter++;
            }
            
            if (safety_counter >= MAX_ITERATIONS) {
                std::cout << "      build_cyclic: WARNING - hit max iterations for i=" << i << ", b=" << b << std::endl;
            }
        }
    }
    std::cout << "      build_cyclic: complete" << std::endl;
}

int BCH::getroots(int k, Polynomial& poly) {
    // Simplified root finding - full implementation would use Chien search
    return -1;
}

} // namespace FlowMark
