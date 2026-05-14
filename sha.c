#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>
#include <math.h>

#define HASH_SIZE 32 // 256 bits
#define SEED_SIZE 32 // 256 bits

// Function to calculate Hamming Distance (bit differences)
int calculate_hamming_distance(unsigned char *h1, unsigned char *h2) {
    int distance = 0;
    for (int i = 0; i < HASH_SIZE; i++) {
        unsigned char xor_val = h1[i] ^ h2[i];
        while (xor_val > 0) {
            if (xor_val & 1) distance++;
            xor_val >>= 1;
        }
    }
    return distance;
}

void analyze_avalanche() {
    unsigned char seed[SEED_SIZE] = "Initial Sophisticated Seed 2026";
    unsigned char modified_seed[SEED_SIZE];
    unsigned char base_hash[HASH_SIZE];
    unsigned char new_hash[HASH_SIZE];
    
    int bit_counts[257] = {0}; // Track frequency of distances
    double total_distance = 0;
    int tests = 0;

    // Generate Base Hash
    SHA256(seed, SEED_SIZE, base_hash);

    printf("--- Statistical Analysis of SHA-256 Seed Variations ---\n");
    printf("Base Seed: %s\n", seed);

    // Systematic Variation: Flip every bit in the seed
    for (int i = 0; i < SEED_SIZE; i++) {
        for (int bit = 0; bit < 8; bit++) {
            memcpy(modified_seed, seed, SEED_SIZE);
            
            // Flip exactly one bit
            modified_seed[i] ^= (1 << bit);
            
            SHA256(modified_seed, SEED_SIZE, new_hash);
            
            int dist = calculate_hamming_distance(base_hash, new_hash);
            bit_counts[dist]++;
            total_distance += dist;
            tests++;
        }
    }

    // Statistical Review
    double mean = total_distance / tests;
    double variance = 0;
    for (int i = 0; i <= 256; i++) {
        if (bit_counts[i] > 0) {
            variance += bit_counts[i] * pow(i - mean, 2);
        }
    }
    variance /= tests;

    printf("\n[Results]\n");
    printf("Total Single-Bit Variations Tested: %d\n", tests);
    printf("Average Bits Changed (Mean): %.2f (Ideal: 128.0)\n", mean);
    printf("Standard Deviation: %.2f\n", sqrt(variance));
    printf("Avalanche Effect Score: %.4f%%\n", (mean / 256.0) * 100);

    printf("\n[Probability Distribution Snippet]\n");
    printf("Dist | Frequency\n----------------\n");
    for(int i = (int)mean-5; i <= (int)mean+5; i++) {
        printf("%d  | %d\n", i, bit_counts[i]);
    }
}

int main() {
    analyze_avalanche();
    re
      turn 0;
}
