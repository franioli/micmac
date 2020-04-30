/** \file MMVII_TreeDist.h
    \brief  classes for fast computation  of distance in tree

     Computation of distance inside a tree,   also it is an easy operation,
   in some context, like  pattern recoginition, we have to do it millions of
   times on thousands of tree, it is then necessary to have specialized efficient method.

     Let N be the size of the graphe, the computation time of implementation described is :

         * N log(N) for precomputation

         * log(N) in worst case for each tested pair, but probably must better in average
           on all pair  (empiricall 1.5 N^2 for all N^2 pair) ; also this constant
           time maybe optimistic for praticall case as with this algoritm we get 
           longer time for shortest dist, and pratically we may have more short dist
           (with the simulation Avg=1.5 on all pair, but Avg=2.7 with pair corresponding
            to Dist<=3)

    All the library is in the namespace NS_MMVII_FastTreeDist.
*/


// Set false for external use, set true inside MMMVII to benefit from functionality
// that will check correctness
#define WITH_MMVII false


#if (WITH_MMVII)
#include "include/MMVII_all.h"
using namespace MMVII;
#else             //========================================================== WITH_MMVI
class cMemCheck
{
};
#include <typeinfo>
#include <math.h>
#include <cassert>
#include <vector>
#include <iostream>
#include <algorithm>

// Some basic rand function defined in MMVII, used to generated random tree
int RandUnif_N(int aNb) { return rand() % aNb; }
#define NB_RAND_UNIF 1000000
float RandUnif_0_1() { return RandUnif_N(NB_RAND_UNIF) / float(NB_RAND_UNIF); }

#endif   //========================================================== WITH_MMVI

namespace NS_MMVII_FastTreeDist
{

/* ======================================= */
/* ======================================= */
/* =                                     = */ 
/* = Pre-Declaration of all used classes = */
/* =                                     = */ 
/* ======================================= */
/* ======================================= */

// class offering the services of fast distance in tree, only required interface
class cFastTreeDist;  

// class for basic representation of adjacency graph (could be reused), cFastTreeDist
// inherits of it, but user don't need to know this class for the servive
class cAdjGraph;    

// auxilary, a "cFastTreeDist" contains several "cOneLevFTD"
class cOneLevFTD;   

/* ======================================= */
/* ======================================= */
/* =                                     = */ 
/* =   Declaration of all used classes   = */
/* =                                     = */ 
/* ======================================= */
/* ======================================= */

/**
    A class for compact representation of graph as adjacency list (ie for each submit
   offer direct access to the set of its neighboor).

     Not especially flexible, rather targeted for specialized algoritm. Could easily 
   evolve to a weighted version.
*/

class cAdjGraph  : public cMemCheck
{
    public :
       /// We dont want unvolontar copy
       cAdjGraph(const cAdjGraph &) = delete;

       /// create with number of submit, reserve the memory that can be allocat
       cAdjGraph(const int & aNbSom);

       /** Creat edge for each pair (aVS1[aK],aVS2[aK]), put it 2 way ,
           remove potential exsiting edge. */
       void InitAdj(const std::vector<int> & aVS1,const std::vector<int> & aVS2); 

       void SupressAdjSom(const int aS); ///< Supress all the neigboor of S in two way
       int  NbNeigh(int) const;  ///<  Number of neigboor
       void Show() const; ///< Print a representation
       
       /// Check that the connected component has no cycle
       void  CheckIsTree(const std::vector<int> & aCC) const; 
       /// Compute distance with basic algorithm; used here to check the fast implementation
       int RawDist(int aS0,int aS1) const;

       /// Compute connected component
       void CalcCC(std::vector<int> & aRes,int aS0,std::vector<int> & aMarq,int aMIn,int aMOut);
    protected :
        void AssertOk(int aNumS) const; ///< Check num is valide range

        int mNbSom;                        ///< Number of submits
        std::vector<int *>    mBeginNeigh; ///< Pointer to begin of neighoor
        std::vector<int *>    mEndNeigh;   ///< Pointer to end   of neighboor
        std::vector<int>      mBufNeigh;   ///< Buffer to store the neigh (Begin/end point in it)
};

/** A cFastTreeDist will make several recursive split and store
   the computation of the corresponing level in cOneLevFTD
*/
class cOneLevFTD : public cMemCheck
{
    public :
        friend class cFastTreeDist;
        
