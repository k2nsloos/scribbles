#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>


#define MAX_PRINT_DIM 40


typedef struct {
    size_t** row_idx;
    size_t* row_count;
    size_t* row_reserved;
    size_t dim;
    size_t alloc_cnt;
} bit_mat_s;

void bit_mat_destroy(bit_mat_s* m)
{
    for (size_t row = 0; row < m->dim; ++row) free(m->row_idx[row]);
    free(m->row_idx);
    free(m->row_count);
    free(m->row_reserved);
    free(m);
}

bit_mat_s* bit_mat_create(size_t dim)
{
    bit_mat_s* m = calloc(1, sizeof(bit_mat_s));
    assert(m);
    
    m->row_idx = calloc(dim, sizeof(size_t*));
    m->row_count = calloc(dim, sizeof(size_t));
    m->row_reserved = calloc(dim, sizeof(size_t));
    m->alloc_cnt = 4;
    assert(m->row_idx);
    assert(m->row_count);
    assert(m->row_reserved);

    m->dim = dim;
    return m;
}

void bit_mat_row_reserve(bit_mat_s* m, size_t row, size_t count)
{
    size_t* row_idx = m->row_idx[row];
    if (m->row_reserved[row] < count) {
        //printf("D: reserve %lu for row %lu\n", count, row);
        m->row_idx[row] = realloc(row_idx, count * sizeof(size_t));
        ++m->alloc_cnt;
        assert(m->row_idx[row]);
        m->row_reserved[row] = count;
    }
}

void bit_mat_set_row(bit_mat_s* m, size_t row, const size_t* row_idx, size_t count)
{
    bit_mat_row_reserve(m, row, count);
    memcpy(m->row_idx[row], row_idx, count * sizeof(size_t));
    m->row_count[row] = count;
}

void bit_mat_swap_rows(bit_mat_s* m, size_t row1, size_t row2)
{
    size_t* tmp_idx = m->row_idx[row1];
    m->row_idx[row1] = m->row_idx[row2];
    m->row_idx[row2] = tmp_idx;

    size_t tmp_cnt = m->row_count[row1];
    m->row_count[row1] = m->row_count[row2];
    m->row_count[row2] = tmp_cnt;
}

size_t find_elem(const size_t* row_idx, size_t row_count, size_t col)
{
    for (size_t pos = 0; pos < row_count; ++pos) {
        if (row_idx[pos] == col) return pos;
    }
    return SIZE_MAX;
}


size_t find_min_elem(const size_t* row_idx, size_t row_count)
{
    size_t min_idx = SIZE_MAX;
    for (size_t pos = 0; pos < row_count; ++pos) {
        min_idx = row_idx[pos] < min_idx ? row_idx[pos] : min_idx;
    }
    return min_idx;
}

size_t flip_elem_unsafe(size_t* row_idx, size_t row_count, size_t col)
{
    size_t pos = find_elem(row_idx, row_count, col);
    if (pos != SIZE_MAX) {
        // Remove elem if exist
        row_idx[pos] = row_idx[--row_count];
    } else {
        // Owtherwise add it
        row_idx[row_count++] = col;
    }

    return row_count;
}

void bit_mat_print(const bit_mat_s* m)
{
    for (size_t row = 0; row < m->dim; ++row) {
        for (size_t col = 0; col < m->dim; ++col) {
            printf("%d ", find_elem(m->row_idx[row], m->row_count[row], col) != SIZE_MAX);
        }
        printf("\n");
    }
}

void precondition_mat(bit_mat_s* m)
{
    for (size_t src_row = 0; src_row < m->dim; ++src_row) {
        size_t dst_row = find_min_elem(m->row_idx[src_row], m->row_count[src_row]);
        bit_mat_swap_rows(m, src_row, dst_row);
    }
}

size_t get_image_dimension(bit_mat_s* m)
{
    size_t* tmp_idx = calloc(m->dim, sizeof(size_t));
    size_t tmp_cnt = 0;

    for (size_t pivot = 0; pivot < m->dim; ++pivot) {
        // Check if no action required
        //if (find_min_elem(m->row_idx[pivot], m->row_count[pivot]) == pivot) continue;

        // Otherwise clear a row upto the pivot
        printf("D: Sweep pivot %lu\n", pivot);
        size_t row = pivot;
        for (; row < m->dim; ++row) {
            // Cache row
            memcpy(tmp_idx, m->row_idx[row], m->row_count[row]);
            tmp_cnt = m->row_count[row];

            // Sweep until we hit diagonal
            size_t min_col;
            while ((min_col = find_min_elem(tmp_idx, tmp_cnt)) < pivot) {
                const size_t* pivot_idx = m->row_idx[min_col];
                size_t pivot_cnt = m->row_count[min_col];

                for (size_t pos = 0; pos < pivot_cnt; ++pos) {
                    tmp_cnt = flip_elem_unsafe(tmp_idx, tmp_cnt, pivot_idx[pos]);
                }
            }

            // set row
            bit_mat_set_row(m, row, tmp_idx, tmp_cnt);

            // If diagonal element present we're done, otherwise try another row
            if (min_col == pivot) break;
        }

        // No suitable pivot found
        if (row >= m->dim) continue;
        
        bit_mat_swap_rows(m, row, pivot);
        if (m->dim < MAX_PRINT_DIM) bit_mat_print(m);
    }

    free(tmp_idx);

    printf("D: counting image dim\n");
    size_t im_dim = 0;
    for (size_t row = 0; row < m->dim; ++row) {
        if (m->row_count[row]) ++im_dim;
    }

    return im_dim;
}

static size_t get_position(size_t row, size_t col, size_t cols)
{
    return col + row * cols;
}

bit_mat_s* generate_matrix(size_t size)
{
    size_t dim = size * size;
    size_t row_idxs[4];
    bit_mat_s* r = bit_mat_create(dim);

    for (size_t action_id = 0; action_id < dim; ++action_id) {
        size_t action_row = action_id / size;
        size_t action_col = action_id % size;
        size_t count = 0;

        if (action_row > 0) row_idxs[count++] = get_position(action_row - 1, action_col, size);
        if (action_col > 0) row_idxs[count++] = get_position(action_row, action_col - 1, size);
        if (action_col + 1 < size) row_idxs[count++] = get_position(action_row, action_col + 1, size);
        if (action_row + 1 < size) row_idxs[count++] = get_position(action_row + 1, action_col, size);

        bit_mat_set_row(r, action_id, row_idxs, count);
    }

    return r;
}

int main(int argc, char **argv)
{
    if (argc == 1) {
        printf("usage: %s <size>\n", argv[0]);
        return 0;
    }

    size_t size = strtoul(argv[1], NULL, 0);
    bit_mat_s* m = generate_matrix(size);

    if (m->dim < MAX_PRINT_DIM) bit_mat_print(m);
    //precondition_mat(m);
 
    if (m->dim < MAX_PRINT_DIM) bit_mat_print(m);
    size_t im_dim = get_image_dimension(m);
    
    printf("Solveable states for %lu x %lu: 2^%lu / 2^%lu\n", 
           size, 
           size, 
           im_dim,
           m->dim);

    printf("calls to malloc: %lu\n", m->alloc_cnt);

    bit_mat_destroy(m);
    return 0;
}