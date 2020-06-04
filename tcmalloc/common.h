// Copyright 2019 The TCMalloc Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Common definitions for tcmalloc code.

#ifndef TCMALLOC_COMMON_H_
#define TCMALLOC_COMMON_H_

#include <bits/wordsize.h>
#include <stddef.h>
#include <stdint.h>

#include "absl/base/attributes.h"
#include "absl/base/internal/spinlock.h"
#include "absl/base/optimization.h"
#include "tcmalloc/internal/bits.h"
#include "tcmalloc/internal/config.h"
#include "tcmalloc/internal/logging.h"
#include "tcmalloc/size_class_info.h"

//-------------------------------------------------------------------
// Configuration
//-------------------------------------------------------------------

// There are four different models for tcmalloc which are created by defining a
// set of constant variables differently:
//
// DEFAULT:
//   The default configuration strives for good performance while trying to
//   minimize fragmentation.  It uses a smaller page size to reduce
//   fragmentation, but allocates per-thread and per-cpu capacities similar to
//   TCMALLOC_LARGE_PAGES / TCMALLOC_256K_PAGES.
//
// TCMALLOC_LARGE_PAGES:
//   Larger page sizes increase the bookkeeping granularity used by TCMalloc for
//   its allocations.  This can reduce PageMap size and traffic to the
//   innermost cache (the page heap), but can increase memory footprints.  As
//   TCMalloc will not reuse a page for a different allocation size until the
//   entire page is deallocated, this can be a source of increased memory
//   fragmentation.
//
//   Historically, larger page sizes improved lookup performance for the
//   pointer-to-size lookup in the PageMap that was part of the critical path.
//   With most deallocations leveraging C++14's sized delete feature
//   (https://isocpp.org/files/papers/n3778.html), this optimization is less
//   significant.
//
// TCMALLOC_256K_PAGES
//   This configuration uses an even larger page size (256KB) as the unit of
//   accounting granularity.
//
// TCMALLOC_SMALL_BUT_SLOW:
//   Used for situations where minimizing the memory footprint is the most
//   desirable attribute, even at the cost of performance.
//
// The constants that vary between models are:
//
//   kPageShift - Shift amount used to compute the page size.
//   kNumClasses - Number of size classes serviced by bucket allocators
//   kMaxSize - Maximum size serviced by bucket allocators (thread/cpu/central)
//   kMinThreadCacheSize - The minimum size in bytes of each ThreadCache.
//   kMaxThreadCacheSize - The maximum size in bytes of each ThreadCache.
//   kDefaultOverallThreadCacheSize - The maximum combined size in bytes of all
//     ThreadCaches for an executable.
//   kStealAmount - The number of bytes one ThreadCache will steal from another
//     when the first ThreadCache is forced to Scavenge(), delaying the next
//     call to Scavenge for this thread.

// Older configurations had their own customized macros.  Convert them into
// a page-shift parameter that is checked below.

#ifndef TCMALLOC_PAGE_SHIFT
#ifdef TCMALLOC_SMALL_BUT_SLOW
#define TCMALLOC_PAGE_SHIFT 12
#define TCMALLOC_USE_PAGEMAP3
#elif defined(TCMALLOC_256K_PAGES)
#define TCMALLOC_PAGE_SHIFT 18
#elif defined(TCMALLOC_LARGE_PAGES)
#define TCMALLOC_PAGE_SHIFT 15
#else
#define TCMALLOC_PAGE_SHIFT 13
#endif
#else
#error "TCMALLOC_PAGE_SHIFT is an internal macro!"
#endif

#if TCMALLOC_PAGE_SHIFT == 12
inline constexpr size_t kPageShift = 12;
inline constexpr size_t kNumClasses = 46;
inline constexpr size_t kMaxSize = 8 << 10;
inline constexpr size_t kMinThreadCacheSize = 4 * 1024;
inline constexpr size_t kMaxThreadCacheSize = 64 * 1024;
inline constexpr size_t kMaxCpuCacheSize = 20 * 1024;
inline constexpr size_t kDefaultOverallThreadCacheSize = kMaxThreadCacheSize;
inline constexpr size_t kStealAmount = kMinThreadCacheSize;
inline constexpr size_t kDefaultProfileSamplingRate = 1 << 19;
inline constexpr size_t kMinPages = 2;
#elif TCMALLOC_PAGE_SHIFT == 15
inline constexpr size_t kPageShift = 15;
inline constexpr size_t kNumClasses = 78;
inline constexpr size_t kMaxSize = 256 * 1024;
inline constexpr size_t kMinThreadCacheSize = kMaxSize * 2;
inline constexpr size_t kMaxThreadCacheSize = 4 << 20;
inline constexpr size_t kMaxCpuCacheSize = 3 * 1024 * 1024;
inline constexpr size_t kDefaultOverallThreadCacheSize =
    8u * kMaxThreadCacheSize;
