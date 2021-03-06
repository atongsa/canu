
/******************************************************************************
 *
 *  This file is part of canu, a software program that assembles whole-genome
 *  sequencing reads into contigs.
 *
 *  This software is based on:
 *    'Celera Assembler' (http://wgs-assembler.sourceforge.net)
 *    the 'kmer package' (http://kmer.sourceforge.net)
 *  both originally distributed by Applera Corporation under the GNU General
 *  Public License, version 2.
 *
 *  Canu branched from Celera Assembler at its revision 4587.
 *  Canu branched from the kmer project at its revision 1994.
 *
 *  Modifications by:
 *
 *    Brian P. Walenz beginning on 2016-OCT-25
 *      are a 'United States Government Work', and
 *      are released in the public domain
 *
 *  File 'README.licenses' in the root directory of this distribution contains
 *  full conditions and disclaimers for each license.
 */

#ifndef AS_OVSTOREHISTOGRAM_H
#define AS_OVSTOREHISTOGRAM_H

//  Automagically gathers statistics on overlaps as they're written:
//    from overlappers, the number of overlaps per read.
//    in the store, the number of overlaps per (evalue,overlapLength)

#include "AS_global.H"
#include "sqStore.H"
#include "ovStoreFile.H"  //  For ovFileType.


#define  N_OVL_SCORE   16   //  Number of overlap scores to save per read


//  Points to estimate the overlap score function for each read.
//  This is populated when overlaps are written to store files.
//  Since each store file only has a small subset of reads,
//  we don't want to allocate one of these for every read in the store,
//  only those reads we see in this file.
//
//  When the store is finalized, all the pieces will be collected
//  into one list.
//
class oSH_ovlSco {
public:
  oSH_ovlSco()  {}
  ~oSH_ovlSco() {};

  uint16    points[N_OVL_SCORE];
  uint16    scores[N_OVL_SCORE];
};



//  There are two types of histograms.
//
//  For ovFileFullWrite   - overlapper output
//     the number of overlaps for each read
//
//  For ovFileNormalWrite - ovlStore files
//     an erateXlength histogram
//     scores for each read
//
//  The parallel store makes the scores complicated, because we don't want
//  to keep scores for reads not in each piece.  When merging, we need
//  to copy scores in, allocating more space for them as needed.

class ovStoreHistogram {
public:
  ~ovStoreHistogram();
  ovStoreHistogram(sqStore *seq);            //  For writing data, allocates as needed.  Also for merging data.
  ovStoreHistogram(const char *path);        //  For loading data, read-only.

  static
  char     *createDataName(char *name, const char *prefix);

  void      saveHistogram(char *prefix);     //  Write data to a file.

  //
  //  For the first constructor, merge in data from another histogram.
  //

private:
  void      mergeOPEL(ovStoreHistogram *other);
  void      mergeScores(ovStoreHistogram *other);
public:
  void      mergeHistogram(ovStoreHistogram *other) {
    mergeOPEL(other);
    mergeScores(other);
  };

  //
  //  For the second constructor:
  //    add a single overlap to the data.
  //

private:
  void      processScores(uint32 Aid=UINT32_MAX);
public:
  void      addOverlap(ovOverlap *overlap);

  //
  //  For erate X length histogram.
  //

  uint32    numEvalueBuckets(void)     {  return(AS_MAX_EVALUE + 1);  };
  uint32    numLengthBuckets(void)     {  return(_opelLen);           };

  uint32    evaluePerBucket(void)      {  return(_epb);  };
  uint32    basesPerBucket(void)       {  return(_bpb);  };

  uint32    numOverlaps(uint32 eb, uint32 lb) {
    assert(eb < numEvalueBuckets());
    assert(lb < numLengthBuckets());

    return((_opel[eb] == NULL) ? 0 : _opel[eb][lb]);
  };

  uint32    maxEvalue(void);
  double    maxErate(void);
  uint32    maxLength(void);

  void      dumpEvalueLength(FILE *out);  //  gnuplot-friendly dump of the evalues-length.

  //
  //  For score data.
  //

  uint32    overlapScoresBaseID(void) { return(_scoresBaseID); };
  uint32    overlapScoresLastID(void) { return(_scoresLastID); };

  uint16    overlapScoreEstimate(uint32 id, uint32 i, FILE *scoreDumpFile=NULL);

private:
  sqStore     *_seq;
  uint32       _maxID;          //  Highest read ID in this assembly.

  //  Overlaps per evalue-length, for overlaps in the store.

  uint32       _epb;            //  Evalues per bucket
  uint32       _bpb;            //  Bases per bucket

  uint32       _opelLen;        //  Length of the data vector for one evalue
  uint32     **_opel;           //  Overlaps per evalue-length

  //  Overlap score for the top overlaps.  Used during correction.
  //  Want to store ~11 values per read, 16 bits each, so 22 bytes.
  //  Human has 14,625,060 reads -> 160,875,660 bytes data.
  //  UINT32_MAX indicates there is no more data.

  uint32       _scoresListLen;  //  Temporary data for collecting overlap scores
  uint32       _scoresListMax;  //  before finding the _scores[] values.
  uint16      *_scoresList;     //

  uint32       _scoresListAid;  //  (current ID being stored in _scoresList)

  uint32       _scoresBaseID;   //  First ID with a score in the array.
  uint32       _scoresLastID;   //  Last  ID with a score in the array.
  uint32       _scoresAlloc;    //  Number of allocated scores.
  oSH_ovlSco  *_scores;         //  Only scores 0 .. _endID-_bgnID+1 are used.
};

#endif  //  AS_OVSTOREHISTOGRAM_H
