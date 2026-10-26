// test.cpp: checks the core with plain asserts. Run: make test
// Author: Ari Klopfer / A_Development. MIT
#include "check.h"
#include "test_pattern.h"
#include "test_clock.h"
#include "test_wav.h"
#include "test_voice.h"
#include "test_seq.h"
#include "test_kit.h"
#include "test_comp.h"
#include "test_filter.h"
#include "test_adsr.h"
#include "test_beats.h"

int main()
{
    testPattern();
    testClock();
    testWav();
    testVoice();
    testChoke();
    testSequencer();
    testKit();
    testComp();
    testFilter();
    testAdsr();
    testBeats();
    std::printf ("%s: %d failure(s)\n", failures ? "FAILED" : "passed", failures);
    return failures ? 1 : 0;
}





