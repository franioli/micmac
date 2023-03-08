#ifndef _CODED_TARGET_H_
#define _CODED_TARGET_H_
#include "MMVII_memory.h"
#include "MMVII_ImageInfoExtract.h"
#include "MMVII_SetITpl.h"
#include "FilterCodedTarget.h"   // Contains image processing that may be not specific to coded target

namespace MMVII
{

// Everything specific to coded target in this namespace
namespace  cNS_CodedTarget
{


class cFullSpecifTarget ;  // main class from user's side to create target, know their specification

//  Bit encondinf
class cSpecBitEncoding;  
class cOneEncoding;
class cBitEncoding;

class cGeomSimDCT;
class cResSimul;
class cParamCodedTarget;
class cDCT;


typedef cSetISingleFixed<tU_INT4>  tBinCodeTarg;
typedef std::vector<tBinCodeTarg> tVSetICT;
typedef cIm2D<tU_INT1>     tImTarget;
typedef cDataIm2D<tU_INT1> tDataImT;


/* ************************************************** */
/*                                                    */
/*       Bits encoding                                */
/*                                                    */
/* ************************************************** */

/**  Contain the specification for a bit encoding :
 *     number of bits, redundancy (Haming distance), number max of code , accepted consecutive identic bits ...
 */
              
class cSpecBitEncoding : public cMemCheck
{
      public :
         cSpecBitEncoding();
         void AddData(const  cAuxAr2007 & anAux); // serialization

	 // ============   Value specified ==============
         eTyCodeTarget mType;           ///< Type amon allowed value
         size_t        mNbBits;         ///< number of bits
              // ===  Specified but have type-dependant def values
         size_t        mFreqCircEq;     ///< all code shift-circulary by mPerCircEq  will be merged together
         size_t        mMinHammingD;    ///< Min Hamming distance acceptable
         bool          mUseHammingCode; ///< use pure Hamming coding, +- redundant with  mMinHammingD
         cPt2di        mMaxRunL;        ///< Minimal run lenght for 0 & 1
         size_t        mParity;         ///<  1  odd , 2 even, 3 all

         size_t        mMaxNb;          ///< max number required
         size_t        mBase4Name;      ///< Base 4 computing names, default 10
					
	 // ============   Value computed ==============
         size_t        mNbDigit;        ///< Number of digit for names  Computed & Specified
	 std::string   mPrefix;        ///< For all name generated
				
         size_t        mMaxNum;         ///< max num of codes
         size_t        mMaxLowCode;     ///< max of all code (in fact max of the lowest representant)
         size_t        mMaxCodeEqui;    ///< max of all equivalent code
};

/**  Helper  for an encoding :
 *     essentially the Num + the code
 *     for technical-tricky reason (to add comment in "xml") contain also the nb of bit 
 */
class cOneEncoding : public cMemCheck
{
        public :
           cOneEncoding();
           cOneEncoding(size_t aNum,size_t aCode);
           size_t Num()  const;
           size_t Code() const;
           const std::string &  Name() const;

	   //  ==== used in construction only ===============================
           void   SetNBB (size_t ) ; ///< used to vehicle info 4 AddComm
           void   SetName (const std::string & ) ; ///< fix name when known

           void AddData(const  cAuxAr2007 & anAux); // serialization
						    //

        private :
           size_t mNC[3];  // Num Code Number of bits
           std::string mName;  // Name
};

/**  result of a bit encoding :  essential set of pair  Num/Code + also the specif used to generate it
 */
class cBitEncoding : public cMemCheck
{
    public :

        void AddData(const  cAuxAr2007 & anAux); // serialization
	    //  accessors 
        const cSpecBitEncoding & Specs() const;
        const std::vector<cOneEncoding> &  Encodings() const;
        std::vector<cOneEncoding> &  Encodings() ;

	    //  modifiers
        void AddOneEncoding(size_t mNum,size_t mCode);
        void SetSpec(const cSpecBitEncoding& );

    private :
        cSpecBitEncoding           mSpecs;
        std::vector<cOneEncoding>  mEncodings;
};
void AddData(const  cAuxAr2007 & anAux,cBitEncoding & aBE);

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
       cGeomSimDCT(const cOneEncoding & anEncod,const  cPt2dr& aC,const double& aR1,const double& aR2);
       /// Do to simulated target intersect, used to avoid overlapping target in images
       bool Intersect(const cGeomSimDCT &  aG2) const ;

