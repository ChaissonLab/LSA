#ifndef LINEAR_EXTEND_H
#define LINEAR_EXTEND_H

#include <vector>
#include "TupleOps.h"
#include "SplitClusters.h"

using namespace std;


void Checkbp(GenomePair & cur, GenomePair & next, Genome & genome, Read & read, int & ChromIndex, GenomePos & qe, GenomePos & te, int & strand, Options & opts, int & mat) {
	GenomePos curQ, curT, nextQ, nextT;
	if (strand == 0) {
		curQ = cur.first.pos + opts.globalK;
		curT = cur.second.pos + opts.globalK;
		nextQ = next.first.pos;
		nextT = next.second.pos;		
	}
	else {
		curQ = cur.first.pos + opts.globalK;
		curT = cur.second.pos - 1;
		nextQ = next.first.pos;
		nextT = next.second.pos + opts.globalK - 1;		
	}

	if (strand == 0) { //curT - genome.header.pos[ChromIndex]
		while (nextQ > curQ and nextT > curT and genome.seqs[ChromIndex][curT] == read.seq[curQ] ) {
			mat++;
			curQ++;
			curT++;
		}
	}
	else {
		while (nextQ > curQ and nextT < curT and genome.seqs[ChromIndex][curT] == read.seq[curQ] ) {
			mat++;
			curQ++;
			curT--;
		}
	}
	qe = curQ; te = curT;
}


int
CheckOverlap(GenomePair & match, Options & opts, vector<pair<GenomePos, bool>> & Set, bool & Ovp) {

	for (int t = 0; t < Set.size(); t++) {
		if (Set[t].second == 0 and Set[t].first >= match.first.pos and Set[t].first < match.first.pos + opts.globalK) {
			Ovp = 1;
			return 0;
		} 
		if (Set[t].second == 1 and Set[t].first >= match.second.pos and Set[t].first < match.second.pos + opts.globalK) {
			Ovp = 1;
			return 0;
		}
	}
	return 0;
}


void 
DecideCoordinates (Cluster & cluster) {

	GenomePos qStart, qEnd, tStart, tEnd;
	qStart = cluster.matches[0].first.pos;
	qEnd = qStart + cluster.matchesLengths[0];
	tStart = cluster.matches[0].second.pos;
	tEnd = tStart + cluster.matchesLengths[0];

	for (int n = 1; n < cluster.matches.size(); n++) {
		qStart = min(qStart, cluster.matches[n].first.pos);
		qEnd   = max(qEnd, cluster.matches[n].first.pos + cluster.matchesLengths[n]);
		tStart = min(tStart, cluster.matches[n].second.pos);
		tEnd   = max(tEnd, cluster.matches[n].second.pos + cluster.matchesLengths[n]);
	}
	cluster.qStart = qStart;
	cluster.qEnd = qEnd;
	cluster.tStart = tStart;
	cluster.tEnd = tEnd;
}


//
// This function implements linear extension for each Cluster on the chain;
// NOTE: The extension for anchors should avoid overlapping;
//

