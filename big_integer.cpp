#include "big_integer.h"
#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <iostream>
#include <array>

constexpr static const uint32_t UINT32_BITS = 32;
constexpr static const uint32_t BASE_DIVIDER = 1000000000;
constexpr static const uint32_t STRING_STEP = 9;
constexpr static const uint32_t HIGHEST_BIT = 1 << (UINT32_BITS - 1);
constexpr static const std::array<uint32_t, 9> POW = {10, 100, 1000,
                                                      10000,100000, 1000000,
                                                      10000000, 100000000, 1000000000};

uint32_t get_low(uint64_t num) {
    return static_cast<uint32_t>(num & UINT32_MAX);
}

uint32_t get_high(uint64_t num) {
    return static_cast<uint32_t>((num >> UINT32_BITS) & UINT32_MAX);
}

uint64_t set_high(uint32_t num) {
    return static_cast<uint64_t>(num) << UINT32_BITS;
}

bool get_highest_bit(uint32_t number) {
    return number & HIGHEST_BIT;
}

big_integer::big_integer() : digits(1, 0) {}

big_integer::big_integer(int value) : digits(1, static_cast<uint32_t>(value)) {}

big_integer::big_integer(unsigned int value) : digits(1, static_cast<uint32_t>(value)) {
    if (get_highest_bit(value)) {
        digits.emplace_back(0);
    }
}

big_integer::big_integer(long value) {
    digits.emplace_back(get_low(static_cast<uint64_t>(value)));
    digits.emplace_back(get_high(static_cast<uint64_t>(value)));
    trim();
}

big_integer::big_integer(unsigned long value) {
    digits.emplace_back(get_low(static_cast<uint64_t>(value)));
    digits.emplace_back(get_high(static_cast<uint64_t>(value)));
    if (get_highest_bit(digits[1])) {
        digits.emplace_back(0);
    }
    trim();
}

big_integer::big_integer(long long value) {
    digits.emplace_back(get_low(static_cast<uint64_t>(value)));
    digits.emplace_back(get_high(static_cast<uint64_t>(value)));
    trim();
}

big_integer::big_integer(unsigned long long value) {
    digits.emplace_back(get_low(static_cast<uint64_t>(value)));
    digits.emplace_back(get_high(static_cast<uint64_t>(value)));
    if (get_highest_bit(digits[1])) {
        digits.emplace_back(0);
    }
    trim();
}

big_integer::big_integer(std::string const& str) {
    if (str.empty()) {
        throw std::invalid_argument("Invalid number");
    }
    size_t start = 0;
    if (str[0] == '-') {
        ++start;
    }
    if (str.size() - start == 0) {
        throw std::invalid_argument("Invalid number");
    }
    std::string tmp;
    for (size_t i = start; i < str.size(); ++i) {
        if (!std::isdigit(static_cast<int>(str[i]))) {
            throw std::invalid_argument("Invalid number");
        }
        tmp += str[i];
        if (tmp.size() == STRING_STEP || i == str.size() - 1) {
            uint32_t pw = POW[tmp.size() - 1];
            *this *= pw;
            *this += std::stol(tmp);
            tmp.clear();
        }
    }
    if (get_highest_bit(digits.back())) {
        digits.emplace_back(0);
    }
    if (str[0] == '-' && *this != 0) {
        negate();
    }
}

void loop_with_carry(size_t n,
                     uint32_t carry,
                     std::function<uint32_t (uint32_t)> const& f,
                     std::function<size_t (uint32_t)> const& get_first,
                     std::function<size_t (uint32_t)> const& get_second,
                     std::function<void (size_t, uint32_t)> const& writer) {
    for(size_t i = 0; i < n; ++i) {
        uint64_t sum = static_cast<uint64_t>(get_first(i)) + static_cast<uint64_t>(f(get_second(i)))
                       + static_cast<uint64_t>(carry);
        writer(i, get_low(sum));
        carry = get_high(sum);
    }
}

int64_t big_integer::div_big_short(const uint32_t divider,
                                   const bool sign_div,
                                   const bool sign_mod) {
    uint64_t carry = 0;
    for (size_t i = length(); i-- > 0;) {
        uint64_t tmp = set_high(carry) | digits[i];
        if (divider != 0) {
            digits[i] = get_low(tmp / divider);
            carry = tmp % divider;
        } else {
            digits[i] = UINT32_MAX;
        }
    }
    if (sign_div) {
        negate();
    }
    trim();
    return static_cast<int64_t>(sign_mod ? -carry : carry);
}

