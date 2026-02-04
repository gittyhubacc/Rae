#include "mf/string.h"

string epsilon = S("");

string string_mask(arena *a, char *c_str)
{
	string s;
	s.len = 0;
	while (c_str[s.len] != '\0') {
		s.len++;
	}
	s.addr = make(a, char, s.len);
	for (int i = 0; i < s.len; i++) {
		s.addr[i] = c_str[i];
	}
	return s;
}

string itos(arena *a, int i)
{
        if (i == 0) {
                char *addr = make(a, char, 1);
                *addr = '0';
                return (string){.addr = addr, .len = 1};
        }

        int copy = i;
        int digits = 0;
        while (copy > 0) {
                digits++;
                copy = copy / 10;
        }
        copy = i;
        string result;
        result.len = 0;
        result.addr = make(a, char, digits);
        while (copy > 0) {
                result.addr[digits - (++result.len)] = (copy % 10) + '0';
                copy = copy / 10;
        }
        return result;
}

string itosb(arena *a, int i, int base)
{
        if (i == 0) {
                char *addr = make(a, char, 1);
                *addr = '0';
                return (string){.addr = addr, .len = 1};
        }

        int copy = i;
        int digits = 0;
        while (copy > 0) {
                digits++;
                copy = copy / base;
        }
        copy = i;
        string result;
        result.len = 0;
        result.addr = make(a, char, digits);
        while (copy > 0) {
                result.addr[digits - (++result.len)] = (copy % base) + '0';
                copy = copy / base;
        }
        return result;
}

float stof(string s)
{
        int i = 0;
        int found = 0;
        float m = 0.1;
        float sign = 1;
        float result = 0;

        if (s.addr[i] == '-') {
                sign = -1;
                i++;
        }

        while (i < s.len) {
                if (s.addr[i] == '.') {
                        found = 1;
                        i++;
                        continue;
                }
                if (!found) {
                        result = (result * 10) + (s.addr[i] - '0');
                } else {
                        result = result + ((s.addr[i] - '0') * m);
                        m = m * 0.1;
                }
                i++;
        }

        return result * sign;
}

int stoi(string s)
{
        int result = 0;
        for (int i = 0; i < s.len; i++) {
                result = (result * 10) + (s.addr[i] - '0');
        }
        return result;
}

int streq(string left, string right)
{
        if (left.len != right.len) {
                return 0;
        }
        if (left.addr == right.addr) {
                return 1;
        }
        for (int i = 0; i < left.len; i++) {
                if (left.addr[i] != right.addr[i]) {
                        return 0;
                }
        }
        return 1;
}

int indexof(string s, char c, int skip)
{
        for (int i = 0; i < s.len; i++) {
                if (s.addr[i] == c && skip-- <= 0) {
                        return i;
                }
        }
        return -1;
}

string concat(arena *a, string left, string right)
{
        string res;
        res.len = left.len + right.len;
        res.addr = make(a, char, res.len);
        mfmemcpy(res.addr, left.addr, left.len);
        mfmemcpy(res.addr + left.len, right.addr, right.len);
        return res;
}
