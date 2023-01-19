#ifndef _CODED_TARGET_H_
#define _CODED_TARGET_H_
#include "MMVII_ImageInfoExtract.h"
#include "MMVII_SetITpl.h"
#include "FilterCodedTarget.h"   // Contains image processing that may be not specific to coded target

namespace MMVII
{

// Everything specific to coded target in this namespace
namespace  cNS_CodedTarget
{

class cGeomSimDCT;
class cResSimul;
class cSetCodeOf1Circle;
class cCodesOf1Target;
class cParamCodedTarget;
class cDCT;


typedef cSetISingleFixed<tU_INT4>  tBinCodeTarg;
typedef std::vector<tBinCodeTarg> tVSetICT;
typedef cIm2D<tU_INT1>     tImTarget;
typedef cDataIm2D<tU_INT1> tDataImT;


/*   ==============  Simulation  =============  */

/**  Contains information pour each target generated by simulation, usefull when
     used as ground truth
*/
class cGeomSimDCT
{
    public :
       /// defaut constructor usefull for serialization
       cGeomSimDCT();
       /// constructor used afetr randoming generating parameters
       cGeomSimDCT(int aNum,const  cPt2dr& aC,const double& aR1,const double& aR2);
       /// Do to simulated target intersect, used to avoid overlapping target in images
       bool Intersect(const cGeomSimDCT &  aG2) const ;

       /// Usable when we use a clipped file
       void Translate(const cPt2dr &);

       cDCT * mResExtr;   ///< contains the potentiel detected target extracted
       int    mNum;       ///< numbering
       cPt2dr mC;         ///< Theoreticall center
       cPt2dr mCornEl1;   ///< Theoreticall corner 1 of ellipse
       cPt2dr mCornEl2;   ///< Theoreticall corner 2 of ellipse
       double mR1;        ///< "small" size of deformaed rectangle
       double mR2;        ///<  "big " size ....
       std::string name;
};
/// method for serializing cGeomSimDCT
void AddData(const  cAuxAr2007 & anAux,cGeomSimDCT & aGSD);

/** Agregate all the generated target, used to put each "meta data" of simulation
in a single xml file */

class cResSimul
{
     public :
       cResSimul() ;
       /// create an object from a file (used no to export AddData)
       static  cResSimul  FromFile(const std::string&);


       double BorderGlob() const ;
       std::string                mCom;  ///< Command used to create the file
       cPt2dr                     mRayMinMax;
       double                     mBorder;
       double                     mRatioMax;
       std::vector<cGeomSimDCT>   mVG;
};
void AddData(const  cAuxAr2007 & anAux,cResSimul & aRS);

/*   ==============  Result extract  =============  */

/**   Result Detect Code Target, contains OK if the object is stil candidate,
      or another value indicating why it was eliminated
*/
enum class eResDCT
{
     Ok,
     Divg,
     LowSadleRel,
     LowSym,
     LowSymMin,
     LowBin,
     LowRad,
     BadDir
};

/**  Classe for storing  Detected Target */

class  cDCT
{
     public  :
         cDCT(const cPt2dr aPtR,eResDCT aState);

         cPt2di  Pix()  const {return ToI(mPt);} /// convert to integer coordinates

         cGeomSimDCT * mGT;        ///< possible ground truth
         cPt2dr        mPt;        ///< refined position sub-pixel
         cPt2dr        mDirC1;     ///< Direction of corner 1 (detected axe of chekboard)
         cPt2dr        mDirC2;     ///< Direction of corner 2 (detected axe of check board)
         eResDCT       mState;     ///< state of detection

         double        mScRadDir;   ///< Score of matching radiom (min of radiom diff)
         double        mCorMinDir;  ///< Score of matching radiom (min of correlation)
         double        mSym;        ///< symetry score (unused ?)
         double        mBin;        ///< Binary score (unused ?)
         double        mRad;        ///< Radiality core (unused ?)

         float         mVBlack;     ///< estimate "black" value of the detected target
         float         mVWhite;     ///< estimate "white" value of the detected target

         cPt2dr mRefinedCenter;      ///< Coordinates of center after detection process
         std::string mDecodedName;   ///< Name of target after detection process
         bool mFinalState;
         double mSizeTargetEllipse;
         std::vector<cPt2dr> mDetectedCorners;
         std::vector<cPt2di> mDetectedVectors;
         std::vector<cPt2di> mDetectedEllipse;
         std::vector<cPt2di> mDetectedFrame;

