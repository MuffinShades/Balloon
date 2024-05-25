#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <assert.h>

#define MAX(a,b) (a>b?a:b);

template<typename _Ty>void ZeroMem(_Ty *buf, size_t sz) {
    memset(buf, NULL, sizeof(_Ty) * sz);
}

class mString {
private:
    char *mem = (char*) malloc(0xff);
    size_t allocSz = 0xff;
    size_t sz = 0;
    const size_t cAlloc = 0xff;
    void alloc() {
        this->allocSz += this->cAlloc;
        char *tmp = (char *) malloc(this->allocSz);
        ZeroMem(tmp, this->allocSz);
        memcpy(tmp, this->mem, this->sz);
        free(this->mem);
        this->mem = tmp;
    }
    void _allocCheck() {
        if (this->sz >= allocSz)
            this->alloc();
    }
    size_t _strlen(const char *dat) {
        size_t sz = 0;
        do {sz++;} while(*++dat != NULL);
        return sz;
    }
public:
    mString(size_t sz) {
        assert(sz > 0);
        this->mem = (char *) malloc(sz * sizeof(char));
    }
    mString(const char *dat) {
        this->sz = _strlen(dat);
        this->allocSz = MAX(this->sz, this->allocSz);
        this->mem = (char*) malloc(sizeof(char) * sz);
        ZeroMem(this->mem, this->sz);
        memcpy(this->mem, dat, sz * sizeof(char));
    }
    ~mString() {
        if (this->mem != nullptr)
            free(this->mem);
    }
    void operator=(const char *dat) {
        this->sz = _strlen(dat);
        this->allocSz = MAX(this->sz, this->allocSz);
        this->mem = (char*) malloc(sizeof(char) * this->sz);
        memcpy(this->mem, dat, this->sz * sizeof(char));
    }
    void operator+(const char c) {
        this->_allocCheck();
        this->mem[this->sz++] = c;
    }
    size_t length() {
        return this->sz;
    }
    const char* c_str() {
        return (const char*) this->mem;
    }
};