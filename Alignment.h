#ifndef ALIGNMENT_TYPE_H_
#define ALIGNMENT_TYPE_H_

#include <vector>
#include "AlignmentBlock.h"
#include "Path.h"
#include "SeqUtils.h"
#include "Read.h"
#include "Options.h"
#include <assert.h>
#include <sstream>
#include <algorithm>
#include <math.h>       
#include <string>
using namespace std;
const unsigned int READ_UNMAPPED=0x4;
const unsigned int READ_REVERSE=0x10;
const unsigned int READ_SECONDARY=0x100;
const unsigned int READ_SUPPLEMENTARY=0x800;

class Alignment {
 public:
	unsigned char mapqv;
	unsigned int flag;
	int chromIndex;
	string chrom;
	string name;
	string queryString, alignString, refString;
	int nblocks;
	int qPos, tPos;
	vector<Block> blocks; // The positions in every block are relative to qPos and tPos;
	int nm, nmm, nins, ndel;
	int tm, tmm, tins, tdel;
	int preClip, sufClip;
	string cigar;
	bool prepared;
	char *read;
	char *forward;
	char *qual;
	int readLen;
	int refLen;
	GenomePos genomeLen;
	GenomePos wholegenomeLen;
	string readName;
	char *genome;
	int strand;
	int order;
	int runtime;
 	bool ISsecondary; // ISsecondary == 1 means this is a secondary chain. Otherwise it's a primary chain
 	bool Supplymentary; // Supplymentary == 1 means this is a Supplymentary alignment;
 	//int primary; // When ISsecondary == 1, primary stores the index of the primary chain in vector<LogCluster>
 	//vector<int> secondary; // When ISsecondary == 0, secondary stores the indices of the secondary chains	
 	float value; // base level alignment value
 	float SecondSDPValue;
 	float FirstSDPValue;
	int NumOfAnchors0; 	
	int NumOfAnchors1;
	int typeofaln; // 0 -> Primary; 1->secondary; 3-> inversion
	GenomePos qStart, qEnd, tStart, tEnd;

	void Clear() {
		queryString=alignString=refString="";
		blocks.clear();
	}
	void SetSecondary() {
		flag = flag | READ_SECONDARY;
	}
	void SetReverse() {
		flag = flag | READ_REVERSE;
	}
	void SetSupplementary() {
		flag = flag | READ_SUPPLEMENTARY;
	}
	void SetUnmapped() {
		flag = flag | READ_UNMAPPED;
	}

	Alignment() {
		runtime=0;
		qual=NULL;
		flag=0;
		mapqv=0;
		nm=nmm=nins=ndel=0;
		tm=tmm=tins=tdel=0;
		nblocks=0;
		preClip=0; sufClip=0;
		prepared=false;
		read=NULL;
		forward=NULL;
		ISsecondary=0;
		Supplymentary=0;
		value=0;
		SecondSDPValue=0;
		FirstSDPValue=0;
		order=0;
		// Will eventually contain quality value strings.
		NumOfAnchors0=0;
		NumOfAnchors1=0;
		typeofaln=0;
	}
 	Alignment(char *_read, int _rl, string _rn, char *_qual) : Alignment() {  // unaligned
		read=_read; 
		qual=_qual;
		readLen = _rl; 
		readName= _rn;
	}

 	Alignment(float _FirstSDPValue, char *_read, char *_forward, 
					 int _rl, string _rn, int _str, 
						char *_qual,
						char *_genome, GenomePos _gl, string &_chrom, int _ci) : Alignment() { 
 		FirstSDPValue = _FirstSDPValue;
		read=_read; 
		qual=_qual;
		forward=_forward;
		readLen = _rl; 
		readName= _rn;
		strand= _str;
		genome=_genome;
		genomeLen = _gl;
		chrom=_chrom;
		chromIndex=_ci;
		typeofaln=0;
		ISsecondary=0;
		Supplymentary=0;
	}

	int GetQStart() const {
		if (blocks.size() > 0) {
			return blocks[0].qPos;
		}		
		return 0;
	}
	int GetTStart() const {
		if (blocks.size() > 0) {
			return blocks[0].tPos;
		}
		return 0;
	}
	int size() const {
		return blocks.size();
	}

