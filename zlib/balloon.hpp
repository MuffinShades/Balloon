/*
Inflate Reference:

https://pyokagan.name/blog/2019-10-18-zlibinflate/
https://web.archive.org/web/20230528220141/https://pyokagan.name/blog/2019-10-18-zlibinflate/

Deflate Reference:

My brain :)
*/


#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cassert>
#include <vector>

#define HUFFMAN_PENDING 0x00
#define HUFFMAN_ERROR 0x01
#define HUFFMAN_RESULT_ENCODE 0x02
#define HUFFMAN_RESULT_DECODE 0x03

//types
typedef long i64;
typedef unsigned long u64;
typedef unsigned int u32;
typedef int i32;
typedef unsigned short u16;
typedef short i64;
typedef unsigned char u8;
typedef char i8;
typedef unsigned char byte;

//huffman tree node
struct HuffmanTreeNode {
	HuffmanTreeNode* left = nullptr;
	HuffmanTreeNode* right = nullptr;
	u32 val = NULL;
	bool branch = false;
	i32 count = 0;
	i32 depth = 0;
	u32* symCodes;
	size_t alphabetSz;
};

//character count
/*struct CharCount {
	i32 count = 0;
	u32 val = NULL;
	operator HuffmanTreeNode() { //conversion
		HuffmanTreeNode res;
		res.val = this->val;
		res.count = this->count;
		return res;
	};
};*/

//byte pointer with length
struct RawBytes {
	u32* bytes = nullptr;
	size_t len = 0;
	void CopyToBuffer(RawBytes* buffer);
	void free() {
		delete[] this->bytes;
	};
};

//result from huffman decode and encode
struct HuffmanResult {
	byte* bytes = nullptr;
	i32 nChars = 0;
	size_t len = 0;
	i32 bitLen = 0;
	i32 bitOverflow = 0;
	i32 resultType = HUFFMAN_PENDING;
};

//bit stream class for reading and doing stuff with bits
class BitStream {
public:
	u32* bytes = nullptr;
	i32 pos = 0, rPos = 0;
	i32 lBit = 0;
	u32 cByte = 0;
	u32 rBit = 0;
	size_t sz, bsz, asz;
	//bit stream for reading existing bytes / bits
	BitStream(u32* bytes, size_t len);
	//basic zero allocation bit stream for writing
	BitStream(size_t len);
	i32 readBit();
	i32 readByte();
	i32 readNBits(i32 nBits);
	i32 readValue(size_t size);
	static u32* bitsToBytes(u32* bits, size_t len);
	static u32* bitsToBytes(u32 bits, size_t len);
	void checkWritePosition();
	void writeBit(u32 bit);
	//write multiple bits
	void writeNBits(u32* bits, size_t len);
	//write short, long, int, uint, etc.
	template<typename _T> void writeValue(_T val);
	//allocate new bit chunk
	void allocNewChunk();
	void seek(i32 pos);
	void seekw(i32 wPos);
	i32 tell();
	i32 tellw();
	//remove unused bytes
	void clip();
	//allocation function
	void calloc(size_t sz);
};

class Huffman {
public:
	static void DebugMain();
	static HuffmanResult Encode(char* bytes, size_t len);
	static HuffmanResult Decode(char* bytes, size_t len);
};

class lz77 {

};

class ZResult {
public:
	u32* bytes;
	size_t len;
	i32 compressionLevel, compressionMode;
	u32 checkSum;
	ZResult() {};
};

/*

Zlib class for the zlib stuff

Deflate levels are 0, 1, or 2 with 0 being no compression,
1 being just huffman, and 2 being both huffman and lz77

Note ill add the level 0-10 sysetm later

*/
class Zlib {
public:
	static ZResult *Inflate(u32* bytes, size_t len);
	static ZResult Deflate(u32* bytes, size_t len, const size_t winBits, const int level);
};