void
LinearExtend(vector<Cluster*> clusters, vector<Cluster> & extCluster, vector<unsigned int> & chain, Options & opts, Genome & genome, Read & read) {

	vector<pair<GenomePos, bool>> Set;
	int next, prev;
	for (int c = 0; c < chain.size(); c++) {

		int mat = 0;
		int cm = chain[c];
		if (clusters[cm]->matches.size() == 0) continue;
		Set.clear();
		prev = c - 1;
		next = c + 1;
		GenomePos qsb = clusters[cm]->qStart;
		GenomePos qeb = clusters[cm]->qEnd;
		GenomePos tsb = clusters[cm]->tStart;
		GenomePos teb = clusters[cm]->tEnd;

		//
		// Compute the ChromIndex for cluster
		//
		extCluster[c].chromIndex = clusters[cm]->chromIndex;


		//
		// Check the previous Cluster to get the overlapping points;
		//			
		GenomePos q1, q2, t1, t2;
		if (prev != -1) {
			q1 = clusters[chain[prev]]->qStart;
			q2 = clusters[chain[prev]]->qEnd;
			if (q1 >= qsb and q1 < qeb) Set.push_back(make_pair(q1, 0)); 
			if (q2 >= qsb and q2 < qeb) Set.push_back(make_pair(q2, 0));

			t1 = clusters[chain[prev]]->tStart;
			t2 = clusters[chain[prev]]->tEnd;
			if (t1 >= tsb and t1 < teb) Set.push_back(make_pair(t1, 1)); 
			if (t2 >= tsb and t2 < teb) Set.push_back(make_pair(t2, 1)); 
		} 

		//
		// Check the next Cluster to get the overlapping points;
		//
		if (next < chain.size()) {
			q1 = clusters[chain[next]]->qStart;
			q2 = clusters[chain[next]]->qEnd;
			if (q1 >= qsb and q1 < qeb) Set.push_back(make_pair(q1, 0)); 
			if (q2 >= qsb and q2 < qeb) Set.push_back(make_pair(q2, 0));

			t1 = clusters[chain[next]]->tStart;
			t2 = clusters[chain[next]]->tEnd;
			if (t1 >= tsb and t1 < teb) Set.push_back(make_pair(t1, 1)); 
			if (t2 >= tsb and t2 < teb) Set.push_back(make_pair(t2, 1)); 			
		}

		//
		// Sort each Cluster
		// NOTICE: if the current Cluster get more anchors in the btwn space or end space, we need to sort them. Otherwise we do not sort them;
		//// TODO(Jingwen): Check whether do we need to sort for every Cluster!!!!!!!!!!!!!!!
		//
		
		if (clusters[cm]->strand == 0) { 
			//if (clusters[cm]->refinespace == 1) DiagonalSort<GenomeTuple>(clusters[cm]->matches.begin(), clusters[cm]->matches.end());
			DiagonalSort<GenomeTuple>(clusters[cm]->matches.begin(), clusters[cm]->matches.end());
			//CartesianSort<GenomeTuple>(clusters[cm]->matches.begin(), clusters[cm]->matches.end());
		}
		else {
			//if (clusters[cm]->refinespace == 1) AntiDiagonalSort<GenomeTuple>(clusters[cm]->matches.begin(), clusters[cm]->matches.end());
			AntiDiagonalSort<GenomeTuple>(clusters[cm]->matches.begin(), clusters[cm]->matches.end(), read.length);
		}

		/*
		if (opts.dotPlot) {
			ofstream clust("RefinedClusters-sorted.tab");


			for (int h = 0; h < clusters[cm]->matches.size(); h++) {

				if (clusters[cm]->strand == 0) {
					clust << clusters[cm]->matches[h].first.pos << "\t"
						  << clusters[cm]->matches[h].second.pos << "\t"
						  << clusters[cm]->matches[h].first.pos + opts.globalK << "\t"
						  << clusters[cm]->matches[h].second.pos + opts.globalK << "\t"
						  << c << "\t"
						  << clusters[cm]->strand << endl;
				}
				else {
					clust << clusters[cm]->matches[h].first.pos << "\t"
						  << clusters[cm]->matches[h].second.pos + opts.globalK << "\t"
						  << clusters[cm]->matches[h].first.pos + opts.globalK << "\t"
						  << clusters[cm]->matches[h].second.pos<< "\t"
						  << c << "\t"
						  << clusters[cm]->strand << endl;					
				}
			}
			clust.close();
		}	
		*/

		
		//
		// Linear Extension;
		// NOTICE: anchors in "clusters[cm]" have the same strand;
		//
		int n = 1;
		int m = 0;
		bool chm = 1; // chm == 1 means we need to check whether m overlaps with "Set";
		while (n < clusters[cm]->matches.size()) {
			//
			// Check whether clusters[cm]->matches[m] overlaps with "Set";
			//
			bool Ovp = 0;
			if (chm == 1) {

				CheckOverlap(clusters[cm]->matches[m], opts, Set, Ovp);

				if (Ovp == 1) {
					extCluster[c].matches.push_back(GenomePair(GenomeTuple(0, clusters[cm]->matches[m].first.pos), GenomeTuple(0, clusters[cm]->matches[m].second.pos)));
					extCluster[c].matchesLengths.push_back(opts.globalK);		
					m = n;
					n++;		
					chm = 1;
					continue;	
				}
				else chm = 0;
			}

			//
			// Check whether clusters[cm]->matches[n] overlaps with "Set";
			//
			Ovp = 0;
			CheckOverlap(clusters[cm]->matches[n], opts, Set, Ovp);
			
			if (Ovp == 1) { // clusters[cm]->matches[n] overlaps with "Set";
				if (clusters[cm]->strand == 0) {
					extCluster[c].matches.push_back(GenomePair(GenomeTuple(0, clusters[cm]->matches[m].first.pos), GenomeTuple(0, clusters[cm]->matches[m].second.pos)));					
				}
				else {
					extCluster[c].matches.push_back(GenomePair(GenomeTuple(0, clusters[cm]->matches[m].first.pos), GenomeTuple(0, clusters[cm]->matches[n-1].second.pos)));					
				}
				extCluster[c].matchesLengths.push_back(clusters[cm]->matches[n-1].first.pos + opts.globalK - clusters[cm]->matches[m].first.pos);
			
				extCluster[c].matches.push_back(GenomePair(GenomeTuple(0, clusters[cm]->matches[n].first.pos), GenomeTuple(0, clusters[cm]->matches[n].second.pos)));
				extCluster[c].matchesLengths.push_back(opts.globalK);

				m = n + 1;
				n = m + 1;
				chm = 1;
				continue;
			}

			//
			// clusters[cm]->matches[n] does not overlap with "Set";
			//
			long long int curDiag, nextDiag;
			if (clusters[cm]->strand == 0) {
				curDiag = (long long int) clusters[cm]->matches[n-1].first.pos - (long long int) clusters[cm]->matches[n-1].second.pos;
				nextDiag = (long long int) clusters[cm]->matches[n].first.pos - (long long int) clusters[cm]->matches[n].second.pos;
			}
			else {
				curDiag = (long long int) clusters[cm]->matches[n-1].first.pos + (long long int) clusters[cm]->matches[n-1].second.pos;
				nextDiag = (long long int) clusters[cm]->matches[n].first.pos + (long long int) clusters[cm]->matches[n].second.pos;			
			}

			if (curDiag == nextDiag) { // those two anchors are on the same diagonal

				if (clusters[cm]->matches[n].first.pos < clusters[cm]->matches[n-1].first.pos + opts.globalK) { // anchor n-1 and anchor n are overlapped
					n++;
				}
				else { //  anchor n-1 and anchor n are not overlapped; Need to extend after anchor n-1
					GenomePos qe, te;
					Checkbp(clusters[cm]->matches[n-1], clusters[cm]->matches[n], genome, read, clusters[cm]->chromIndex, qe, te, clusters[cm]->strand, opts, mat);
					if (clusters[cm]->strand == 0 and qe == clusters[cm]->matches[n].first.pos and te == clusters[cm]->matches[n].second.pos) {
						n++;
					}
					else if (clusters[cm]->strand == 1 and qe == clusters[cm]->matches[n].first.pos and te == clusters[cm]->matches[n].second.pos + opts.globalK - 1) {
						n++;
					}
					else {
						if (clusters[cm]->strand == 0) {
							extCluster[c].matches.push_back(GenomePair(GenomeTuple(0, clusters[cm]->matches[m].first.pos), GenomeTuple(0, clusters[cm]->matches[m].second.pos)));
						}
						else {
							extCluster[c].matches.push_back(GenomePair(GenomeTuple(0, clusters[cm]->matches[m].first.pos), GenomeTuple(0, te + 1)));							
						}
						extCluster[c].matchesLengths.push_back(qe - clusters[cm]->matches[m].first.pos);
						m = n;
						n++;
					}
				}
			}
			else {
				if (clusters[cm]->strand == 0) {
					extCluster[c].matches.push_back(GenomePair(GenomeTuple(0, clusters[cm]->matches[m].first.pos), GenomeTuple(0, clusters[cm]->matches[m].second.pos)));					
				}
				else {
					extCluster[c].matches.push_back(GenomePair(GenomeTuple(0, clusters[cm]->matches[m].first.pos), GenomeTuple(0, clusters[cm]->matches[n-1].second.pos)));					
				}
				extCluster[c].matchesLengths.push_back(clusters[cm]->matches[n-1].first.pos + opts.globalK - clusters[cm]->matches[m].first.pos);
				m = n;
				n++;
			}
			chm = 0;	
		}

		if (n == clusters[cm]->matches.size()) { // and m != n-1
			if (clusters[cm]->strand == 0) {
				extCluster[c].matches.push_back(GenomePair(GenomeTuple(0, clusters[cm]->matches[m].first.pos), GenomeTuple(0, clusters[cm]->matches[m].second.pos)));
			}
			else {
				extCluster[c].matches.push_back(GenomePair(GenomeTuple(0, clusters[cm]->matches[m].first.pos), GenomeTuple(0, clusters[cm]->matches[n-1].second.pos)));
			}
			extCluster[c].matchesLengths.push_back(clusters[cm]->matches[n-1].first.pos + opts.globalK - clusters[cm]->matches[m].first.pos);		
		}

		extCluster[c].strand = clusters[cm]->strand;
		DecideCoordinates(extCluster[c]);
		//cerr <<"c: " << c << "   mat:" << mat << endl;
	
		assert(clusters[cm]->matches.size() >= extCluster[c].matches.size());
	}
}



