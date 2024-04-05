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
#define u32 unsigned int
#define i32 int
#define u8 unsigned char
#define i8 char
#define byte unsigned char

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
	~HuffmanTreeNode() {
		//delete[] this->left;
		//delete[] this->right;
	}
};

//character count
struct CharCount {
	i32 count = 0;
	u32 val = NULL;
	operator HuffmanTreeNode() { //conversion
		HuffmanTreeNode res;
		res.val = this->val;
		res.count = this->count;
		return res;
	};
};

//byte pointer with length
struct RawBytes {
	u32* bytes = nullptr;
	i32 len = 0;
	void CopyToBuffer(RawBytes* buffer);
	void free() {
		delete[] this->bytes;
	};
};

//result from huffman decode and encode
struct HuffmanResult {
	u32* bytes = nullptr;
	std::vector<CharCount> charCount;
	i32 nChars = 0;
	size_t len = 0;
	i32 bitLen = 0;
	float compressionRatio = 0.0f;
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
	u32 rBit = 8;
	size_t sz, bsz, asz;

	//bit stream for reading existing bytes / bits
	BitStream(u32 *bytes, size_t len) {
		this->bytes = new u32[len];
		memcpy(this->bytes, bytes, len * sizeof(u32));
		this->bsz = len * 8;
		this->sz  = len;
		this->asz = sz;
		this->rPos = this->sz;
	}

	//basic zero allocation bit stream for writing
	BitStream(size_t len) {
		assert(len > 0);
		this->bytes = new u32[len];
		memset(this->bytes, 0, len * sizeof(u32));
		this->bsz = len * 8;
		this->asz = len;
		this->rPos = 0;
	}

	i32 readBit() {
		if (this->lBit <= 0) {
			//std::cout << "Bit Pos: " << this->pos << " " << this->sz << std::endl;
			assert(this->pos < this->bsz && this->pos >= 0);

			//advance a byte
			this->cByte = this->bytes[this->pos];
			this->pos++;
			this->lBit = 8;
		}

		this->lBit--;

		//extract le bit
		i32 bit = this->cByte & 1; //extract first bit
		this->cByte >>= 1; //advance a bit
		return bit; //return extracted bit
	}

	i32 readByte() {
		this->lBit = 0;
		return this->bytes[this->pos++];
	}

	i32 readNBits(i32 nBits) {
		i32 r = 0, c = 0;
		while (c < nBits)
			r |= (this->readBit() << (c++));
		return r;
	}

	i32 readValue(size_t size) {
		i32 r = 0;
		for (size_t i = 0; i < size; i++)
			r |= (this->readByte() << (i * 8));

		return r;
	}

	static u32* bitsToBytes(u32 *bits, size_t len) {
		u32 *res = new u32[ceil((float)len / 8.0f)];

		i32 bCollect = 0, iBit = 0, cByte = 0;
		for (i32 i = len - 1; i >= 0; i--) {
			bCollect |= (bits[i]) << iBit;
			iBit++;
			if (iBit >= 8) {
				res[cByte++] = bCollect;
				bCollect = 0;
				iBit = 0;
			}
		}

		if (iBit > 0) res[cByte] = bCollect;

		return res;
	}

	static u32* bitsToBytes(u32 bits, size_t len) {
		u32* res = new u32[ceil((float)len / 8.0f)];

		i32 bCollect = 0, iBit = 0, cByte = 0;
		for (i32 i = len - 1; i >= 0; i--) {
			bCollect |= ((bits >> i) & 1) << iBit;
			iBit++;
			if (iBit >= 8) {
				res[cByte++] = bCollect;
				bCollect = 0;
				iBit = 0;
			}
		}

		if (iBit > 0) res[cByte] = bCollect;

		return res;
	}

	void checkWritePosition() {
		if (this->rPos == this->asz)
			this->allocNewChunk();
	}

	void writeBit(u32 bit) {

		//advance a byte if were out of range
		if (this->rBit <= 0) {
			//std::cout << "ALLOC " << this->sz << std::endl;
			this->rBit = 8;
			this->rPos++;
			this->sz++;
			this->checkWritePosition();
		}
		//std::cout << "Bit Write: " << (bit & 1) << std::endl;
		//std::cout << "Alloc done" << std::endl;
		//now write bit
		this->bytes[this->rPos] <<= 1;
		this->bytes[this->rPos] |= (bit & 1); //& with 1 to just get first bit
		//std::cout << this->rPos << " " << this->rBit << "  " << this->sz << std::endl;
		this->rBit--;
	}


	//write multiple bits
	void writeNBits(u32* bits, size_t len) {
		for (size_t i = 0; i < len; i++)
			this->writeBit(bits[i]);
	}

	//write short, long, int, uint, etc.
	template<typename _T> void writeValue(_T val) {
		size_t vsz = sizeof(_T) * 8;
		;
		for (i32 i = vsz - 1; i >= 0; i--)
			this->writeBit((val >> i) & 1);
	}

	//allocate new bit chunk
	void allocNewChunk() {
		u32* tBytes = new u32[this->asz];
		u32 osz = this->asz;
		memcpy(tBytes, this->bytes, this->asz);
		this->asz += 256;
		delete[] this->bytes;
		this->bytes = new u32[this->asz];
		memset(this->bytes, 0, sizeof(u32) * this->asz);
		memcpy(this->bytes, tBytes, osz);
		delete[] tBytes;
	}

	void seek(i32 pos) {
		assert(pos >= 0 && pos < this->asz);
		this->pos = pos;
	}

	void seekw(i32 wPos) {
		assert(wPos >= 0 && wPos < this->asz);
		this->rPos = wPos;
	}

	i32 tell() {
		return this->pos;
	}

	i32 tellw() {
		return this->pos;
	}

	//remove unused bytes
	void clip() {
		u32* tBytes = new u32[this->asz];
		u32 osz = this->asz;
		memcpy(tBytes, this->bytes, this->asz);
		this->asz = this->sz;
		delete[] this->bytes;
		this->bytes = new u32[this->asz];
		memset(this->bytes, 0, sizeof(u32) * this->asz);
		memcpy(this->bytes, tBytes, osz);
		delete[] tBytes;
	}

	void calloc(size_t sz) {
		if (this->bytes)
			delete[] this->bytes;

		this->bytes = new u32[sz];

		this->asz = sz;
		this->bsz = sz * 8;
		this->pos = this->rPos = this->lBit = this->cByte = this->sz = 0;

		i32 pos = 0, rPos = 0;
		i32 lBit = 0;
		u32 cByte = 0;
		u32 rBit = 8;
	}
};

class Huffman {
public:
	static void DebugMain();
	static HuffmanResult Encode(u32* bytes, size_t len);
	static HuffmanResult Decode(u32* bytes, size_t len);
};

struct ZResult {
	char* bytes;
	size_t len;
	i32 compressionLevel, compressionMode;
	u32 checkSum;
};

class Zlib {
public:
	ZResult Inflate(u32* bytes, size_t len);
	void Deflate(u32* bytes, size_t len);
};