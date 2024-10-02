#pragma once

#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace TestRunnerPrivate 
{
    template <typename K, typename V, template <typename, typename> class Map>
    std::ostream& PrintMap(std::ostream& os_, const Map<K, V>& m_) 
    {
        os_ << "{";
        bool first = true;

        for (const auto& kv : m_) 
        {
            if (!first) 
            {
                os_ << ", ";
            }
            first = false;
            os_ << kv.first << ": " << kv.second;
        }
        return os_ << "}";
    }
}

template <class T>
std::ostream& operator<<(std::ostream& os_, const std::vector<T>& s_) 
{
    os_ << "{";
    bool first = true;

    for (const auto& x : s_) 
    {
        if (!first) 
        {
            os_ << ", ";
        }
        first = false;
        os_ << x;
    }
    return os_ << "}";
}

template <class T>
std::ostream& operator<<(std::ostream& os_, const std::set<T>& s_) 
{
    os_ << "{";
    bool first = true;

    for (const auto& x : s_) 
    {
        if (!first) 
        {
            os_ << ", ";
        }
        first = false;
        os_ << x;
    }
    return os_ << "}";
}

template <class K, class V>
std::ostream& operator<<(std::ostream& os_, const std::map<K, V>& m_) 
{
    return TestRunnerPrivate::PrintMap(os_, m_);
}

template <class K, class V>
std::ostream& operator<<(std::ostream& os_, const std::unordered_map<K, V>& m_) 
{
    return TestRunnerPrivate::PrintMap(os_, m_);
}

template <class T, class U>
void AssertEqual(const T& t_, const U& u_, const std::string& hint_ = {}) 
{
    if (!(t_ == u_)) 
    {
        std::ostringstream os;
        os << "Assertion failed: " << t_ << " != " << u_;

        if (!hint_.empty()) 
        {
            os << " hint: " << hint_;
        }
        throw std::runtime_error(os.str());
    }
}

inline void Assert(bool b_, const std::string& hint_) 
{
    AssertEqual(b_, true, hint_);
}

class TestRunner 
{
public:

    template <class TestFunc>
    void RunTest(TestFunc func_, const std::string& test_name_)
    {
        try 
        {
            func_();
            std::cerr << test_name_ << " OK" << std::endl;
        }
        catch (std::exception& e_) 
        {
            ++fail_count;
            std::cerr << test_name_ << " fail: " << e_.what() << std::endl;
        }
        catch (...) 
        {
            ++fail_count;
            std::cerr << "Unknown exception caught" << std::endl;
        }
    }

    ~TestRunner() 
    {
        std::cerr.flush();
        if (fail_count > 0) 
        {
            std::cerr << fail_count << " unit tests failed. Terminate" << std::endl;
            exit(1);
        }
    }

private:
    int fail_count = 0;
};

#ifndef FILE_NAME
#define FILE_NAME __FILE__
#endif

#define ASSERT_EQUAL(x, y)                                               \
  {                                                                      \
    std::ostringstream __assert_equal_private_os;                        \
    __assert_equal_private_os << #x << " != " << #y << ", " << FILE_NAME \
                              << ":" << __LINE__;                        \
    AssertEqual(x, y, __assert_equal_private_os.str());                  \
  }

#define ASSERT(x)                                                  \
  {                                                                \
    std::ostringstream __assert_private_os;                        \
    __assert_private_os << #x << " is false, " << FILE_NAME << ":" \
                        << __LINE__;                               \
    Assert(x, __assert_private_os.str());                          \
  }

#define RUN_TEST(tr, func) tr.RunTest(func, #func)