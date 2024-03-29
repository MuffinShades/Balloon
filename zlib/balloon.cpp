#include "balloon.hpp"
#include <algorithm>

#define INRANGE(v,m,x) (v >= m && v < x)
#define MIN(a, b) (a <= b ? a : b)

template<typename _Ty> void ZeroMem(_Ty* mem, size_t len) {
	memset(mem, 0, sizeof(_Ty) * len);
}

void FreeTree(HuffmanTreeNode* tree) {
	if (tree != nullptr) {
		if (tree->left != nullptr) FreeTree(tree->left);
		if (tree->right != nullptr) FreeTree(tree->right);

		delete tree;
	}
}

void PrintTree(HuffmanTreeNode* tree, std::string tab = "", std::string code = "") {
	if (tree->right == nullptr && tree->left == nullptr) {
		std::cout << tab << "  - " << tree->count << " " << code << " " << "  " << tree->val << "   " << (char)tree->val << std::endl;
		return;
	}

	std::cout << tab << "|_ " << tree->count << std::endl;
	if (tree->right != nullptr)
		PrintTree(tree->right, tab+" ",code+"1");

	if (tree->left != nullptr)
		PrintTree(tree->left, tab+" ", code + "0");
}

u32 DecodeSymbol(BitStream& stream, HuffmanTreeNode* tree) {
	HuffmanTreeNode* n = tree;

	while (n->left != nullptr || n->right != nullptr) {
		i32 b = stream.readBit();

		//std::cout << "Decode Bit: " << b << std::endl;

		if (b) {
			if (n->right == nullptr) break;

			n = n->right;
		}
		else {
			if (n->left == nullptr) break;

			n = n->left;
		}
	}

	return n->val;
}

void InsertNode(u32 code, size_t codeLen, u32 val, HuffmanTreeNode* tree) {
	HuffmanTreeNode* current = tree;
	//std::cout << " ------------- " << std::endl;
	for (int i = codeLen - 1; i >= 0; i--) {
		int bit = (code >> i) & 1;

		//std::cout << "Tbit: " << bit << " ";

		if (bit) {
			//std::cout << "R" << std::endl;
			if (current->right == nullptr)
				current->right = new HuffmanTreeNode();

			current = current->right;
		}
		else {
			//std::cout << "L" << std::endl;
			if (current->left == nullptr)
				current->left = new HuffmanTreeNode();

			current = current->left;
		}
	}

	current->val = val;
}

/*

Python thing:

a[b:] creates an array with everything from a[b] - a[len-1]
a[:b] creates an array with everything from a[0] - a[b-1]


*/

i32 aMax(u32* a, size_t len) {
	u32 max = -INFINITY;

	for (int i = 0; i < len; i++) {
		if (a[i] > max) max = a[i];
	}

	return max;
}

/*

Function to get the number of occuraces and bit length appears

*/
i32* getBitLenCounts(u32* bitLens, size_t blLen, i32 MAX_BITS) {
	i32* res = new i32[MAX_BITS+1];

	memset(res, 0, (MAX_BITS + 1) * sizeof(i32));
	for (int i = 0; i <= MAX_BITS; i++) {
		for (int j = 0; j < blLen; j++) {
			if (i == bitLens[j]) res[i] ++;
		}
	}

	return res;
}

HuffmanTreeNode* BitLengthsToHTree(u32* bitLens, size_t blLen, u32* alphabet, size_t aLen) {
	i32 MAX_BITS = aMax(bitLens, blLen);
	i32* blCount = getBitLenCounts(bitLens, blLen, MAX_BITS);

	std::vector<int> nextCode = {0,0};

	//get the next code for the tree items
	for (int codeIdx = 2; codeIdx <= MAX_BITS; codeIdx++) {
		nextCode.push_back((nextCode[codeIdx - 1] + blCount[codeIdx - 1]) << 1);
	}

	//construct tree
	int i = 0;
	HuffmanTreeNode* res = new HuffmanTreeNode();
	
	while (i < aLen && i < blLen) {
		if (bitLens[i] != 0) {
			InsertNode(nextCode[bitLens[i]], bitLens[i], alphabet[i], res);
			nextCode[bitLens[i]] ++;
		}
		i++;
	}

	//free and return
	delete[] blCount;

	return res;
}