inline constexpr size_t kStealAmount = 1 << 16;
inline constexpr size_t kDefaultProfileSamplingRate = 1 << 21;
inline constexpr size_t kMinPages = 8;
#elif TCMALLOC_PAGE_SHIFT == 18
inline constexpr size_t kPageShift = 18;
inline constexpr size_t kNumClasses = 89;
inline constexpr size_t kMaxSize = 256 * 1024;
inline constexpr size_t kMinThreadCacheSize = kMaxSize * 2;
inline constexpr size_t kMaxThreadCacheSize = 4 << 20;
inline constexpr size_t kMaxCpuCacheSize = 3 * 1024 * 1024;
inline constexpr size_t kDefaultOverallThreadCacheSize =
    8u * kMaxThreadCacheSize;
inline constexpr size_t kStealAmount = 1 << 16;
inline constexpr size_t kDefaultProfileSamplingRate = 1 << 21;
inline constexpr size_t kMinPages = 8;
#elif TCMALLOC_PAGE_SHIFT == 13
inline constexpr size_t kPageShift = 13;  //用来计算 page 大小的， page 大小 1 << 13 = 8KB
inline constexpr size_t kNumClasses = 86;
inline constexpr size_t kMaxSize = 256 * 1024;
inline constexpr size_t kMinThreadCacheSize = kMaxSize * 2;
inline constexpr size_t kMaxThreadCacheSize = 4 << 20;
inline constexpr size_t kMaxCpuCacheSize = 3 * 1024 * 1024;
inline constexpr size_t kDefaultOverallThreadCacheSize =
    8u * kMaxThreadCacheSize;
inline constexpr size_t kStealAmount = 1 << 16;
inline constexpr size_t kDefaultProfileSamplingRate = 1 << 21;
inline constexpr size_t kMinPages = 8;
#else
#error "Unsupported TCMALLOC_PAGE_SHIFT value!"
#endif

// Minimum/maximum number of batches in TransferCache per size class.
// Actual numbers depends on a number of factors, see TransferCache::Init
// for details.
inline constexpr size_t kMinObjectsToMove = 2;
inline constexpr size_t kMaxObjectsToMove = 128;

inline constexpr size_t kPageSize = 1 << kPageShift;  //page 页大小
// Verify that the page size used is at least 8x smaller than the maximum
// element size in the thread cache.  This guarantees at most 12.5% internal
// fragmentation (1/8). When page size is 256k (kPageShift == 18), the benefit
// of increasing kMaxSize to be multiple of kPageSize is unclear. Object size
// profile data indicates that the number of simultaneously live objects (of
// size >= 256k) tends to be very small. Keeping those objects as 'large'
// objects won't cause too much memory waste, while heap memory reuse is can be
// improved. Increasing kMaxSize to be too large has another bad side effect --
// the thread cache pressure is increased, which will in turn increase traffic
// between central cache and thread cache, leading to performance degradation.
static_assert((kMaxSize / kPageSize) >= kMinPages || kPageShift >= 18,
              "Ratio of kMaxSize / kPageSize is too small");

inline constexpr size_t kAlignment = 8;
// log2 (kAlignment)
inline constexpr size_t kAlignmentShift =
    tcmalloc::tcmalloc_internal::Bits::Log2Ceiling(kAlignment);

// The number of times that a deallocation can cause a freelist to
// go over its max_length() before shrinking max_length().
inline constexpr int kMaxOverages = 3;

// Maximum length we allow a per-thread free-list to have before we
// move objects from it into the corresponding central free-list.  We
// want this big to avoid locking the central free-list too often.  It
// should not hurt to make this list somewhat big because the
// scavenging code will shrink it down when its contents are not in use.
inline constexpr int kMaxDynamicFreeListLength = 8192;

#if defined __x86_64__
// All current and planned x86_64 processors only look at the lower 48 bits
// in virtual to physical address translation.  The top 16 are thus unused.
// TODO(b/134686025): Under what operating systems can we increase it safely to
// 17? This lets us use smaller page maps.  On first allocation, a 36-bit page
// map uses only 96 KB instead of the 4.5 MB used by a 52-bit page map.
inline constexpr int kAddressBits =
    (sizeof(void*) < 8 ? (8 * sizeof(void*)) : 48);
