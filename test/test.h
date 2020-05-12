#ifndef TEST_H_
#define TEST_H_

#include <cstdio>
#include <cstdlib>

#define TO_STRING(x) #x
#define TEST(x) if (!(x)) { printf("Assertion failed: (%s) in %s@%d.\n", TO_STRING(x), __FILE__, __LINE__); abort(); }

#endif // TEST_H_
