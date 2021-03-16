#ifndef MAP_READ_H_
#define MAP_READ_H_
#include <math.h>
#include "MMIndex.h"
#include "Genome.h"
#include "Read.h"
#include "Options.h"
#include "CompareLists.h"
#include "Sorting.h"
#include "TupleOps.h"
#include "Clustering.h"
#include "AffineOneGapAlign.h"
#include "TupleOps.h"
#include "SparseDP.h"
#include "SparseDP_Forward.h"
#include "Chain.h"
#include "overload.h"
#include "LinearExtend.h"
#include "SplitClusters.h"
#include "Timing.h"
#include "ClusterRefine.h"
#include "IndelRefine.h"
#include "LocalRefineAlignment.h"

#include <iostream>
#include <algorithm>
#include <iterator>
#include <ctime>
#include <cmath>	// std::log 
#include <sstream>
#include <thread>
#include <climits>
#include <map>

using namespace std;

void RemoveOverlappingClusters(vector<Cluster> &clusters, vector<int> &clusterOrder, Options &opts) {
	int a=0;
	int ovp=a;

	if (clusters.size() == 0) {
		return;
	}
	vector<long> forDiagonals, revDiagonals;
	vector<long> *diagPtr;
	int nForCandidates=0, nRevCandidates=0;
	int maxCand=opts.maxCandidates;
	vector<bool> keep(clusters.size(), true);
	std::map<long, vector<int> > diagToCluster;
	long targetDiag=0;
	for (a=0; a < clusters.size(); a++) { 
		
		int orderIndex=clusterOrder[a];
		clusters[orderIndex].rank=a;
		float num=1.0;
		float denom=1.0;
		long diag=(long)clusters[orderIndex].tStart - (long)clusters[orderIndex].qStart;
		bool foundDiag=false;
		long clusterDiag, clusterEndDiag;
		//		cerr << "processing cluster on query " << clusters[orderIndex].qStart << "\t" << clusters[orderIndex].qEnd << "\t" << diag << "\t" << orderIndex << "\t" << clusters[orderIndex].tStart << "\t" << clusters[orderIndex].tEnd << endl;
		if (clusters[orderIndex].strand == 0) {
			diagPtr = &forDiagonals;
		}
		else {
			diagPtr = &revDiagonals;
		}
		bool encompassed=false;		
		bool onDiag=false;
		bool nearPoint=false;
		long curClusterDiag=0;
		long diagDist=0;
		long targetClusterDist=0;
		long targetDiagDist=1000;
		for (int d=0; d < diagPtr->size() and encompassed == false; d++) {							
			curClusterDiag=(*diagPtr)[d];
			assert (diagToCluster.find(curClusterDiag) != diagToCluster.end());
			assert (diagToCluster[curClusterDiag].size() > 0);
			for (int di = 0; di < diagToCluster[curClusterDiag].size(); di++) {
				int c=diagToCluster[curClusterDiag][di];
				clusterDiag=(long)clusters[c].tStart - (long) clusters[c].qStart;
				clusterEndDiag=(long)clusters[c].tEnd - (long) clusters[c].qEnd;
				long fey=(long)clusters[c].tStart - (long)clusters[orderIndex].tEnd;
				long fex=(long)clusters[c].qStart - (long)clusters[orderIndex].qEnd;
				long efy=(long)clusters[orderIndex].tStart - (long)clusters[c].tEnd;
				long efx =(long)clusters[orderIndex].tStart - (long)clusters[c].tEnd;
				long fe=(long) sqrt(fex*fex+fey*fey);
				long ef=(long) sqrt(efx*efx+efy*efy);
				diagDist=min(fe,ef);
					
				if (clusters[c].EncompassesInRectangle(clusters[orderIndex],0.5)) {
					//					cout << "cluster " << c << " encompasses " << orderIndex << endl;
					encompassed=true;
					break;
				}
				else {
					//					cout << "cluster " << c << " does not encompass " << orderIndex << "\t" << clusters[c].tEnd-clusters[c].tStart << "\t" << clusters[orderIndex].tEnd - clusters[orderIndex].tStart << endl;
				}					
				if ((abs(clusterDiag - diag) < 1000) or (abs(clusterEndDiag - diag) < 1000)) {
					foundDiag=true;					
					onDiag = true;
					targetDiag=curClusterDiag;
					targetDiagDist = min(abs(clusterDiag - diag), abs(clusterEndDiag - diag));
					//break;
				}
				if (abs(diagDist) < 1000) {
					nearPoint=true;
					targetClusterDist=diagDist;
					targetDiag=curClusterDiag;
					//break;
				}
			}

			if (encompassed == false and (onDiag==true or nearPoint==true)) {
				foundDiag=true;
				break;
			}
			else {
				if (encompassed) {
					break;
				}
			}
		}
		if (foundDiag == false and diagPtr->size() < maxCand and encompassed == false) {
			(*diagPtr).push_back(diag);
			//cerr << "Creating diagonal " << diag << "\t" << clusters[orderIndex].matches.size() << "\t" << clusters[orderIndex].tEnd - clusters[orderIndex].tStart << endl;
			diagToCluster[diag].push_back(orderIndex);
			foundDiag=true;
		}
		else if (foundDiag == true and encompassed == false) {
			/*			cerr << "Keeping match " << clusters[orderIndex].matches.size() << "\t" << orderIndex 
							<< "\ton diag " << diag << "\t" << diagDist << "\t" << (int) nearPoint << "\t" << (int) encompassed << "\t" << targetDiagDist << "\t" << targetClusterDist << "\t" << clusters[orderIndex].qStart << "\t" << clusters[orderIndex].tStart << endl;*/
			assert(targetDiag != 0);
			diagToCluster[targetDiag].push_back(orderIndex);
		}			
		else {
			//			cerr << "Discarding cluster of size " << clusters[orderIndex].matches.size() << " on diag " << diag << endl;
			clusters[orderIndex].matches.resize(0);
		}
	}
	int c=0;
	for (int i=0; i < clusters.size(); i++) {
		if (clusters[i].matches.size() > 0) {
			clusters[c] = clusters[i];
			c++;
		}
	}
	clusters.resize(c);
}


void SimpleMapQV(AlignmentsOrder &alignmentsOrder, Read &read, Options &opts) {
	if (read.unaligned) return;
	float q_coef;
	if (opts.bypassClustering) q_coef = 20.0f; // 40
	else q_coef = 1.0f; // 40
	int len = alignmentsOrder.size(); // number of primary aln and secondary aln
	for (int r = 0; r < len; r++) {
		if (r == 0 and len == 1) {
			// set mapq for each segment
			for (int s = alignmentsOrder[r].SegAlignment.size() - 1; s >= 0 ; s--) {
				float pen_cm_1 = (alignmentsOrder[r].SegAlignment[s]->NumOfAnchors0 > 20? 1.0f : 0.05f ) * alignmentsOrder[r].SegAlignment[s]->NumOfAnchors0;
				pen_cm_1 = (alignmentsOrder[r].SegAlignment[s]->NumOfAnchors0 >= 5? 1.0f : 0.1f ) * pen_cm_1;
				float identity = (float) alignmentsOrder[r].SegAlignment[s]->nm / (alignmentsOrder[r].SegAlignment[s]->nmm + 
																	alignmentsOrder[r].SegAlignment[s]->ndel + alignmentsOrder[r].SegAlignment[s]->nins);
				identity = (identity < 1? identity : 1);
				float l = ( alignmentsOrder[r].SegAlignment[s]->value > 3? logf(alignmentsOrder[r].SegAlignment[s]->value / opts.globalK) : 0);
				// long mapq = (int)(pen_cm_1 * q_coef * l);
				long mapq = (int)(pen_cm_1 * q_coef * l * identity);
				mapq = mapq > 0? mapq : 0;
				// if (1/identity >= 0.95f) {mapq = mapq < 60? mapq : 10;}
				alignmentsOrder[r].SegAlignment[s]->mapqv = mapq < 60? mapq : 60;
				if (r == 0 && len == 2 && alignmentsOrder[r].SegAlignment[s]->mapqv == 0) alignmentsOrder[r].SegAlignment[s]->mapqv = 1;
			}			
		}
		else if (r == 0 and len > 1) {
			// set mapq for each segment
			float x = (alignmentsOrder[r + 1].value) / (alignmentsOrder[r].value);
			for (int s = alignmentsOrder[r].SegAlignment.size() - 1; s >= 0 ; s--) {
				float pen_cm_1 = (alignmentsOrder[r].SegAlignment[s]->NumOfAnchors0 > 20? 1.0f : 0.05f ) * alignmentsOrder[r].SegAlignment[s]->NumOfAnchors0;
				pen_cm_1 = (alignmentsOrder[r].SegAlignment[s]->NumOfAnchors0 >= 5? 1.0f : 0.1f ) * pen_cm_1;
				float identity = (float) alignmentsOrder[r].SegAlignment[s]->nm / (alignmentsOrder[r].SegAlignment[s]->nmm + 
																	alignmentsOrder[r].SegAlignment[s]->ndel + alignmentsOrder[r].SegAlignment[s]->nins);
				float l = ( alignmentsOrder[r].SegAlignment[s]->value > 3? logf(alignmentsOrder[r].SegAlignment[s]->value / opts.globalK) : 0);
				identity = (identity < 1? identity : 1);
				long mapq = (int)(pen_cm_1 * q_coef * (1.0f - x) * l * identity);
				// long mapq = (int)(pen_cm_1 * q_coef * (1.0f - x) * l);
				mapq -= (int)(4.343f * logf(len) + .499f);
				mapq = mapq > 0? mapq : 0;
				// if (1/identity >= 0.95f) {mapq = mapq < 60? mapq : 10;}
				alignmentsOrder[r].SegAlignment[s]->mapqv = mapq < 60? mapq : 60;
				if (r == 0 && len == 2 && alignmentsOrder[r].SegAlignment[s]->mapqv == 0) alignmentsOrder[r].SegAlignment[s]->mapqv = 1;
			}
		}
		else {
			for (int s = alignmentsOrder[r].SegAlignment.size() - 1; s >= 0 ; s--) {
				alignmentsOrder[r].SegAlignment[s]->mapqv = 0;
			}
		}
	}
}