#elif defined __powerpc64__ && defined __linux__
// Linux(4.12 and above) on powerpc64 supports 128TB user virtual address space
// by default, and up to 512TB if user space opts in by specifing hint in mmap.
// See comments in arch/powerpc/include/asm/processor.h
// and arch/powerpc/mm/mmap.c.
inline constexpr int kAddressBits =
    (sizeof(void*) < 8 ? (8 * sizeof(void*)) : 49);
#elif defined __aarch64__ && defined __linux__
// According to Documentation/arm64/memory.txt of kernel 3.16,
// AARCH64 kernel supports 48-bit virtual addresses for both user and kernel.
inline constexpr int kAddressBits =
    (sizeof(void*) < 8 ? (8 * sizeof(void*)) : 48);
#else
inline constexpr int kAddressBits = 8 * sizeof(void*);
#endif

namespace tcmalloc {
inline constexpr uintptr_t kTagMask = uintptr_t{1}
                                      << std::min(kAddressBits - 4, 42);

#if !defined(TCMALLOC_SMALL_BUT_SLOW) && __WORDSIZE != 32
// Always allocate at least a huge page
inline constexpr size_t kMinSystemAlloc = kHugePageSize;
inline constexpr size_t kMinMmapAlloc = 1 << 30;  // mmap() in 1GiB ranges.
#else
// Allocate in units of 2MiB. This is the size of a huge page for x86, but
// not for Power.
inline constexpr size_t kMinSystemAlloc = 2 << 20;
// mmap() in units of 32MiB. This is a multiple of huge page size for
// both x86 (2MiB) and Power (16MiB)
inline constexpr size_t kMinMmapAlloc = 32 << 20;
#endif

static_assert(kMinMmapAlloc % kMinSystemAlloc == 0,
              "Minimum mmap allocation size is not a multiple of"
              " minimum system allocation size");

// Returns true if ptr is tagged.
inline bool IsTaggedMemory(const void* ptr) {
  return (reinterpret_cast<uintptr_t>(ptr) & kTagMask) == 0;
}

// Size-class information + mapping
class SizeMap {
 public:
  // All size classes <= 512 in all configs always have 1 page spans.
  static constexpr size_t kMultiPageSize = 512;
  // Min alignment for all size classes > kMultiPageSize in all configs.
  static constexpr size_t kMultiPageAlignment = 64;
  // log2 (kMultiPageAlignment)
  static constexpr size_t kMultiPageAlignmentShift =
      tcmalloc::tcmalloc_internal::Bits::Log2Ceiling(kMultiPageAlignment);

 private:
  //-------------------------------------------------------------------
  // Mapping from size to size_class and vice versa
  //-------------------------------------------------------------------

  // Sizes <= 1024 have an alignment >= 8.  So for such sizes we have an
  // array indexed by ceil(size/8).  Sizes > 1024 have an alignment >= 128.
  // So for these larger sizes we have an array indexed by ceil(size/128).
  //
  // We flatten both logical arrays into one physical array and use
  // arithmetic to compute an appropriate index.  The constants used by
  // ClassIndex() were selected to make the flattening work.
  //
  // Examples:
  //   Size       Expression                      Index
  //   -------------------------------------------------------
  //   0          (0 + 7) / 8                     0
  //   1          (1 + 7) / 8                     1
  //   ...
  //   1024       (1024 + 7) / 8                  128
  //   1025       (1025 + 127 + (120<<7)) / 128   129
  //   ...
  //   32768      (32768 + 127 + (120<<7)) / 128  376
  //   ...
  //   256*1024   (256*1024 + 127 + (120<<7)) / 128  2168
  static constexpr int kMaxSmallSize = 1024;
  static constexpr size_t kClassArraySize =
      ((kMaxSize + 127 + (120 << 7)) >> 7) + 1;

