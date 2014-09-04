// Copyright 2010 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// This file is an internal atomic implementation, use atomicops.h instead.
//

#ifndef V8_ATOMICOPS_INTERNALS_PPC_H_
#define V8_ATOMICOPS_INTERNALS_PPC_H_

namespace v8 {
namespace internal {

inline Atomic32 NoBarrier_CompareAndSwap(volatile Atomic32* ptr,
                                         Atomic32 old_value,
                                         Atomic32 new_value) {
  return(__sync_val_compare_and_swap( ptr, old_value, new_value));
}

inline Atomic32 NoBarrier_AtomicExchange(volatile Atomic32* ptr,
                                         Atomic32 new_value) {
  Atomic32 old_value;
  do {
    old_value = *ptr;
  } while (__sync_bool_compare_and_swap(ptr, old_value, new_value) == false);
  return old_value;
}

inline Atomic32 NoBarrier_AtomicIncrement(volatile Atomic32* ptr,
                                          Atomic32 increment) {
  return Barrier_AtomicIncrement(ptr, increment);
}

inline Atomic32 Barrier_AtomicIncrement(volatile Atomic32* ptr,
                                        Atomic32 increment) {
  for (;;) {
    Atomic32 old_value = *ptr;
    Atomic32 new_value = old_value + increment;
    if (__sync_bool_compare_and_swap(ptr, old_value, new_value)) {
      return new_value;
      // The exchange took place as expected.
    }
    // Otherwise, *ptr changed mid-loop and we need to retry.
  }
}

inline Atomic32 Acquire_CompareAndSwap(volatile Atomic32* ptr,
                                       Atomic32 old_value,
                                       Atomic32 new_value) {
  return NoBarrier_CompareAndSwap(ptr, old_value, new_value);
}

inline Atomic32 Release_CompareAndSwap(volatile Atomic32* ptr,
                                       Atomic32 old_value,
                                       Atomic32 new_value) {
  return NoBarrier_CompareAndSwap(ptr, old_value, new_value);
}

inline void NoBarrier_Store(volatile Atomic8* ptr, Atomic8 value) {
  *ptr = value;
}

inline void NoBarrier_Store(volatile Atomic32* ptr, Atomic32 value) {
  *ptr = value;
}

inline void MemoryBarrier() {
  __asm__ __volatile__("sync" : : : "memory");
}

inline void Acquire_Store(volatile Atomic32* ptr, Atomic32 value) {
  *ptr = value;
  MemoryBarrier();
}

inline void Release_Store(volatile Atomic32* ptr, Atomic32 value) {
  MemoryBarrier();
  *ptr = value;
}

inline Atomic8 NoBarrier_Load(volatile const Atomic8* ptr) {
  return *ptr;
}

inline Atomic32 NoBarrier_Load(volatile const Atomic32* ptr) {
  return *ptr;
}

inline Atomic32 Acquire_Load(volatile const Atomic32* ptr) {
  Atomic32 value = *ptr;
  MemoryBarrier();
  return value;
}

inline Atomic32 Release_Load(volatile const Atomic32* ptr) {
  MemoryBarrier();
  return *ptr;
}

#ifdef V8_TARGET_ARCH_PPC64
inline Atomic64 NoBarrier_CompareAndSwap(volatile Atomic64* ptr,
                                         Atomic64 old_value,
                                         Atomic64 new_value) {
  return(__sync_val_compare_and_swap( ptr, old_value, new_value));
}

inline Atomic64 NoBarrier_AtomicExchange(volatile Atomic64* ptr,
                                         Atomic64 new_value) {
  Atomic64 old_value;
  do {
    old_value = *ptr;
  } while (__sync_bool_compare_and_swap(ptr, old_value, new_value) == false);
  return old_value;
}

inline Atomic64 NoBarrier_AtomicIncrement(volatile Atomic64* ptr,
                                          Atomic64 increment) {
  return Barrier_AtomicIncrement(ptr, increment);
}

inline Atomic64 Barrier_AtomicIncrement(volatile Atomic64* ptr,
                                        Atomic64 increment) {
  for (;;) {
    Atomic64 old_value = *ptr;
    Atomic64 new_value = old_value + increment;
    if (__sync_bool_compare_and_swap(ptr, old_value, new_value)) {
      return new_value;
      // The exchange took place as expected.
    }
    // Otherwise, *ptr changed mid-loop and we need to retry.
  }
}

inline Atomic64 Acquire_CompareAndSwap(volatile Atomic64* ptr,
                                       Atomic64 old_value,
                                       Atomic64 new_value) {
  return NoBarrier_CompareAndSwap(ptr, old_value, new_value);
}

inline Atomic64 Release_CompareAndSwap(volatile Atomic64* ptr,
                                       Atomic64 old_value,
                                       Atomic64 new_value) {
  return NoBarrier_CompareAndSwap(ptr, old_value, new_value);
}

inline void NoBarrier_Store(volatile Atomic64* ptr, Atomic64 value) {
  *ptr = value;
}

inline void Acquire_Store(volatile Atomic64* ptr, Atomic64 value) {
  *ptr = value;
  MemoryBarrier();
}

inline void Release_Store(volatile Atomic64* ptr, Atomic64 value) {
  MemoryBarrier();
  *ptr = value;
}

inline Atomic64 NoBarrier_Load(volatile const Atomic64* ptr) {
  return *ptr;
}

inline Atomic64 Acquire_Load(volatile const Atomic64* ptr) {
  Atomic64 value = *ptr;
  MemoryBarrier();
  return value;
}

inline Atomic64 Release_Load(volatile const Atomic64* ptr) {
  MemoryBarrier();
  return *ptr;
}

#endif

} }  // namespace v8::internal

#endif  // V8_ATOMICOPS_INTERNALS_PPC_GCC_H_
