#pragma once

#include <functional>
#include <iosfwd>
#include <string>
#include <vector>
#include <iostream>

struct big_integer {
    big_integer();
    big_integer(big_integer const& other) = default;
    big_integer(int value);
    big_integer(unsigned int value);
    big_integer(long value);
    big_integer(unsigned long value);
    big_integer(long long value);
    big_integer(unsigned long long value);
    explicit big_integer(std::string const& str);
    ~big_integer() = default;

    big_integer& operator=(big_integer const& other) = default;

    big_integer& operator+=(big_integer const& rhs);
    big_integer& operator-=(big_integer const& rhs);
    big_integer& operator*=(big_integer const& rhs);
    big_integer& operator/=(big_integer const& rhs);
    big_integer& operator%=(big_integer const& rhs);

    big_integer& operator&=(big_integer const& rhs);
    big_integer& operator|=(big_integer const& rhs);
    big_integer& operator^=(big_integer const& rhs);

    big_integer& operator<<=(int val);
    big_integer& operator>>=(int val);

    big_integer operator+() const;
    big_integer operator-() const;
    big_integer operator~() const;

    big_integer& operator++();
    big_integer operator++(int);

    big_integer& operator--();
    big_integer operator--(int);

    friend bool operator==(big_integer const& a, big_integer const& b);
    friend bool operator!=(big_integer const& a, big_integer const& b);
    friend bool operator<(big_integer const& a, big_integer const& b);
    friend bool operator>(big_integer const& a, big_integer const& b);
    friend bool operator<=(big_integer const& a, big_integer const& b);
    friend bool operator>=(big_integer const& a, big_integer const& b);

    friend std::string to_string(big_integer const& lhs);

    big_integer abs() const;

private:
    std::vector<uint32_t> digits; // 2's implementation, sign in the last vector element
    void add(big_integer const& rhs, uint32_t carry, const std::function<uint32_t (uint32_t)>& function);
    void iterate(big_integer const& rhs, const std::function<uint32_t (uint32_t, uint32_t)>& function);
    void invert();
    void negate();
    int64_t div_big_short(uint32_t divider, bool sign_div, bool sign_mod);
    std::pair<big_integer, big_integer> div(big_integer const& rhs) const;
    uint32_t get(size_t index) const;
    size_t length() const;
    bool get_sign() const;
    void trim();
    uint64_t get_significant_digit();
    void sub_div_result(big_integer const& divider, uint32_t rest, size_t shift);
    void shift_sub(std::vector<uint32_t> const& rhs, size_t shift);
    bool shift_compare(big_integer const & rhs, size_t shift);
};

big_integer operator+(big_integer a, big_integer const& b);
big_integer operator-(big_integer a, big_integer const& b);
big_integer operator*(big_integer a, big_integer const& b);
big_integer operator/(big_integer a, big_integer const& b);
big_integer operator%(big_integer a, big_integer const& b);

big_integer operator&(big_integer a, big_integer const& b);
big_integer operator|(big_integer a, big_integer const& b);
big_integer operator^(big_integer a, big_integer const& b);

big_integer operator<<(big_integer a, int b);
big_integer operator>>(big_integer a, int b);

bool operator==(big_integer const& a, big_integer const& b);
bool operator!=(big_integer const& a, big_integer const& b);
bool operator<(big_integer const& a, big_integer const& b);
bool operator>(big_integer const& a, big_integer const& b);
bool operator<=(big_integer const& a, big_integer const& b);
bool operator>=(big_integer const& a, big_integer const& b);

std::string to_string(big_integer const& lhs);
std::ostream& operator<<(std::ostream& s, big_integer const& a);