        cOneLevFTD(int aNbSom); ///< create alloocating data
    private :
        std::vector<int>      mDistTop; ///< distance to subtree top
        std::vector<int>      mLabels;  ///< Label of subtree
         
};

/**  class offering all necessary services for fast computation of dist in a tree.
     Interface is limited to 3 method

       1- cFastTreeDist => constructor, allocate memory with a number N of submit
       2- MakeDist  => make the precomputation on give set of edges, time "N log(N)"
       3-  Dist => compute the dist between two subimit, time log(N) in worst case 
       
*/

class cFastTreeDist : private cAdjGraph 
{
    public :
        //    =====  Constructor / Destructor =====
            /// Copy has no added value, so forbid it to avoid unwanted use
        cFastTreeDist(const cFastTreeDist &) = delete;
            /// just need to know the number of summit to allocate
        cFastTreeDist(const int & aNbSom);
        
        /**  Make all the computation necessary to compute dist, if the graph is
        not a forest, generate an error */
        
        void MakeDist(const std::vector<int> & aVS1,const std::vector<int> & aVS2); 

        /// compute the distance between two submit; return -1 if not connected
        int Dist(int aI1,int aI2) const;

        /// Not 4 computation. Usefull only to make empiricall stat on the number of step
        int TimeDist(int aI1,int aI2) const;

    private :
        void RecComputeKernInd(int aS);
        void ComputeKernInd(int aS);

        void Explorate(int aS,int aLev,int aNumCC);

        bool mShow;
        int  mNbLevel;
        std::vector<int>    mMarq;      ///< Marker used for connected component (CC)
        std::vector<int>    mNumInsideCC;   ///< reach order in CC, used for orienting graph
        std::vector<int>    mNbDesc;    ///< Number of descend in the oriented tree
        std::vector<int>    mOrigQual;  ///< Quality  to select  as Origin
        std::vector<int>    mNumCC;     ///< Num of initial CC, special case for dist