	void AppendAlignment(Alignment &next) {
		int qOffset = next.qPos - qPos;
		int tOffset = next.tPos - tPos;
		Block tempBlock(0,0,0);
		int n;
		for (n = 0; n < next.blocks.size(); n++ ) {
			tempBlock = next.blocks[n];
			tempBlock.qPos += qOffset;
			tempBlock.tPos += tOffset;
			blocks.push_back(tempBlock);
		}
	}

	void ArrowPathToAlignment(vector<Arrow> &optPath) {
		int q, t;
		int a = 1;
		q = 0; t = 0;
		Block b;
		a = 0;
		while (a < optPath.size()) {
			if (optPath[a] == Diagonal) {
				// Start of a block;
				b.qPos = q;
				b.tPos = t;
				b.length = 0;
				while(a < optPath.size() and optPath[a] == Diagonal) {
					b.length++;
					a++;
					t++;
					q++;
				}
				blocks.push_back(b);
			}
			while(a < optPath.size() and optPath[a] == Left) {
				t++;
				a++;
			}
			while(a < optPath.size() and optPath[a] == Up) {
				q++;
				a++;
			}
		}
	}

	void CreateAlignmentStrings(char *query, char* text, string &queryStr, string &alignStr, string &textStr) {
		GenomePos q = qPos;
		GenomePos t = tPos;
		// GenomePos qPos, tPos;
		GenomePos  g;
		char mismatchChar = '*';
		char matchChar = '|';
		char gapChar = '-';
		char gapSeparationChar = ' ';
		refLen=0;
		int refStart=0;
		textStr = "";
		alignStr = "";
		queryStr = "";

		if (blocks.size() == 0) {
			return;
		}

		if (blocks.size() > 0) {
			q=blocks[0].qPos;
			t=blocks[0].tPos;
		}

		for (int b = 0; b < blocks.size(); b++) {

			for (int bl = 0; bl < blocks[b].length; bl++) {
				assert(t < genomeLen);
				queryStr.push_back(query[q]);
				textStr.push_back(text[t]);
				if (seqMap[query[q]] != seqMap[text[t]]) alignStr.push_back(mismatchChar);
				else alignStr.push_back(matchChar);
				q++;
				t++;
			}
			//
			//  There are no gaps to count after the last block, so 
			//  don't add the gapped characters for this.
			//
			if (blocks.size() == 0)
				continue;
			if (b == blocks.size() - 1) {
				continue;
			}
			int queryGapLen = (blocks[b+1].qPos - blocks[b].qPos - blocks[b].length);
			int textGapLen  = (blocks[b+1].tPos - blocks[b].tPos - blocks[b].length);
			assert(queryGapLen >= 0);
			assert(textGapLen >= 0);
			if (queryGapLen > 0 or textGapLen > 0) {
				// commonGapLen should be the shorter gap.
				int commonGapLen = queryGapLen; 
				if (queryGapLen > textGapLen) {
					commonGapLen = textGapLen;
				}
				textGapLen -= commonGapLen;
				queryGapLen -= commonGapLen;

				for (g = 0; g < queryGapLen; g++, q++){
					assert(t < genomeLen);
					textStr.push_back(gapChar);
					alignStr.push_back(gapSeparationChar);
					queryStr.push_back(query[q]);
				}
				for (g = 0; g < textGapLen; g++, t++){
					assert(t < genomeLen);
					textStr.push_back(text[t]);
					alignStr.push_back(gapSeparationChar);
					queryStr.push_back(gapChar);
				}

				for (g = 0; g < commonGapLen; g++) {
					assert(t < genomeLen);
					textStr.push_back(text[t]);
					if (seqMap[query[q]] != seqMap[text[t]]) alignStr.push_back(mismatchChar);
					else alignStr.push_back(matchChar);

					queryStr.push_back(query[q]);
					t++;
					q++;
				}
			}
		}
		refLen=t-refStart;
	}

