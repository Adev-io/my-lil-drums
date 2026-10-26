// test.cpp: checks the core with plain asserts. Run: make test
// Author: Ari Klopfer / A_Development. MIT
#include "check.h"
#include "test_pattern.h"
#include "test_clock.h"
#include "test_wav.h"

int main()
{
    testPattern();
    testClock();
    testWav();
    std::printf ("%s: %d failure(s)\n", failures ? "FAILED" : "passed", failures);
    return failures ? 1 : 0;
}