bool big_integer::shift_compare(big_integer const & rhs, size_t shift) {
    if (rhs.length() + shift != length()) {
        return rhs.length() + shift < length();
    }
    for (size_t i = rhs.length(); i-- > 0;) {
        if (digits[i + shift] != rhs.digits[i]) {
            return digits[i + shift] > rhs.digits[i];
        }
    }
    return true;
}

void big_integer::shift_sub(std::vector<uint32_t> const& rhs, size_t shift) {
    uint64_t carry = 0;
    digits.resize(std::max(rhs.size(), digits.size()) + shift, 0);
    for (size_t i = 0; i < rhs.size(); ++i) {
        uint64_t cur = static_cast<uint64_t>(digits[i + shift]) - rhs[i] - carry;
        digits[i + shift] = static_cast<uint32_t>(cur);
        carry = get_highest_bit(get_high(cur));
    }
    for (size_t i = rhs.size() + shift; i < length() && carry; ++i) {
        carry = (digits[i]-- == 0);
    }
}

uint64_t big_integer::get_significant_digit() {
    if (digits.size() != 1 && digits[digits.size() - 1] == 0) {
        digits.pop_back();
    }
    return digits.back();
}

void big_integer::sub_div_result(big_integer const& divider, uint32_t rest, size_t shift) {
    std::vector<uint32_t> result = std::vector<uint32_t>(divider.length() + 1, 0);
    uint64_t carry = 0;
    for (size_t i = 0; i < divider.length(); ++i) {
        uint64_t cur = static_cast<uint64_t>(divider.digits[i]) * rest + carry;
        result[i] = static_cast<uint32_t>(cur);
        carry = get_high(cur);
    }
    result[result.size() - 1] = static_cast<uint32_t>(carry);
    shift_sub(result, shift);
}

std::pair<big_integer, big_integer> big_integer::div(big_integer const& rhs) const {
    big_integer result = abs();
    big_integer divider = rhs.abs();
    if (result < divider) {
        return std::make_pair(0, *this);
    } else if (divider.length() == 1) {
        int64_t rest = result.div_big_short(divider.digits[0], get_sign() ^ rhs.get_sign(), get_sign());
        return std::make_pair(result, rest);
    }
    int norm = __builtin_clz(divider.get_significant_digit());
    result <<= norm;
    divider <<= norm;
    uint64_t divider_high = divider.get_significant_digit();
    size_t n = divider.length();
    size_t m = result.length() - divider.length();
    big_integer rest(0);
    rest.digits.resize(m + 1, 0);
    if (result.shift_compare(divider, m)) {
        rest.digits[m] = 1;
        result.shift_sub(divider.digits, m);
    }
    for (size_t i = m; i-- > 0;) {
        uint64_t tmp = (set_high(result.get(n + i)) + result.get(n + i - 1)) / divider_high;
        rest.digits[i] = static_cast<uint32_t>(tmp <= UINT32_MAX ? tmp : UINT32_MAX);
        result.sub_div_result(divider, rest.digits[i], i);
        size_t divider_len = divider.length();
        while (result.get_sign()) {
            result.digits.resize(result.digits.size() + 1, UINT32_MAX);
            --rest.digits[i];
            loop_with_carry(
                result.length() - i, 0, [](uint32_t e) { return e; },
                [&result, i](size_t j) { return result.get(i + j); },
                [&divider, divider_len](size_t j)
                { return static_cast<uint64_t>(j < divider_len ? divider.digits[j] : 0); },
                [&result, i](size_t j, uint32_t val) mutable { result.digits[j + i] = val; });
            result.trim();
        }
    }
    rest.trim();
    if (get_sign() ^ rhs.get_sign()) {
        rest.negate();
    }
    result >>= norm;
    if (get_sign()) {
        result.negate();
    }
    return std::make_pair(rest, result);
}

uint32_t big_integer::get(size_t index) const {
    return (index < digits.size()) ? digits[index] : (get_sign() ? UINT32_MAX : 0);
}

size_t big_integer::length() const {
    return digits.size();
}

void big_integer::trim() {
    bool cur_sign = get_highest_bit(digits.back());
    while (length() > 1) {
        if (digits.back() == (cur_sign ? UINT32_MAX : 0) &&
            get_highest_bit(digits[digits.size() - 2]) == cur_sign) {
            digits.pop_back();
        } else {
            break;
        }
    }
}

bool big_integer::get_sign() const {
    return !digits.empty() && get_highest_bit(digits.back());
}