	// Print out sv signatures within-alignment
	void Printsvsig(char *query, char* text, Options & opts, ostream* clust) {
		stringstream svsigstrm;
		GenomePos q, t;
		GenomePos  g;

		if (blocks.size() == 0) {
			return;
		}

		for (int b = 0; b < blocks.size(); b++) {

			q = blocks[b].qPos + blocks[b].length;
			t = blocks[b].tPos + blocks[b].length;
			//
			//  There are no gaps to count after the last block, so 
			//  don't add the gapped characters for this.
			//
			if (blocks.size() == 0)
				continue;
			if (b == blocks.size() - 1) {
				continue;
			}

			int queryGapLen = (blocks[b+1].qPos - blocks[b].qPos - blocks[b].length);
			int textGapLen  = (blocks[b+1].tPos - blocks[b].tPos - blocks[b].length);
			assert(queryGapLen >= 0);
			assert(textGapLen >= 0);

			if (queryGapLen > 0 or textGapLen > 0) {
				// commonGapLen should be the shorter gap.
				int commonGapLen = queryGapLen; 
				if (queryGapLen > textGapLen) {
					commonGapLen = textGapLen;
				}
				textGapLen -= commonGapLen;
				queryGapLen -= commonGapLen;

				if (queryGapLen > opts.svsigLen) {
					svsigstrm << chrom << "\t"
							  << readName << "\t"
							  << t << "\t"
							  << t << "\t"
							  << queryGapLen << "\t"
							  << "INS" << "\t";

					for (g = 0; g < queryGapLen; g++, q++ ){
						assert(t < genomeLen);
						svsigstrm << query[q];
					}		
					svsigstrm << endl;			
				}

				if (textGapLen > opts.svsigLen) {
					svsigstrm << chrom << "\t"
							  << readName << "\t"
							  << t << "\t"
							  << t + textGapLen - 1 << "\t"
							  << textGapLen << "\t"
							  << "DEL" << "\t";
					for (g = 0; g < textGapLen; g++, t++ ){
						assert(t < genomeLen);
						svsigstrm << text[t];
					}					
					svsigstrm << endl;								
				}

				for (g = 0; g < commonGapLen; g++ ) {
					assert(t < genomeLen);
					t++;
					q++;
				}
			}
		}

		*clust << svsigstrm.str();
	}


	void AlignStringsToCigar(string &query, string &target, string &cigar, int &nm, int &nmm, int &nins, int &ndel, Options &opts, const std::vector<float> & LookUpTable) {
		stringstream cigarstrm;
		int i=0;
		int p=0;
		nm=nmm=nins=ndel=0;
		value=0;
		float coefficient = 3.0f;//3
		while (i < query.size()) {
			p=i;
			while (i < query.size() and seqMap[query[i]] == seqMap[target[i]] and query[i] != '-' and target[i] != '-') {	i++;}
			if (i > p) {
				cigarstrm << i-p << 'M';
				nm += i-p;				
				value += i-p;
				continue;
			}
			while (i < query.size() and seqMap[query[i]] != seqMap[target[i]] and query[i] != '-' and target[i] != '-') {	i++;}
			if (i > p) {
				cigarstrm << i-p << 'X';
				nmm += i-p;
				value -= i-p;
				continue;
			}
			while (i < query.size() and query[i] == '-' and target[i] != '-') {	i++;}
			if (i > p) {
				cigarstrm << i-p << 'D';
				tdel += i-p;
				ndel++;
				if (i-p <= 20) {
					value -= i-p;
				}
				else if (i-p <= 10001){
					int a = (int)floor((i-p-1)/5);
					value += -coefficient*LookUpTable[a] - 1;
				}
				else if (i-p <= 100001) {value += -1000;}
				else {value += -2000;}
				continue;
			}
			while (i < query.size() and query[i] != '-' and target[i] == '-') {	i++;}
			if (i > p) {
				cigarstrm << i-p << 'I';
				tins+=i-p;
				nins++;
				if (i-p <= 20) {
					value -= i-p;
				}
				else if (i-p <= 10001){
					int a = (int)floor((i-p-1)/5);
					value += -coefficient*LookUpTable[a] - 1;
				}
				else if (i-p <= 100001) {value += -1000;}
				else {value += -2000;}
				continue;
			}
		}
		cigar=cigarstrm.str();
		prepared=true;
	}

