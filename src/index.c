#include <string.h>

#include "btree.h"
#include "cs165_api.h"
#include "index.h"
#include "utils.h"

Status construct_sorted_index(Column* column, Table* table, bool clustered) {
	Status ret_status;

	int* sorted_copy = malloc(column->length * sizeof *sorted_copy);
	if (!sorted_copy) {
		ret_status.code = ERROR;
		log_err("Could not allocate memory for sorted copy of column %s.\n", column->name);
		return ret_status;
	}

	if (!memcpy(sorted_copy, column->data, column->length * sizeof *sorted_copy)) {
		ret_status.code = ERROR;
		log_err("Could not copy data from column %s for sorted index.\n");
		return ret_status;
	}

	int* positions = malloc(column->length * sizeof *positions);
	if (!positions) {
		ret_status.code = ERROR;
		log_err("Could not allocate memory for positions array for sorted index on column %s.\n"
				, column->name);
		return ret_status;
	}
	for (int i = 0; i < column->length; i++)
		positions[i] = i;

	positions = sort(sorted_copy, column->length, positions, clustered ? table : NULL);

	int** idx_data = NULL;
	if (clustered) {
		idx_data = malloc(sizeof *idx_data * table->columns_size);
		for (size_t i = 0; i < table->columns_size; i++) {
			idx_data[i] = table->columns[i].data;
		}
	} else {
		idx_data = malloc(sizeof *idx_data);	
		*idx_data = sorted_copy;
	}
	column->index->data = idx_data;
	column->index->positions = positions;
	
	ret_status.code = OK;
	log_info("Successfully constructed sorted index on column %s in table %s.\n", column->name
			, table->name);
	return ret_status;
}

Status construct_btree_index(Column* column) {
	Status ret_status;
	
	Btree* index = alloc_btree();

	for (size_t i = 0; i < column->length; i += DEFAULT_BTREE_NODE_CAPACITY) {
		insert(&column->data[i], index);
	}

	column->index->tree = index;
	ret_status.code = OK;
	return ret_status;
}

Status construct_index(Column* column, Table* table) {
	Status ret_status;
	ret_status = construct_sorted_index(column, table, column->clustered);
	if (ret_status.code != OK) {
		log_err("Could not construct sorted index on column %s\n", column->name);
		return ret_status;
	}

	if (column->index->type == BTREE) {
		ret_status = construct_btree_index(column);
		if (ret_status.code != OK) {
			log_err("Could not construct btree index on column %s\n", column->name);
			return ret_status;
		}
	}

	ret_status.code = OK;
	log_info("CONSTRUCTED INDEX ON COLUMN %s\n", column->name);
	return ret_status;
}

/*
 *
 * Sorting used for creating sorted indices
 * Simple quicksort implementation using last element as pivot
 */

void swap(int* a, int* b) {
	int tmp = *a;
	*a = *b;
	*b = tmp;
}

void quickSort(int* arr, int* positions, Table* table, int low, int high) {
	if (low >= high) 
		return;
	int lo = low;
	int hi = high;
	int pivot = arr[low + (high - low) / 2];
	
	while (lo <= hi) {
		while (arr[lo] < pivot)
			lo++;
		while (arr[hi] > pivot)
			hi--;

		if (lo <= hi) {
			swap(&arr[lo], &arr[hi]);
			if (positions)
				swap(&positions[lo], &positions[hi]);
			if (table)
				for (size_t i = 0; i < table->columns_size; i++) 
					swap(&table->columns[i].data[lo], &table->columns[i].data[hi]);

			lo++;
			hi--;
		}
	}
	
	if (low < hi) 
		quickSort(arr, positions, table, low, hi);
	if (lo < high) 
		quickSort(arr, positions, table, lo, high);
	return;
}

// returns vector of positions corresponding to sorted order of arr
int* sort(int* arr, int arr_len, int* positions, Table* table) {
	quickSort(arr, positions, table, 0, arr_len - 1);
	return positions;
}