void big_integer::add(big_integer const& rhs, uint32_t carry, std::function<uint32_t (uint32_t)> const& f) {
    size_t n = std::max(length(), rhs.length()) + 2;
    digits.resize(n, get_sign() ? UINT32_MAX : 0);
    loop_with_carry(
        n,
        carry,
        f,
        [&, this](size_t i) { return get(i); },
        [&, rhs](size_t i) { return rhs.get(i); },
        [&, this](size_t i, uint32_t val) { digits[i] = val; });
    trim();
}

big_integer& big_integer::operator+=(big_integer const& rhs) {
    add(rhs, 0, [](uint32_t e) {return e;});
    return *this;
}

big_integer& big_integer::operator-=(big_integer const& rhs) {
    add(rhs, 1, [](uint32_t e) {return ~e;});
    return *this;
}

void big_integer::iterate(const big_integer& rhs, std::function<uint32_t(uint32_t, uint32_t)>const& f) {
    size_t n = std::max(length(), rhs.length()) + 1;
    digits.resize(n, get_sign() ? UINT32_MAX : 0);
    for(size_t i = 0; i < n; ++i) {
        digits[i] = f(get(i), rhs.get(i));
    }
    trim();
}

big_integer big_integer::abs() const {
    return get_sign() ? -*this : *this;
}

big_integer& big_integer::operator*=(big_integer const& rhs) {
    big_integer l = abs();
    big_integer r = rhs.abs();
    size_t n = std::max(length(), rhs.length()) << 1;
    std::vector<uint32_t> res(n, 0);
    for (size_t i = 0; i != length(); ++i) {
        uint64_t carry = 0;
        for (size_t j = 0; j != r.length(); ++j) {
            uint64_t mul = static_cast<uint64_t>(l.get(i)) * static_cast<uint64_t>(r.get(j));
            uint64_t t = static_cast<uint64_t>(get_low(mul)) + carry + static_cast<uint64_t>(res[i + j]);
            res[i + j] = t;
            carry = static_cast<uint64_t>(get_high(mul)) + static_cast<uint64_t>(get_high(t));
        }
        res[i + r.length()] += static_cast<uint32_t>(carry);
    }
    bool real_sign = get_sign() ^ rhs.get_sign();
    digits = res;
    trim();
    if (real_sign) {
        negate();
    }
    return *this;
}

big_integer& big_integer::operator/=(big_integer const& rhs) {
    return *this = div(rhs).first;
}

big_integer& big_integer::operator%=(big_integer const& rhs) {
    return *this = div(rhs).second;
}

big_integer& big_integer::operator&=(big_integer const& rhs) {
    iterate(rhs, [](uint32_t lhs, uint32_t rhs) {return lhs & rhs;});
    return *this;
}

big_integer& big_integer::operator|=(big_integer const& rhs) {
    iterate(rhs, [](uint32_t lhs, uint32_t rhs) {return lhs | rhs;});
    return *this;
}

big_integer& big_integer::operator^=(big_integer const& rhs) {
    iterate(rhs, [](uint32_t lhs, uint32_t rhs) {return lhs ^ rhs;});
    return *this;
}

big_integer& big_integer::operator<<=(int val) {
    if (val > 0) {
        size_t total = val / UINT32_BITS;
        uint32_t r = val % UINT32_BITS;
        size_t len = digits.size();
        digits.resize(total + length() + (r == 0 ? 0 : 1) + 1, get_sign() ? UINT32_MAX: 0);
        uint32_t carry = get(len);
        uint64_t buf = 0;
        for (size_t i = len; i-- > 0;) {
            uint32_t tmp = get(i);
            buf = static_cast<uint64_t>(tmp) | set_high(carry);
            carry = tmp;
            buf <<= r;
            digits[i + total + 1] = get_high(buf);
        }
        digits[total] = get_low(buf);
        for (size_t i = 0; i < total; ++i) {
            digits[i] = 0;
        }
        trim();
    }
    return *this;
}

big_integer& big_integer::operator>>=(int val) {
    if (val > 0) {
        size_t total = val / UINT32_BITS;
        uint32_t r = val % UINT32_BITS;
        if (total >= length()) {
            *this = big_integer(0);
            return *this;
        }
        for (size_t i = total; i < length(); ++i) {
            uint64_t buf = static_cast<uint64_t>(get(i)) | set_high(get(i + 1));
            buf >>= r;
            digits[i - total] = get_low(buf);
        }
        for (size_t i = length() - total; i < length(); ++i) {
            digits[i] = get_sign() ? UINT32_MAX : 0;
        }
        trim();
    }
    return *this;
}