/*u32* CreateAlphabet(size_t len) {
	std::cout << "Alpha Len: " << len << std::endl;
	u32* res = new u32[len];

	for (int i = 0; i < len; i++)
		res[i] = (u32)i;

	return res;
}*/

const i32 CodeLengthCodesOrder[] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
const char CodeLengthAlphabet[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18};

//to construct the codes for the huffman alphabets
int* CreateAlphabet(int len) {
	int* res = new int[len];

	for (int i = 0; i < len; i++)
		res[i] = i;

	return res;
}

std::vector<u32> _treeLens;

HuffmanTreeNode** decodeTrees(BitStream& stream) {
	i32 nTreeCodes = stream.readNBits(5) + 257; //main tree
	i32 nDistCodes = stream.readNBits(5) + 1; //distance tree
	i32 nClCodes = stream.readNBits(4) + 4; //tree tree

	//allocate and get get bit lens
	u32* bitLens = new u32[19];
	memset(bitLens, 0, sizeof(u32) * 19);

	//create bit lengths for the decode tree tree
	for (int i = 0; i < nClCodes; i++)
		bitLens[CodeLengthCodesOrder[i]] = stream.readNBits(3);


	/*std::cout << "Tree Codes: ";
	for (int i = 0; i < 19; i++) {
		std::cout << bitLens[i] << " ";
	}
	std::cout << std::endl;*/

	//create the code length tree
	HuffmanTreeNode* codeLenTree = BitLengthsToHTree(bitLens, 19, (u32*)CreateAlphabet(19), 19);

	//decode le trees
	std::vector<u32> combinedBitLens;

	while (combinedBitLens.size() < nTreeCodes + nDistCodes) {
		i32 sym = DecodeSymbol(stream, codeLenTree);

		//0-15 normal symbol
		if (sym <= 15) {
			combinedBitLens.push_back(sym);
		}
		//16 copy last 3-6 times
		else if (sym == 16) {
			i32 copyLen = stream.readNBits(2) + 3;
			i32 prev = combinedBitLens[combinedBitLens.size() - 1];

			for (int i = 0; i < copyLen; i++)
				combinedBitLens.push_back(prev);
		}
		//17 copy 0 3-10 times
		else if (sym == 17) {
			i32 copyLen = stream.readNBits(3) + 3;

			for (int i = 0; i < copyLen; i++)
				combinedBitLens.push_back(0);
		}
		//18 copy 0 11 - 138 times
		else if (sym == 18) {
			i32 copyLen = stream.readNBits(7) + 11;

			for (int i = 0; i < copyLen; i++)
				combinedBitLens.push_back(0);
		}
		else {
			std::cout << "Invalid Symbol Found! " << std::endl;
		}
	}

	//construct trees
	std::vector<u32> distLens, treeLens;

	for (int i = 0; i < combinedBitLens.size(); i++) {
		if (i < nTreeCodes) {
			treeLens.push_back(combinedBitLens[i]);
			_treeLens.push_back(combinedBitLens[i]);
		} 
		else
			distLens.push_back(combinedBitLens[i]);
	}
	
	u32* da = (unsigned*)CreateAlphabet(30);
	u32* dm = (unsigned*)CreateAlphabet(286);

	for (int i = 0; i < treeLens.size(); i++)
		if (treeLens[i] > 0) std::cout << "Bit Len: " << treeLens[i] << "  " << i << "   " << (char)i << std::endl;

	for (int i = 0; i < treeLens.size(); i++)
		if (treeLens[i] > 0) std::cout << treeLens[i];

	std::cout << std::endl;

	//create le trees
	HuffmanTreeNode* distTree = BitLengthsToHTree(distLens.data(), distLens.size(), da, 30);
	HuffmanTreeNode* mainTree = BitLengthsToHTree(treeLens.data(), treeLens.size(), dm, 286);

	//free memory
	delete[] bitLens;
	FreeTree(codeLenTree);
	delete[] da;
	delete[] dm;

	//return the trees
	HuffmanTreeNode** res = new HuffmanTreeNode * [2];
	res[0] = mainTree;
	res[1] = distTree;

	return res;
}

#define IBLOCK_END 0x100

//LZ77 tables:

const int LengthExtraBits[] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 };
const int LengthBase[] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258 };
const int DistanceExtraBits[] = { 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };
const int DistanceBase[] = { 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577 };

//inflate block with LZ77 and huffman
void InflateBlock(BitStream& stream, HuffmanTreeNode* llTree, HuffmanTreeNode* distTree, std::vector<char>& res) {
	i32 tok;
	for (;;) {
		tok = DecodeSymbol(stream, llTree); //get next token
		
		//normal token / byte so append
		if (tok <= 255) {
			res.push_back(tok);
		}
		//block end
		else if (tok == 256) {
			return;
		}
		//back reference (> 257)
		else {
			i32 lenIdx = tok - 257;

			//get extra length bits and length base
			i32 lenExtraBits = LengthExtraBits[lenIdx],
				len = LengthBase[lenIdx] + stream.readNBits(lenExtraBits);

			//now get distance from distance tree
			i32 distIdx = DecodeSymbol(stream, distTree);
			
			//compute distance
			i32 distExtraBits = DistanceExtraBits[distIdx],
				dist = DistanceBase[distIdx] + stream.readNBits(distExtraBits);


			//now append characters from back reference
			for (int i = 0; i < len; i++) {
				res.push_back(res[res.size() - dist]);
				//go back only the distance since as you add nodes the lengths increase 
				//thus causing it to read from the next symbol
			}
		}
	}
}

void InflateBlockDynamic(BitStream& stream, std::vector<char>& res) {
	HuffmanTreeNode** trees = decodeTrees(stream);
	PrintTree(trees[0]);
	PrintTree(trees[1]);
	InflateBlock(stream, trees[0], trees[1], res);
	FreeTree(trees[0]);
	FreeTree(trees[1]);
	delete[] trees;
}

template<typename _Ty> void memsetbutbetter(_Ty* dest, _Ty val, size_t sz) {
	for (i32 i = 0; i < sz; i++)
		dest[i] = val;
}


void InflateBlockStatic(BitStream& stream, std::vector<char>& res) {
	//static bit lists
	u32* bitList = new u32[288];

	memsetbutbetter(bitList,     (unsigned) 8, 144);
	memsetbutbetter(bitList+144, (unsigned) 9, 112);
	memsetbutbetter(bitList+256, (unsigned) 7, 24 );
	memsetbutbetter(bitList+280, (unsigned) 8, 8  );

	HuffmanTreeNode* llTree = BitLengthsToHTree(bitList, 288, (unsigned*)CreateAlphabet(286), 286); //main tree

	//distance tree
	delete[] bitList;
	bitList = new u32[30];

	memsetbutbetter(bitList, (unsigned)5, 30);

	HuffmanTreeNode* distTree = BitLengthsToHTree(bitList, 30, (unsigned*)CreateAlphabet(30), 30);

	//inflate le data
	InflateBlock(stream, llTree, distTree, res);

	delete[] bitList, llTree, distTree;
}

void InflateBlockNone(BitStream& stream, std::vector<char>& res) {
	i32 blockSize = stream.readValue(2);
	i32 nBlockSize = stream.readValue(2);
	for (i32 i = 0; i < blockSize; i++)
		res.push_back(stream.readByte());
}

struct BData {
	char* bytes;
	size_t len;
};

BData _Inflate(BitStream& stream) {
	bool eos = false;

	std::vector<char> res;

	while (!eos) {
		i32 bFinal = stream.readBit();
		i32 bType = stream.readNBits(2);

		switch (bType) {
		case 0: {
			InflateBlockNone(stream, res);
			break;
		}
		case 1: {
			InflateBlockStatic(stream, res);
			break;
		}
		case 2: {
			InflateBlockDynamic(stream, res);
			break;
		}
		default: {
			std::cout << "Error invalid block!" << std::endl;
		}
		}

		eos = bFinal;
	}

	BData _res;

	_res.bytes = new char[res.size()];
	memcpy(_res.bytes, res.data(), res.size());
	_res.len = res.size();

	return _res;
}