        std::vector<cOneLevFTD> mLevels;
};

/* ======================================= */
/* ======================================= */
/* =                                     = */ 
/* =   Definition of all used classes    = */
/* =                                     = */ 
/* ======================================= */
/* ======================================= */

/* ---------------------------------- */
/* |         cAdjGraph              | */
/* ---------------------------------- */

cAdjGraph::cAdjGraph(const int & aNbSom) :
    mNbSom    (aNbSom),
    mBeginNeigh   (mNbSom,nullptr),
    mEndNeigh   (mNbSom,nullptr)
{
}

void cAdjGraph::InitAdj(const std::vector<int> & aVS1,const std::vector<int> & aVS2)
{
  bool IsSym = true;
  // 0  vector counting number of succ
  std::vector<int> aVNbSucc(mNbSom,0);

   // 1  count the number of succ
   assert(aVS1.size()==aVS2.size());
   for (int aKS=0 ; aKS<int(aVS1.size()) ; aKS++)
   {
       // chekc they are valide
       AssertOk(aVS1[aKS]);
       AssertOk(aVS2[aKS]);
       // update number of succ
       aVNbSucc[aVS1[aKS]]++;
       if (IsSym)
          aVNbSucc[aVS2[aKS]]++;
   }

   // 2  set the buffer size
   mBufNeigh.resize((IsSym ? 2 : 1)*aVS1.size());

   // 3 initialize the adjacency list by pointing inside the mBufNeigh
   int aSumNb = 0;
   for (int aKS=0 ; aKS<mNbSom ; aKS++)
   {
       mBeginNeigh[aKS] = mBufNeigh.data() + aSumNb;
       mEndNeigh[aKS] =  mBeginNeigh[aKS];
       aSumNb += aVNbSucc[aKS];
   }


  // 5  Finally create adjajency lists
   for (int aKS=0 ; aKS<int(aVS1.size()) ; aKS++)
   {
       // Memorise summit
       int aS1 = aVS1[aKS];
       int aS2 = aVS2[aKS];
       // Add new neigh
  
       *(mEndNeigh[aS1]++) = aS2;
       if (IsSym)
          *(mEndNeigh[aS2]++) = aS1;
   }
}

void cAdjGraph::AssertOk(int aNumS) const
{
    assert(aNumS>=0);
    assert(aNumS<int(mNbSom));
}


void cAdjGraph::Show() const
{
    std::cout << "------------------------------\n";
    for (int aKS1=0 ; aKS1<mNbSom ; aKS1++)
    {
        std::cout << " " << aKS1 << ":"; 
        for (auto aPS2 = mBeginNeigh[aKS1] ; aPS2 <mEndNeigh[aKS1] ; aPS2++)
            std::cout << " " << *aPS2 ;
        std::cout << "\n";
    }
}

void cAdjGraph::SupressAdjSom(const int aS1)
{
    AssertOk(aS1);
    for (auto aS2 = mBeginNeigh[aS1] ; aS2 <mEndNeigh[aS1] ; aS2++)
    {
        int * aBS2In = mBeginNeigh[*aS2]; 
        int * aBS2Out = aBS2In;
        // Copy in aBS2Out the element of aBS2In that are not equal to S1
        while (aBS2In!= mEndNeigh[*aS2])
        {
            if (*aBS2In != aS1)
               *(aBS2Out++) = *aBS2In;
             aBS2In++;
        }
        // Less succ to *aS2
        mEndNeigh[*aS2] = aBS2Out;
    }
    // No succ to S1
    mEndNeigh[aS1] = mBeginNeigh[aS1];
}

int  cAdjGraph::NbNeigh(int aS) const
{
    return mEndNeigh[aS] - mBeginNeigh[aS];
}

void cAdjGraph::CalcCC(std::vector<int> & aRes,int aS0,std::vector<int> & aMarq,int aMIn,int aMOut)
{
    assert(aMarq[aS0]==aMIn);
    aMarq[aS0] = aMOut;      // It's explored
    aRes.push_back(aS0);     //  Add it  to component
    int aKS = 0;             ///< index of next som

    while (aKS != int(aRes.size()))
    {
        int aS = aRes[aKS];
        for (auto aNeigh = mBeginNeigh[aS] ; aNeigh <mEndNeigh[aS] ; aNeigh++)
        {
            if (aMarq[*aNeigh] == aMIn)
            {
                aMarq[*aNeigh] = aMOut;
                aRes.push_back(*aNeigh);
            }
	}
        aKS ++;
    }
}
int cAdjGraph::RawDist(int aS0,int aS1) const
{
    //if (aS0==aS1) 
       //return 0;
    std::vector<int> aVMarq(mNbSom,0);
    std::vector<int> aVSom;   ///< Submit of component

    aVMarq[aS0] = 1;
    aVSom.push_back(aS0);     //  Add it  to component
    int aK0 = 0;             ///< index of next som
    int aK1 = 1;             ///< index of next som
    int aDist = 0;

    while (aK0!=aK1)
    {
        for (int aK=aK0; aK<aK1; aK++)
        {
           int aS = aVSom[aK];
           if (aS==aS1) 
              return aDist;
           for (auto aNeigh = mBeginNeigh[aS] ; aNeigh <mEndNeigh[aS] ; aNeigh++)
           {
               if (aVMarq[*aNeigh] == 0)
               {
                   aVMarq[*aNeigh] = 1;
                   aVSom.push_back(*aNeigh);
               }
	   }
        }
        aK0 = aK1;
        aK1 = aVSom.size();
        aDist++;
    }
    return -1;
}


void   cAdjGraph::CheckIsTree(const std::vector<int> & aCC) const
{
   //  Check there is no cycle by consistency nb submit/ nb edges
   int aNbE=0;
   for (const auto & aS : aCC)
       aNbE +=   NbNeigh(aS);

   if (aNbE!= 2*int(aCC.size()-1))
   {
      std::cout << "Not a forest, NbE=" << aNbE << " NbS=" << aCC.size()  << "\n";
      assert(false);
   }
}

/* ---------------------------------- */
/* |         cOneLevFTD             | */
/* ---------------------------------- */

cOneLevFTD::cOneLevFTD(int aNbSom) :
   mDistTop (aNbSom,-1), 
   mLabels  (aNbSom,-1)
{
}

/* ---------------------------------- */
/* |       cFastTreeDist            | */
/* ---------------------------------- */


cFastTreeDist::cFastTreeDist(const int & aNbSom) :
    cAdjGraph    (aNbSom),
    mShow        (false),
    mNbLevel     (1+ceil(log(1+aNbSom)/log(2.0))),
    mMarq        (aNbSom,-1),
    mNumInsideCC (aNbSom),
    mNbDesc      (aNbSom),
    mOrigQual    (aNbSom),
    mNumCC       (aNbSom,-1)
{
    mLevels.reserve(mNbLevel);
    cOneLevFTD aLev(aNbSom);
    for (int aK=0 ; aK<mNbLevel ; aK++)
        mLevels.push_back(aLev);
}

void cFastTreeDist::ComputeKernInd(int aS1)
{
/*
    mOrigQual[aS1] = 0;
    for (auto aPS2 = mBeginNeigh[aS1] ; aPS2 <mEndNeigh[aS1] ; aPS2++)
        mOrigQual[aS1] = std::max(mOrigQual[aS1],mNbDesc[*aPS2]);
*/

}

void cFastTreeDist::RecComputeKernInd(int aS1)
{
    // Compute the indicator itself
    mOrigQual[aS1] = 0;
    for (auto aPS2 = mBeginNeigh[aS1] ; aPS2 <mEndNeigh[aS1] ; aPS2++)
        mOrigQual[aS1] = std::max(mOrigQual[aS1],mNbDesc[*aPS2]);

    // Now recursive formula
    for (auto aPS2 = mBeginNeigh[aS1] ; aPS2 <mEndNeigh[aS1] ; aPS2++)
    {
       // to go in oriented graph
       if (mNumInsideCC[*aPS2]>mNumInsideCC[aS1])
       {
           // Save current value of  Number of desc
           int aNbDesc1 = mNbDesc[aS1];
           int aNbDesc2 = mNbDesc[*aPS2];

           // Modify  the nuber of desc, reflecting that we change the head of the tree
           mNbDesc[aS1] -= aNbDesc2;
           mNbDesc[*aPS2] += mNbDesc[aS1];
           // recursive call 
           RecComputeKernInd(*aPS2);

           // Restore previous value
           mNbDesc[aS1] = aNbDesc1;
           mNbDesc[*aPS2] = aNbDesc2;
       }
    }
}

void cFastTreeDist::Explorate(int aS0,int aLev,int aNumCC)
{
    // Probably not necessary, but I have no formall proof that pre computation is enough
    while (int(mLevels.size())<=aLev)
    {
        mLevels.push_back(cOneLevFTD(mNbSom));
    }

    //-1-  Explore connected component, compute mNumInsideCC 
    std::vector<int> aCC;
    mMarq[aS0] = aLev;      // It's explored
    aCC.push_back(aS0);     //  Add it  to component
    {
       int aKS = 0;             ///< index of next som already explore
       while (aKS != int(aCC.size()))  // it is not at the top
       {
           int aS = aCC[aKS];  // get the value and parse its neigboors
           mNumInsideCC[aS] = aKS; // memorize order of reaching
           mNbDesc[aS] = 1;    // Number of desc before propagation
           if (aLev==0)   // Memo num CC to know of two som are disconnected
              mNumCC[aS] = aNumCC;
           for (auto aNeigh = mBeginNeigh[aS] ; aNeigh <mEndNeigh[aS] ; aNeigh++)
           {
               if (mMarq[*aNeigh] == (aLev-1))  // if not explored
               {
                   mMarq[*aNeigh] = aLev;   // marq it exlpored
                   aCC.push_back(*aNeigh);  // and store it
               }
	   }
           aKS ++;
       }
   }
   //2- Check users gave a forest (i.e. this CC is a tree)
   if (aLev==0)
   {
      CheckIsTree(aCC);
   }

   //3- if size =1, we are done
   if (aCC.size()==1)
      return;
 
   // Taking a orientate graph
   //  compute mNbDesc = recursive som of each son
   for (int aK=(aCC.size()-1) ; aK>=0 ; aK--)
   {
        int aS1 = aCC[aK];
        for (auto aPS2 = mBeginNeigh[aS1] ; aPS2 <mEndNeigh[aS1] ; aPS2++)
        {
            if (mNumInsideCC[*aPS2]<mNumInsideCC[aS1])
               mNbDesc[*aPS2] += mNbDesc[aS1];
        }
   }
   //4- Make the computation of quality as origin on all the component
   RecComputeKernInd(aS0);
 
   //5- The kern is the summit minimizing the kern indicator
   int aBestOrig    = aCC[0];
   int aQualOrig =  mOrigQual[aBestOrig];
   for (int aK=1 ; aK<int(aCC.size()) ; aK++)
   {
       int aS = aCC[aK];
       int aCrit = mOrigQual[aS];
       if (aCrit<aQualOrig)
       {
           aBestOrig  = aS;
           aQualOrig = aCrit;
       }
   }

   // 6 - Compute distance to origin and labels of subtree
   //     6.1  compute value of origins and its neighboor
   std::vector<int> &  aDistTop =  mLevels.at(aLev).mDistTop;
   std::vector<int> &  aLabels  =  mLevels.at(aLev).mLabels;
   aDistTop[aBestOrig] = 0;
   aLabels[aBestOrig]  = 0;
   int aLab=1;
   for (auto aNeigh = mBeginNeigh[aBestOrig] ; aNeigh <mEndNeigh[aBestOrig] ; aNeigh++)
   {
        // aDistTop[*aNeigh] = 1;
        aLabels[*aNeigh] = aLab;
        aLab++;
   }
   // 6.2 Now a connected component again starting from origin 
   // to propagate distance & labels
   aCC.clear();
   aCC.push_back(aBestOrig);     //  Add it  to component
   {
       int aKS = 0;             ///< index of next som already explore
       while (aKS != int(aCC.size()))  // it is not at the top
       {
           int aS = aCC[aKS];  // get the value and parse its neigboors
           for (auto aNeigh = mBeginNeigh[aS] ; aNeigh <mEndNeigh[aS] ; aNeigh++)
           {
               if (aDistTop[*aNeigh] == -1)
               {
                   aCC.push_back(*aNeigh);  // and store it
                   aDistTop[*aNeigh] = 1+aDistTop[aS];
                   if (aDistTop[aS]>0)
                       aLabels[*aNeigh] =aLabels[aS];
               }
	   }
           aKS ++;
       }
    }
   
    if (mShow)
    {
       std::cout << "L=" << aLev << " [S,NbD,Qual,Dist,Lab] " ;
       for (auto aS : aCC)
       {
           std::cout << " [" 
                     << aS            << "," 
                     << mNbDesc[aS]   << "," 
                     << mOrigQual[aS] << ","
                     << aDistTop[aS] << ","
                     << aLabels[aS] 
                     << "]";
           if (aS==aBestOrig)
              std::cout << "*";
        }
        std::cout << "\n";
    }

    // 7 Recursive call on trees after supression of origin
    SupressAdjSom(aBestOrig);
    for (const auto  & aS : aCC)
    {
        if ((aS!=aBestOrig) && (mMarq[aS]==aLev))
        {
            Explorate(aS,aLev+1,-1);  // NumCC unused after lev 0
        }
    }
}

void cFastTreeDist::MakeDist(const std::vector<int> & aVS1,const std::vector<int> & aVS2)
{
    InitAdj(aVS1,aVS2);

    // Reset all state
    for (auto & aM : mMarq)
        aM = -1;
    for (auto & aL : mLevels)
        for (auto & aD : aL.mDistTop)
            aD=-1;

    int aNumCC=0;
    for (int aS=0 ; aS<mNbSom ; aS++)
    {
         if (mMarq[aS] == -1)
         {
            Explorate(aS,0,aNumCC);
            aNumCC++;
         }
    }
}

int cFastTreeDist::TimeDist(int aI1,int aI2) const
{
   if (aI1==aI2) return 1; // Case not well handled else
   if (mNumCC[aI1]!=mNumCC[aI2]) return 1;

   for (int aK=0 ; aK<int(mLevels.size()) ; aK++)
       if (mLevels[aK].mLabels[aI1]!=mLevels[aK].mLabels[aI2])
          return aK+1;

   assert(false);
   return 0;
}

int cFastTreeDist::Dist(int aI1,int aI2) const
{
   if (aI1==aI2) return 0; // Case not well handled else

   // Case not connected; return conventionnal value
   if (mNumCC[aI1]!=mNumCC[aI2]) return -1;

   for (const auto & aLev : mLevels)
       if (aLev.mLabels[aI1]!=aLev.mLabels[aI2])
          return aLev.mDistTop[aI1]+aLev.mDistTop[aI2];

   assert(false);
   return 0;
}

   
};