  //0 - 256K ,范围类的size 输入小内存， 小内存首先会按某字节向上对齐，
  //tcmalloc 对于 0-256K 范围内 256K + 1 个数字， 对齐后只有 86 种 size（kNumClasses），实际上这 86 种 size 是一个数组，且通过数组索引就可以算出相应的值。
  //那么 最简单的方法是用 256K+1 这么大的 char 数组来保存相应索引。
  //但这么做会存在大量冗余信息， 如 1-8 都要重复映射到8，
  //为了节省这个数组的内存， 就有了上面的算法。
  //0-1024 这个范围内的 数字， 都是至少8字节对齐（这个要看对齐的算法），那么从1 开始每8个数字，都会对齐到同一个值， 即映射到同一个索引。
  //1025-256K 这个范围至少是128对齐（这个要看对齐的算法），那么从1025开始每128个数字，都会对齐到同一个值， 映射一个索引即可
  //这样用 2169 个 char的数组就可以了， 且计算的也方法简单。
  //只需要将 size 通过上面描述的算法计算即可得到一个索引，这个索引就可以在另外一个数组中取出这个 size 被对齐后的大小。 


  // Batch size is the number of objects to move at once.
  typedef unsigned char BatchSize;

  // class_array_ is accessed on every malloc, so is very hot.  We make it the
  // first member so that it inherits the overall alignment of a SizeMap
  // instance.  In particular, if we create a SizeMap instance that's cache-line
  // aligned, this member is also aligned to the width of a cache line.
  // class_array_ 会被 malloc 调用直接访问， 所以它会被频繁的使用。将它做为类的第一个成员，这样它可以继承类本身的内存对齐。
  unsigned char class_array_[kClassArraySize]; // 这个数组用来保存 class_to_size_ 的索引， kClassArraySize 可以通过 size 计算得来

  // Number of objects to move between a per-thread list and a central
  // list in one shot.  We want this to be not too small so we can
  // amortize the lock overhead for accessing the central list.  Making
  // it too big may temporarily cause unnecessary memory wastage in the
  // per-thread free list until the scavenger cleans up the list.
  BatchSize num_objects_to_move_[kNumClasses];

  // If size is no more than kMaxSize, compute index of the
  // class_array[] entry for it, putting the class index in output
  // parameter idx and returning true. Otherwise return false.
  // 计算 s 大小， 映射到 class_array_ 数组元素的索引
  static inline bool ABSL_ATTRIBUTE_ALWAYS_INLINE ClassIndexMaybe(size_t s,
                                                                  uint32_t* idx) {
  // 0 - 256K 都属于小内存， 这个方法是计算小内存的映射索引， 即在 class_array_ 中的索引，
  // class_array_[index] 通用指向一个索引，是 class_to_size_ 的索引， class_to_size_[index] 即是 这个size 对齐后的大小
    if (ABSL_PREDICT_TRUE(s <= kMaxSmallSize)) {
      *idx = (static_cast<uint32_t>(s) + 7) >> 3;// 计算 
      return true;
    } else if (s <= kMaxSize) {
      *idx = (static_cast<uint32_t>(s) + 127 + (120 << 7)) >> 7;
      return true;
    }
    //不是小内存，字节返回false
    return false;
  }

  static inline size_t ClassIndex(size_t s) {
    uint32_t ret;
    CHECK_CONDITION(ClassIndexMaybe(s, &ret));
    return ret;
  }

  // Mapping from size class to number of pages to allocate at a time
  unsigned char class_to_pages_[kNumClasses];

  // Mapping from size class to max size storable in that class
  uint32_t class_to_size_[kNumClasses]; //这个数组保存 所有小内存 0 - 256K ，对齐后的 size 

  // If environment variable defined, use it to override sizes classes.
  // Returns true if all classes defined correctly.
  bool MaybeRunTimeSizeClasses();

 protected:
  // Set the give size classes to be used by TCMalloc.
  void SetSizeClasses(int num_classes, const SizeClassInfo* parsed);

  // Check that the size classes meet all requirements.
  bool ValidSizeClasses(int num_classes, const SizeClassInfo* parsed);

  // Definition of size class that is set in size_classes.cc
  static const SizeClassInfo kSizeClasses[kNumClasses];

  // Definition of size class that is set in size_classes.cc
  static const SizeClassInfo kExperimentalSizeClasses[kNumClasses];

  // Definition of size class that is set in size_classes.cc
  static const SizeClassInfo kExperimental4kSizeClasses[kNumClasses];

 public:
  // Constructor should do nothing since we rely on explicit Init()
  // call, which may or may not be called before the constructor runs.
  SizeMap() { }

  // Initialize the mapping arrays
  void Init();