ZResult Zlib::Inflate(u32* bytes, size_t len) {
	BitStream stream = BitStream(bytes, len);

	i32 compressionInfo = stream.readByte();

	//check copression mode
	i32 compressionMode = compressionInfo & 0xf;
	assert(compressionMode == 8);


	//copmression flags
	i32 compressionFlags = stream.readByte();

	//check sum for header info
	assert((compressionInfo * 256 + compressionFlags) % 31 == 0);


	//check for dictionary
	bool dictionary = (compressionFlags >> 5) & 0x1;

	if (dictionary)
		return ZResult(); //no dictionary support yet :/

	ZResult res;

	res.compressionLevel = (compressionFlags >> 6) & 0b11;
	res.compressionMode = compressionMode;

	BData iDat = _Inflate(stream);

	res.bytes = new char[iDat.len];
	memcpy(res.bytes, iDat.bytes, iDat.len);
	delete[] iDat.bytes;
	res.len = iDat.len;

	return res;
};

std::vector<HuffmanTreeNode> _SortVector(std::vector<HuffmanTreeNode>& _vec) {
	std::sort(_vec.begin(), _vec.end(), [](HuffmanTreeNode& a, HuffmanTreeNode& b) {

		/*if (a.count == b.count) {
			if (a.branch) return false;

			return true;
		}*/

		//if (a.count == b.count) {
			//return a.val > b.val;
		//}

		return a.count < b.count;
	});

	return _vec;
}

//get char count
u32* GetCharCount(u32* bytes, size_t len, size_t alphabetSize) {
	u32* _charCount = new u32[alphabetSize]; //allocate memory for the counts

	ZeroMem(_charCount, alphabetSize); //zeromemory

	for (i32 i = 0; i < len; i++) {
		if (INRANGE(bytes[i], 0, alphabetSize)) {
			_charCount[bytes[i]]++;
			std::cout << "Sym: " << bytes[i] << " " << (char) bytes[i] <<" " << _charCount[bytes[i]] <<  std::endl;
		} 
		else
			std::cout << "Error, unknown symbol: " << bytes[i] << std::endl;
	}

	return _charCount;
}

#include <queue>
#include <map>

#define SIGN(v) (v >= 0 ? 1 : -1)
#define MAX(a,b) (a >= b ? a : b)

//keep track of each node's depth with 0 beign the deepest and calculating a 
//new nodes depth with max(left, right) + 1 and when sorting use: #define smaller(tree, n, m, depth) (tree[n].Freq < tree[m].Freq || (tree[n].Freq == tree[m].Freq && depth[n] <= depth[m]))

struct treeComparison {
	bool operator()(HuffmanTreeNode* a, HuffmanTreeNode* b) {
		if (a->count == b->count) {
			//if (a->depth == b->depth) return a->val > b->val;
			return a->depth > b->depth;
		}
		return a->count > b->count;
	}
};

//TODO try priority queue
HuffmanTreeNode* GenerateBaseTree(u32* _charCount, size_t alphabetSize) {
	HuffmanTreeNode* root = nullptr;

	//
	std::priority_queue<HuffmanTreeNode*, std::vector<HuffmanTreeNode*>, treeComparison> tNodes;

	//convert char counts to temp tree vector
	for (size_t i = 0; i < alphabetSize; i++) {
		if (_charCount[i] <= 0) continue;

		HuffmanTreeNode* _node = new HuffmanTreeNode;

		std::cout << "Char Count: " << _charCount[i] << " " << (char)i << std::endl;

		_node->count = _charCount[i];
		_node->val = i;

		tNodes.push(_node);
	}

	//HuffmanTreeNode* root = nullptr;

	while (tNodes.size() > 1) {
		HuffmanTreeNode* newNode = new HuffmanTreeNode;

		HuffmanTreeNode* left = tNodes.top();
		tNodes.pop();

		HuffmanTreeNode* right = tNodes.top();
		tNodes.pop();

		newNode->left = left;
		newNode->right = right;
		newNode->val = -1;

		std::cout << "Combine: " << (char)newNode->left->val << " " << newNode->left->count << "    R: " << (char)newNode->right->val << " " << newNode->right->count << std::endl;

		newNode->depth = MAX(left->depth, right->depth) + 1;

		//memcpy(newNode->left, &left, sizeof(HuffmanTreeNode));
		//memcpy(newNode->right, &right, sizeof(HuffmanTreeNode));

		newNode->count = left->count + right->count;

		root = newNode;

		tNodes.push(newNode);
	}


	HuffmanTreeNode* fNode = tNodes.top();

	PrintTree(root);

	//memcpy(root, fNode, sizeof(HuffmanTreeNode));

	return root;
}