       /// Usable when we use a clipped file
       void Translate(const cPt2dr &);

       cDCT *       mResExtr;   ///< contains the potentiel detected target extracted
       // int    mNum;       ///< numbering
       cOneEncoding mEncod; 
       cPt2dr       mC;         ///< Theoreticall center
       cPt2dr       mCornEl1;   ///< Theoreticall corner 1 of ellipse
       cPt2dr       mCornEl2;   ///< Theoreticall corner 2 of ellipse
       double       mR1;        ///< "small" size of deformaed rectangle
       double       mR2;        ///<  "big " size ....
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

/*   ==============  Target geometric specification   =============  */

class cParamCodedTarget : public cMemCheck
{
    public :
       cParamCodedTarget();
       void InitFromFile(const std::string & aNameFile);

       int &     NbRedond();  // Redundancy = number of repetition of a pattern in a circle
       int &     NbCircle();  // Redundancy = number of repetition of a pattern in a circle
       double &  RatioBar();  // Ratio on codin bar

       int ToMultiple_2DeZoom(int) const;
       cPt2di ToMultiple_2DeZoom(const cPt2di&) const;

       /// Set value that are computed from other like mRho_0... , mRho_1...
       void      Finish();
       /// Set default value that depend from the type , used only in create target
       void      FinishInitOfSpec(const cSpecBitEncoding & aSpec);


       int NbCodeAvalaible() const;                           // Number of different code we can generate
       int BaseForNum() const;                                // Base used for converting integer to string
							      //
       std::string  NameOfBinCode(int aNum) const; // -1 if bad code
       void AddData(const cAuxAr2007 & anAux);


       std::string NameOfNum(int) const; ///  Juste the apha num
       std::string NameFileOfNum(int) const; ///  Juste the apha num


       cPt2dr    mCenterF;   // symetry center at end
       // cPt2di    mSzF;       // sz at end
       cPt2dr    mCornEl1;
       cPt2dr    mCornEl2;
       cPt2dr    mMidle;  // Middle 
    // private :

       cPt2dr    Pix2Norm(const cPt2di &) const;
       cPt2dr    Norm2PixR(const cPt2dr &) const;
       cPt2di    Norm2PixI(const cPt2dr &) const;


       eTyCodeTarget mType;  // type among enumerated value
       int       mNbBit;  // Do not include parity, so 5=> gives 16 if parity used
       bool      mWithParity;  // Do we use parirty check
       int       mNbRedond;  // Redundancy = number of repetition of a pattern in a circle
       int       mNbCircle;  // Number of circles encoding information
       int       mSzGaussDeZoom	;
       int       mNbPixelBin;        // Number of pixel  Binary image
       double    mSz_CCB;      // size of central chekcboard/target , everything prop to it, 1 by convention




       double    mThickN_WInt;  ///< Thickness white circle separating code/
       double    mThickN_Code;  ///< Thickness of coding part
       double    mThickN_WExt;  ///< Thickness of white separatio,
       double    mThickN_Car;  ///< thickness of black border 
       double    mThickN_BorderExt;  ///< thickness of border 

       double    mChessboardAng;     ///< Origine angle of chessboard pattern
       bool      mWithChessboard;     ///< do we have a cental chess board, true 4 IGN
       bool      mWhiteBackGround;     ///< black on white, true 4 IGN
       bool      mZeroIsBackGround;
       bool      mAntiClockWiseBit;        ///< Do  growin bits go in trigonometric sens (!  visuel repair is clokwise)


       double    mRayOrientTablet;    
       tPt2dr    mCenterOrientTablet;
       double    mRayCenterMiniTarget;

       bool mModeFlight;  // Special mode for Patricio
       bool mCBAtTop;     // mean Check board at top (initial drone)			  
       // bool mCodeCirc;  // Special mode for Patricio
       double          mRho_0_EndCCB;// End of Central CB , here Rho=ThickN ...
       double          mRho_1_BeginCode;// ray where begins the coding stuff
       double          mRho_2_EndCode;// ray where begins the coding stuff
       double          mRho_3_BeginCar;// ray where begins the coding stuff
       double          mRho_4_EndCar;  // ray where begins the coding stuff
       double          mRho_EndIm;  // ray where begins the coding stuff
       double          mSignAngle;