/*
//
// This function just do linear extension without avoiding overlapping points;
//
void LinearExtend(vector<Cluster*> clusters, vector<Cluster> & extCluster, vector<unsigned int> & chain, Options & opts, Genome & genome, Read & read) {

	for (int c = 0; c < chain.size(); c++) {

		int cm = chain[c];

		//
		// Compute the ChromIndex for cluster
		//
		extCluster[c].chromIndex = clusters[cm]->chromIndex;

		//
		// Sort each Cluster
		// NOTICE: if the current Cluster get more anchors in the btwn space or end space, we need to sort them. Otherwise we do not sort them;
		//// TODO(Jingwen): Check whether do we need to sort for every Cluster!!!!!!!!!!!!!!!
		//
		if (clusters[cm]->strand == 0) { 
			//if (clusters[cm]->refinespace == 1) DiagonalSort<GenomeTuple>(clusters[cm]->matches.begin(), clusters[cm]->matches.end());
			DiagonalSort<GenomeTuple>(clusters[cm]->matches.begin(), clusters[cm]->matches.end());
		}
		else {
			//if (clusters[cm]->refinespace == 1) AntiDiagonalSort<GenomeTuple>(clusters[cm]->matches.begin(), clusters[cm]->matches.end());
			AntiDiagonalSort<GenomeTuple>(clusters[cm]->matches.begin(), clusters[cm]->matches.end(), read.length);
		}

		//
		// Linear Extension;
		// NOTICE: anchors in "clusters[cm]" have the same strand;
		//
		int n = 1;
		int m = 0;
		while (n < clusters[cm]->matches.size()) {

			long long int curDiag, nextDiag;
			if (clusters[cm]->strand == 0) {
				curDiag = (long long int) clusters[cm]->matches[n-1].first.pos - (long long int) clusters[cm]->matches[n-1].second.pos;
				nextDiag = (long long int) clusters[cm]->matches[n].first.pos - (long long int) clusters[cm]->matches[n].second.pos;
			}
			else {
				curDiag = (long long int) clusters[cm]->matches[n-1].first.pos + (long long int) clusters[cm]->matches[n-1].second.pos;
				nextDiag = (long long int) clusters[cm]->matches[n].first.pos + (long long int) clusters[cm]->matches[n].second.pos;			
			}

			if (curDiag == nextDiag) { // those two anchors are on the same diagonal

				if (clusters[cm]->matches[n].first.pos < clusters[cm]->matches[n-1].first.pos + opts.globalK) { // anchor n-1 and anchor n are overlapped
					n++;
				}
				else { //  anchor n-1 and anchor n are not overlapped; Need to extend after anchor n-1
					GenomePos qe, te;
					Checkbp(clusters[cm]->matches[n-1], clusters[cm]->matches[n], genome, read, clusters[cm]->chromIndex, qe, te, clusters[cm]->strand, opts);
					if (clusters[cm]->strand == 0 and qe == clusters[cm]->matches[n].first.pos and te == clusters[cm]->matches[n].second.pos) {
						n++;
					}
					else if (clusters[cm]->strand == 1 and qe == clusters[cm]->matches[n].first.pos and te == clusters[cm]->matches[n].second.pos + opts.globalK - 1) {
						n++;
					}
					else {
						extCluster[c].matches.push_back(GenomePair(GenomeTuple(0, clusters[cm]->matches[m].first.pos), GenomeTuple(0, clusters[cm]->matches[m].second.pos)));
						extCluster[c].matchesLengths.push_back(qe - clusters[cm]->matches[m].first.pos);
						m = n;
						n++;
					}
				}
			}
			else {
				extCluster[c].matches.push_back(GenomePair(GenomeTuple(0, clusters[cm]->matches[m].first.pos), GenomeTuple(0, clusters[cm]->matches[m].second.pos)));
				extCluster[c].matchesLengths.push_back(clusters[cm]->matches[n-1].first.pos + opts.globalK - clusters[cm]->matches[m].first.pos);
				m = n;
				n++;
			}
		}

		if (n == clusters[cm]->matches.size()) { // and m != n-1
			extCluster[c].matches.push_back(GenomePair(GenomeTuple(0, clusters[cm]->matches[m].first.pos), GenomeTuple(0, clusters[cm]->matches[m].second.pos)));
			extCluster[c].matchesLengths.push_back(clusters[cm]->matches[n-1].first.pos + opts.globalK - clusters[cm]->matches[m].first.pos);		
		}

		extCluster[c].strand = clusters[cm]->strand;
		DecideCoordinates(extCluster[c]);
	}
*/


#endif