class SortClusterBySize {
public:
	bool operator()(Cluster &a, Cluster &b) {
		return a.matches.size() > b.matches.size();
	}
};

class SortAlignmentsByMatches {
public:
	bool operator()(const SegAlignmentGroup a, const SegAlignmentGroup b) const {
		return a.nm > b.nm;
	}
};

void RankClustersByScore(vector<Cluster> &clusters) {
	sort(clusters.begin(), clusters.end(), SortClusterBySize());
}

int SetStrand(Read &read, Genome &genome, Options &opts, GenomePairs &matches) { 
	int nSame=0;
	int nDifferent=0;
	for (int m=0; m< matches.size(); m++) {
		int chromIndex = genome.header.Find(matches[m].second.pos);
		char *chrom=genome.seqs[chromIndex];
		int chromPos = matches[m].second.pos - genome.header.pos[chromIndex];
		GenomeTuple readTup, genomeTup;
		StoreTuple(read.seq, matches[m].first.pos, opts.globalK, readTup);
		StoreTuple(chrom, chromPos, opts.globalK, genomeTup);
		if (readTup.t == genomeTup.t) {
			nSame++;
		}
		else {
			nDifferent++;
		}
	}
	if (nSame > nDifferent) {
		return 0;
	}
	else {
		return 1;
	}
}

template<typename T>
void SwapReadCoordinates(vector<T> &matches, GenomePos readLength, GenomePos kmer){

	for (int i=0; i < matches.size(); i++) {
		matches[i].first.pos = readLength - (matches[i].first.pos+ kmer);
	}
}

void ReverseClusterStrand(Read &read, Genome &genome, Options &opts, 
											vector<Cluster> &clusters) {
	for (int c = 0; c < clusters.size(); c++) {
			SwapStrand(read, opts, clusters[c].matches);
			clusters[c].strand = 1;
	}
}

void SetClusterStrand(Read &read, Genome &genome, Options &opts, 
											vector<Cluster> &clusters) {
	for (int c = 0; c < clusters.size(); c++) {
		clusters[c].strand = SetStrand(read, genome, opts, clusters[c].matches);
		if (clusters[c].strand == 1) {
			SwapStrand(read, opts, clusters[c].matches);
		}
	}
}

// template<typename T>
// void 
// UpdateBoundaries(T &matches, GenomePos &qStart, GenomePos &qEnd, GenomePos &tStart, GenomePos &tEnd) {
// 	for (int i =0; i< matches.size(); i++) {
// 		qStart=min(qStart, matches[i].first.pos);
// 		qEnd=max(qEnd, matches[i].first.pos);
// 		tStart=min(tStart, matches[i].second.pos);
// 		tEnd=max(tEnd, matches[i].second.pos);
// 	}
// }
// // int nSSE=0;


void 
SeparateMatchesByStrand(Read &read, Genome &genome, int k, vector<GenomePair> &allMatches,  vector<GenomePair> &forMatches,
								vector<GenomePair> &revMatches, string &baseName) {
	//
	// A value of 0 implies forward strand match.
	//
	vector<bool> strand(allMatches.size(), 0);
	int nForward=0;
	for (int i=0; i < allMatches.size(); i++) {
		int readPos = allMatches[i].first.pos; 
		uint64_t refPos = allMatches[i].second.pos;
		char *genomePtr = genome.GlobalIndexToSeq(refPos);
		//
		// Read and genome are identical, the match is in the forward strand
		if (strncmp(&read.seq[readPos], genomePtr, k) == 0) {
			nForward++;
		}
		else {
			//
			// The k-mers are not identical, but a match was stored between
			// readPos and *genomePtr, therefore the match must be reverse.
			//
			strand[i] = true;
		}
	}
	//
	// Populate two lists, one for forward matches one for reverse.
	//
	forMatches.resize(nForward);
	revMatches.resize(allMatches.size() - nForward);
	int i = 0,r = 0,f = 0;
	for (i = 0,r = 0,f = 0; i < allMatches.size(); i++) {
		if (strand[i] == 0) {
			forMatches[f] = allMatches[i];
			f++;
		}
		else {
			revMatches[r] = allMatches[i];
			r++;
		}
	}
}

//
// This function switches index in splitclusters back 
//
int
switchindex (vector<Cluster> & splitclusters, vector<Primary_chain> & Primary_chains, vector<Cluster> & clusters, Genome &genome, Read &read) {
	if (read.unaligned) return 0;
	for (int p = 0; p < Primary_chains.size(); p++) {
		for (int h = 0; h < Primary_chains[p].chains.size(); h++) {
			for (int c = 0; c < Primary_chains[p].chains[h].ch.size(); c++) {
				Primary_chains[p].chains[h].ch[c] = splitclusters[Primary_chains[p].chains[h].ch[c]].coarse;
			}
		}
	}
	//
	// Change "vector<bool> link" accordingly
	//
	for (int p = 0; p < Primary_chains.size(); p++) {
		for (int h = 0; h < Primary_chains[p].chains.size(); h++) {
			if (Primary_chains[p].chains[h].link.size() > 0) {
				vector<bool> rm(Primary_chains[p].chains[h].link.size(), 0);
				for (int c = 1; c < Primary_chains[p].chains[h].ch.size(); c++) {
					if (Primary_chains[p].chains[h].ch[c] == Primary_chains[p].chains[h].ch[c-1]) rm[c-1] = 1;
				}	

				int sm = 0; 
				for (int c = 0; c < Primary_chains[p].chains[h].link.size(); c++) {
					if (rm[c] == 0) {
						Primary_chains[p].chains[h].link[sm] = Primary_chains[p].chains[h].link[c];	
						sm++;					
					}
				}	
				Primary_chains[p].chains[h].link.resize(sm);						
			}
		}
	}
	//
	// Remove the dupplicates in consecutive group of elements
	//
	for (int p = 0; p < Primary_chains.size(); p++) {
		for (int h = 0; h < Primary_chains[p].chains.size(); h++) {
	  		vector<unsigned int>::iterator itp;
	  		itp = unique(Primary_chains[p].chains[h].ch.begin(), Primary_chains[p].chains[h].ch.end()); 
	  		int pdist = distance(Primary_chains[p].chains[h].ch.begin(), itp);
	  		Primary_chains[p].chains[h].ch.resize(pdist);			
		}
	}
	//
	// For cases like chain = {22, 125, 19, 125, 16, 17, 125, 57, 125}, compress multiple 125 to one 125;
	//
	for (int p = 0; p < Primary_chains.size(); p++) {
		for (int h = 0; h < Primary_chains[p].chains.size(); h++) {
			map<int, int> appeartimes;
			map<int, int> start_pos;
			map<int, int> end_pos;
			for (int c = 0; c < Primary_chains[p].chains[h].ch.size(); c++){
				int ats = Primary_chains[p].chains[h].ch[c];
				if (appeartimes.count(ats) > 0) {
					appeartimes[ats] += 1;
					end_pos[ats] = c + 1;
				}
				else {
					appeartimes[ats] = 1;
					start_pos[ats] = c;
					end_pos[ats] = c + 1;
				}
			}	

			if (start_pos.size() == 0) {continue;}

			vector<tuple<int, int> > start_end;
			for (std::map<int,int>::iterator ait=start_pos.begin(); ait!=start_pos.end(); ++ait) {
				if (end_pos[ait->first] > ait->second + 1) {
					start_end.push_back(make_tuple(ait->second, end_pos[ait->first]));					
				}
			}
			sort(start_end.begin(), start_end.end());

			vector<unsigned int> newch;
			vector<bool> newlink;
			int ste = 0;
			int nc = 0;
			int cur_ste_end = 0;
			while (ste < start_end.size()) {
				while (nc <= get<0>(start_end[ste])) {
					newch.push_back(Primary_chains[p].chains[h].ch[nc]);
					if (newch.size() > 1) {
						newlink.push_back(Primary_chains[p].chains[h].link[nc-1]);
					}
					nc++;					
				}
				nc = get<1>(start_end[ste]);
				//newch.push_back(Primary_chains[p].chains[h].ch[nc]); // add the end
				ste++;
			}
			while (nc < Primary_chains[p].chains[h].ch.size()) {
				newch.push_back(Primary_chains[p].chains[h].ch[nc]);
				if (newch.size() > 1) {
					newlink.push_back(Primary_chains[p].chains[h].link[nc-1]);
				}
				nc++;
			}
			Primary_chains[p].chains[h].ch = newch;
			int snch = newch.size(); int snlink = newlink.size();
			Primary_chains[p].chains[h].ch.resize(snch);
			Primary_chains[p].chains[h].link = newlink;
			Primary_chains[p].chains[h].link.resize(snlink);
		}
	}	
	//
	// Remove small clusters that are total covered by larger cluster on q coordinates
	//
	for (int p = 0; p < Primary_chains.size(); p++) {
		for (int h = 0; h < Primary_chains[p].chains.size(); h++) {
			vector<bool> cremove(Primary_chains[p].chains[h].ch.size(), 0);
			for (int c = 1; c < Primary_chains[p].chains[h].ch.size(); c++) {
				int cr = Primary_chains[p].chains[h].ch[c];
				int cp = Primary_chains[p].chains[h].ch[c - 1];
				if (cremove[c - 1] == 0 and clusters[cr].qStart >= clusters[cp].qStart and clusters[cr].qEnd <= clusters[cp].qEnd){
					cremove[c] = 1;
				}
			}
			int sc = 0;
			for (int c = 0; c < Primary_chains[p].chains[h].ch.size(); c++) {
				if (cremove[c] == 0) {
					Primary_chains[p].chains[h].ch[sc] = Primary_chains[p].chains[h].ch[c];
					if (sc >= 1) Primary_chains[p].chains[h].link[sc - 1] = Primary_chains[p].chains[h].link[c - 1];
					sc++;
				}
			}
			Primary_chains[p].chains[h].ch.resize(sc);
			Primary_chains[p].chains[h].link.resize(sc - 1);
		}
	}
	return 0;
}

