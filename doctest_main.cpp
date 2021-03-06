#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <iostream>
#include "doctest.h"

using doctest::ConsoleReporter;
using doctest::SubcaseSignature;
using doctest::TestCaseData;
namespace Color = doctest::Color;

struct VerboseConsoleReporter : public ConsoleReporter {
    using ConsoleReporter::ConsoleReporter;

    void test_case_start(const TestCaseData &in) override {
        ConsoleReporter::test_case_start(in);
        s << Color::None << "TEST_CASE(" << in.m_name << ")\n";
        s.flush();
    }

    void subcase_start(const SubcaseSignature &subc) override {
        ConsoleReporter::subcase_start(subc);
        s << Color::None << "    SUBCASE(" << subc.m_name << ")\n";
        s.flush();
    }
};

REGISTER_REPORTER("verbose", 1, VerboseConsoleReporter);
