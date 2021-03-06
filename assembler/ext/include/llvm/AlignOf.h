//===--- AlignOf.h - Portable calculation of type alignment -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the AlignOf function that computes alignments for
// arbitrary types.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SUPPORT_ALIGNOF_H
#define LLVM_SUPPORT_ALIGNOF_H

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace llvm {

namespace detail {

// For everything other than an abstract class we can calulate alignment by
// building a class with a single character and a member of the given type.
template <typename T, bool = std::is_abstract<T>::value>
struct AlignmentCalcImpl {
  char x;
  T t;
private:
  AlignmentCalcImpl() {} // Never instantiate.
};

// Abstract base class helper, this will have the minimal alignment and size
// for any abstract class. We don't even define its destructor because this
// type should never be used in a way that requires it.
struct AlignmentCalcImplBase {
  virtual ~AlignmentCalcImplBase() = 0;
};

// When we have an abstract class type, specialize the alignment computation
// engine to create another abstract class that derives from both an empty
// abstract base class and the provided type. This has the same effect as the
// above except that it handles the fact that we can't actually create a member
// of type T.
template <typename T>
struct AlignmentCalcImpl<T, true> : AlignmentCalcImplBase, T {
  virtual ~AlignmentCalcImpl() = 0;
};

} // End detail namespace.

/// AlignOf - A templated class that contains an enum value representing
///  the alignment of the template argument.  For example,
///  AlignOf<int>::Alignment represents the alignment of type "int".  The
///  alignment calculated is the minimum alignment, and not necessarily
///  the "desired" alignment returned by GCC's __alignof__ (for example).  Note
///  that because the alignment is an enum value, it can be used as a
///  compile-time constant (e.g., for template instantiation).
template <typename T>
struct AlignOf {
  // Avoid warnings from GCC like:
  //   comparison between 'enum llvm::AlignOf<X>::<anonymous>' and 'enum
  //   llvm::AlignOf<Y>::<anonymous>' [-Wenum-compare]
  // by using constexpr instead of enum.
  // (except on MSVC, since it doesn't support constexpr yet).
  static constexpr unsigned Alignment = static_cast<unsigned int>(
      sizeof(detail::AlignmentCalcImpl<T>) - sizeof(T));
  enum { Alignment_GreaterEqual_2Bytes = Alignment >= 2 ? 1 : 0 };
  enum { Alignment_GreaterEqual_4Bytes = Alignment >= 4 ? 1 : 0 };
  enum { Alignment_GreaterEqual_8Bytes = Alignment >= 8 ? 1 : 0 };
  enum { Alignment_GreaterEqual_16Bytes = Alignment >= 16 ? 1 : 0 };

  enum { Alignment_LessEqual_2Bytes = Alignment <= 2 ? 1 : 0 };
  enum { Alignment_LessEqual_4Bytes = Alignment <= 4 ? 1 : 0 };
  enum { Alignment_LessEqual_8Bytes = Alignment <= 8 ? 1 : 0 };
  enum { Alignment_LessEqual_16Bytes = Alignment <= 16 ? 1 : 0 };
};

template <typename T> constexpr unsigned AlignOf<T>::Alignment;

/// alignOf - A templated function that returns the minimum alignment of
///  of a type.  This provides no extra functionality beyond the AlignOf
///  class besides some cosmetic cleanliness.  Example usage:
///  alignOf<int>() returns the alignment of an int.
template <typename T>
inline unsigned alignOf() { return AlignOf<T>::Alignment; }

/// \struct AlignedCharArray
/// \brief Helper for building an aligned character array type.
///
/// This template is used to explicitly build up a collection of aligned
/// character array types. We have to build these up using a macro and explicit
/// specialization to cope with old versions of MSVC and GCC where only an
/// integer literal can be used to specify an alignment constraint. Once built
/// up here, we can then begin to indirect between these using normal C++
/// template parameters.

#ifndef __has_feature
# define __has_feature(x) 0
#endif

#ifndef __has_extension
# define __has_extension(x) 0
#endif

#ifndef __has_attribute
# define __has_attribute(x) 0
#endif

#ifndef __has_builtin
# define __has_builtin(x) 0
#endif

#if __has_feature(cxx_alignas)
template<std::size_t Alignment, std::size_t Size>
struct AlignedCharArray {
  alignas(Alignment) char buffer[Size];
};

#elif defined(__GNUC__) || defined(__IBM_ATTRIBUTES)
/// \brief Create a type with an aligned char buffer.
template<std::size_t Alignment, std::size_t Size>
struct AlignedCharArray;

#define LLVM_ALIGNEDCHARARRAY_TEMPLATE_ALIGNMENT(x) \
  template<std::size_t Size> \
  struct AlignedCharArray<x, Size> { \
    __attribute__((aligned(x))) char buffer[Size]; \
  };

LLVM_ALIGNEDCHARARRAY_TEMPLATE_ALIGNMENT(1)
LLVM_ALIGNEDCHARARRAY_TEMPLATE_ALIGNMENT(2)
LLVM_ALIGNEDCHARARRAY_TEMPLATE_ALIGNMENT(4)
LLVM_ALIGNEDCHARARRAY_TEMPLATE_ALIGNMENT(8)
LLVM_ALIGNEDCHARARRAY_TEMPLATE_ALIGNMENT(16)
LLVM_ALIGNEDCHARARRAY_TEMPLATE_ALIGNMENT(32)
LLVM_ALIGNEDCHARARRAY_TEMPLATE_ALIGNMENT(64)
LLVM_ALIGNEDCHARARRAY_TEMPLATE_ALIGNMENT(128)

#undef LLVM_ALIGNEDCHARARRAY_TEMPLATE_ALIGNMENT

#else
# error No supported align as directive.
#endif

namespace detail {
template <typename T1,
          typename T2 = char, typename T3 = char, typename T4 = char,
          typename T5 = char, typename T6 = char, typename T7 = char,
          typename T8 = char, typename T9 = char, typename T10 = char>
class AlignerImpl {
  T1 t1; T2 t2; T3 t3; T4 t4; T5 t5; T6 t6; T7 t7; T8 t8; T9 t9; T10 t10;

  AlignerImpl(); // Never defined or instantiated.
};

template <typename T1,
          typename T2 = char, typename T3 = char, typename T4 = char,
          typename T5 = char, typename T6 = char, typename T7 = char,
          typename T8 = char, typename T9 = char, typename T10 = char>
union SizerImpl {
  char arr1[sizeof(T1)], arr2[sizeof(T2)], arr3[sizeof(T3)], arr4[sizeof(T4)],
       arr5[sizeof(T5)], arr6[sizeof(T6)], arr7[sizeof(T7)], arr8[sizeof(T8)],
       arr9[sizeof(T9)], arr10[sizeof(T10)];
};
} // end namespace detail

/// \brief This union template exposes a suitably aligned and sized character
/// array member which can hold elements of any of up to ten types.
///
/// These types may be arrays, structs, or any other types. The goal is to
/// expose a char array buffer member which can be used as suitable storage for
/// a placement new of any of these types. Support for more than ten types can
/// be added at the cost of more boilerplate.
template <typename T1,
          typename T2 = char, typename T3 = char, typename T4 = char,
          typename T5 = char, typename T6 = char, typename T7 = char,
          typename T8 = char, typename T9 = char, typename T10 = char>
struct AlignedCharArrayUnion : llvm::AlignedCharArray<
    AlignOf<detail::AlignerImpl<T1, T2, T3, T4, T5,
                                T6, T7, T8, T9, T10> >::Alignment,
    sizeof(detail::SizerImpl<T1, T2, T3, T4, T5,
                             T6, T7, T8, T9, T10>)> {
};
} // end namespace llvm
#endif
