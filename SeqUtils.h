#ifndef SEQ_UTILS_H_
#define SEQ_UTILS_H_



//
// Map from ascii to 2 bit representation.
//
static int seqMap[] = {
  0,1,2,3,0,1,2,3,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,1,0,0,0,2,
  0,0,0,0,0,0,0,0,
  0,0,0,0,3,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,1,0,0,0,2,
  0,0,0,0,0,0,0,0,
  0,0,0,0,3,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0
};

static int seqMapN[] = {
  0,1,2,3,0,1,2,3,
  4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,
  4,0,4,1,4,4,4,2,
  4,4,4,4,4,4,4,4,
  4,4,4,4,3,4,4,4,
  4,4,4,4,4,4,4,4,
  4,0,4,1,4,4,4,2,
  4,4,4,4,4,4,4,4,
  4,4,4,4,3,4,4,4,
  4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4,
  4,4,4,4,4,4,4,4
};

static int revComp[] = {
  3,2,1,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,3,0,2,0,0,0,1,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,3,0,2,0,0,0,1,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0
};

static unsigned char RevCompNuc[] = {
  'N','N','N','N','N','N','N','N',
  'N','N','N','N','N','N','N','N',
  'N','N','N','N','N','N','N','N',
  'N','N','N','N','N','N','N','N',
  'N','N','N','N','N','N','N','N',
  'N','N','N','N','N','N','N','N',
  'N','N','N','N','N','N','N','N',
  'N','N','N','N','N','N','N','N',
  'N','T','N','G','N','N','N','C',
  'N','N','N','N','N','N','N','N',
  'N','N','N','N','A','N','N','N',
  'N','N','N','N','N','N','N','N',
  'N','t','N','g','N','N','N','c',
  'N','N','N','N','N','N','n','N',
  'N','N','N','N','a','N','N','N',
  'N','N','N','N','N','N','N','N',
  'N','N','N','N','N','N','N','N',
  'N','N','N','N','N','N','N','N',
  'N','N','N','N','N','N','N','N',
  'N','N','N','N','N','N','N','N',
  'N','N','N','N','N','N','N','N',
  'N','N','N','N','N','N','N','N',
  'N','N','N','N','N','N','N','N',
  'N','N','N','N','N','N','N','N',
  'N','N','N','N','N','N','N','N',
  'N','N','N','N','N','N','N','N',
  'N','N','N','N','N','N','N','N',
  'N','N','N','N','N','N','N','N',
  'N','N','N','N','N','N','N','N',
  'N','N','N','N','N','N','N','N',
  'N','N','N','N','N','N','N','N',
  'N','N','N','N','N','N','N','N'
};



const char *binMap = "ACGT";

void CreateRC(char* seq, long l, char *& dest) {
  dest = new char[l];

  for (long i = 0; i < l; i++) {
    dest[l-i-1] = RevCompNuc[seq[i]];
  }
}

#endif