//
// This function splits the chain if Clusters on the chain are mapped to different chromosomes or different locations (quite far, default: 100000) on the same chromosome;
// Also split the chain when two forward/reverse clusters are chained in reverse/forward direction.
void
SPLITChain(Read &read, vector<Cluster_SameDiag *> &ExtendClusters, vector<SplitChain> & splitchains, vector<bool> & link, Options & opts) {
	int im = 0;
	vector<unsigned int> onec; 
	vector<bool> lk;
	onec.push_back(im);
	int cur = 0, prev = 0;

	while (im < ExtendClusters.size() - 1) {
		cur = im + 1; prev = im;
		if (ExtendClusters[cur]->tStart > ExtendClusters[prev]->tEnd + opts.splitdist // too far
			or ExtendClusters[cur]->tEnd + opts.splitdist < ExtendClusters[prev]->tStart
			or (link[im] == 1 and ExtendClusters[cur]->strand == 0 and ExtendClusters[prev]->strand == 0) // repetitive mapping
			or (link[im] == 0 and ExtendClusters[cur]->strand == 1 and ExtendClusters[prev]->strand == 1) // repetitive mapping
			or (ExtendClusters[cur]->strand == 0 and ExtendClusters[prev]->strand == 1) // inversion
			or (ExtendClusters[cur]->strand == 1 and ExtendClusters[prev]->strand == 0) // inversion
			or (ExtendClusters[prev]->OverlaprateOnGenome(ExtendClusters[cur]) >= 0.3)
			or  (ExtendClusters[prev]->OverlapOnGenome(ExtendClusters[cur]) >= 100 // If two clusters overlap exceeds 0.3, then it is a DUP
				and ExtendClusters[prev]->anchorfreq <= 1.05f and ExtendClusters[cur]->anchorfreq <= 1.05f)) {  // If two clusters overlap exceeds 100bp and they are both linear, then it is a DUP
			splitchains.push_back(SplitChain(onec, lk));
			onec.clear();
			lk.clear();
			onec.push_back(cur);
		}	
		else {
			onec.push_back(cur);
			lk.push_back(link[im]);
		}	
		im++;
	}
	if (!onec.empty()) splitchains.push_back(SplitChain(onec, lk));
}

//
// This function splits the chain if Clusters on the chain are mapped to different chromosomes or different locations (quite far, default: 100000) on the same chromosome;
// Also split the chain when two forward/reverse clusters are chained in reverse/forward direction.
void
SPLITChain(Read &read, UltimateChain &chain, vector<SplitChain> &splitchains, vector<bool> &link, Options &opts) {
	int im = 0;
	vector<unsigned int> onec; 
	vector<bool> lk;
	onec.push_back(im);
	int cur = 0, prev = 0;

	while (im < chain.size() - 1) {
		cur = im + 1; prev = im;
		if (chain.tStart(cur) > chain.tEnd(prev) + opts.splitdist // too far
			or chain.tEnd(cur) + opts.splitdist < chain.tStart(prev)
			or (link[im] == 1 and chain.strand(cur) == 0 and chain.strand(prev) == 0) // repetitive mapping and DUP
			or (link[im] == 0 and chain.strand(cur)== 1 and chain.strand(prev) == 1) // repetitive mapping and DUP
			or (chain.strand(cur) == 0 and chain.strand(prev) == 1) // inversion
			or (chain.strand(cur) == 1 and chain.strand(prev) == 0)) {
			splitchains.push_back(SplitChain(onec, lk));
			onec.clear();
			lk.clear();
			onec.push_back(cur);
		}	
		else {
			onec.push_back(cur);
			lk.push_back(link[im]);
		}	
		im++;
	}
	if (!onec.empty()) splitchains.push_back(SplitChain(onec, lk));
}

int 
LargestSplitChain(vector<SplitChain> &splitchains) {
	int maxi = 0;
	for (int mi = 1; mi < splitchains.size(); mi++) {
		if (splitchains[mi].sptc.size() > splitchains[maxi].sptc.size()) {
			maxi = mi;
		}
	}
	return maxi;
}

void 
output_unaligned(Read &read, Options &opts, ostream &output) {
	// cerr << "unmapped: " << read.name << endl;
	if (opts.printFormat == "s") {
		Alignment unaligned = Alignment(read.seq, read.length, read.name, read.qual);
		unaligned.SimplePrintSAM(output, opts, read.passthrough);
	}
}

void 
OUTPUT(AlignmentsOrder &alignmentsOrder, Read &read, Options &opts, Genome &genome, ostream *output){

	if (alignmentsOrder.size() > 0 and alignmentsOrder[0].SegAlignment.size() > 0) {
	for (int a = 0; a < (int) min(alignmentsOrder.size(), opts.PrintNumAln); a++){
		int primary_num = 0;
		for (int s = alignmentsOrder[a].SegAlignment.size() - 1; s >= 0; s--) {
			if (alignmentsOrder[a].SegAlignment[s]->Supplymentary == 0) primary_num++;
			alignmentsOrder[a].SegAlignment[s]->order = alignmentsOrder[a].SegAlignment.size() - 1 - s;
			alignmentsOrder[a].SegAlignment[s]->wholegenomeLen = genome.header.pos[alignmentsOrder[a].SegAlignment[s]->chromIndex];
			if (opts.printFormat == "b") {
				alignmentsOrder[a].SegAlignment[s]->PrintBed(*output);
			}
			else if (opts.printFormat == "s") {
				alignmentsOrder[a].SegAlignment[s]->PrintSAM(*output, opts, alignmentsOrder[a].SegAlignment, s, read.passthrough);
			}
			else if (opts.printFormat == "a") {
				alignmentsOrder[a].SegAlignment[s]->PrintPairwise(*output);
			}
			else if (opts.printFormat == "p" or opts.printFormat == "pc") {
				alignmentsOrder[a].SegAlignment[s]->PrintPAF(*output, opts.printFormat == "pc");
			}
		}
		assert(primary_num == opts.PrintNumAln);
	}
	}
	else if (read.unaligned == 1) {
	output_unaligned(read, opts, *output);
	}
}

