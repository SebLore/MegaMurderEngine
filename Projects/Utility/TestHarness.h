// TestHarness.h
#pragma once

#include <cstddef>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "include/ConsoleManip.h"

namespace TestHarness
{
    struct Stats
    {
        int passed = 0;
        int failed = 0;

        int Total() const { return passed + failed; }
    };

    struct Reporter
    {
        bool verboseChecks = false; // set true to print every CHECK

        void SuiteBegin(std::string_view name)
        {
            ColorGuard g(ConsoleColor::CYAN);
            std::cout << "\n== " << name << " ==\n";
        }

        void SuiteEnd(std::string_view name, const Stats& s)
        {
            if (s.failed == 0)
            {
                ColorGuard g(ConsoleColor::GREEN);
                std::cout << "-- " << name << " : PASS (" << s.passed << " checks)\n";
            }
            else
            {
                ColorGuard g(ConsoleColor::RED);
                std::cout << "-- " << name << " : FAIL (" << s.failed << " failed, " << s.passed << " passed)\n";
            }
        }

        void CaseBegin(std::string_view name)
        {
            ColorGuard g(ConsoleColor::WHITE);
            std::cout << "\n  [CASE] " << name << "\n";
        }

        void CheckPass(std::string_view expr) const
        {
            if (!verboseChecks)
                return;
            ColorGuard g(ConsoleColor::GREEN);
            std::cout << "    PASS: " << expr << "\n";
        }

        void CheckFail(std::string_view expr, const char* file, int line)
        {
            ColorGuard g(ConsoleColor::RED);
            std::cout << "    FAIL: " << expr << "\n";
            ColorGuard g2(ConsoleColor::DEFAULT);
            std::cout << "          at " << file << ":" << line << "\n";
        }

        void Info(std::string_view msg)
        {
            ColorGuard g(ConsoleColor::DEFAULT);
            std::cout << "    " << msg << "\n";
        }
    };

    struct Context
    {
        Reporter* reporter = nullptr;
        Stats     stats{};

        void Pass(std::string_view expr)
        {
            ++stats.passed;
            if (reporter)
                reporter->CheckPass(expr);
        }

        void Fail(std::string_view expr, const char* file, int line)
        {
            ++stats.failed;
            if (reporter)
                reporter->CheckFail(expr, file, line);
        }

        void Info(std::string_view msg)
        {
            if (reporter)
                reporter->Info(msg);
        }
    };

    using CaseFn = std::function<void(Context&)>;

    struct Case
    {
        std::string name;
        CaseFn      fn;
    };

    inline bool EndsWith(const std::string& s, const std::string& suffix)
    {
        if (suffix.size() > s.size())
            return false;
        return s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    class Suite
    {
      public:
        explicit Suite(std::string name) : m_name(std::move(name)) {}

        Suite& Add(std::string caseName, CaseFn fn)
        {
            m_cases.push_back(Case{ .name = std::move(caseName), .fn = std::move(fn) });
            return *this;
        }

        Stats Run(Reporter& r) const
        {
            r.SuiteBegin(m_name);

            Stats total{};
            for (auto& c : m_cases)
            {
                r.CaseBegin(c.name);
                Context ctx{ &r, {} };
                try
                {
                    c.fn(ctx);
                }
                catch (const std::exception& e)
                {
                    ctx.Fail("Unhandled std::exception", __FILE__, __LINE__);
                    ctx.Info(e.what());
                }
                catch (...)
                {
                    ctx.Fail("Unhandled non-std exception", __FILE__, __LINE__);
                }

                total.passed += ctx.stats.passed;
                total.failed += ctx.stats.failed;
            }

            r.SuiteEnd(m_name, total);
            return total;
        }

      private:
        std::string       m_name;
        std::vector<Case> m_cases;
    };

} // namespace TestHarness

// ---- Macros ----
// Keep these in header for convenient file/line capture.
#define TH_CHECK(ctx, expr)                                                                                            \
    do                                                                                                                 \
    {                                                                                                                  \
        if (expr)                                                                                                      \
            (ctx).Pass(#expr);                                                                                         \
        else                                                                                                           \
            (ctx).Fail(#expr, __FILE__, __LINE__);                                                                     \
    } while (0)

#define TH_CHECK_EQ(ctx, a, b) TH_CHECK(ctx, ((a) == (b)))
#define TH_CHECK_NE(ctx, a, b) TH_CHECK(ctx, ((a) != (b)))

#define TH_CHECK_THROWS_ANY(ctx, stmt)                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        bool threw = false;                                                                                            \
        try                                                                                                            \
        {                                                                                                              \
            (void)(stmt);                                                                                              \
        }                                                                                                              \
        catch (...)                                                                                                    \
        {                                                                                                              \
            threw = true;                                                                                              \
        }                                                                                                              \
        TH_CHECK(ctx, threw);                                                                                          \
    } while (0)