big_integer big_integer::operator+() const {
    return *this;
}

big_integer big_integer::operator-() const {
    return big_integer(0) - *this;
}

void big_integer::invert() {
    for (uint32_t &number : digits) {
        number ^= UINT32_MAX;
    }
}

void big_integer::negate() {
    uint64_t carry = 1;
    for(uint32_t &number : digits) {
        number ^= UINT32_MAX;
        uint64_t sum = static_cast<uint64_t>(number) + static_cast<uint64_t>(carry);
        number = get_low(sum);
        carry = get_high(sum);
    }
    if (carry != 0) {
        digits.emplace_back(get_sign() ? UINT32_MAX : 0);
    }
    trim();
}

big_integer big_integer::operator~() const {
    big_integer tmp = *this;
    tmp.invert();
    return tmp;
}

big_integer& big_integer::operator++() {
    return *this += 1;
}

big_integer big_integer::operator++(int) {
    big_integer res = *this;
    *this += 1;
    return res;
}

big_integer& big_integer::operator--() {
    return *this -= 1;
}

big_integer big_integer::operator--(int) {
    big_integer res = *this;
    *this -= 1;
    return res;
}

big_integer operator+(big_integer lhs, big_integer const& rhs) {
    return lhs += rhs;
}

big_integer operator-(big_integer lhs, big_integer const& rhs) {
    return lhs -= rhs;
}

big_integer operator*(big_integer lhs, big_integer const& rhs) {
    return lhs *= rhs;
}

big_integer operator/(big_integer lhs, big_integer const& rhs) {
    return lhs /= rhs;
}

big_integer operator%(big_integer lhs, big_integer const& rhs) {
    return lhs %= rhs;
}

big_integer operator&(big_integer lhs, big_integer const& rhs) {
    return lhs &= rhs;
}

big_integer operator|(big_integer lhs, big_integer const& rhs) {
    return lhs |= rhs;
}

big_integer operator^(big_integer lhs, big_integer const& rhs) {
    return lhs ^= rhs;
}

big_integer operator<<(big_integer lhs, int rhs) {
    return lhs <<= rhs;
}

big_integer operator>>(big_integer lhs, int rhs) {
    return lhs >>= rhs;
}

bool operator==(big_integer const& lhs, big_integer const& rhs) {
    return lhs.get_sign() == rhs.get_sign() && lhs.digits == rhs.digits;
}

bool operator!=(big_integer const& lhs, big_integer const& rhs) {
    return !(lhs == rhs);
}

bool operator<(big_integer const& lhs, big_integer const& rhs) {
    if (lhs.get_sign() && !rhs.get_sign()) {
        return true;
    } else if (!lhs.get_sign() && rhs.get_sign()){
        return false;
    }
    size_t length_l = lhs.length();
    size_t length_r = rhs.length();
    if (lhs.digits.back() == 0 || lhs.digits.back() == UINT32_MAX) {
        --length_l;
    }
    if (rhs.digits.back() == 0 || rhs.digits.back() == UINT32_MAX) {
        --length_r;
    }
    if (length_l != length_r) {
        return length_l < length_r;
    }
    for (size_t i = length_l; i > 0; --i) {
        if (lhs.digits[i - 1] != rhs.digits[i - 1]) {
            return lhs.digits[i - 1] < rhs.digits[i - 1];
        }
    }
    return false;
}

bool operator>(big_integer const& lhs, big_integer const& rhs) {
    return rhs < lhs;
}

bool operator<=(big_integer const& lhs, big_integer const& rhs) {
    return !(rhs < lhs);
}

bool operator>=(big_integer const& lhs, big_integer const& rhs) {
    return !(lhs < rhs);
}

std::string to_string(big_integer const& lhs) {
    std::string result;
    big_integer p(lhs.abs());
    while (p > 0) {
        uint64_t rest = p.div_big_short(BASE_DIVIDER, false, false);
        std::string tmp = std::to_string(rest);
        std::reverse(tmp.begin(), tmp.end());
        result += tmp;
        if (p.digits[0] != 0) {
            result.append(STRING_STEP - tmp.size(), '0');
        }
    }
    if (result.empty()) {
        result = "0";
    }
    if (lhs.get_sign()) {
        result.push_back('-');
    }
    std::reverse(result.begin(), result.end());
    return result;
}

std::ostream& operator<<(std::ostream& s, big_integer const& lhs) {
    return s << to_string(lhs);
}