         bool mRecomputed;

};

/*   ==============  Target spec  =============  */


class cSetCodeOf1Circle
{
    public :
      cSetCodeOf1Circle(const std::vector<int> & aCards,int aN);
      int  NbSub() const;
      const tBinCodeTarg & CodeOfNum(int aNum) const;
      int N() const;
    private :
      std::vector<int>   mVCards;
      int      mN;
      tVSetICT mVSet ;  //   All the binary code of one target
};


class cCodesOf1Target
{
   public :
      cCodesOf1Target(int aNum);

      void AddOneCode(const tBinCodeTarg &);
      void  Show();
      const tBinCodeTarg & CodeOfNumC(int) const;
      int  Num() const;
      int getCodeLength() const;
   private :
      int                        mNum;
      std::vector<tBinCodeTarg>  mCodes;
};




class cParamCodedTarget
{
    public :
       cParamCodedTarget();
       void InitFromFile(const std::string & aNameFile);

       int &     NbRedond();  // Redundancy = number of repetition of a pattern in a circle
       int &     NbCircle();  // Redundancy = number of repetition of a pattern in a circle
       double &  RatioBar();  // Ratio on codin bar
       void      Finish();

       int NbCodeAvalaible() const;                           // Number of different code we can generate
       int BaseForNum() const;                                // Base used for converting integer to string
       cCodesOf1Target CodesOfNum(int);                       // One combinaison of binary code
       tImTarget  MakeImDrone(const cCodesOf1Target &);       // Generate the image of 1 combinaison
       tImTarget  MakeImCircle(const cCodesOf1Target &, bool);
       tImTarget  MakeImCodeExt(const cCodesOf1Target &);

       void AddData(const cAuxAr2007 & anAux);

       bool CodeBinOfPts(double aRho,double aTeta,const cCodesOf1Target & aSetCodesOfT,double aRho0,double aThRho);

       std::string NameOfNum(int) const; ///  Juste the apha num
       std::string NameFileOfNum(int) const; ///  Juste the apha num


       cPt2dr    mCenterF;   // symetry center at end
       cPt2di    mSzF;       // sz at end
       cPt2dr    mCornEl1;
       cPt2dr    mCornEl2;
       cPt2dr    mMidle;
    // private :

       cPt2dr    Pix2Norm(const cPt2di &) const;
       cPt2dr    Norm2PixR(const cPt2dr &) const;
       cPt2di    Norm2PixI(const cPt2dr &) const;


       int       mNbBit;  // Do not include parity, so 5=> gives 16 if parity used
       bool      mWithParity;  // Do we use parirty check
       int       mNbRedond;  // Redundancy = number of repetition of a pattern in a circle
       int       mNbCircle;  // Number of circles encoding information
       int       mNbPixelBin;        // Number of pixel  Binary image
       double    mSz_CCB;      // size of central chekcboard/target , everything prop to it, 1 by convention


       double    mThickN_WInt;  // Thickness white circle separating code/
       double    mThickN_Code;  // Thickness of coding part
       double    mThickN_WExt;  // Thickness of white separatio,
       double    mThickN_Car;  // thickness of black border (needed only on pannel)
       double    mThickN_BExt;  // thickness of black border (needed only on pannel)
       double    mChessboardAng;     // Origine angle of chessboard pattern

       bool mModeFlight;  // Special mode for Patricio


       double          mRho_0_EndCCB;// End of Central CB , here Rho=ThickN ...
       double          mRho_1_BeginCode;// ray where begins the coding stuff
       double          mRho_2_EndCode;// ray where begins the coding stuff
       double          mRho_3_BeginCar;// ray where begins the coding stuff
       double          mRho_4_EndCar;  // ray where begins the coding stuff


       cPt2di    mSzBin;
       double    mScale;  // Sz of Pixel in normal coord

       std::vector<cSetCodeOf1Circle>     mVecSetOfCode;
       cDecomposPAdikVar                  mDecP;
};

void AddData(const  cAuxAr2007 & anAux,cParamCodedTarget & aPCT);
};
};
#endif // _CODED_TARGET_H_