	void SimpleAlignStringsToCigar(string &query, string &target, string &cigar) {
		stringstream cigarstrm;
		int i=0;
		int p=0;

		while (i < query.size()) {
			p=i;
			while (i < query.size() and seqMap[query[i]] == seqMap[target[i]] and query[i] != '-' and target[i] != '-') {	i++;}
			
			if (i > p) {
				cigarstrm << i-p << 'M';
				continue;
			}
			while (i < query.size() and seqMap[query[i]] != seqMap[target[i]] and query[i] != '-' and target[i] != '-') {	i++;}
			if (i > p) {
				cigarstrm << i-p << 'X';
				continue;
			}
			while (i < query.size() and query[i] == '-' and target[i] != '-') {	i++;}
			if (i > p) {
				cigarstrm << i-p << 'D';
				//ndel+=i-p;
				continue;
			}
			while (i < query.size() and query[i] != '-' and target[i] == '-') {	i++;}
			if (i > p) {
				cigarstrm << i-p << 'I';
				//nins+=i-p;
				continue;
			}
		}
		cigar=cigarstrm.str();
		prepared=true;
	}

	void UpdateParameters(bool &str, Options &opts, const std::vector<float> & LookUpTable, ostream *svsigstrm, char *strands[2]) {
		read = strands[str];
		strand = str;
		nblocks = blocks.size();
		// CalculateStatistics(opts, svsigstrm, LookUpTable);
	}

	void CalculateStatistics(Options & opts, ostream *svsigstrm, const std::vector<float> & LookUpTable) {

		CreateAlignmentStrings(read, genome, queryString, alignString, refString);
		AlignStringsToCigar(queryString, refString, cigar, nm, nmm, ndel, nins, opts, LookUpTable);
		if (opts.Printsvsig == true) Printsvsig(read, genome, opts, svsigstrm);
		preClip = 0;
		sufClip=0;
		if (blocks.size() > 0) {
			int last=blocks.size();
			preClip = blocks[0].qPos;
			sufClip = readLen - blocks[last-1].qPos - blocks[last-1].length;
			qStart = blocks[0].qPos;
			qEnd   = blocks[last-1].qPos + blocks[last-1].length;
			tStart = blocks[0].tPos;
			tEnd   = blocks[last-1].tPos + blocks[last-1].length;
		}
	}

	//
	// Mark
	//
	void RetrieveEnd(int s){
		//
		// blocks -- which contain matches
		//
		if (s == 0) {
			// the right segment -- add to the back of blocks
		}
		else {
			// the left segment -- add to the front of blocks
		}
	}

	bool Overlaps(const Alignment &b, float frac) const {
		int ovp=0;
		if (b.qStart >= qStart and b.qStart < qEnd) {
			ovp=min(qEnd, b.qEnd)-b.qStart;
		}
		else if (b.qEnd > qStart and b.qEnd < qEnd) {
			ovp=b.qEnd-max(qStart, b.qStart);
		}
		else if (b.qStart <= qStart and b.qEnd > qEnd) {
			ovp=qEnd-qStart;
		}
		float denom=qEnd-qStart;
		if (ovp/denom > frac) { return true; }
		else { return false; }
	}

	void PrintPairwise(ostream &out) const {
		assert(prepared);				
				
		int i=0;
		int q=0;
		int t=0;
		int nBlocks = blocks.size();
		out << readName << endl;
		if (nBlocks > 0) {
			out << "Interval:\t" << chrom<< ":" << blocks[0].tPos << "-" << blocks[0].tPos +refLen << endl;
		}
		while (i < queryString.size()) {
			int end = min((int) queryString.size(), i+50);
			string qsub = queryString.substr(i,end-i);
			out.width(10);
			out << q + GetQStart() << " q: " << qsub << endl;
			q+= qsub.size() - std::count(qsub.begin(),qsub.end(),'-');
			out << "              " << alignString.substr(i,end-i) << endl;
			string tsub = refString.substr(i,end-i);
			out.width(10);
			out << t + GetTStart() << " t: " << tsub << endl;
			t+= tsub.size() - std::count(tsub.begin(), tsub.end(),'-');
			cout <<endl;
			i=end;
		}
	}

	void PrintBed(ostream &out) {
		out  << chrom << "\t" 
				 << tStart << "\t" 
				 << tEnd << "\t"
				 << (int) mapqv << "\t" 
				 << readName << "\t" << readLen << "\t" << qStart << "\t" << qEnd << "\t"
				 << nm << "\t" << nmm << "\t" << nins << "\t" << ndel << "\t" << value << "\t" << flag << "\t" << NumOfAnchors1 << "\t" << NumOfAnchors1/(float)readLen << endl;
	}

