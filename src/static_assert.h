/**
 * @file static_assert.h
 * @brief boost的static_assert 实现for gcc 4.0+
 * @author litang
 * @version 1.0
 * @date 2016-05-13
 */
#ifndef __STATIC_ASSERT_H
#define __STATIC_ASSERT_H

template <bool x> struct STATIC_ASSERTION_FAILURE;
template <> struct STATIC_ASSERTION_FAILURE<true> { enum { value = 1 }; };
template<int x> struct static_assert_test{};
#define MY_STATIC_ASSERT_UNUSED_ATTRIBUTE __attribute__((unused))
#define MY_STATIC_ASSERT_BOOL_CAST( ... ) ((__VA_ARGS__) == 0 ? false : true)
#define MY_STATIC_ASSERT( B ) \
  typedef static_assert_test<\
  sizeof(STATIC_ASSERTION_FAILURE< MY_STATIC_ASSERT_BOOL_CAST( B ) >)>\
  my_static_assert_typedef_##__LINE__ MY_STATIC_ASSERT_UNUSED_ATTRIBUTE

#endif // __STATIC_ASSERT_H
