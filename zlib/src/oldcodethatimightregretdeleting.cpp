#ifdef _OLD_CODE

#include "balloon.hpp"
#include "balloon.cpp"

//write trees
void writeTrees_old(BitStream& stream, HuffmanTreeNode* litTree, HuffmanTreeNode* distTree) {
	//get codes
	//std::cout << "----------------\nLit Tree\n------------------------" << std::endl;
	//PrintTree(litTree);

	std::cout << "WRITING TREES!" << std::endl;

	u32* litCodes = getTreeBitLens(litTree, litTree->alphabetSz);
	u32* distCodes = getTreeBitLens(distTree, distTree->alphabetSz);

	for (int i = 0; i < litTree->alphabetSz; i++) {
		std::cout << litCodes[i] << " lc" << std::endl;
	}

	for (int i = 0; i < distTree->alphabetSz; i++) {
		std::cout << distCodes[i] << " dc" << std::endl;
	}

	//so some clipping and other stuff
	arr_container<u32>* litClip = clipArr(litCodes, litTree->alphabetSz);
	arr_container<u32>* distClip = clipArr(distCodes, distTree->alphabetSz);

	//now get some header info
	i32 HLIT = litClip->sz - 257;
	i32 HDIST = distClip->sz - 1;

	if (distClip->sz <= 0) HDIST = 0;
	if (litClip->sz <= 0) HLIT = 0;

	std::cout << "HLIT: " << HLIT + 257 << " " << " HDIST: " << HDIST+1 << std::endl;

	//allocate some memory ill probably forget to free later lol
	u32* treeBitCounts = new u32[19];

	u32* precompressedCodeLengths = new u32[litClip->sz + distClip->sz];

	//do some stuff
	memcpy(
		precompressedCodeLengths,
		litClip->dat,
		litClip->sz * sizeof(u32)
	); //3 bits per code
	memcpy(
		precompressedCodeLengths + litClip->sz,
		distClip->dat,
		distClip->sz * sizeof(u32)
	);

	for (int i = 0; i < litClip->sz + distClip->sz; i++) {
		//if (i % litClip->sz == 0) std::cout << "--------------------" << std::endl;
		std::cout << "Precompessed CL: " << precompressedCodeLengths[i] << " " << i << std::endl;
	}

	//now compress code lengths
	const int lengthMask = 0x7; //0b111

	std::vector<u32> compressedLengths, compressedSymbols;

	u32* current = precompressedCodeLengths; //yay glitchy ass pointers
	u32* end = precompressedCodeLengths + (litClip->sz + distClip->sz);

	//run length encoding
	do {
		if (current == nullptr) {
			break;
		}
		//0 back ref stuff
		//std::cout << "Cur: " << *current << " " << precompressedCodeLengths << " " << (end - precompressedCodeLengths) << " " << (current - precompressedCodeLengths) << std::endl;
		if (*current == 0) {
			u32* temp = current;

			do {} while (*temp++ == 0 && (u64)(current - temp) < (RLE_Z_BASE + RLE_Z2_MASK));

			u64 copyLen = (u64)(temp - current);
			copyLen = MAX(copyLen+1, 1);

			//std::cout << "COPY LEN: " << copyLen << std::endl;

			if (copyLen >= RLE_Z_BASE) {
				//back reference

				if (copyLen >= RLE_L_BASE) {
					//back reference
					//ll 1
					if (copyLen > RLE_L_BASE + RLE_Z1_MASK) {
						compressedLengths.push_back(17);
						compressedSymbols.push_back(17);
						compressedLengths.push_back((copyLen - RLE_L_BASE) & RLE_Z1_MASK);
						current += copyLen;
					}
					//ll 2
					else {
						compressedLengths.push_back(18);
						compressedSymbols.push_back(18);
						compressedLengths.push_back((copyLen - RLE_L_BASE) & RLE_Z2_MASK);
						current += copyLen;
					}
				}
			}
			else {
				current += copyLen;
				while (copyLen-- > 0) {
					compressedLengths.push_back(0);
					compressedSymbols.push_back(0);
				}
			}
		}
		//else just do normal run length encoding
		else {
			u32* temp = current;

			//goofy do while to screw with pointers
			do {} while (*temp++ == *current && (u64)(current - temp) < (RLE_L_BASE + RLE_L_MASK));

			u64 copyLen = (u64)(temp - current);

			copyLen = MAX(copyLen+1, 1);

			//std::cout << "COPY LEN: " << copyLen << std::endl;

			if (copyLen >= RLE_L_BASE) {
				//back reference
				//ll 1
				if (copyLen > RLE_L_BASE + RLE_L_MASK) {
					compressedLengths.push_back(16);
					compressedSymbols.push_back(16);
					compressedLengths.push_back((copyLen - RLE_L_BASE) & RLE_L_MASK);
					current += copyLen;
				}
			}
			//just add stuff ya know
			else {
				byte tCopy = *current;
				while (copyLen-- > 0) {
					compressedLengths.push_back(tCopy);
					compressedSymbols.push_back(tCopy);
				}

				current += copyLen;
			}
		}
	} while (current < end);

	for (auto v : compressedSymbols) {
		std::cout << "\teBit Lens: " << v << std::endl;
	}

	delete[] precompressedCodeLengths, treeBitCounts, litClip->dat, distClip->dat;
	delete litClip, distClip;

	for (auto o : compressedSymbols) {
		std::cout << "CompressedLen: " << o << std::endl;
	}

	//generate tree tree... yeah the tree tree
	u32* codeCounts = GetCharCount(compressedSymbols.data(), compressedSymbols.size(), 19);
	//std::cout << "! " << std::endl;
	HuffmanTreeNode* tempTree = GenerateBaseTree(codeCounts, 19);
	//std::cout << "! " << std::endl;
	HuffmanTreeNode* codeLengthTree = CovnertTreeToCanonical(tempTree, 19);
	//std::cout << "! " << std::endl;
	u32* codeTreeBitLen = getTreeBitLens(codeLengthTree, 19);
	//std::cout << "! " << std::endl;
	//write bit counts for code length tree
	arr_container<u32>* ccClip = clipArr(codeTreeBitLen, 19);

	for (int i = 0; i < 19; i++) {
		std::cout << "CCCOUNT: " << codeCounts[i] << " " << i << std::endl;
	}
	//std::cout << "! " << std::endl;
	u32* cc = ccClip->dat;
	u32 HCLEN = ccClip->sz - 4;

	//write tree header
	WriteVBitsToStream(stream, HLIT, 5);
	WriteVBitsToStream(stream, HDIST, 5);
	WriteVBitsToStream(stream, HCLEN, 4);

	std::cout << "LIT CODES 1: " << HLIT << " DIST CODES 1: " << HDIST << " CL CODES 1: " << HCLEN << std::endl;
	PrintTree(codeLengthTree);
	//write the code lengths into the tree thingy
	do {
		size_t i = (int)(cc - ccClip->dat);
		WriteVBitsToStream(stream, *(ccClip->dat + CodeLengthCodesOrder[i]), 3);
		std::cout << "e: " << i << std::endl;
		std::cout << "\tuBitLens: " << *(ccClip->dat + CodeLengthCodesOrder[i]) << std::endl;
	} while (cc++ < ccClip->dat + ccClip->sz);

	//final step, encode+write the other 2 trees to the stream
	u32 sym = NULL;
	for (i32 i = 0; i < compressedLengths.size(); i++) {
		std::cout << i << " " << compressedLengths.size() << std::endl;
		sym = compressedLengths[i];
		WriteVBitsToStream(
			stream,
			EncodeSymbol(
				sym,
				codeLengthTree
			),
			codeTreeBitLen[compressedLengths[i]]
		);

		std::cout << "pBitLen: " << sym << std::endl;

		//back reference bits
		if (sym > 15) {
			switch (sym) {
			case 16: {
				WriteVBitsToStream(stream, sym, 2);
				break;
			}
			case 17: {
				WriteVBitsToStream(stream, sym, 3);
				break;
			}
			case 18: {
				WriteVBitsToStream(stream, sym, 7);
				break;
			}
			}

			i++;
		}
	}

	delete[] codeTreeBitLen, litCodes, distCodes;

	std::cout << "Done Writing Trees!" << std::endl;
};

#endif