	void PrintPAF(ostream &out, bool printCigar=false) {
		char strandChar = '+';
		if (strand == 1) {
			strandChar = '-';
		}

		out << readName << "\t" << queryString.size() << "\t";
		if (strand == 0) out << "qStart:" << qStart << "\t" << "qEnd:" << qEnd << "\t";
		else out << "qStart:" << readLen - qEnd << "\t" << "qEnd:" << readLen - qStart << "\t";
		out << strandChar << "\t" << chrom << "\t" << genomeLen << "\t" 
			<< "wholegenomeLen:" << wholegenomeLen <<  "\t" // (Revision) delete wholegenomeLen
			<< "tStart:" << tStart << "\t" << "tEnd:" << tEnd 
			<< "\t" << "Number of residue matches:" << nm << "\t" << "mapqv: " << (int)mapqv << "\t" << "AO:i:" << order;
		out << "\tNM:i:" << nmm + ndel + nins << "\t";
		// out << "NX:i:" << nmm << "\t";
		// out << "ND:i:" << ndel << "\t";
		// out << "TD:i:" << tdel << "\t";
		// out << "NI:i:" << nins << "\t";
		// out << "TI:i:" << tins << "\t";
    	out << "NV:f:" << value << "\t";
		if (typeofaln == 0) {
			out << "TP:A:" << "P\t";
		}
		else if (typeofaln == 1) {
			out << "TP:A:" << "S\t";
		}
		else {
			out << "TP:A:" << "I\t";
		}
		if (NumOfAnchors1 > 0) {
			out << "\tNA:i:" << NumOfAnchors1;
		}
		if (runtime > 0) {
			out << "\tRT:i:" << runtime;
		}
		if (printCigar) {
			out << "\tCG:z:";
			char clipOp = 'S';
			if (preClip > 0) {
				out << preClip << clipOp;
			}
			out << cigar;
			if (sufClip > 0) {
				out << sufClip << clipOp;
			}

		}
		out << endl;
	}

