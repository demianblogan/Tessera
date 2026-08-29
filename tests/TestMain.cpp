// The one translation unit that compiles doctest's implementation and provides
// main(). Every other *Tests.cpp file just includes the header.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
