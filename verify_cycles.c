#define _ISOC11_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

/* 移除所有平台特定和并行依赖，使用标准malloc/free */
#define CACHE_LINE 64   /* 仅为提示，实际未使用对齐分配 */

typedef struct {
    uint32_t m;
    uint64_t m2;
    uint64_t m3;
    uint32_t *next[3];
    uint8_t *bitmap;
} GraphData;

static inline uint64_t idx_encode(uint32_t i, uint32_t j, uint32_t k, uint32_t m, uint64_t m2) {
    return (uint64_t)i * m2 + (uint64_t)j * m + k;
}

static void idx_decode(uint64_t idx, uint32_t m, uint64_t m2, uint32_t *i, uint32_t *j, uint32_t *k) {
    *i = (uint32_t)(idx / m2);
    uint64_t rem = idx % m2;
    *j = (uint32_t)(rem / m);
    *k = (uint32_t)(rem % m);
}

static void compute_next(GraphData *g) {
    uint32_t m = g->m;
    uint64_t m2 = g->m2;
    uint64_t m3 = g->m3;
    
    for (uint32_t c = 0; c < 3; c++) {
        g->next[c] = (uint32_t *)malloc(m3 * sizeof(uint32_t));
        if (!g->next[c]) {
            fprintf(stderr, "Memory allocation failed for cycle %u\n", c);
            exit(1);
        }
    }
    
    /* 串行计算，若需性能可手动并行化 */
    for (uint64_t idx = 0; idx < m3; idx++) {
        uint32_t i, j, k;
        idx_decode(idx, m, m2, &i, &j, &k);
        
        uint32_t s = (i + j + k) % m;
        uint32_t ip1 = (i + 1) % m;
        uint32_t jp1 = (j + 1) % m;
        uint32_t kp1 = (k + 1) % m;
        
        uint32_t next_i = i, next_j = j, next_k = k;
        
        /* Cycle 0 */
        if (s == 0) {
            if (j == m - 1) {
                next_i = ip1;
            } else {
                next_k = kp1;
            }
        } else if (s < m - 1) {
            if (i == m - 1) {
                next_k = kp1;
            } else {
                next_j = jp1;
            }
        } else {
            if (i > 0) {
                next_j = jp1;
            } else {
                next_k = kp1;
            }
        }
        g->next[0][idx] = (uint32_t)idx_encode(next_i, next_j, next_k, m, m2);
        
        /* Cycle 1 */
        next_i = i; next_j = j; next_k = k;
        if (s == 0) {
            next_j = jp1;
        } else if (s < m - 1) {
            next_i = ip1;
        } else {
            if (i > 0) {
                next_k = kp1;
            } else {
                next_j = jp1;
            }
        }
        g->next[1][idx] = (uint32_t)idx_encode(next_i, next_j, next_k, m, m2);
        
        /* Cycle 2 */
        next_i = i; next_j = j; next_k = k;
        if (s == 0) {
            if (j < m - 1) {
                next_i = ip1;
            } else {
                next_k = kp1;
            }
        } else if (s < m - 1) {
            if (i < m - 1) {
                next_k = kp1;
            } else {
                next_j = jp1;
            }
        } else {
            next_i = ip1;
        }
        g->next[2][idx] = (uint32_t)idx_encode(next_i, next_j, next_k, m, m2);
    }
}

static int verify_hamiltonian(GraphData *g, int cycle) {
    uint64_t m3 = g->m3;
    uint32_t *next = g->next[cycle];
    
    uint64_t bitmap_size = (m3 + 7) / 8;
    g->bitmap = (uint8_t *)malloc(bitmap_size);
    if (!g->bitmap) {
        fprintf(stderr, "Bitmap allocation failed\n");
        return 0;
    }
    memset(g->bitmap, 0, bitmap_size);
    
    uint64_t current = 0;
    uint64_t visited_count = 0;
    
    for (uint64_t step = 0; step < m3; step++) {
        uint64_t byte_idx = current >> 3;
        uint64_t bit_idx = current & 7;
        
        if (g->bitmap[byte_idx] & (1 << bit_idx)) {
            uint32_t ci, cj, ck;
            idx_decode(current, g->m, g->m2, &ci, &cj, &ck);
            printf("  Cycle %d: vertex (%u,%u,%u) already visited at step %llu\n", 
                   cycle, ci, cj, ck, (unsigned long long)step);
            free(g->bitmap);
            g->bitmap = NULL;
            return 0;
        }
        
        g->bitmap[byte_idx] |= (1 << bit_idx);
        visited_count++;
        current = next[current];
    }
    
    if (current != 0) {
        uint32_t ci, cj, ck;
        idx_decode(current, g->m, g->m2, &ci, &cj, &ck);
        printf("  Cycle %d: did not return to start, ended at (%u,%u,%u)\n", 
               cycle, ci, cj, ck);
        free(g->bitmap);
        g->bitmap = NULL;
        return 0;
    }
    
    if (visited_count != m3) {
        printf("  Cycle %d: visited %llu vertices, expected %llu\n", 
               cycle, (unsigned long long)visited_count, (unsigned long long)m3);
        free(g->bitmap);
        g->bitmap = NULL;
        return 0;
    }
    
    free(g->bitmap);
    g->bitmap = NULL;
    return 1;
}