	void PrintSAM(ostream &out, Options &opts, const vector<Alignment*> &alngroup, int as, char *passthrough=NULL) {
		stringstream samStrm;
		samStrm << readName << "\t";
		assert(prepared);
		if (blocks.size() == 0) {
			//
			// Create a null alignment
			//
			chrom="*";
			tStart=0;
			tEnd=0;
			order=0;
			samStrm << "4\t*\t0\t0\t*\t*\t0\t0\t" << string(read,readLen) << "\t*";
		}
		else {
			int last = blocks.size();
			samStrm << (unsigned int) flag << "\t" << chrom << "\t" << tStart+1 << "\t" << (unsigned int) mapqv << "\t";
			char clipOp = 'S';
			if (opts.hardClip) {
				clipOp = 'H';
			}
			if (preClip > 0) {
				samStrm << preClip << clipOp;
			}
			samStrm << cigar;
			if (sufClip > 0) {
				samStrm << sufClip << clipOp;
			}
			// Rnext, Pnext
			samStrm << "\t*\t0\t";
			// Template length
			samStrm << tEnd - tStart << "\t";
			string qualStr;
			if (opts.hardClip) {
				string subStr;
				subStr=string(read, blocks[0].qPos, blocks[last-1].qPos + blocks[last-1].length);								
				samStrm << subStr;
				if (qual != NULL and strncmp(qual,"*",1) != 0) {
					qualStr = string(qual, blocks[0].qPos, blocks[last-1].qPos + blocks[last-1].length);
				}
				else {
					qualStr= "*";
				}
			}
			else {
				string readStr(read, 0, readLen);
				samStrm << readStr;
				if (qual[0] == '*') {
					qualStr = "*";
				}
				else {
					qualStr.assign(qual, readLen);
				}
			}
			samStrm << "\t";
			if ( qual == NULL ) {
				samStrm << "*";
			}
			else {
				samStrm << qualStr;
			}
			// char clipOp = 'S';
			// if (Supplymentary and opts.hardClip) {
			// 	clipOp = 'H';				
			// }
			// if (preClip > 0) {
			// 	samStrm << preClip << clipOp;
			// }
			// samStrm << cigar;
			// if (sufClip > 0) {
			// 	samStrm << sufClip << clipOp;
			// }
			// // Rnext, Pnext
			// samStrm << "\t*\t0\t";
			// // Template length
			// samStrm << "0\t"; // samStrm << tEnd - tStart << "\t";
			// string qualStr;
			// string readStr;	
			// if (Supplymentary == 0) assert(flag == 0 or flag == 16 or flag == 256 or flag == 272);
			// if (!Supplymentary) {
			// 	readStr = string(read, readLen);
			// 	samStrm << readStr;					
			// }
			// else {
			// 	if (opts.hardClip) {
			// 		readStr = string(read + qStart, qEnd - qStart);
			// 		samStrm << readStr;						
			// 	}
			// 	else{
			// 		readStr = string(read, readLen);
			// 		samStrm << readStr;						
			// 	}	
			// }
			// if (qual[0] == '*') {
			// 	qualStr = "*";
			// }
			// else {
			// 	if (Supplymentary and opts.hardClip) qualStr = string(qual + blocks[0].qPos, blocks[last-1].qPos + blocks[last-1].length - blocks[0].qPos);
			// 	else qualStr.assign(qual, readLen);
			// }
			// samStrm << "\t";
			// if ( qual == NULL ) {
			// 	samStrm << "*";
			// }
			// else {
			// 	samStrm << qualStr;
			// }
			samStrm << "\t";
			samStrm << "NM:i:" << nmm + ndel + nins << "\t";
			// samStrm << "NX:i:" << nmm << "\t";
			// samStrm << "ND:i:" << ndel << "\t";
			// samStrm << "TD:i:" << tdel << "\t";
			// samStrm << "NI:i:" << nins << "\t";
			// samStrm << "TI:i:" << tins << "\t";
			samStrm << "NV:f:" << value << "\t";
			samStrm << "AO:i:" << order << "\t";

			// output SA tag
			if (alngroup.size() > 1) {
				samStrm << "SA:Z:";
			}
			for (int ag = alngroup.size() - 1; ag >= 0; ag--) {
				if (ag == as) {continue;}
				samStrm << alngroup[ag]->chrom << "," 
						<< alngroup[ag]->tStart + 1 << ",";
				if (alngroup[ag]->strand == 0) {
					samStrm << "+" << ",";
				}
				else {
					samStrm << "-" << ",";
				}
				if (opts.hardClip) {
					clipOp = 'H';
				}
				if (alngroup[ag]->preClip > 0) {
					samStrm << alngroup[ag]->preClip << clipOp;
				}
				samStrm << alngroup[ag]->cigar;
				if (alngroup[ag]->sufClip > 0) {
					samStrm << alngroup[ag]->sufClip << clipOp;
				}
				samStrm << ","
						<< (unsigned int) alngroup[ag]->mapqv << ","
						<< int(alngroup[ag]->nm) << ";";
			}

		}
		out << samStrm.str();
		if (opts.passthroughtag and passthrough != NULL ) {
		 // // converts character array to string 
		 //    int iq = strlen((char*)passthrough);
		 //    string passthrough_string = ""; 
		 //    for (int qt = 0; qt < iq; qt++) { 
		 //        passthrough_string.push_back(passthrough[qt]); 
		 //    } 
			out << "\t" << passthrough;
		}
		out << endl;
		out.flush();
	}