namespace MMVII
{

class cOneBenchFastTreeDist
{
    public :
  
      cOneBenchFastTreeDist(int aNbSom,int aNbCC);
      void DoOne();
    private :
      typedef NS_MMVII_FastTreeDist::cFastTreeDist tFTD;
      typedef NS_MMVII_FastTreeDist::cAdjGraph     tAdjG;

      int                mNbSom;
      int                mNbCC;
      tFTD               mFTD;
      tAdjG              mAdjG;
};

cOneBenchFastTreeDist::cOneBenchFastTreeDist(int aNbSom,int aNbCC) :
   mNbSom  (aNbSom),
   mNbCC   (std::min(aNbCC,aNbSom)),
   mFTD    (mNbSom),
   mAdjG   (mNbSom)
   // mGr     (nullptr)
{
}

void cOneBenchFastTreeDist::DoOne()
{
    std::vector<std::vector<int>> aVCC(mNbCC); // set of connected components
    for (int aK=0 ; aK<mNbSom ; aK++)
    {
        aVCC.at(RandUnif_N(mNbCC)).push_back(aK);
    }

    std::vector<int> aV1;
    std::vector<int> aV2;
    for (auto & aCC : aVCC)
    {
        std::vector<double> aVR;
        for (int aK=0 ; aK<int(aCC.size()) ; aK++)
        {
            aVR.push_back(RandUnif_0_1());
        }
        std::sort
        (
            aCC.begin(),aCC.end(),
            [aVR](int i1,int i2) {return aVR[i1]<aVR[i1];}
        );
        for (int aK1=1 ; aK1<int(aCC.size()) ; aK1++)
        {
             // On doit tirer dans [0,K[ en privilegiant les valeur haute
             //  pour avoir des  chaines longue
             double aPds = sqrt(RandUnif_0_1());
             int aK2 = floor(aPds*aK1);
             aK2 = std::max(0,std::min(aK2,aK1-1));
             aV1.push_back(aCC[aK1]);
             aV2.push_back(aCC[aK2]);
        }
    }
    mFTD.MakeDist(aV1,aV2);
    mAdjG.InitAdj(aV1,aV2);

    for (int aS1=0 ; aS1<mNbSom ; aS1++)
    {
        for (int aS2=0 ; aS2<mNbSom ; aS2++)
        {
            int aD= mFTD.Dist(aS1,aS2);
            int aD2= mAdjG.RawDist(aS1,aS2);
            assert(aD==aD2);
        }
    }


    if (mNbCC==1)
    {
       int aNbTest =0;
       int aSomT  =0;
       int aMaxT  =0;

       const int aSeuiLowD = 3;
       int aSomLowD  = 0;
       int aNbLowD   = 0;

       for (int aS1=0 ; aS1<mNbSom ; aS1++)
       {
           for (int aS2=0 ; aS2<aS1 ; aS2++)
           {
               aNbTest++;
               int aD = mFTD.Dist(aS1,aS2);
               int aT = mFTD.TimeDist(aS1,aS2);
               aSomT += aT;
               aMaxT = std::max(aMaxT,aT);
               if (aD<=aSeuiLowD)
               {
                  aSomLowD += aT;
                  aNbLowD++;
               }
           }
       }
       std::cout << "Nb=" << mNbSom  
                 << " AvgT=" << aSomT/double(aNbTest) 
                 << " MaxT=" << aMaxT
                 << " AvgLow=" << aSomLowD/double(aNbLowD) 
                 << "\n";
    }
}

void OneBenchFastTreeDist(int aNbSom,int aNbCC)
{
    cOneBenchFastTreeDist aBFTD(aNbSom,aNbCC);

    for (int aK=0 ; aK<4 ; aK++)
    {
        aBFTD.DoOne();
    }
    std::cout << "DONNE  "<<  aNbSom << " " << aNbCC << "\n";
}


void BenchFastTreeDist()
{
    for (int aNb=1 ; aNb<15 ; aNb++)
    {
         int aNb2 = aNb*aNb;
         OneBenchFastTreeDist(aNb2,1);
         OneBenchFastTreeDist(aNb2,2);
         OneBenchFastTreeDist(aNb2,3);
    }


    NS_MMVII_FastTreeDist::cFastTreeDist aFTD(11);
/*
    1
     \
      0-3-4   5-6-7 8  9-10
     /
    2
*/
/*
    aFTD.MakeDist
    (
       {0,2,3,3,5,7,9},
       {1,0,4,0,6,6,10}
    );
*/
/*
    aFTD.Show();

    aFTD.SupressAdjSom(2);
    aFTD.Show();
*/
}

};