/**
 * 
 * getTreeBitLens
 * 
 * Get the bit lengths of each symbol in a huffman tree give
 * the tree. 
 *
 * This function calls itself recursivley
 * 
 * Params: 
 * 
 *	tree -> a pointer to the root node in a huffman tree
 *	alphabetSize -> number of characters that the tree can hold (normally 288)
 * 
 *  currentLen and cbl should not be set
 *
 */

u32* getTreeBitLens(HuffmanTreeNode* tree, size_t alphabetSize, i32 currentLen = 0, u32* cbl = nullptr) {
	if (!cbl) {
		cbl = new u32[alphabetSize];
		ZeroMem(cbl, alphabetSize);
	}

	if (tree->right != nullptr || tree->left != nullptr) {
		if (tree->left != nullptr) getTreeBitLens(tree->left, alphabetSize, currentLen + 1, cbl);
		if (tree->right != nullptr) getTreeBitLens(tree->right, alphabetSize, currentLen + 1, cbl);
	}
	else {
		cbl[tree->val] = currentLen;
	}

	return cbl;
}
/*

Function to convert a huffman tree into a canonical huffman tree
via BitLengthsToHTree

Params:

	tree -> pointer to root node of a Huffman tree
	alphabetSize -> number of characters that the tree can hold (normally 288)

*/

HuffmanTreeNode* CovnertTreeToCanonical(HuffmanTreeNode* tree, size_t alphabetSize) {
	//get bit lengths of each code
	u32* bitLens = getTreeBitLens(tree, alphabetSize);

#ifdef BALLOON_DEBUG
	std::cout << "BitLen: ";
	for (int i = 0; i < alphabetSize; i++)
		if (bitLens[i] > 0) std::cout << bitLens[i];

	std::cout << std::endl;

	std::cout << "BitDiffs: " << std::endl;

	for (int i = 0; i < alphabetSize; i++) {
		if (bitLens[i] > 0 && i < _treeLens.size() && bitLens[i] != _treeLens[i]) {
			std::cout << (char)i << "  " << i << "   " << bitLens[i] << "  " << _treeLens[i] << std::endl;
		}
	}

	std::cout << "BitLens: " << std::endl;
	for (int i = 0; i < alphabetSize; i++)
		if (bitLens[i] > 0) std::cout << "BitLen: " << bitLens[i] << " " << i << " " << (char)i << std::endl;

	std::cout << std::endl;
#endif

	//start to create le tree
	HuffmanTreeNode* cTree = BitLengthsToHTree(bitLens, alphabetSize, (u32*)CreateAlphabet(alphabetSize), alphabetSize);

	//free memory and return new tree
	delete[] bitLens;

	return cTree;
}

#define LZ77_MIN_MATCH 3 // min match size for back reference
#define LZ77_MAX_MATCH 256

const int compression_table[10][5] = {
 /*      good lazy nice max_chain_length */
 /* 0 */ {0,    0,  0,    0},    /* store only */
 /* 1 */ {4,    4,  8,    4},    /* max speed, no lazy matches */
 /* 2 */ {4,    5, 16,    8},
 /* 3 */ {4,    6, 32,   32},
 /* 4 */ {4,    4, 16,   16},    /* lazy matches */
 /* 5 */ {8,   16, 32,   32},
 /* 6 */ {8,   16, 128, 128},
 /* 7 */ {8,   32, 128, 256},
 /* 8 */ {32, 128, 258, 1024},
 /* 9 */ {32, 258, 258, 4096}    /* max compression */
};

//q is the big ol prime number
RawBytes searchBuffer(char* buf, size_t bufSz, char* query, size_t qSz, i32 q) {

}

//shift window to the left by 1
void ShiftWindow(char* win, size_t winSz, i32 amount) {
	memcpy(win, win + amount, winSz - amount);
}

/**
 *
 * LZ77 Encode
 *
 * This function is for the LZ77 step of the DEFLATE algorith.
 * This function takes in a pointer to some memory that contains
 * the bytes to be compressed along with the size of the memory
 * location and specificaiton such as the sliding window size
 *
 * This function is called during Zlib::Deflate
 *
 */