int 
MapRead(const vector<float> & LookUpTable, Read &read, Genome &genome, vector<GenomeTuple> &genomemm, LocalIndex &glIndex, Options &opts, 
				ostream *output, ostream *svsigstrm, Timing &timing, IndelRefineBuffers &indelRefineBuffers, pthread_mutex_t *semaphore=NULL) {
	read.unaligned = 0;
	string baseName = read.name;
	for (int i=0; i < baseName.size(); i++) {	
		if (baseName[i] == '/') baseName[i] = '_';	
		if (baseName[i] == '|') baseName[i] = '_';
	}
	vector<GenomeTuple> readmm; // readmm stores minimizers
	vector<pair<GenomeTuple, GenomeTuple> > allMatches, forMatches, revMatches;
	timing.Start();
	//
	// Add pointers to seq that make code more readable.
	//
	char *readRC;
	CreateRC(read.seq, read.length, readRC);
	char *strands[2] = { read.seq, readRC };

	if (opts.storeAll) {
		Options allOpts = opts;
		allOpts.globalW=1;
		StoreMinimizers<GenomeTuple, Tuple>(read.seq, read.length, allOpts.globalK, allOpts.globalW, readmm, true, false);			
	}
	else {
		StoreMinimizers<GenomeTuple, Tuple>(read.seq, read.length, opts.globalK, opts.globalW, readmm, true, false);
	}
	timing.Tick("Store minimizers");
	sort(readmm.begin(), readmm.end()); //sort kmers in readmm(minimizers)
	timing.Tick("Sort minimizers");

	// if (opts.dotPlot and !opts.readname.empty() and read.name == opts.readname) {
	// 	stringstream outNameStrm;
	// 	outNameStrm << "read.minimizers";
	// 	ofstream baseDots(outNameStrm.str().c_str());
	// 	for (int m=0; m < readmm.size(); m++) {
	// 		baseDots << readmm[m].t << "\t"
	// 				 << readmm[m].pos << "\t" 
	// 				 << readmm[m].pos + opts.globalK << endl;				
	// 	}
	// 	baseDots.close();
	// }
	// cerr << "maxGap: " << opts.maxGap << " maxDiag: " << opts.maxDiag << " cleanMaxDiag: " << opts.cleanMaxDiag << " minDiagCluster: " << opts.minDiagCluster
	// 	<< " minClusterSize: " << opts.minClusterSize << " opts.firstcoefficient: " << opts.firstcoefficient  << " opts.secondcoefficient: " << opts.secondcoefficient << endl;

	//
	// Add matches between the read and the genome.
	//
	CompareLists<GenomeTuple, Tuple>(readmm, genomemm, allMatches, opts, true);
	timing.Tick("CompareLists");

	if (opts.dotPlot and !opts.readname.empty() and read.name == opts.readname) {
		ofstream clust("all-matches.dots");
		for (int m = 0; m < allMatches.size(); m++) {
			clust << allMatches[m].first.pos << "\t" << allMatches[m].second.pos
						<< "\t" << allMatches[m].first.pos + opts.globalK << "\t" 
						<< allMatches[m].second.pos+ opts.globalK << endl;
		}
		clust.close();
	}

	SeparateMatchesByStrand(read, genome, opts.globalK, allMatches, forMatches, revMatches, baseName);
	if (forMatches.size() == 0 and revMatches.size() == 0) {
		read.unaligned = 1;
		output_unaligned(read, opts, *output);
		return 0;
	} 
	if (opts.dotPlot and !opts.readname.empty() and read.name == opts.readname) {
		ofstream fclust("for-matches_original.dots");
		for (int m = 0; m < forMatches.size(); m++) {
			fclust << forMatches[m].first.pos << "\t" << forMatches[m].second.pos << "\t" << opts.globalK + forMatches[m].first.pos << "\t"
					<< forMatches[m].second.pos + opts.globalK << "\t" << m << endl;
		}
		fclust.close();
		ofstream rclust("rev-matches_original.dots");
		for (int m=0; m < revMatches.size(); m++) {			
			rclust << revMatches[m].first.pos << "\t" << revMatches[m].second.pos + opts.globalK << "\t" << opts.globalK + revMatches[m].first.pos  << "\t"
					 << revMatches[m].second.pos << "\t" << m << endl;
		}
		rclust.close();
	}		
	vector<Cluster> clusters;



	if (opts.bypassClustering) {

		CleanMatches(forMatches, clusters, genome, read, opts, timing);
		CleanMatches(revMatches, clusters, genome, read, opts, timing, 1);
		forMatches.clear(); revMatches.clear();
		if (clusters.size() == 0) {
			read.unaligned = 1;
			output_unaligned(read, opts, *output);
			return 0;
		}
		bool NolinearCluster = true;
		for (int s = 0; s < clusters.size(); s++) {	
			if (clusters[s].anchorfreq <= 1.5f and clusters[s].matches.size() >= 10) {NolinearCluster = false; break;}
		}		
		if (NolinearCluster) {
			read.unaligned = 1;
			output_unaligned(read, opts, *output);
			return 0;
		}
		if (opts.dotPlot and !opts.readname.empty() and read.name == opts.readname) {
			ofstream cpclust("clusters-pre-remove.tab");
			for (int m = 0; m < clusters.size(); m++) {
				for (int n = 0; n < clusters[m].matches.size(); n++) {
					if (clusters[m].strand == 0) {
						cpclust << clusters[m].matches[n].first.pos << "\t"
							  << clusters[m].matches[n].second.pos << "\t"
							  << clusters[m].matches[n].first.pos + opts.globalK << "\t"
							  << clusters[m].matches[n].second.pos + opts.globalK << "\t"
							  << m << "\t"
							  << genome.header.names[clusters[m].chromIndex]<< "\t"
							  << clusters[m].strand << "\t"
							  << clusters[m].anchorfreq<< endl;
					}
					else {
						cpclust << clusters[m].matches[n].first.pos << "\t"
							  << clusters[m].matches[n].second.pos + opts.globalK << "\t"
							  << clusters[m].matches[n].first.pos + opts.globalK << "\t"
							  << clusters[m].matches[n].second.pos << "\t"
							  << m << "\t"
							  << genome.header.names[clusters[m].chromIndex]<< "\t"
							  << clusters[m].strand << "\t"
							  << clusters[m].anchorfreq<< endl;
					}				
				}
			}
			cpclust.close();
		}


		for (int s = 0; s < clusters.size(); s++) {	
			// Subtract chromOffSet from t coord.
			GenomePos chromOffset = genome.header.pos[clusters[s].chromIndex];
			for (int m = 0; m < clusters[s].matches.size(); m++) {
				clusters[s].matches[m].second.pos -= chromOffset;
			}
			clusters[s].tStart -= chromOffset;
			clusters[s].tEnd -= chromOffset;
		}
		vector<Cluster> ext_clusters(clusters.size());
		vector<UltimateChain> chains;
		//
		// Linear Extend on pure matches
		//
		for (int d = 0; d < clusters.size(); d++) {
			LinearExtend(&clusters[d].matches, ext_clusters[d].matches, ext_clusters[d].matchesLengths, opts, genome, read, clusters[d].chromIndex, clusters[d].strand, 1);
			ext_clusters[d].strand = clusters[d].strand;
			ext_clusters[d].chromIndex = clusters[d].chromIndex;
			DecideCoordinates(ext_clusters[d]);
		}
		//
		// Linear Extend efficiency
		//
		int o = 0, a = 0;
		for (int s = 0; s < clusters.size(); s++) {
			o += clusters[s].matches.size();
		}
		for (int s = 0; s < clusters.size(); s++) {
			a += ext_clusters[s].matches.size();
		}
		//cerr << "Linear Extend efficiency: " << (float) a/o << endl;
		if (opts.dotPlot and !opts.readname.empty() and read.name == opts.readname) {
			ofstream clust("ExtendClusters.tab", ofstream::app);
			for (int ep = 0; ep < ext_clusters.size(); ep++) {
				for (int eh = 0; eh < ext_clusters[ep].matches.size(); eh++) {	
					if (ext_clusters[ep].strand == 0) {
						clust << ext_clusters[ep].matches[eh].first.pos << "\t"
							  << ext_clusters[ep].matches[eh].second.pos << "\t"
							  << ext_clusters[ep].matches[eh].first.pos + ext_clusters[ep].matchesLengths[eh] << "\t"
							  << ext_clusters[ep].matches[eh].second.pos + ext_clusters[ep].matchesLengths[eh] << "\t"
							  << genome.header.names[ext_clusters[ep].chromIndex]<< "\t"
							  << ext_clusters[ep].strand << "\t"
							  << ep << endl;
					}
					else {
						clust << ext_clusters[ep].matches[eh].first.pos << "\t"
							  << ext_clusters[ep].matches[eh].second.pos + ext_clusters[ep].matchesLengths[eh] << "\t"
							  << ext_clusters[ep].matches[eh].first.pos + ext_clusters[ep].matchesLengths[eh] << "\t"
							  << ext_clusters[ep].matches[eh].second.pos<< "\t"
							  << genome.header.names[ext_clusters[ep].chromIndex]<< "\t"
							  << ext_clusters[ep].strand << "\t"
							  << ep << endl;					
					}
				}
			}
			clust.close();
		}
		for (int s = 0; s < ext_clusters.size(); s++) {	
			// Subtract chromOffSet from t coord.
			GenomePos chromOffset = genome.header.pos[clusters[s].chromIndex];
			for (int m = 0; m < ext_clusters[s].matches.size(); m++) {
				ext_clusters[s].matches[m].second.pos += chromOffset;
			}
			ext_clusters[s].tStart += chromOffset;
			ext_clusters[s].tEnd += chromOffset;
		}	
		clusters.clear();
		//
		// SDP on matches
		//
		SparseDP(ext_clusters, chains, opts, LookUpTable, read);
		if (chains.size() == 0) {
			read.unaligned = 1;
			output_unaligned(read, opts, *output);
			return 0;
		} 		
		for (int s = 0; s < ext_clusters.size(); s++) {	
			// Subtract chromOffSet from t coord.
			GenomePos chromOffset = genome.header.pos[clusters[s].chromIndex];
			for (int m = 0; m < ext_clusters[s].matches.size(); m++) {
				ext_clusters[s].matches[m].second.pos -= chromOffset;
			}
			ext_clusters[s].tStart -= chromOffset;
			ext_clusters[s].tEnd -= chromOffset;
		}
		if (opts.dotPlot and !opts.readname.empty() and read.name == opts.readname) {
			ofstream clust("SparseDP.tab", ofstream::app);
			for (int s = 0; s < chains.size(); s++) {
				for (int ep = 0; ep < chains[s].chain.size(); ep++) {
					if (chains[s].strand(ep) == 0) {
						clust << chains[s].qStart(ep) << "\t"
							  << chains[s].tStart(ep) << "\t"
							  << chains[s].qEnd(ep) << "\t"
							  << chains[s].tEnd(ep) << "\t"
							  << s << "\t"
							  << chains[s].ClusterNum(ep) << "\t"
							  << chains[s].strand(ep) << endl;
					}
					else {
						clust << chains[s].qStart(ep) << "\t"
							  << chains[s].tEnd(ep) << "\t"
							  << chains[s].qEnd(ep) << "\t"
							  << chains[s].tStart(ep) << "\t"
							  << s << "\t"
							  << chains[s].ClusterNum(ep) << "\t"
							  << chains[s].strand(ep) << endl;					
					}
				}				
			}

			clust.close();
		}	
		//
		// Close the space btwn matches
		//
		Options smallOpts=opts;
		//smallOpts.secondcoefficient=opts.predefined_coefficient; // used to be 12
		Options tinyOpts=smallOpts;
		tinyOpts.globalMaxFreq=3;
		tinyOpts.maxDiag=5;
		tinyOpts.minDiagCluster=2;
		smallOpts.globalK=glIndex.k;
		smallOpts.globalW=glIndex.w;
		smallOpts.secondcoefficient+=3; // used to be 15
		smallOpts.globalMaxFreq=6;
		smallOpts.cleanMaxDiag=10;// used to be 25
		smallOpts.maxDiag=50;
		smallOpts.maxGapBtwnAnchors=100; // used to be 200 // 200 seems a little bit large
		smallOpts.minDiagCluster=3; // used to be 3
		tinyOpts.globalK=smallOpts.globalK-3;

		vector<SegAlignmentGroup> alignments;
		AlignmentsOrder alignmentsOrder(&alignments);
		AffineAlignBuffers buff;
		for (int p = 0; p < chains.size(); p++) {
			assert(chains[p].chain.size() > 0);
			alignments.resize(alignments.size() + 1);	
			vector<SplitChain> splitchains;
			SPLITChain(read, chains[p], splitchains, chains[p].link, opts);
			int LSC = LargestSplitChain(splitchains);
			SparseDP_and_RefineAlignment_btwn_anchors(chains[p], splitchains, ext_clusters, alignments, smallOpts, 
									LookUpTable, read, strands, p, genome, LSC, tinyOpts, buff, svsigstrm);
			if (alignments.back().SegAlignment.size() == 0) {
				read.unaligned = 1;
				break;
			}
			for (int s = 0; s < alignments.back().SegAlignment.size(); s++) {
				if (opts.skipBandedRefine == false) { IndelRefineAlignment(read, genome, *alignments.back().SegAlignment[s], opts, indelRefineBuffers); }
				alignments.back().SegAlignment[s]->CalculateStatistics(smallOpts, svsigstrm, LookUpTable);
			}
			alignments.back().SetFromSegAlignment(smallOpts);
		}
		if (read.unaligned) {
			output_unaligned(read, opts, *output);
			return 0;
		} 	
		alignmentsOrder.Update(&alignments);
		SimpleMapQV(alignmentsOrder, read, smallOpts);	
		OUTPUT(alignmentsOrder, read, opts, genome, output);
		//
		// Done with one read. Clean memory.
		//
		delete[] readRC;
		for (int a = 0; a < alignments.size(); a++) {
			for (int s = 0; s < alignments[a].SegAlignment.size(); s++) {
				delete alignments[a].SegAlignment[s];
			}
		}
		if (alignments.size() > 0) {
			return 1;
		}
		return 0;
	}


	MatchesToFineClusters(forMatches, clusters, genome, read, opts, timing);
	MatchesToFineClusters(revMatches, clusters, genome, read, opts, timing, 1);
	if (opts.dotPlot and !opts.readname.empty() and read.name == opts.readname) {
		ofstream cpclust("clusters-pre-remove.tab");
		for (int m = 0; m < clusters.size(); m++) {
			for (int n = 0; n < clusters[m].matches.size(); n++) {
				if (clusters[m].strand == 0) {
					cpclust << clusters[m].matches[n].first.pos << "\t"
						  << clusters[m].matches[n].second.pos << "\t"
						  << clusters[m].matches[n].first.pos + opts.globalK << "\t"
						  << clusters[m].matches[n].second.pos + opts.globalK << "\t"
						  << m << "\t"
						  << genome.header.names[clusters[m].chromIndex]<< "\t"
						  << clusters[m].strand << endl;
				}
				else {
					cpclust << clusters[m].matches[n].first.pos << "\t"
						  << clusters[m].matches[n].second.pos + opts.globalK << "\t"
						  << clusters[m].matches[n].first.pos + opts.globalK << "\t"
						  << clusters[m].matches[n].second.pos << "\t"
						  << m << "\t"
						  << genome.header.names[clusters[m].chromIndex]<< "\t"
						  << clusters[m].strand << endl;
				}				
			}
		}
		cpclust.close();
	}
	//
	// Continue work on Clusters
	//
	if (opts.CheckTrueIntervalInFineCluster) {
		CheckTrueIntervalInFineCluster(clusters, read.name, genome, read);
	}
	if (clusters.size() == 0) {
		read.unaligned = 1;
		output_unaligned(read, opts, *output);
		return 0;
	}
	//cerr << "clusters.size(): " <<  clusters.size() << endl;


	allMatches.clear();
	forMatches.clear(); 
	revMatches.clear();

	// cerr << "before removing clusters.size(): " << clusters.size() << endl;
	// cerr << "before removing splitclusters.size(): " << splitclusters.size() << endl;
	// if (clusters.size() >= 4000) {
	// 	ClusterOrder fineClusterOrder(&clusters, 1);  // has some bug (delete clusters which should be kept -- cluster9_scaffold_58.fasta and cluster18_contig_234.fasta)
	// 	RemoveOverlappingClusters(clusters, fineClusterOrder.index, opts);
	// }
	// if (clusters.size() == 0) {
	// 	cerr << "unmapped " << read.name << endl;	
	// 	return 0; // This read cannot be mapped to the genome;
	// }

	if (opts.dotPlot and !opts.readname.empty() and read.name == opts.readname) {
		ofstream clust("clusters-post-remove.tab");
		for (int m = 0; m < clusters.size(); m++) {
			for (int n = 0; n < clusters[m].matches.size(); n++) {
				if (clusters[m].strand == 0) {
					clust << clusters[m].matches[n].first.pos << "\t"
						  << clusters[m].matches[n].second.pos << "\t"
						  << clusters[m].matches[n].first.pos + opts.globalK << "\t"
						  << clusters[m].matches[n].second.pos + opts.globalK << "\t"
						  << m << "\t"
						  << genome.header.names[clusters[m].chromIndex]<< "\t"
						  << clusters[m].strand << endl;
				}
				else {
					clust << clusters[m].matches[n].first.pos << "\t"
						  << clusters[m].matches[n].second.pos + opts.globalK << "\t"
						  << clusters[m].matches[n].first.pos + opts.globalK << "\t"
						  << clusters[m].matches[n].second.pos << "\t"
						  << m << "\t"
						  << genome.header.names[clusters[m].chromIndex]<< "\t"
						  << clusters[m].strand << endl;
				}				
			}
		}
		clust.close();

		ofstream sclust("clusters_coarse.tab");
		for (int m = 0; m < clusters.size(); m++) {
				if (clusters[m].strand == 0) {
					sclust << clusters[m].qStart << "\t"
						  << clusters[m].tStart << "\t"
						  << clusters[m].qEnd << "\t"
						  << clusters[m].tEnd << "\t"
						  << m << "\t"
						  << genome.header.names[clusters[m].chromIndex]<< "\t"
						  << clusters[m].rank << "\t"
						  << clusters[m].strand << endl;
				}
				else {
					sclust << clusters[m].qStart << "\t"
						  << clusters[m].tEnd << "\t"
						  << clusters[m].qEnd << "\t"
						  << clusters[m].tStart << "\t"
						  << m << "\t"
						  << genome.header.names[clusters[m].chromIndex]<< "\t"
						  << clusters[m].rank << "\t"
						  << clusters[m].strand << endl;
				}				
		}
		sclust.close();
	}
	//
	// Split clusters on x and y coordinates, vector<Cluster> splitclusters, add a member for each splitcluster to specify the original cluster it comes from.
	// INPUT: vector<Cluster> clusters   OUTPUT: vector<Cluster> splitclusters with member--"coarse" specify the index of the original cluster splitcluster comes from
	//
	vector<Cluster> splitclusters;
	SplitClusters(clusters, splitclusters, read);
	DecideSplitClustersValue(clusters, splitclusters, opts, read);
	if (splitclusters.size() == 0) {
		read.unaligned = 1;
		output_unaligned(read, opts, *output);
		return 0;
	}
	timing.Tick("SplitClusters");

	if (opts.dotPlot and !opts.readname.empty() and read.name == opts.readname) {
		ofstream clust("splitclusters-coarse.tab");
		for (int m = 0; m < splitclusters.size(); m++) {
			if (splitclusters[m].strand == 0) {
				clust << splitclusters[m].qStart << "\t" 
					  << splitclusters[m].tStart << "\t"
					  << splitclusters[m].qEnd   << "\t"
					  << splitclusters[m].tEnd   << "\t"
					  << m << "\t"
					  << splitclusters[m].coarse << "\t"
					  << genome.header.names[clusters[splitclusters[m].coarse].chromIndex]<< "\t"
					  << splitclusters[m].strand << endl;
			}
			else {
				clust << splitclusters[m].qStart << "\t" 
					  << splitclusters[m].tEnd << "\t"
					  << splitclusters[m].qEnd   << "\t"
					  << splitclusters[m].tStart   << "\t"
					  << m << "\t"
					  << splitclusters[m].coarse << "\t"
					  << genome.header.names[clusters[splitclusters[m].coarse].chromIndex]<< "\t"
					  << splitclusters[m].strand << endl;
			}
		}
		clust.close();
		ofstream sclust("splitclusters-decideval.tab");
		for (int m = 0; m < splitclusters.size(); m++) {
			if (splitclusters[m].Val !=0) {
				if (splitclusters[m].strand == 0) {
					sclust << splitclusters[m].qStart << "\t" 
						  << splitclusters[m].tStart << "\t"
						  << splitclusters[m].qEnd   << "\t"
						  << splitclusters[m].tEnd   << "\t"
						  << m << "\t"
						  << splitclusters[m].coarse << "\t"
						  << genome.header.names[clusters[splitclusters[m].coarse].chromIndex]<< "\t"
						  << splitclusters[m].strand << "\t"
						  << splitclusters[m].Val << endl;
				}
				else {
					sclust << splitclusters[m].qStart << "\t" 
						  << splitclusters[m].tEnd << "\t"
						  << splitclusters[m].qEnd   << "\t"
						  << splitclusters[m].tStart   << "\t"
						  << m << "\t"
						  << splitclusters[m].coarse << "\t"
					 	 << genome.header.names[clusters[splitclusters[m].coarse].chromIndex]<< "\t"
						  << splitclusters[m].strand << "\t"
						  << splitclusters[m].Val << endl;
				}				
			}
		}
		sclust.close();
	}

	//
	// Apply SDP on splitclusters. Based on the chain, clean clusters to make it only contain clusters that are on the chain.   --- vector<Cluster> clusters
	// class: chains: vector<chain> chain: vector<vector<int>>     Need parameters: PrimaryAlgnNum, SecondaryAlnNum
	// NOTICE: chains in Primary_chains do not overlap on Cluster
	//
	vector<Primary_chain> Primary_chains;
	float rate = opts.anchor_rate;
	if (splitclusters.size()/clusters.size() > 20) rate = rate / 2.0;// mapping to repetitive region

	// cerr << "clusters.size(): " << clusters.size() << " splitclusters.size(): " << splitclusters.size() << " rate: " << rate << " read.name: " << read.name << endl;

	SparseDP (splitclusters, Primary_chains, opts, LookUpTable, read, rate);
	// cerr << "1st SDP is done" << endl;
	if (Primary_chains.size() == 0) {
		read.unaligned = 1;
		output_unaligned(read, opts, *output);
		return 0;
	}	

	if (opts.dotPlot and !opts.readname.empty() and read.name == opts.readname) {
		ofstream clust("Chains.tab");
		for (int p = 0; p < Primary_chains.size(); p++) {
			for (int h = 0; h < Primary_chains[p].chains.size(); h++){
				for (int c = 0; c < Primary_chains[p].chains[h].ch.size(); c++) {
					int ph = Primary_chains[p].chains[h].ch[c];
					if (splitclusters[ph].strand == 0) {
						clust << splitclusters[ph].qStart << "\t" 
							  << splitclusters[ph].tStart << "\t"
							  << splitclusters[ph].qEnd   << "\t"
							  << splitclusters[ph].tEnd   << "\t"
							  << splitclusters[ph].Val << "\t"
							  << p << "\t"
							  << h << "\t"
							  << Primary_chains[p].chains[h].ch[c] << "\t"
							  << splitclusters[ph].strand << "\t"
							  << splitclusters[ph].NumofAnchors0 << "\t"
							  << splitclusters[ph].coarse << endl;
					} 
					else {
						clust << splitclusters[ph].qStart << "\t" 
							  << splitclusters[ph].tEnd << "\t"
							  << splitclusters[ph].qEnd   << "\t"
							  << splitclusters[ph].tStart  << "\t"
							  << splitclusters[ph].Val << "\t"							  
							  << p << "\t"
							  << h << "\t"
							  << Primary_chains[p].chains[h].ch[c] << "\t"
							  << splitclusters[ph].strand << "\t"
							  << splitclusters[ph].NumofAnchors0 << "\t"						
							  << splitclusters[ph].coarse << endl;
					}
				}
			}
		}
		clust.close();
	}	
	switchindex(splitclusters, Primary_chains, clusters, genome, read);
	timing.Tick("Sparse DP - clusters");
	splitclusters.clear();
	//
	// Remove Clusters in "clusters" that are not on the chains;
	//
	int ChainNum = 0;
	for (int p = 0; p < Primary_chains.size(); p++) {
		ChainNum += Primary_chains[p].chains.size();
	}

	vector<bool> Remove(clusters.size(), 1);
	for (int p = 0; p < Primary_chains.size(); p++) {
		for (int h = 0; h < Primary_chains[p].chains.size(); h++){
			for (int c = 0; c < Primary_chains[p].chains[h].ch.size(); c++) {
				Remove[Primary_chains[p].chains[h].ch[c]] = 0;
			}
		}
	}

	int lm = 0;
	for (int s = 0; s < clusters.size(); s++) {
		if (Remove[s] == 0) {
			clusters[lm] = clusters[s];
			lm++;
		}
	}
	clusters.resize(lm);	
	if (lm == 0) {
		read.unaligned = 1;
		output_unaligned(read, opts, *output);
		return 0;		
	}
	//
	// Change the index stored in Primary_chains, since we remove some Clusters in "clusters";
	//
	vector<int> NumOfZeros(Remove.size(), 0);
	int num = 0;
	for (int s = 0; s < Remove.size(); s++) {
		if (Remove[s] == 0) {
			num++;
			NumOfZeros[s] = num;
		}
	}

	for (int p = 0; p < Primary_chains.size(); p++) {
		for (int h = 0; h < Primary_chains[p].chains.size(); h++){
			for (int c = 0; c < Primary_chains[p].chains[h].ch.size(); c++) {
				Primary_chains[p].chains[h].ch[c] = NumOfZeros[Primary_chains[p].chains[h].ch[c]] - 1;
			}
		}
	}
	if (Primary_chains.size() == 0) {
		read.unaligned = 1;
		output_unaligned(read, opts, *output);
		return 0;
	}

	if (opts.dotPlot and !opts.readname.empty() and read.name == opts.readname) {
		ofstream clust("CoarseChains.tab");

		for (int p = 0; p < Primary_chains.size(); p++) {
			for (int h = 0; h < Primary_chains[p].chains.size(); h++){
				//cerr << "p: " << p << " h: " << h << " chr: " << genome.header.names[genome.header.Find(Primary_chains[p].chains[h].tStart)] << 
				//" value: " << Primary_chains[p].chains[h].value << " # of Anchors: " << Primary_chains[p].chains[h].NumOfAnchors << " tStart: " <<  Primary_chains[p].chains[h].tStart << endl;
				for (int c = 0; c < Primary_chains[p].chains[h].ch.size(); c++) {
					int ph = Primary_chains[p].chains[h].ch[c];
					if (clusters[ph].strand == 0) {
						clust << clusters[ph].qStart << "\t" 
							  << clusters[ph].tStart << "\t"
							  << clusters[ph].qEnd   << "\t"
							  << clusters[ph].tEnd   << "\t"
							  << p << "\t"
							  << h << "\t"
							  << Primary_chains[p].chains[h].ch[c] << "\t"
							  << genome.header.names[clusters[ph].chromIndex]<< "\t"
							  << clusters[ph].strand << endl;
					} 
					else {
						clust << clusters[ph].qStart << "\t" 
							  << clusters[ph].tEnd << "\t"
							  << clusters[ph].qEnd   << "\t"
							  << clusters[ph].tStart  << "\t"
							  << p << "\t"
							  << h << "\t"
							  << Primary_chains[p].chains[h].ch[c] << "\t"
							  << genome.header.names[clusters[ph].chromIndex]<< "\t"
							  << clusters[ph].strand << endl;
					}
				}
			}
		}
		clust.close();
	}	

	if (opts.dotPlot and !opts.readname.empty() and read.name == opts.readname) {
		ofstream clust("clusters_1stSDP.tab");
		for (int m = 0; m < clusters.size(); m++) {
			for (int n = 0; n < clusters[m].matches.size(); n++) {
				if (clusters[m].strand == 0) {
					clust << clusters[m].matches[n].first.pos << "\t"
						  << clusters[m].matches[n].second.pos << "\t"
						  << clusters[m].matches[n].first.pos + opts.globalK << "\t"
						  << clusters[m].matches[n].second.pos + opts.globalK << "\t"
						  << m << "\t"
						  << genome.header.names[clusters[m].chromIndex]<< "\t"
						  << clusters[m].strand << endl;
				}
				else {
					clust << clusters[m].matches[n].first.pos << "\t"
						  << clusters[m].matches[n].second.pos + opts.globalK << "\t"
						  << clusters[m].matches[n].first.pos + opts.globalK << "\t"
						  << clusters[m].matches[n].second.pos << "\t"
						  << m << "\t"
						  << genome.header.names[clusters[m].chromIndex]<< "\t"
						  << clusters[m].strand << endl;
				}				
			}
		}
		clust.close();
	}
	//
	// Build local index for refining alignments.
	//
	LocalIndex forwardIndex(glIndex);
	LocalIndex reverseIndex(glIndex);
	LocalIndex *localIndexes[2] = {&forwardIndex, &reverseIndex};
	forwardIndex.IndexSeq(read.seq, read.length);
	reverseIndex.IndexSeq(readRC, read.length); 
	//
	// Set the parameters for merging anchors and 1st SDP
	//
	Options smallOpts=opts;
	//smallOpts.secondcoefficient=opts.predefined_coefficient; // used to be 12
	Options tinyOpts=smallOpts;
	tinyOpts.globalMaxFreq=3;
	tinyOpts.maxDiag=5;
	tinyOpts.minDiagCluster=2;
	//
	// Decide whether the number of anchors in each Cluster is enough to skip refining step;
	//
	bool sparse = 0;
	for (int p = 0; p < clusters.size(); p++) {
		if (((float)(clusters[p].Val)/(clusters[p].qEnd - clusters[p].qStart)) < 0.01 and read.length <= 50000) sparse = 1;
		else sparse = 0;
	}
	//
	// Refining each cluster in "clusters" needed if CLR reads are aligned OR CCS reads with very few anchors. Otherwise, skip this step
	// After this step, the t coordinates in clusters and refinedclusters have been substract chromOffSet. 
	//
	vector<Cluster> refinedclusters(clusters.size());
 	vector<Cluster*> RefinedClusters(clusters.size());
	if (!opts.SkipLocalMinimizer and (opts.HighlyAccurate == false or (opts.HighlyAccurate == true and sparse == 1))) {
			
		smallOpts.globalK=glIndex.k;
		smallOpts.globalW=glIndex.w;
		smallOpts.secondcoefficient+=3; // used to be 15
		smallOpts.globalMaxFreq=6;
		smallOpts.cleanMaxDiag=10;// used to be 25
		smallOpts.maxDiag=50;
		smallOpts.maxGapBtwnAnchors=100; // used to be 200 // 200 seems a little bit large
		smallOpts.minDiagCluster=3; // used to be 3
		tinyOpts.globalK=smallOpts.globalK-3;

		REFINEclusters(clusters, refinedclusters, genome, read,  glIndex, localIndexes, smallOpts, opts);
		// cerr << "refine cluster done!" << endl;
		// refinedclusters have GenomePos, chromIndex, coarse, matches, strand, refinespace;
		for (int s = 0; s < clusters.size(); s++) {
			refinedclusters[s].anchorfreq = clusters[s].anchorfreq; // inherit anchorfreq
			RefinedClusters[s] = &refinedclusters[s];
		}
		clusters.clear();
	}
	else {
		tinyOpts.globalK=smallOpts.globalK-3;
		for (int s = 0; s < clusters.size(); s++) {	
			// Subtract chromOffSet from t coord.
			GenomePos chromOffset = genome.header.pos[clusters[s].chromIndex];
			for (int m = 0; m < clusters[s].matches.size(); m++) {
				clusters[s].matches[m].second.pos -= chromOffset;
			}
			clusters[s].tStart -= chromOffset;
			clusters[s].tEnd -= chromOffset;
			RefinedClusters[s] = &clusters[s];
		}
	}
	timing.Tick("Refine_clusters");

	if (opts.dotPlot and !opts.readname.empty() and read.name == opts.readname) {
		ofstream clust("RefinedClusters.tab", std::ofstream::app);
		for (int p = 0; p < RefinedClusters.size(); p++) {
			for (int h = 0; h < RefinedClusters[p]->matches.size(); h++) {
				if (RefinedClusters[p]->strand == 0) {
					clust << RefinedClusters[p]->matches[h].first.pos << "\t"
						  << RefinedClusters[p]->matches[h].second.pos << "\t"
						  << RefinedClusters[p]->matches[h].first.pos + smallOpts.globalK << "\t"
						  << RefinedClusters[p]->matches[h].second.pos + smallOpts.globalK << "\t"
						  << p << "\t"
						  << genome.header.names[RefinedClusters[p]->chromIndex] <<"\t"
						  << RefinedClusters[p]->strand << endl;
				}
				else {
					clust << RefinedClusters[p]->matches[h].first.pos << "\t"
						  << RefinedClusters[p]->matches[h].second.pos + smallOpts.globalK << "\t"
						  << RefinedClusters[p]->matches[h].first.pos + smallOpts.globalK << "\t"
						  << RefinedClusters[p]->matches[h].second.pos<< "\t"
						  << p << "\t"
						  << genome.header.names[RefinedClusters[p]->chromIndex] <<"\t"
						  << RefinedClusters[p]->strand << endl;					
				}
			}
		}
		clust.close();
	}	

	if (RefinedClusters.size() == 0) {
		read.unaligned = 1;
		output_unaligned(read, opts, *output);
		return 0;
	}	

	//
	// Remove RefinedCluster without matches;
	//
	for (int p = 0; p < Primary_chains.size(); p++) {
		for (int h = 0; h < Primary_chains[p].chains.size(); h++) {
			int cp = 0;
			for (int c = 0; c < Primary_chains[p].chains[h].ch.size(); c++) {
				if (RefinedClusters[Primary_chains[p].chains[h].ch[c]]->matches.size() != 0) {
					Primary_chains[p].chains[h].ch[cp] = Primary_chains[p].chains[h].ch[c];
					cp++;
				}
			}
			Primary_chains[p].chains[h].ch.resize(cp);
		}	
	}
	if (Primary_chains.size() == 0 or Primary_chains[0].chains.size() == 0) {
		read.unaligned = 1;
		output_unaligned(read, opts, *output);
		return 0;
	}	
	//
	// For each chain, check the two ends and spaces between adjacent clusters. If the spaces are too wide, go to find anchors in the banded region.
	// For each chain, we have vector<Cluster> btwnClusters to store anchors;
	// RefineBtwnClusters and Do Liear extension for anchors
	//
	vector<Cluster> RevBtwnCluster;
	vector<tuple<int, int, int> > tracerev;
	for (int p = 0; p < Primary_chains.size(); p++) {
		for (int h = 0; h < Primary_chains[p].chains.size(); h++) {
			if (Primary_chains[p].chains[h].ch.size() == 0) continue;
				RefineBtwnClusters_chain(Primary_chains, RefinedClusters, RevBtwnCluster, tracerev, genome, read, smallOpts, p, h, strands);
		}
	}	
	//
	// Add back RevBtwnCluster; Edit Primary_chains based on tracerev;
	//
	for (int p = 0; p < tracerev.size() ; p++) {
		int h = get<0>(tracerev[p]); 
		int I = get<1>(tracerev[p]); 
		Primary_chains[0].chains[h].ch.insert(Primary_chains[0].chains[h].ch.begin() + I, get<2>(tracerev[p]) + RefinedClusters.size());
		Primary_chains[0].chains[h].link.insert(Primary_chains[0].chains[h].link.begin() + (I - 1), 1);
		Primary_chains[0].chains[h].link[I] = 1;
	}
	int a = RefinedClusters.size();
	RefinedClusters.resize(a + RevBtwnCluster.size());
	for (int p = 0; p < RevBtwnCluster.size(); p++) {
		RefinedClusters[a + p] = &RevBtwnCluster[p];
	}

	timing.Tick("Refine_btwnclusters");

	vector<Cluster> extend_clusters;
	int overlap = 0;
	for (int p = 0; p < Primary_chains.size(); p++) {
		for (int h = 0; h < Primary_chains[p].chains.size(); h++) {
			if (Primary_chains[p].chains[h].ch.size() == 0) continue;
				int cur_s = extend_clusters.size();
				extend_clusters.resize(cur_s + Primary_chains[p].chains[h].ch.size());
				LinearExtend_chain(Primary_chains[p].chains[h].ch, extend_clusters, RefinedClusters, smallOpts, genome, read, cur_s, overlap);
		}
	}	
	timing.Tick("LinearExtend");

	int SizeRefinedClusters = 0, SizeExtendClusters = 0;
	for (int p = 0; p < RefinedClusters.size(); p++) {
		SizeRefinedClusters += RefinedClusters[p]->matches.size();
	}
	for (int ep = 0; ep < extend_clusters.size(); ep++) {
		SizeExtendClusters += extend_clusters[ep].matches.size();
	}	
	if (SizeRefinedClusters == 0) {
		read.unaligned = 1;
		output_unaligned(read, opts, *output);
		return 0;
	}	
	// cerr << "SizeRefinedClusters: " << SizeRefinedClusters << "   SizeExtendClusters: " << SizeExtendClusters << endl;
	// 	   << "  read.name:"<< read.name <<  endl;
	// cerr << "LinearExtend efficiency: " << (float)SizeExtendClusters/(float)SizeRefinedClusters << endl;
	// cerr << "overlapped anchors: " << overlap << " total:" << SizeExtendClusters <<endl;
	
	if (opts.dotPlot and !opts.readname.empty() and read.name == opts.readname) {
		ofstream clust("ExtendClusters.tab", ofstream::app);
		for (int ep = 0; ep < extend_clusters.size(); ep++) {
			for (int eh = 0; eh < extend_clusters[ep].matches.size(); eh++) {	
				if (extend_clusters[ep].strand == 0) {
					clust << extend_clusters[ep].matches[eh].first.pos << "\t"
						  << extend_clusters[ep].matches[eh].second.pos << "\t"
						  << extend_clusters[ep].matches[eh].first.pos + extend_clusters[ep].matchesLengths[eh] << "\t"
						  << extend_clusters[ep].matches[eh].second.pos + extend_clusters[ep].matchesLengths[eh] << "\t"
						  << genome.header.names[extend_clusters[ep].chromIndex]<< "\t"
						  << extend_clusters[ep].strand << "\t"
						  << ep << endl;
				}
				else {
					clust << extend_clusters[ep].matches[eh].first.pos << "\t"
						  << extend_clusters[ep].matches[eh].second.pos + extend_clusters[ep].matchesLengths[eh] << "\t"
						  << extend_clusters[ep].matches[eh].first.pos + extend_clusters[ep].matchesLengths[eh] << "\t"
						  << extend_clusters[ep].matches[eh].second.pos<< "\t"
						  << genome.header.names[extend_clusters[ep].chromIndex]<< "\t"
						  << extend_clusters[ep].strand << "\t"
						  << ep << endl;					
				}
			}
		}
		clust.close();
	}	

	clusters.clear();
	refinedclusters.clear();
	RefinedClusters.clear();
	RevBtwnCluster.size();

	vector<Cluster_SameDiag> samediag_clusters;
	MergeMatchesSameDiag(extend_clusters, samediag_clusters, opts); // There are a lot matches on the same diagonal, especially for contig;
	timing.Tick("Merged ExtendClusters");

	vector<SegAlignmentGroup> alignments;
	AlignmentsOrder alignmentsOrder(&alignments);
	AffineAlignBuffers buff;
	if (read.unaligned == 0) {
		int cur_cluster = 0;
		for (int p = 0; p < Primary_chains.size(); p++) {
			for (int h = 0; h < Primary_chains[p].chains.size(); h++) {
				if (Primary_chains[p].chains[h].ch.size() == 0) continue;
				alignments.resize(alignments.size() + 1);	

				vector<Cluster_SameDiag *> ExtendClusters(Primary_chains[p].chains[h].ch.size());
				for (int v = 0; v < Primary_chains[p].chains[h].ch.size(); v++) {
					// ExtendClusters[v] = &(extend_clusters[cur_cluster + v]);
					ExtendClusters[v] = &(samediag_clusters[cur_cluster + v]);
					assert(ExtendClusters[v]->coarse == cur_cluster + v);
				}

				//
				// Split the chain Primary_chains[p].chains[h] if clusters are aligned to different chromosomes; 
				// SplitAlignment is class that vector<* vector<Cluster>>
				// INPUT: vector<Cluster> ExtendClusters; OUTPUT:  vector<vector<unsigned int>> splitchain;
				//
				vector<SplitChain> splitchains;
				SPLITChain(read, ExtendClusters, splitchains, Primary_chains[p].chains[h].link, smallOpts);
				// cerr << "splitchains.size(): " << splitchains.size() << endl;
				int LSC = LargestSplitChain(splitchains);
				//
				// Apply SDP on all splitchains to get the final rough alignment path;
				// store the result in GenomePairs tupChain; 
				// We need vector<Cluster> tupClusters for tackling anchors of different strands
				// NOTICE: Insert 4 points for anchors in the overlapping regions between Clusters;
				//
				//cerr << "splitchains.size(): " << splitchains.size()  << endl;
				SparseDP_and_RefineAlignment_btwn_anchors(Primary_chains, splitchains, ExtendClusters, alignments, smallOpts, 
										LookUpTable, read, strands, p, h, genome, LSC, tinyOpts, buff, svsigstrm, extend_clusters);
				ExtendClusters.clear();

				for (int s = 0; s < alignments.back().SegAlignment.size(); s++) {
					if (opts.skipBandedRefine == false) {
						IndelRefineAlignment(read, genome, *alignments.back().SegAlignment[s], opts, indelRefineBuffers);
					}
					alignments.back().SegAlignment[s]->CalculateStatistics(smallOpts, svsigstrm, LookUpTable);
				}
				alignments.back().SetFromSegAlignment(smallOpts);
				cur_cluster += Primary_chains[p].chains[h].ch.size();
			}
			alignmentsOrder.Update(&alignments);
		}	
	}
	extend_clusters.clear();
	SimpleMapQV(alignmentsOrder, read, smallOpts);		

	timing.Tick("2nd SDP + local alignment");
	if (opts.dotPlot and !opts.readname.empty() and read.name == opts.readname) {
		ofstream baseDots("alignment.dots");
		for (int a=0; a < (int) alignmentsOrder.size(); a++){
			for (int s = 0; s < alignmentsOrder[a].SegAlignment.size(); s++) {

				for (int c = 0; c < alignmentsOrder[a].SegAlignment[s]->blocks.size(); c++) {
					if (alignmentsOrder[a].SegAlignment[s]->strand == 0) {
						baseDots << alignmentsOrder[a].SegAlignment[s]->blocks[c].qPos << "\t" 
								 << alignmentsOrder[a].SegAlignment[s]->blocks[c].tPos << "\t" 
								 << alignmentsOrder[a].SegAlignment[s]->blocks[c].qPos + alignmentsOrder[a].SegAlignment[s]->blocks[c].length << "\t" 
								 << alignmentsOrder[a].SegAlignment[s]->blocks[c].tPos + alignmentsOrder[a].SegAlignment[s]->blocks[c].length << "\t"
								 << a << "\t"
								 << s << "\t"
								 << alignmentsOrder[a].SegAlignment[s]->strand << endl;							
					} 
					else {
						baseDots << read.length - alignmentsOrder[a].SegAlignment[s]->blocks[c].qPos - alignmentsOrder[a].SegAlignment[s]->blocks[c].length << "\t" 
								 << alignmentsOrder[a].SegAlignment[s]->blocks[c].tPos + alignmentsOrder[a].SegAlignment[s]->blocks[c].length << "\t" 
								 << read.length - alignmentsOrder[a].SegAlignment[s]->blocks[c].qPos << "\t" 
								 << alignmentsOrder[a].SegAlignment[s]->blocks[c].tPos << "\t"
								 << a << "\t"
								 << s << "\t"
								 << alignmentsOrder[a].SegAlignment[s]->strand << endl;
					}
				}		
			}
		}
		baseDots.close();
	}

	if (opts.storeTiming) {
		for (int a=0; a < (int) min(alignmentsOrder.size(), opts.PrintNumAln); a++){
			for (int s = 0; s < alignmentsOrder[a].SegAlignment.size(); s++) {
				alignmentsOrder[a].SegAlignment[s]->runtime=timing.Elapsed();
			}
		}
	}
	OUTPUT(alignmentsOrder, read, opts, genome, output);


	/*
	if (semaphore != NULL ) {
		pthread_mutex_unlock(semaphore);
	}
	*/
	//
	// Done with one read. Clean memory.
	//
	delete[] readRC;
	for (int a = 0; a < alignments.size(); a++) {
		for (int s = 0; s < alignments[a].SegAlignment.size(); s++) {
			delete alignments[a].SegAlignment[s];
		}
	}
	//read.Clear();
	if (alignments.size() > 0) return 1;
	return 0;
}

#endif