static int verify_arc_coverage(GraphData *g) {
    uint32_t m = g->m;
    uint64_t m2 = g->m2;
    uint64_t m3 = g->m3;
    int failed = 0;
    
    for (uint64_t idx = 0; idx < m3; idx++) {
        uint32_t i, j, k;
        idx_decode(idx, m, m2, &i, &j, &k);
        
        uint32_t t0 = (i + 1) % m;
        uint32_t t1 = (j + 1) % m;
        uint32_t t2 = (k + 1) % m;
        
        uint64_t target0 = idx_encode(t0, j, k, m, m2);
        uint64_t target1 = idx_encode(i, t1, k, m, m2);
        uint64_t target2 = idx_encode(i, j, t2, m, m2);
        
        uint32_t n0 = g->next[0][idx];
        uint32_t n1 = g->next[1][idx];
        uint32_t n2 = g->next[2][idx];
        
        int found0 = 0, found1 = 0, found2 = 0;
        
        if (n0 == target0) found0 = 1;
        else if (n0 == target1) found1 = 1;
        else if (n0 == target2) found2 = 1;
        
        if (n1 == target0) found0 = 1;
        else if (n1 == target1) found1 = 1;
        else if (n1 == target2) found2 = 1;
        
        if (n2 == target0) found0 = 1;
        else if (n2 == target1) found1 = 1;
        else if (n2 == target2) found2 = 1;
        
        if (!found0 || !found1 || !found2) {
            printf("  Arc coverage failed at vertex (%u,%u,%u)\n", i, j, k);
            printf("    Expected targets: %llu, %llu, %llu\n", 
                   (unsigned long long)target0, (unsigned long long)target1, (unsigned long long)target2);
            printf("    Got: %u, %u, %u\n", n0, n1, n2);
            failed = 1;
        }
        
        if (n0 == n1 || n0 == n2 || n1 == n2) {
            printf("  Duplicate arc detected at vertex (%u,%u,%u): %u, %u, %u\n", 
                   i, j, k, n0, n1, n2);
            failed = 1;
        }
    }
    
    return !failed;
}

static int verify_claude_cycles(uint32_t m) {
    printf("\n=== Verifying m = %u ===\n", m);
    
    GraphData g;
    g.m = m;
    g.m2 = (uint64_t)m * m;
    g.m3 = (uint64_t)m * m * m;
    g.bitmap = NULL;
    
    printf("Graph: %u^3 = %llu vertices\n", m, (unsigned long long)g.m3);
    
    clock_t start_build = clock();
    compute_next(&g);
    double build_time = (double)(clock() - start_build) / CLOCKS_PER_SEC;
    printf("Build time: %.4f seconds\n", build_time);
    
    int all_passed = 1;
    
    clock_t start_verify = clock();
    for (int c = 0; c < 3; c++) {
        int result = verify_hamiltonian(&g, c);
        if (result) {
            printf("Cycle %d: PASSED (Hamiltonian)\n", c);
        } else {
            printf("Cycle %d: FAILED (Hamiltonian)\n", c);
            all_passed = 0;
        }
    }
    double verify_time = (double)(clock() - start_verify) / CLOCKS_PER_SEC;
    printf("Hamiltonian verification time: %.4f seconds\n", verify_time);
    
    clock_t start_arc = clock();
    if (!verify_arc_coverage(&g)) {
        all_passed = 0;
    }
    double arc_time = (double)(clock() - start_arc) / CLOCKS_PER_SEC;
    printf("Arc coverage verification time: %.4f seconds\n", arc_time);
    
    for (int c = 0; c < 3; c++) {
        free(g.next[c]);
    }
    
    if (all_passed) {
        printf("\n*** ALL TESTS PASSED for m = %u ***\n", m);
    } else {
        printf("\n*** TESTS FAILED for m = %u ***\n", m);
    }
    
    return all_passed;
}

int main(int argc, char *argv[]) {
    printf("Claude's Cycles Verification Tool (Pure C, no external dependencies)\n");
    printf("====================================================================\n");
    
    uint32_t test_values[4] = {3, 5, 7, 9};
    int num_tests = 4;
    
    if (argc > 1) {
        test_values[0] = atoi(argv[1]);
        num_tests = 1;
    }
    
    int all_passed = 1;
    clock_t total_start = clock();
    
    for (int t = 0; t < num_tests; t++) {
        uint32_t m = test_values[t];
        if (m % 2 == 0 || m < 3) {
            printf("\nSkipping m = %u (must be odd and > 2)\n", m);
            continue;
        }
        if (!verify_claude_cycles(m)) {
            all_passed = 0;
        }
    }
    
    double total_time = (double)(clock() - total_start) / CLOCKS_PER_SEC;
    printf("\n====================================================================\n");
    printf("Total execution time: %.4f seconds\n", total_time);
    
    return all_passed ? 0 : 1;
}