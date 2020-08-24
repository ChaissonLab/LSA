#ifndef OPTIONS_H_
#define OPTIONS_H_

const unsigned int REF_LOC=1;
const unsigned int REF_DYN=2;
const unsigned int REF_DP=4;

class Options {
public:
	enum AlignType { raw, ccs, contig};
	int globalK;
	int localK;
	int globalW;
	int localW;
	int globalMaxFreq;
	int localMaxFreq;
	int maxDiag;
	int cleanMaxDiag;
	int minClusterSize;
	int minClusterLength;
	int window;
	bool dotPlot;
	bool mergeClusters;
	bool mergeGapped;
	int minDiagCluster;
	int minRefinedClusterSize;
	bool viewPairwise;
	bool hardClip;
	string printFormat;
	//int bestn;
	bool storeAll;
	int nproc;
	string outfile;
	string outsvfile;
	int maxCandidates;
	int refineLevel;
	bool doBandedAlignment;
	int maxGap;
	int maxGapBtwnAnchors;
	int localIndexWindow;
	bool NaiveDP;
	bool SparseDP;
	bool LookUpTable;
	int readStart;
	int readStride;
	bool seqan;
	int localMatch;
 	int localMismatch;
	int localIndel;
	int localBand;
	int MergeSplit;
	int flagRemove;
	int maxRemovePairedIndelsLength; // if an anchor's length is larger than this parameter, 
									// then even if it has paired indels before and after it, we do not delete this anchor.
	int maxRemoveSpuriousAnchorsDist; 
	int minRemoveSpuriousAnchorsNum;
	int minRemoveSpuriousAnchorsLength;
	int NumAln;
	int BtnSubClusterswindow;
	int binLength;
	int minBinNum;
	bool HighlyAccurate;
	int splitdist;
	float coefficient;
	//int minimizerFreq;
	int NumOfminimizersPerWindow;
	bool Printsvsig;
	int svsigLen;
	float alnthres;
	string timing;
	int PrintNumAln;
	AlignType readType;
	bool storeTiming;
	int sseBand;
	int globalWinsize;
	int minUniqueStretchNum;
	int minUniqueStretchDist;
	float slope; 
	float rate_FirstSDPValue;
	float rate_value;
	long maxDrift;
	int minTightCluster;
	bool RefineBySDP;
	int refineSpaceDiag;
	float anchor_rate;
	int predefined_coefficient;
	float anchorstoosparse;

	Options() {
		storeTiming=false;
		readType=Options::raw;
		localMatch=4;
		localMismatch=-3;
		localIndel=-3;
		localBand=15;
		sseBand=30;
		readStart=0;
		readStride=1;
		dotPlot=false;
		globalK=17;
		globalW=10; 
		localK=7;
		localW=5;
		//bestn=1;
		globalMaxFreq=50;
		localMaxFreq=30;
		maxDiag=500; // We want maxDiag to be a small number  (used to be 500) //// For CCS, need to be smaller!!! //// lots of unmapped reads due to 500;
		cleanMaxDiag=100; 
		minDiagCluster=10; 	// used to be 20
							// This parameter is used in CleanOffDiagonal function; It's better not to set it to a single value. 
							// This parameter is used in another CleanOFFDiagonal function
							// This parameter can be deleted here

		minClusterSize=5; // For CCS, need to be larger!(20) // 5
		minClusterLength=50;  // For CCS, need to be larger!(200)
		minRefinedClusterSize=40;
		window=2000; 
		mergeGapped=false;
		viewPairwise=false;
		hardClip=false;
		printFormat="p";
		storeAll=false;
		nproc=1;
		outfile="";
		outsvfile="";
		maxCandidates=10;
		doBandedAlignment=true;
		refineLevel= REF_LOC | REF_DYN | REF_DP;
		maxGap=10000;  
		maxGapBtwnAnchors=1000; // no larger than 2000 // used to be 1500 // 1000
		mergeClusters=true;
		NaiveDP=false;
		seqan=false;
		SparseDP=true;
		LookUpTable=true;
		MergeSplit=true;
   		flagRemove=0;
   		maxRemovePairedIndelsLength=500; // used to be 50
   		//maxRemoveSpuriousAnchorsDist=200;
   		//minRemoveSpuriousAnchorsNum=15;
   		//minRemoveSpuriousAnchorsLength=100;
     	maxRemoveSpuriousAnchorsDist=500;
   		minRemoveSpuriousAnchorsNum=10;
   		NumAln = 6;//3
   		PrintNumAln = 1;
   		BtnSubClusterswindow = 800;
		binLength = 20000;
		minBinNum = 3;
		HighlyAccurate = false;
		splitdist = 100000;
		coefficient = 18;  
		//minimizerFreq = 50;
		NumOfminimizersPerWindow = 5;
		Printsvsig=false;
		svsigLen = 25;
		alnthres = 0.7;
		timing="";
		localIndexWindow=256;
		globalWinsize = 16;
		minUniqueStretchNum = 1;
		minUniqueStretchDist = 50;	
		slope=1;
		rate_FirstSDPValue=0.2;
		rate_value=0.8;
		maxDrift=400;
		minTightCluster=10;
		RefineBySDP=true;
		refineSpaceDiag=5;
		anchor_rate=1.0;
		predefined_coefficient=12;
		anchorstoosparse=0.02;
	}
};
#endif