	// No SA tag
	void SimplePrintSAM(ostream &out, Options &opts, char *passthrough=NULL) {
		stringstream samStrm;
		samStrm << readName << "\t";
		if (blocks.size() == 0) {
			//
			// Create a null alignment
			//
			chrom="*";
			tStart=0;
			tEnd=0;
			order=0;
			samStrm << "4\t*\t0\t0\t*\t*\t0\t0\t" << string(read,readLen) << "\t*";
		}
		else {
			int last = blocks.size();
			samStrm << (unsigned int) flag << "\t" << chrom << "\t" << tStart+1 << "\t" << (unsigned int) mapqv << "\t";
			char clipOp = 'S';
			if (opts.hardClip) {
				clipOp = 'H';
			}
			if (preClip > 0) {
				samStrm << preClip << clipOp;
			}
			samStrm << cigar;
			if (sufClip > 0) {
				samStrm << sufClip << clipOp;
			}
			// Rnext, Pnext
			samStrm << "\t*\t0\t";
			// Template length
			samStrm << tEnd - tStart << "\t";
			string qualStr;
			if (opts.hardClip) {
				string subStr;
				subStr=string(read + blocks[0].qPos, blocks[last-1].qPos + blocks[last-1].length - blocks[0].qPos);								
				samStrm << subStr;
				if (qual != NULL and strncmp(qual,"*",1) != 0) {
					qualStr = string(qual + blocks[0].qPos, blocks[last-1].qPos + blocks[last-1].length - blocks[0].qPos);
				}
				else {
					qualStr= "*";
				}
			}
			else {
				string readStr(read, readLen);
				samStrm << readStr;
				if (qual[0] == '*') {
					qualStr = "*";
				}
				else {
					qualStr.assign(qual, readLen);
				}
			}
			samStrm << "\t";
			if ( qual == NULL ) {
				samStrm << "*";
			}
			else {
				samStrm << qualStr;
			}
			samStrm << "\t";
			samStrm << "NM:i:" << nmm + ndel + nins << "\t";
			// samStrm << "NX:i:" << nmm << "\t";
			// samStrm << "ND:i:" << ndel << "\t";
			// samStrm << "TD:i:" << tdel << "\t";
			// samStrm << "NI:i:" << nins << "\t";
			// samStrm << "TI:i:" << tins << "\t";
			samStrm << "NV:f:" << value << "\t";
			samStrm << "AO:i:" << order;
		}
		out << samStrm.str();
		if (opts.passthroughtag and passthrough != NULL ) {
			out << "\t" << passthrough;
		}
		out << endl;
		out.flush();
	}
};



class SegAlignmentGroup {
public:
	vector<Alignment*> SegAlignment;
	GenomePos qStart, qEnd, tStart, tEnd;
	unsigned char mapqv;
	int nm;
	int nmm;
	int ndel;
	int nins;
	bool ISsecondary;
	float value;
	float SecondSDPValue;
	float FirstSDPValue;
	int NumOfAnchors0; 	
	int NumOfAnchors1;

	SegAlignmentGroup () {
		qStart = 0;
		qEnd = 0;
		tStart = 0;
		tEnd = 0;
		nm = 0;
		nmm = 0;
		ndel = 0;
		nins = 0;
		ISsecondary = 0;
		value = 0;
		SecondSDPValue = 0;
		FirstSDPValue = 0;
		NumOfAnchors0 = 0; 	
		NumOfAnchors1 = 0;
	};
	~SegAlignmentGroup () {};

	void SetFromSegAlignment(Options &opts) {
		if (SegAlignment.size() == 0) {return;}
		ISsecondary = SegAlignment[0]->ISsecondary; // the SegAlignment[0] contains the info for the whole alignment
		NumOfAnchors0 =  SegAlignment[0]->NumOfAnchors0;
		FirstSDPValue = SegAlignment[0]->FirstSDPValue;
		SecondSDPValue = SegAlignment[0]->SecondSDPValue;

		for (int s = 0; s < SegAlignment.size(); s++) {
			NumOfAnchors1 +=  SegAlignment[s]->NumOfAnchors1;
			qStart = min(qStart, SegAlignment[s]->qStart);
			qEnd   = max(qEnd, SegAlignment[s]->qEnd);
			tStart = min(tStart, SegAlignment[s]->tStart);
			tEnd   = max(tEnd, SegAlignment[s]->tEnd);
			nm += SegAlignment[s]->nm;
			nmm += SegAlignment[s]->nmm;
			ndel += SegAlignment[s]->ndel;
			nins += SegAlignment[s]->nins;
			value += SegAlignment[s]->value;
			// SegAlignment[s]->SecondValue = SecondValue;
		}
		// SecondValue = opts.rate_FirstSDPValue*FirstSDPValue + opts.rate_value*value;
		int pry = 0;
		for (int s = 0; s < SegAlignment.size(); s++) {	
			if (SegAlignment[s]->Supplymentary == 0) pry++; 
		}	
		if (pry == 0) SegAlignment[0]->Supplymentary = 0;
		for (int s = 0; s < SegAlignment.size(); s++) {
			// SegAlignment[s]->SecondValue = SecondValue;
			if (s >= 1) {
				SegAlignment[s]->ISsecondary = ISsecondary;
				// SegAlignment[s]->Supplymentary = 1; already set in LcoalRefineAlignment.h
			}
			//
			// Flag that the stats are calculated for methods that need them.
			// 
			SegAlignment[s]->prepared=true;
			if (SegAlignment[s]->strand == 1) SegAlignment[s]->SetReverse(); //SegAlignment[s]->flag = SegAlignment[s]->flag | READ_REVERSE;
			if (SegAlignment[s]->Supplymentary == 1) SegAlignment[s]->SetSupplementary(); //SegAlignment[s]->flag = SegAlignment[s]->flag | READ_SUPPLEMENTARY;
		}		
	}

