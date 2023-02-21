#pragma once

#include <iris/mm/virtual_address.h>

extern "C" {
extern void __iris_kernel_start();
extern void __iris_kernel_end();

extern void __iris_text_segment_start();
extern void __iris_text_segment_end();

extern void __iris_rodata_segment_start();
extern void __iris_rodata_segment_end();

extern void __iris_data_segment_start();
extern void __iris_data_segment_end();
}

namespace iris::mm {
static inline VirtualAddress kernel_start((u64) &__iris_kernel_start);
static inline VirtualAddress kernel_end((u64) &__iris_kernel_end);

static inline VirtualAddress text_segment_start((u64) &__iris_text_segment_start);
static inline VirtualAddress text_segment_end((u64) &__iris_text_segment_end);

static inline VirtualAddress rodata_segment_start((u64) &__iris_rodata_segment_start);
static inline VirtualAddress rodata_segment_end((u64) &__iris_rodata_segment_end);

static inline VirtualAddress data_segment_start((u64) &__iris_data_segment_start);
static inline VirtualAddress data_segment_end((u64) &__iris_data_segment_end);
}
