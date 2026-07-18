#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

bool whitespace(char ch) {
    return (' ' == ch) || ('\n' == ch) || ('\t' == ch);
}

int64_t parse(const char* str) {
    // trim off starting whitespace
    while (whitespace(*str)) {
        if (*str == 0) {
            // PANIC("invalid string");
            return 0;
        }
        str += 1;
    }

    // trim and store sign if exists
    bool negative = false;
    if (*str == '+') {
        str += 1;
    } else if (*str == '-') {
        negative = true;
        str += 1;
    }

    // move end_str to the character past digit sequence
    /* "   234345453e "
           ^str     ^end_str
        "0"
         ^str
          ^end_str (NUL)
        ""
         (both str and end_str on NUL)
    */
    const char* end_str = str;
    while (*end_str != 0) {
        if (whitespace(*end_str)) {
            break;
        }
        if (*end_str < '0' || *end_str > '9') {
            break;
        }
        end_str += 1;
    }

    if (end_str == str) {
        // PANIC("no number or sign immediately following whitespace");
        return 0;
    }

    // between in range [str, end), we only have chars between '0' and '9'

    int64_t value = 0;
    int64_t multiplier = 1;
    // work backwards from last good character until first good character
    for (const char* ch = (end_str - 1); ch >= str; ch--) {
        value += (*ch - '0') * multiplier;
        multiplier *= 10;
    }

    if (negative) {
        return -value;
    } else {
        return value;
    }
}

int atoi(const char* str) {
    return (int)parse(str);
}

long atol(const char* str) {
    return (long)parse(str);
}

long long atoll(const char* str) {
    return (long long)parse(str);
}