	bool Overlaps(const SegAlignmentGroup &b, float frac) const {
		int ovp=0;
		if (b.qStart >= qStart and b.qStart < qEnd) {
			ovp=min(qEnd, b.qEnd)-b.qStart;
		}
		else if (b.qEnd > qStart and b.qEnd < qEnd) {
			ovp=b.qEnd-max(qStart, b.qStart);
		}
		else if (b.qStart <= qStart and b.qEnd > qEnd) {
			ovp=qEnd-qStart;
		}
		float denom=qEnd-qStart;
		if (ovp/denom > frac) { return true; }
		else { return false; }
	}

	void SetMapqv () {
		for (int s = 0; s < SegAlignment.size(); s++) {
			SegAlignment[s]->mapqv = mapqv;
		}
	}

	int size() {
		return SegAlignment.size();
	}	
};


class AlignmentsOrder {
public:
	vector<SegAlignmentGroup> *alignments;
	vector<int> index;
	int Oldend;

	// constructor
	AlignmentsOrder(vector<SegAlignmentGroup> *a): alignments(a) {
		Oldend = 0;
	}

	void Update(vector<SegAlignmentGroup> *a) {
		index.resize(a->size());
		for (int i=Oldend;i < index.size(); i++) {index[i]=i;}
		Sort();
		//
		// Update the flag
		//
		(*alignments)[index[Oldend]].ISsecondary = 0;
		for (int i=Oldend+1; i<index.size(); i++) {
			(*alignments)[index[i]].ISsecondary = 1;
		}
		Oldend = index.size();
		//
		// Set primary to be the first one if on one is primary
		//
		for (int i = 0; i< (*alignments).size(); i++) {
			if ((*alignments)[i].ISsecondary == 1) {
				for (int z = 0; z < (*alignments)[i].SegAlignment.size(); z++) {
					(*alignments)[i].SegAlignment[z]->SetSecondary();
					//(*alignments)[i].SegAlignment[z]->flag = (*alignments)[i].SegAlignment[z]->flag | READ_SECONDARY;
					if ((*alignments)[i].SegAlignment[z]->typeofaln != 3) (*alignments)[i].SegAlignment[z]->typeofaln = 2;
				}
			}
		}
	}

	int operator()(const int i, const int j) {
		//return (*alignments)[i].value > (*alignments)[j].value;
		// return 0.5f * (*alignments)[i].value + 0.4f * (*alignments)[i].SecondSDPValue + 0.1f * (*alignments)[i].FirstSDPValue > 
		// 		0.5f * (*alignments)[j].value + 0.4f * (*alignments)[j].SecondSDPValue + 0.1f * (*alignments)[j].FirstSDPValue;
		//return 0.8f * (*alignments)[i].value + 0.2f * (*alignments)[i].FirstSDPValue > 0.8f * (*alignments)[j].value + 0.2f * (*alignments)[j].FirstSDPValue;
		if ((*alignments)[i].value != (*alignments)[j].value) return (*alignments)[i].value > (*alignments)[j].value;
		else return (*alignments)[i].NumOfAnchors0 > (*alignments)[j].NumOfAnchors0;
	}

	void Sort() {
		sort(index.begin()+Oldend, index.end(), *this);
	}

	SegAlignmentGroup & operator[](int i) {
		return (*alignments)[index[i]];
	}

	int size() {
		return index.size();
	}	
};


#endif