RawBytes lz77_encode(u32* bytes, size_t len, i32 lookAheadSz = 256, i32 storeSz = 4096) {
	lookAheadSz = MIN(len, lookAheadSz);

	//restult bytes
	std::vector<u32> res;

	//constants and some vars
	const i32 winSz = lookAheadSz + storeSz;
	const i32 readPos = storeSz;
	i32 bPos = 0;

	//sliding window
	char* window = new char[winSz];

	ZeroMem(window, winSz); // clear sliding window

	//initial population of window
	for (i32 i = 0; i < lookAheadSz; i++)
		window[winSz] = bytes[bPos++];

	//parse everything
	while (bPos < len + lookAheadSz) {
		//search for match
		i32 matchLength = LZ77_MIN_MATCH, bestMatch = 0;

		do {
			RawBytes r = searchBuffer(window, winSz, window + storeSz, matchLength, INT_MAX);

			if (r.len <= 0)
				break;

			bestMatch = matchLength;
		} while (matchLength++);

		//determine whether or not there is a back reference
		if (matchLength >= LZ77_MIN_MATCH) {

		}
		else
			//just add current character
			res.push_back(window[readPos]);


		//shift look window
		ShiftWindow(window, winSz, 1);
		bPos++;
		//add new char to le window
		if (bPos < len)
			window[winSz - 1] = bytes[bPos];
	}
}

/**
 *
 *  DEFLATE
 * 
 * This is the main function for deflate called with Zlib::Deflate.
 * 
 * This function takes some bytes and applies huffman coding and LZ77
 * to compress the data.
 * 
 * Idk what else to put here to sound fancy
 * 
 */

void Zlib::Deflate(u32* bytes, size_t len) {
	if (bytes == nullptr || len <= 0) return; //quick length check

	//compress the data first

}

//debug function don't call unless testing, wait, why is this here

