#include "stdio.h"
#include "MySynthesizer.hpp"

MySynthesizer mySynth;

inline void Greet(){
    printf("hello i'm c++");
    mySynth.noteOn(60, 0.5);
}