  // Returns the non-zero matching size class for the provided `size`.
  // Returns true on success, returns false if `size` exceeds the maximum size
  // class value `kMaxSize'.
  // Important: this function may return true with *cl == 0 if this
  // SizeMap instance has not (yet) been initialized.
  // 这个方法可能返回true， 且 *cl == 0, 这代表这个SizeMap对象还没初始化， class_to_size_[0] 实际等于 0
  //
  // 获取小内存 size， 对齐后的大小在 class_to_size_ 中的索引，保存在 cl中。如果超出小内存范围0-256K,返回 false, 否则返回true
  inline bool ABSL_ATTRIBUTE_ALWAYS_INLINE GetSizeClass(size_t size,
                                                        uint32_t* cl) {
    uint32_t idx;
    if (ABSL_PREDICT_TRUE(ClassIndexMaybe(size, &idx))) {
      *cl = class_array_[idx];
      return true;
    }
    return false;
  }

  // Returns the size class for size `size` aligned at `align`
  // Returns true on success. Returns false if either:
  // - the size exceeds the maximum size class size.
  // - the align size is greater or equal to the default page size
  // - no matching properly aligned size class is available
  // 返回参数size 经对齐后的大小
  // 成功返回true， 失败有以下原因
  // - size 超过了小内存的范围
  // - align 比page大或等于page
  // - 没有合适的对齐size 可用 
  //
  // Requires that align is a non-zero power of 2.要求 align 必须是 2的非零次方
  //
  // Specifying align = 1 will result in this method using the default
  // alignment of the size table. Calling this method with a constexpr
  // value of align = 1 will be optimized by the compiler, and result in
  // the inlined code to be identical to calling `GetSizeClass(size, cl)`
  // 指定 align = 1将导致， 对齐方式为 tcmalloc 内部定义的对齐方式。
  // 如果 的调用 这个方法，使用常量1，编译其会优化成 直接调用 GetSizeClass(size, cl)
  inline bool ABSL_ATTRIBUTE_ALWAYS_INLINE GetSizeClass(size_t size,
                                                        size_t align,
                                                        uint32_t* cl) {
    ASSERT(align > 0);
    ASSERT((align & (align - 1)) == 0);//align 必须是2的次方

    if (ABSL_PREDICT_FALSE(align >= kPageSize)) { //对齐不能等于或超过 page 大小
      return false;
    }
    if (ABSL_PREDICT_FALSE(!GetSizeClass(size, cl))) {
      return false; //不是小内存返回 false
    }

    // Predict that size aligned allocs most often directly map to a proper
    // size class, i.e., multiples of 32, 64, etc, matching our class sizes.
    const size_t mask = (align - 1);
    do {
      if (ABSL_PREDICT_TRUE((class_to_size(*cl) & mask) == 0)) { // 判断当前 cl 索引对应的size 是否满足对齐要求
        return true;
      }
    } while (++*cl < kNumClasses);//不满足 尝试下一个索引的 size

    return false;
  }

  // Returns size class for given size, or 0 if this instance has not been
  // initialized yet. REQUIRES: size <= kMaxSize.
  // 返回 size 对应 class_to_size_ 数组元素的索引
  inline size_t ABSL_ATTRIBUTE_ALWAYS_INLINE SizeClass(size_t size) {
    ASSERT(size <= kMaxSize);
    uint32_t ret = 0;
    GetSizeClass(size, &ret);
    return ret;
  }

  // Get the byte-size for a specified class. REQUIRES: cl <= kNumClasses.
  // 获取 class_to_size_ 元素， class_to_size_[cl] 即对齐后的大小
  inline size_t ABSL_ATTRIBUTE_ALWAYS_INLINE class_to_size(size_t cl) {
    ASSERT(cl < kNumClasses);
    return class_to_size_[cl];
  }

  // Mapping from size class to number of pages to allocate at a time
  inline size_t class_to_pages(size_t cl) {
    ASSERT(cl < kNumClasses);
    return class_to_pages_[cl];
  }

  // Number of objects to move between a per-thread list and a central
  // list in one shot.  We want this to be not too small so we can
  // amortize the lock overhead for accessing the central list.  Making
  // it too big may temporarily cause unnecessary memory wastage in the
  // per-thread free list until the scavenger cleans up the list.
  inline SizeMap::BatchSize num_objects_to_move(size_t cl) {
    ASSERT(cl < kNumClasses);
    return num_objects_to_move_[cl];
  }
};

// Linker initialized, so this lock can be accessed at any time.
extern absl::base_internal::SpinLock pageheap_lock;

}  // namespace tcmalloc

#endif  // TCMALLOC_COMMON_H_