void Huffman::DebugMain() {
	//const int testData[] = { 0x78,0x9c,0xf3,0x48,0xcd,0xc9,0xc9,0x57,0x70,0x4a,0xcc,0x03,0x42,0x45,0x00,0x20,0x1f,0x04,0x77 };
	//const int testData[] = { 120,156,243,76,83,168,204,47,85,72,78,204,83,40,74,77,76,81,40,201,200,44,86,168,202,201,76,82,0,210,229,249,69,217,153,121,233,10,149,169,152,16,0,88,5,21,205 };
	//const int testData[] = { 120,156,61,80,75,174,3,49,8,59,83,219,69,219,227,32,17,2,9,18,29,33,196,245,159,51,139,183,8,230,19,108,39,107,115,146,175,88,58,45,170,117,216,44,165,30,40,116,146,69,15,160,21,41,167,172,77,154,188,151,79,37,171,30,103,160,196,233,123,9,90,190,118,242,212,77,203,147,117,58,146,84,158,219,215,89,59,36,163,205,111,58,236,18,192,111,142,174,163,65,105,92,162,148,44,101,255,241,14,247,145,239,227,243,124,61,62,223,231,235,125,204,93,13,113,177,186,180,37,144,86,95,64,180,71,152,104,161,12,27,32,12,92,4,98,65,135,192,187,246,5,244,165,176,203,48,38,112,127,252,57,134,71,253,23,120,54,132,129,112,156,28,63,252,8,101,48,224,15,220,12,123,106 };
	const int testData[] = { 120,156,5,193,215,22,67,48,0,0,208,95,34,162,234,145,14,85,35,246,57,30,69,82,82,43,66,140,126,125,239,173,112,77,232,167,105,217,183,235,135,113,226,179,88,86,185,237,199,249,179,236,219,253,241,116,94,238,219,243,131,16,69,113,146,102,121,161,168,64,131,250,197,184,154,85,141,9,48,53,21,78,7,158,203,156,33,30,11,11,167,0,14,129,116,145,66,213,29,80,205,129,4,156,134,52,25,104,244,22,142,33,182,59,175,247,39,196,163,57,22,201,146,174,153,204,183,226,15,241,70,44,236 };
	//u32 _bitLens[] = { 2, 1, 3, 3 };
	//std::cout << "Creating Tree" << std::endl;
	//HuffmanTreeNode* testRoot = BitLengthsToHTree(_bitLens, 4, (u32*)"ABCD", 4);
	//std::cout << "Created Tree" << std::endl;

	//PrintTree(testRoot);
	//test tree
	//InsertNode(0b01, 2, 'A', testRoot);
	///InsertNode(0b1, 1, 'B', testRoot);
	//InsertNode(0b000, 3, 'C', testRoot);
	//InsertNode(0b001, 3, 'D', testRoot);

	//
	/*u32* bitBytes = BitStream::bitsToBytes(0b00010110111, 11);
	BitStream r = BitStream(bitBytes, 2);
	delete[] bitBytes;

	std::cout << "Byte: " << "  " << std::endl;
	std::cout << "Bytes: " << BitStream::bitsToBytes(0b11101000001, 11)[1] << std::endl;

	std::cout << "Symbol 1: " << (char)DecodeSymbol(r, testRoot) << std::endl;
	std::cout << "Symbol 2: " << (char)DecodeSymbol(r, testRoot) << std::endl;
	std::cout << "Symbol 3: " << (char)DecodeSymbol(r, testRoot) << std::endl;
	std::cout << "Symbol 4: " << (char)DecodeSymbol(r, testRoot) << std::endl;
	std::cout << "Symbol 5: " << (char)DecodeSymbol(r, testRoot) << std::endl;
	std::cout << "Symbol 6: " << (char)DecodeSymbol(r, testRoot) << std::endl;*/
	Zlib z;
	ZResult testRes = z.Inflate((u32*)testData, 183);

	std::cout << "Test Result: " << testRes.bytes << std::endl;

	for (int i = 0; i < testRes.len; i++)
		std::cout << testRes.bytes[i];
	
	std::cout << std::endl << " --------------------- " << std::endl << "Len: " << testRes.len << std::endl;

	/*
	
	jkdsaljojhgiouwheiguhaweiouhgaioweuhgiuahdsfjkahsdkjlghaiuwehgiuhadslkjfghaljksdghkajlsdhglkajshdgkljahsdgiuaewiluhgiuwehagiulwehgiwueuhgasidufhasdfuiasdfuiasduiasdiasdif91823189237iouhqwiuhfiuqhwfoiuhuwqfoihqweoifhuuwqoieufhoqwieufuhqhefkjlhwqefkljhljkdahsflkjahsdlfkjhasdpogaisdupogiuasdopiguasodigu
	
	*/
	//abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUV0123456789acbd29314oxbqYUiOpQrAbS24mMuIO0e1w2e3G4d2y7u9i2g5h4nNbBkKlLoOpPqQrRsStTuUvV
	const char* tBytes = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUV0123456789acbd29314oxbqYUiOpQrAbS24mMuIO0e1w2e3G4d2y7u9i2g5h4nNbBkKlLoOpPqQrRsStTuUvV";
	//const char* tBytes = "bbbacd";
	//const char* tBytes = "aaaaaaaaaabcccccccccccccccddddddd";

	u32* uBytes = new u32[134];
	
	for (int i = 0; i < 133; i++)
		uBytes[i] = (u32) tBytes[i];

	uBytes[133] = 256;

	u32* charCounts = GetCharCount(uBytes, 134, 288);

	for (int i = 0; i < 288; i++)
		if (charCounts[i] > 0) std::cout << "Char count: " << charCounts[i] << " " << i << " " << (char)i << std::endl;

	HuffmanTreeNode* baseTree = GenerateBaseTree(charCounts, 288);
	HuffmanTreeNode* testTree = CovnertTreeToCanonical(baseTree, 288);

	PrintTree(baseTree);
	PrintTree(testTree);

	//u32* bitLens = new u32[288];

	//memset(bitLens, 0, sizeof(u32) * 288);

	//GetBitLens(testTree, 0, bitLens, 288);

	//for (int i = 0; i < 288; i++)
		//if (bitLens[i] > 0) std::cout << bitLens[i];

	std::cout << std::endl;


	//testing with actual zlib
	//Bytef* out = new Bytef[4096];
	//uLongf sz = 4096;
	//compress(out, &sz, (const Bytef*)tBytes, 133);

	//ZResult testRes2 = z.Inflate((u32*)out, sz);

	//std::cout << "Test Result 2: " << testRes2.bytes << std::endl;
}