       cPt2di    mSzBin;
       double    mScale;  // Sz of Pixel in normal coord

       cDecomposPAdikVar                  mDecP;
};

void AddData(const  cAuxAr2007 & anAux,cParamCodedTarget & aPCT);

typedef cParamCodedTarget cParamRenderingTarget;

class cCodedTargetPatternIm;
class cAppliGenCodedTarget;
class cCircNP2B;
class cStraightNP2B;

class cFullSpecifTarget : public cMemCheck
{
      public :
         friend cCodedTargetPatternIm;
         friend cAppliGenCodedTarget;
         friend cCircNP2B;
         friend cStraightNP2B;

	 typedef tU_INT1            tElem;
         typedef cIm2D<tElem>       tIm;
         typedef cDataIm2D<tElem>   tDataIm;

	 //  -----------   Construction/destruction -----------------

	    ///  read from an existing file
         static cFullSpecifTarget *  CreateFromFile(const std::string &);
	    ///  destructor must be accessible
         ~cFullSpecifTarget();
	    /// serialization  : public because called by MMVII::AddData 
         void AddData(const  cAuxAr2007 & anAux); 

	 //  -----------   Access to encodings -----------------

	     /// return all possible encodings  (Num+Name+Binary code)
         const std::vector<cOneEncoding> &  Encodings() const;
	     ///  Get encoding from bit-code,  work with any of the circular permutation for this code, null if none
	 const cOneEncoding * EncodingFromCode(size_t aBitFlag) const;
	     ///  Get encoding from name, null ptr if none
	 const cOneEncoding * EncodingFromName(const std::string &) const;


	 //  -----------   Creation of images -----------------

	     ///  Generate the image of one encoding
	 tIm   OneImTarget(const cOneEncoding & aCode);
	     /// get the pattern for generating all image 
	 tIm   ImagePattern();

	 //  -----------   Accessors to geometry & encoding  -----------------

	            //   encoding
         eTyCodeTarget Type() const;           ///< Type amon allowed value
	 size_t NbBits() const; ///<  Number of bits
         const  std::string &     Prefix()    const;  ///< Prefix used in name-generation
	 size_t MinHammingD() const;       ///<  Number of bits
         tREAL8 Rho_0_EndCCB() const;      /// End of Central Checkboard
         tREAL8 Rho_1_BeginCode() const;   /// ray where begins the coding stuff
         tREAL8 Rho_2_EndCode() const;     /// ray where ends the coding stuff


	 bool BitIs1(bool IsWhite) const;
	 bool AntiClockWiseBit() const;
	            //   geometry 
         const std::vector<cPt2dr>& BitsCenters() const; /// Access to all bits centers
	 const cPt2dr & Center() const; ///<  Center of bits
	 const cPt2dr & CornerlEl_BW() const; ///<  Corner of ellipse transition B->W ( trigonometric sense)
	 const cPt2dr & CornerlEl_WB() const; ///<  Corner of ellipse transition W->B ( trigonometric sense)

      private :
	 ///  default constructor required for step by step buildin
         cFullSpecifTarget();

         cFullSpecifTarget(const cBitEncoding&,const cParamRenderingTarget&);
         cFullSpecifTarget(const cFullSpecifTarget &) = delete;

	 /// Make a zoom vision for test geom + test reload
	 // static void TestReloadAndShow(const std::string & aName,int aZoom);


         const  cSpecBitEncoding &          Specs()     const;
	 int    DeZoomIm() const;
         const  cParamRenderingTarget &     Render()    const;

	 std::string NameOfImPattern() const;
	 std::string NameOfEncode(const cOneEncoding & anEnCode) const;

	 void SetBitCenter(size_t aBit,const cPt2dr&);


	 cCodedTargetPatternIm * AllocCTPI();
	 cCompEquiCodes *         CEC() const;

	 cPt2di  PDiag(tREAL8) const;

	 mutable cCompEquiCodes *         mCEC;
	 cCodedTargetPatternIm*   mCTPI;
         cBitEncoding             mBE;
         cParamRenderingTarget    mRender;
         std::vector<cPt2dr>      mBitsCenters;
};




};


};
#endif // _CODED_TARGET_H_

