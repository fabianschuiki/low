/* Copyright (c) 2015-2016 Fabian Schuiki */
#include "array.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>


static void
array_reallocate(array_t *self, unsigned capacity) {
	assert(self);
	if (self->items) {
		self->items = realloc(self->items, capacity * self->item_size);
	} else {
		self->items = malloc(capacity * self->item_size);
	}
	self->capacity = capacity;
}

static void
array_grow(array_t *self, unsigned additional) {
	assert(self);
	unsigned req = self->size + additional;
	if (self->capacity < req) {
		unsigned cap = self->capacity;
		if (cap == 0) cap = 1;
		while (cap < req) cap *= 2;
		array_reallocate(self, cap);
	}
}

static void
array_move_tail(array_t *self, unsigned from, unsigned to) {
	assert(self);
	memmove(
		self->items + to*self->item_size,
		self->items + from*self->item_size,
		(self->size-from)*self->item_size);
}


/// Initializes an array.
void
array_init(array_t *self, unsigned item_size) {
	assert(self);
	assert(item_size > 0);
	self->item_size = item_size;
	self->size = 0;
	self->capacity = 0;
	self->items = 0;
}

/// Disposes of the resources held by an array.
void
array_dispose(array_t *self) {
	assert(self);
	if (self->items) {
		free(self->items);
		self->items = 0;
	}
}

/// Makes sure that the array can hold at least \a capacity number of items
/// without having to reallocate its memory.
void
array_reserve(array_t *self, unsigned capacity) {
	assert(self);
	if (capacity <= self->capacity)
		return;
	array_reallocate(self, capacity);
}

/// Resizes the array to hold exactly the given number of items.
void
array_resize(array_t *self, unsigned items) {
	assert(self);
	if (items > self->capacity)
		array_reallocate(self, items);
	self->size = items;
}

/// Reduces the array's capacity and memory footprint to hold exactly its
/// current number of items.
void
array_shrink(array_t *self) {
	assert(self);
	if (self->size != self->capacity)
		array_reallocate(self, self->size);
}


/// Returns a pointer to the item at the given index.
void*
array_get(const array_t *self, unsigned index) {
	assert(self);
	assert(index < self->size);
	return self->items + (index * self->item_size);
}

/// Copies multiple items from the array, starting at the given index.
void
array_get_many(const array_t *self, unsigned index, void *items, unsigned num_items) {
	assert(self);
	assert(index < self->size);
	assert(index+num_items <= self->size);
	memcpy(items,
	       self->items + index*self->item_size,
	       num_items*self->item_size);
}

/// Copies an item to the given index in the array, replacing the item that was
/// previously there.
void
array_set(array_t *self, unsigned index, const void *item) {
	assert(self);
	assert(index < self->size);
	memcpy(self->items + index*self->item_size, item, self->item_size);
}

/// Copies multiple items to the array, starting at the given index. Replaces
/// the items that were previously there.
void
array_set_many(array_t *self, unsigned index, const void *items, unsigned num_items) {
	assert(self);
	assert(index+num_items <= self->size);
	memcpy(self->items + index*self->item_size,
	       items,
	       num_items*self->item_size);
}


/// Inserts an item at the given index into the array. If \a item is 0, the
/// location in the array remains uninitialized and may be populated by
/// different means.
/// \return Returns a pointer to the location in the array.
void*
array_insert(array_t *self, unsigned index, const void *item) {
	assert(self);
	assert(index <= self->size);
	array_grow(self, 1);
	if (index < self->size)
		array_move_tail(self, index, index+1);
	self->size++;
	if (item)
		array_set(self, index, item);
	return self->items + index*self->item_size;
}

/// Inserts multiple items at the given index into the array. If \a items is 0,
/// the locations in the array remain unitialized and may be populated by
/// different means.
/// \return Returns a pointer to the location in the array.
void*
array_insert_many(array_t *self, unsigned index, const void *items, unsigned num_items) {

	assert(self);
	assert(index <= self->size);
	array_grow(self, num_items);
	if (index < self->size)
		array_move_tail(self, index, index+num_items);
	self->size += num_items;
	if (items)
		array_set_many(self, index, items, num_items);
	return self->items + index*self->item_size;
}


/// Erases the item at the given index from the array.
void
array_erase(array_t *self, unsigned index) {
	assert(self);
	assert(index < self->size);
	if (index < self->size-1)
		array_move_tail(self, index+1, index);
	self->size--;
}

/// Erases the items in the range [first,last) from the array.
void
array_erase_range(array_t *self, unsigned first, unsigned last) {
	assert(self);
	assert(first <= last);
	assert(last <= self->size);
	if (last < self->size)
		array_move_tail(self, last, first);
	self->size -= (last-first);
}


/// Adds an item to the end of the array. If \a item is 0, an unitialized item
/// is added to the array that may be populated by different means.
/// \return Returns a pointer to the location in the array.
void*
array_add(array_t *self, const void *item) {
	assert(self);
	return array_insert(self, self->size, item);
}

/// Adds multiple items to the end of the array. If \a item is 0, unitialized
/// items are added to the array that may be populated by different means.
/// \return Returns a pointer to the location in the array.
void*
array_add_many(array_t *self, const void *items, unsigned num_items) {
	assert(self);
	return array_insert_many(self, self->size, items, num_items);
}

/// Removes an item from the end of the array.
void
array_remove(array_t *self) {
	assert(self);
	assert(self->size > 0);
	self->size--;
}

/// Removes multiple items from the end of the array.
void
array_remove_many(array_t *self, unsigned num_items) {
	assert(self);
	assert(num_items <= self->size);
	self->size -= num_items;
}


/// Removes all items from the array.
void
array_clear(array_t *self) {
	assert(self);
	self->size = 0;
}
