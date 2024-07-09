#include "CodedTarget.h"
#include "MMVII_2Include_Serial_Tpl.h"
#include "MMVII_Tpl_Images.h"
#include "MMVII_Interpolators.h"
#include "MMVII_Linear2DFiltering.h"
#include "MMVII_ImageMorphoMath.h"
#include "MMVII_2Include_Tiling.h"
#include "MMVII_Sensor.h"
#include "MMVII_HeuristikOpt.h"
#include "MMVII_ExtractLines.h"
#include "MMVII_TplImage_PtsFromValue.h"


namespace MMVII
{



static constexpr tU_INT1 eNone = 0 ;
static constexpr tU_INT1 eTopo0  = 1 ;
static constexpr tU_INT1 eTopoTmpCC  = 2 ;
static constexpr tU_INT1 eTopoMaxOfCC  = 3 ;
static constexpr tU_INT1 eTopoMaxLoc  = 4 ;
static constexpr tU_INT1 eFilterSym  = 5 ;
static constexpr tU_INT1 eFilterRadiom  = 6 ;


class cCdSadle;
class cCdSym;
class cCdRadiom;
class cTmpCdRadiomPos ;

/* **************************************************** */
/*                     cCdSadle                         */
/* **************************************************** */

/// candidate that are pre-selected on the sadle-point criteria
class cCdSadle
{
    public :
        cCdSadle (const cPt2dr & aC,tREAL8 aCrit) : mC (aC) , mSadCrit (aCrit) {}
        cCdSadle (){}
        cPt2dr mC;

	///  Criterion of sadle point, obtain by fiting a quadric on gray level
        tREAL8 mSadCrit;
};

/* **************************************************** */
/*                     cCdSym                           */
/* **************************************************** */

/// candidate that are pre-selected after symetry criterion
class cCdSym : public cCdSadle
{
    public :
        cCdSym(const cCdSadle &  aCdSad,tREAL8 aCrit) : cCdSadle  (aCdSad), mSymCrit  (aCrit) { }
	/// Criterion of symetry, obtain after optimisation of center
	tREAL8   mSymCrit;

};

/// candidate obtain after radiometric modelization, 

class cCdRadiom : public cCdSym
{
      public :
          /// Cstr use the 2 direction + Thickness of transition between black & white
          cCdRadiom(const cCdSym &,const cDataIm2D<tREAL4> & aDIm,tREAL8 aTeta1,tREAL8 aTeta2,tREAL8 aLength,tREAL8 aThickness);

          ///  Theoretical threshold
          tREAL8 Threshold() const ;
	  /// Once black/white & teta are computed, refine seg using 
	  void OptimSegIm(const cDataIm2D<tREAL4> & aDIm,tREAL8 aLength);

	  /// compute an ellipse that "safely" contains the points  of checkboard
          void ComputePtsOfEllipse(std::vector<cPt2di> & aRes,tREAL8 aLength) const;
	  ///  call previous with length
          void ComputePtsOfEllipse(std::vector<cPt2di> & aRes) const;

	  /// compute an ellipse that contain 
          bool FrontBlackCC(std::vector<cPt2di> & aRes,cDataIm2D<tU_INT1> & aMarq) const;
	  ///  Select point of front that are on ellipse
          void SelEllAndRefineFront(std::vector<cPt2dr> & aRes,const std::vector<cPt2di> &) const;


	  /// Is the possibly the point on arc of black  ellipse
          bool PtIsOnEll(cPt2dr &) const;
	  ///  Is the point on the line for one of angles
          bool PtIsOnLine(const cPt2dr &,tREAL8 aTeta) const;

	  ///  Make a visualisation of geometry
	  void ShowDetail(int aCptMarq,const cScoreTetaLine & aSTL,const std::string &,cDataIm2D<tU_INT1> & aMarq) const;

	  const cDataIm2D<tREAL4> * mDIm;
	  tREAL8  mTetas[2];
	  tREAL8  mLength;     ///< length of the lines
	  tREAL8  mThickness;  ///< thickness of the transition B/W

	  tREAL8  mCostCorrel;  // 1-Correlation of model
	  tREAL8  mRatioBW;  // ratio min/max of BW
	  tREAL8  mScoreTeta;  // ratio min/max of BW
          tREAL8  mBlack;
          tREAL8  mWhite;
};
				
class cCdEllipse : public cCdRadiom
{
	public : 
           cCdEllipse(const cCdRadiom &,cDataIm2D<tU_INT1> & aMarq);
	   bool IsOk() const;
	   const cEllipse & Ell() const;
           const cPt2dr &   V1() const;
           const cPt2dr &   V2() const;

	private : 
           void AssertOk() const;

           bool       mIsOk;
	   cEllipse   mEll;
	   cPt2dr     mV1;
	   cPt2dr     mV2;
};


enum class eTPosCB
{
      eUndef,
      eInsideBlack,
      eInsideWhite,
      eBorderLeft,
      eBorderRight

};

inline bool IsInside(eTPosCB aState) {return (aState==eTPosCB::eInsideBlack)  ||   (aState==eTPosCB::eInsideWhite) ;}
inline bool IsOk(eTPosCB aState) {return aState!=eTPosCB::eUndef;}

///  Used temporary for compilation of radiom
class cTmpCdRadiomPos : public cCdRadiom
{
	public :
          cTmpCdRadiomPos(const cCdRadiom &,tREAL8 aThickness);

	  /// Theoreticall radiom of modelize checkboard + bool if was computed
	  std::pair<eTPosCB,tREAL8>  TheorRadiom(const cPt2dr &) const;

	  tREAL8                     mThickness;
          cSegment2DCompiled<tREAL8> mSeg0 ;
          cSegment2DCompiled<tREAL8> mSeg1 ;
};

/* ***************************************************** */
/*                                                       */
/*                    cCdRadiom                          */
/*                                                       */
/* ***************************************************** */

cCdRadiom::cCdRadiom
(
    const cCdSym & aCdSym,
    const cDataIm2D<tREAL4> & aDIm,
    tREAL8 aTeta0,
    tREAL8 aTeta1,
    tREAL8 aLength,
    tREAL8 aThickness
) :
       cCdSym      (aCdSym),
       mDIm        (&aDIm),
       mTetas      {aTeta0,aTeta1},
       mLength     (aLength),
       mThickness  (aThickness),
       mCostCorrel (2.001),   // over maximal theoreticall value
       mRatioBW    (0)
{
    static int aCpt=0 ; aCpt++;

    int aNbIn0=0,aNbIn1=0;

    cMatIner2Var<tREAL8> aCorGrayAll;
    cMatIner2Var<tREAL8> aCorGrayInside;

    cTmpCdRadiomPos  aCRC(*this,aThickness);

    std::vector<cPt2di> aVPixEllipse;
    ComputePtsOfEllipse(aVPixEllipse);
    for (const auto & aPImI : aVPixEllipse)
    {
	tREAL8 aValIm = aDIm.GetV(aPImI);
	cPt2dr aPImR = ToR(aPImI);

	auto [aState,aGrayTh] = aCRC.TheorRadiom(aPImR);

	if  (IsInside(aState))
	{
            aCorGrayInside.Add(aGrayTh,aValIm);
            aNbIn0 += (aState == eTPosCB::eInsideBlack);
            aNbIn1 += (aState == eTPosCB::eInsideWhite);
	}
	if  (IsOk(aState))
	{
            aCorGrayAll.Add(aGrayTh,aValIm);
	}
    }

    mRatioBW = std::min(aNbIn0,aNbIn1) / (tREAL8) std::max(aNbIn0,aNbIn1);
    if (mRatioBW <0.05)
    {
       return ;
    }

     mCostCorrel = 1-aCorGrayAll.Correl();
     auto [a,b] = aCorGrayInside.FitLineDirect();
     mBlack = b ;
     mWhite = a+b;
}

tREAL8 cCdRadiom::Threshold() const {return (mBlack+mWhite)/2.0;}

void cCdRadiom::OptimSegIm(const cDataIm2D<tREAL4> & aDIm,tREAL8 aLength)
{
     std::vector<cSegment2DCompiled<tREAL8>> aVSegOpt;
     for (int aKTeta=0 ; aKTeta<2 ; aKTeta++)
     {
         cPt2dr aTgt = FromPolar(aLength,mTetas[aKTeta]);
         tSeg2dr aSegInit(mC-aTgt,mC+aTgt);
         cOptimSeg_ValueIm<tREAL4>  aOSVI(aSegInit,0.5,aDIm,Threshold());
	 tSeg2dr  aSegOpt = aOSVI.OptimizeSeg(0.5,0.01,true,2.0);

	 aVSegOpt.push_back(aSegOpt);
	 mTetas[aKTeta] = Teta(aSegOpt.V12());
	 // mTetas[aKTeta] = aSegOpt.I//
     }

     cPt2dr aC = aVSegOpt.at(0).InterSeg(aVSegOpt.at(1));

     mC = aC;
     cScoreTetaLine::NormalizeTeta(mTetas);
}

void cCdRadiom::ComputePtsOfEllipse(std::vector<cPt2di> & aRes) const
{
	ComputePtsOfEllipse(aRes,mLength);
}


void cCdRadiom::ComputePtsOfEllipse(std::vector<cPt2di> & aRes,tREAL8 aLength) const
{
    aRes.clear();
    cPt2dr aV0 = FromPolar(aLength,mTetas[0]);
    cPt2dr aV1 = FromPolar(aLength,mTetas[1]);

    //  ----  x,y ->   mC + x V0 + y V1  ------
    cAffin2D<tREAL8>  aMapEll2Ori(mC,aV0,aV1);
    cAffin2D<tREAL8>  aMapOri2Ell = aMapEll2Ori.MapInverse();

    cTplBoxOfPts<tREAL8,2> aBox;
    int aNbTeta = 100;
    for (int aKTeta=0 ; aKTeta<aNbTeta ; aKTeta++)
    {
         aBox.Add(aMapEll2Ori.Value(FromPolar(1.0, (2.0*M_PI * aKTeta) / aNbTeta)));
    }

    cBox2di aBoxI = aBox.CurBox().Dilate(2.0).ToI();

    for (const auto & aPix : cRect2(aBoxI))
    {
         if (Norm2(aMapOri2Ell.Value(ToR(aPix))) < 1)
            aRes.push_back(aPix);
    }
}

bool cCdRadiom::PtIsOnLine(const cPt2dr & aPAbs,tREAL8 aTeta) const
{
    cSegment2DCompiled<tREAL8> aSeg(mC,mC+FromPolar(1.0,aTeta));

    cPt2dr aPLoc = aSeg.ToCoordLoc(aPAbs);

    if (std::abs(aPLoc.y()) <= 1.0 + std::abs(aPLoc.x()) /30.0)
       return true;

    

    return false;
}

bool cCdRadiom::PtIsOnEll(cPt2dr & aPtAbs) const
{
    if  (Norm2(aPtAbs - mC)<3.0)
        return false;

    for (const auto & aTeta : mTetas )
        if (PtIsOnLine(aPtAbs,aTeta))
           return false;

    cGetPts_ImInterp_FromValue<tREAL4> aGIFV(*mDIm,Threshold(),0.1,aPtAbs, VUnit(aPtAbs - mC));
    cPt2dr aNewP = aPtAbs;
    if (aGIFV.Ok())
    {
        aNewP = aGIFV.PRes();
	if (Norm2(aPtAbs-aNewP)>2.0)
           return false;

        cPt2dr aPGr =  Proj(mDIm->GetGradAndVBL(aNewP));
	tREAL8 aSc =  std::abs(Cos(aPGr,aNewP-mC));
	if (aSc<0.5)
           return false;

	aPtAbs = aNewP;
    }
    else
	  return false;

   // cGetPts_ImInterp_FromValue<tREAL4> aGIFV(*mDIm,aV,0.1,aPt+ToR(aDec)-aNorm, aNorm);
    /*
    cPt2dr  aPGr =  Proj(mDIm->GetGradAndVBL(aPtAbs));
    if (IsNull(aPGr)) return false;

    cPt2dr aDir = (aPtAbs-mC) / aPGr;
    tREAL8 aTeta = 
    if (Norm2(aPGr) 

    */
     


    return true;
}

void cCdRadiom::SelEllAndRefineFront(std::vector<cPt2dr> & aRes,const std::vector<cPt2di> & aFrontI) const
{
    aRes.clear();
    for (const auto & aPix : aFrontI)
    {
         cPt2dr aRPix = ToR(aPix);
	 if (PtIsOnEll(aRPix))
            aRes.push_back(aRPix);
    }
}

bool cCdRadiom::FrontBlackCC(std::vector<cPt2di> & aVFront,cDataIm2D<tU_INT1> & aDMarq) const
{
    std::vector<cPt2di> aRes;
    aVFront.clear();

    std::vector<cPt2di> aVPtsEll;
    ComputePtsOfEllipse(aVPtsEll,5.0);

    tREAL8 aThrs = Threshold();
    for (const auto & aPix : aVPtsEll)
    {
        if (mDIm->GetV(aPix)<aThrs)
	{
            aDMarq.SetV(aPix,1);
	    aRes.push_back(aPix);
	}
    }

    size_t aIndBot = 0;
    const std::vector<cPt2di> & aV4 = Alloc4Neighbourhood();

    cRect2  aImOk(aDMarq.Dilate(-10));
    bool isOk = true;

    while (aIndBot != aRes.size())
    {
          for (const auto & aDelta : aV4)
          {
              cPt2di aPix = aRes.at(aIndBot) + aDelta;
	      if ((aDMarq.GetV(aPix)==0) && (mDIm->GetV(aPix)<aThrs) )
	      {
                 if (aImOk.Inside(aPix))
		 {
                    aDMarq.SetV(aPix,1);
		    aRes.push_back(aPix);
	         }
	         else
	         {
                    isOk = false;
	         }
	      }
          }
	  aIndBot++;
    }

    const std::vector<cPt2di> & aV8 = Alloc8Neighbourhood();
    // compute frontier points
    for (const auto & aPix : aRes)
    {
        bool has8NeighWhite = false;
        for (const auto & aDelta : aV8)
	{
	     if (aDMarq.GetV(aPix+aDelta)==0) 
                has8NeighWhite = true;
	}

	if (has8NeighWhite)
            aVFront.push_back(aPix);
    }
    // StdOut() << "FFFF=" << aVFront << "\n";

    for (const auto & aPix : aRes)
        aDMarq.SetV(aPix,0);

    return isOk;
}


	// if (has8NeighWhite && PtIsOnEll(aRPix))

void  cCdRadiom::ShowDetail(int aCptMarq,const cScoreTetaLine & aSTL,const std::string & aNameIm,cDataIm2D<tU_INT1> & aMarq) const
{
      std::pair<tREAL8,tREAL8> aPairTeta(mTetas[0],mTetas[1]);

      StdOut()    << " CPT=" << aCptMarq 
		  << "  Corrrr=" <<  mCostCorrel 
                   << " Ratio=" <<  mRatioBW
		  << " V0="<< mBlack << " V1=" << mWhite 
		  << " ScTeta=" << aSTL.Score2Teta(aPairTeta,2.0)
		  << " LLL=" << mLength
		  << " ThickN=" << mThickness
		  << "\n";

      int aZoom = 9;
      cPt2di aSz(100,100);
      cPt2di aDec =  ToI(mC) - aSz/2;
      cPt2dr aCLoc = mC-ToR(aDec);

      cRGBImage  aIm = cRGBImage:: FromFile(aNameIm,cBox2di(aDec,aDec+aSz),aZoom);
      aIm.ResetGray();

      cPt3di  aCol_BlRight = cRGBImage::Red;  // color for teta wih blakc on right (on visualization)
      cPt3di  aCol_BlLeft = cRGBImage::Blue;  // color for teta wih blakc on left (on visualization)
      cPt3di  aCol_StrL = cRGBImage::Yellow;  // color for theoreticall straight line
      cPt3di  aCoulFront = cRGBImage::Cyan; 
      cPt3di  aCoulEllFront = cRGBImage::Orange; 
					     
      if (0)   // generate the theoretical image + the area (ellipse) of gray modelization
      {
          cTmpCdRadiomPos aCRC(*this,mThickness);
	  std::vector<cPt2di> aVEllipse;
          ComputePtsOfEllipse(aVEllipse);

          for (const auto & aPix :  aVEllipse)
          {
              auto [aState,aWeightWhite] = aCRC.TheorRadiom(ToR(aPix));
              if (aState != eTPosCB::eUndef)
              {
                 tREAL8 aGr = mBlack+ aWeightWhite*(mWhite-mBlack);
		 cPt3di aCoul (128,aGr,aGr);
		 // cPt3di aCoul (aGr,aGr,aGr);
                 aIm.SetRGBPix(aPix-aDec,aCoul);
              }
          }
      }

      if (1)   // generate the connected 
      {
          std::vector<cPt2di> aIFront;
          FrontBlackCC(aIFront,aMarq);
	  if (0)
	  {
             for (const auto & aPix : aIFront)
             {
                 aIm.SetRGBPix(aPix-aDec,aCoulFront);
             }
	  }

          std::vector<cPt2dr> aEllFr;
	  SelEllAndRefineFront(aEllFr,aIFront);
          for (const auto & aPt : aEllFr)
          {
              aIm.SetRGBPoint(aPt-ToR(aDec),aCoulEllFront);
          }
      }

      if (1)
      {
           cCdEllipse aCDE(*this,aMarq);
	   int aNb= 500 ; 
	   for (int aK=0 ; aK<aNb ; aK++)
	   {
                cPt2dr aPt = aCDE.Ell().PtOfTeta((2*M_PI*aK)/aNb);
                aIm.SetRGBPoint(aPt-ToR(aDec),cRGBImage::Red);
	   }
	   aIm.DrawCircle(aCol_BlLeft,aCDE.V1()-ToR(aDec),0.5);
	   aIm.DrawCircle(aCol_BlRight,aCDE.V2()-ToR(aDec),0.5);
      }

      //  visualize orientation draw the line detected : straight line, orientation, level curve
      if (1)
      {
	  int aKT=0;
	  for (const auto & aTeta  : mTetas)
	  {
              for (int aK= -aZoom * 20 ; aK<=aZoom*20 ; aK++)
	      {
		  tREAL8 aAbsc= aK/ (2.0 * aZoom);
		  cPt2dr aPt = aCLoc + FromPolar(aAbsc,aTeta);

	          aIm.SetRGBPoint(aPt,aCol_StrL); // show straight line

		  //  show orientation of line
		  // if (aK==6*aZoom)
                  //    aIm.DrawCircle((aKT==0) ?  aCol_BlLeft : aCol_BlRight ,aPt,0.5);
		   
		  // Now show interpolation (but only  if far enough of intersection)
		  if (std::abs(aAbsc) > 1.0) 
		  {
                      // The orientation of normal change : (1) when we cross intersection
		      // (2) when we change the segment 
		      tREAL8 aSign = ((aAbsc>0) ? 1.0 : -1.0) * ((aKT==0) ? 1 : -1) ;
		      cPt2dr aNorm = FromPolar(1.0,aTeta + M_PI/2.0) * aSign;
                      tREAL8 aV = Threshold();

		      // note that we retract from segment position by "-aNorm" , because the "cGetPts_ImInterp_FromValue"
		      // is looking only in one direction !
                      cGetPts_ImInterp_FromValue<tREAL4> aGIFV(*mDIm,aV,0.1,aPt+ToR(aDec)-aNorm, aNorm);
		      if (aGIFV.Ok())
		      {
	                  aIm.SetRGBPoint(aGIFV.PRes()-ToR(aDec),cRGBImage::Green);
                      }
		      // StdOut() << "OKKK " << aGIFV.Ok()  << " K=" << aK << "\n";
		  }
		  /*
		  */
	          // aIm.SetRGBPoint(aPt,cRGBImage::Green);
	      }
	      aKT++;
	  }
      }


      aIm.SetRGBPoint(aCLoc,cRGBImage::Red);
      aIm.ToFile("TestCenter_" + ToStr(aCptMarq) + ".tif");
}

/* ***************************************************** */
/*                                                       */
/*                    cCdEllipse                         */
/*                                                       */
/* ***************************************************** */

cCdEllipse::cCdEllipse(const cCdRadiom & aCdR,cDataIm2D<tU_INT1> & aMarq) :
     cCdRadiom (aCdR),
     mIsOk     (false),
     mEll      (cPt2dr(0,0),0,1,1)
{
     std::vector<cPt2di> aIFront;
     FrontBlackCC(aIFront,aMarq);

     std::vector<cPt2dr> aEllFr;
     SelEllAndRefineFront(aEllFr,aIFront);

     if (aEllFr.size() < 6)
     {
        return;
     }

     mIsOk = true;

     cEllipse_Estimate anEE(mC,false);
     for (const auto & aPixFr : aEllFr)
     {
         anEE.AddPt(aPixFr);
     }

     mEll = anEE.Compute();

     mV1 = mEll.InterSemiLine(mTetas[0]);
     mV2 = mEll.InterSemiLine(mTetas[1]);
}

bool cCdEllipse::IsOk() const {return mIsOk;}
void cCdEllipse::AssertOk() const
{
    MMVII_INTERNAL_ASSERT_tiny(mIsOk,"No ellipse Ok in cCdEllipse");
}

const cEllipse & cCdEllipse::Ell() const {AssertOk(); return mEll;}
const cPt2dr & cCdEllipse::V1() const {AssertOk(); return mV1;}
const cPt2dr & cCdEllipse::V2() const {AssertOk(); return mV2;}

/* ***************************************************** */
/*                                                       */
/*                    cTmpCdRadiomPos                    */
/*                                                       */
/* ***************************************************** */


cTmpCdRadiomPos::cTmpCdRadiomPos(const cCdRadiom & aCDR,tREAL8 aThickness) :
    cCdRadiom   (aCDR),
    mThickness  (aThickness),
    mSeg0       (mC,mC+FromPolar(1.0,mTetas[0])),
    mSeg1       (mC,mC+FromPolar(1.0,mTetas[1]))
{
}

std::pair<eTPosCB,tREAL8>  cTmpCdRadiomPos::TheorRadiom(const cPt2dr &aPt) const
{
    eTPosCB aPos = eTPosCB::eUndef;
    tREAL8 aGrayTh = -1;

    // we compute locacl coordinates because the sign of y indicate if we are left/right of the oriented segment
    // and sign of x indicate if we are before/after the centre
    cPt2dr aLoc0 = mSeg0.ToCoordLoc(aPt);
    tREAL8  aY0 = aLoc0.y();

    cPt2dr aLoc1 = mSeg1.ToCoordLoc(aPt);
    tREAL8  aY1 = aLoc1.y();

    // compute if we are far enough of S0/S1 because the computation of gray will change
    //  black/white if far  enough, else interpolation
    bool FarS0 = std::abs(aY0)> mThickness; 
    bool FarS1 = std::abs(aY1)> mThickness;

    if ( FarS0 && FarS1)
    {
       if ((aY0>0)!=(aY1>0))
       {
           aPos = eTPosCB::eInsideBlack;
	   aGrayTh = 0.0;
       }
       else
       {
           aPos = eTPosCB::eInsideWhite;
	   aGrayTh = 1.0;
       }
    }
    else if  ((!FarS0) && FarS1)
    {
        // (! FarS0) => teta1
        // Red = teta1 , black on left on image, right on left in coord oriented
	 aPos = eTPosCB::eBorderRight;
         int aSignX = (aLoc0.x() >0) ? -1 : 1;
         aGrayTh = (mThickness+aSignX*aY0) / (2.0*mThickness);
    }
    else if  (FarS0 && (!FarS1))
    {
	 aPos = eTPosCB::eBorderLeft;
	 int aSignX = (aLoc1.x() <0) ? -1 : 1;
	 aGrayTh = (mThickness+aSignX*aY1) / (2.0 * mThickness);
    }

    return std::pair<eTPosCB,tREAL8>(aPos,aGrayTh);
}




/*  *********************************************************** */
/*                                                              */
/*              cAppliCheckBoardTargetExtract                   */
/*                                                              */
/*  *********************************************************** */

class cScoreTetaLine;

class cAppliCheckBoardTargetExtract : public cMMVII_Appli
{
     public :
        typedef tREAL4            tElem;
        typedef cIm2D<tElem>      tIm;
        typedef cDataIm2D<tElem>  tDIm;
        typedef cAffin2D<tREAL8>  tAffMap;


        cAppliCheckBoardTargetExtract(const std::vector<std::string> & aVArgs,const cSpecMMVII_Appli & aSpec);

     private :
        // =========== overridding cMMVII_Appli::methods ============
        int Exe() override;
        cCollecSpecArg2007 & ArgObl(cCollecSpecArg2007 & anArgObl) override ;
        cCollecSpecArg2007 & ArgOpt(cCollecSpecArg2007 & anArgOpt) override ;

        bool IsPtTest(const cPt2dr & aPt) const;  ///< Is it a point marqed a test


	/// Method called do each image
	void DoOneImage() ;
	    ///  Read Image from files, init Label Image, init eventually masq 4 debug, compute blurred version of input image
           void ReadImagesAndBlurr();
	   /// compute points that are "topologicall" : memorized in  label image as  label "eTopoTmpCC"
           void ComputeTopoSadles();
	   /// start from topo point, compute the "saddle" criterion, and use it for filtering on relative  max
           void SaddleCritFiler() ;
	   /// start from saddle point, optimize position on symetry criterion then filter on sym thresholds
           void SymetryFiler() ;

	void MakeImageSaddlePoints(const tDIm &,const cDataIm2D<tU_INT1> & aDMasq) const;

	cPhotogrammetricProject     mPhProj;
	cTimerSegm                  mTimeSegm;

        cCdRadiom TestBinarization(cScoreTetaLine&,const cCdSym &,tREAL8 aThickness);

        // =========== Mandatory args ============

	std::string mNameIm;       ///< Name of background image
	std::string mNameSpecif;   ///< Name of specification file

        // =========== Optionnal args ============

                //  --

	tREAL8            mThickness;  ///<  used for fine estimation of radiom
        bool              mOptimSegByRadiom;  ///< Do we optimize the segment on average radiom     

        tREAL8            mLengtSInit;      ///<  = 05.0;
        tREAL8            mLengtProlong;    ///<  = 20.0;
        tREAL8            mStepSeg;         ///<  = 0.5;
        tREAL8            mMaxCostCorrIm;   ///<  = 0.1;
	int               mNbBlur1;         ///< = 4,  Number of initial blurring
					    //
        // ---------------- Thresholds for Saddle point criteria --------------------
        tREAL8            mDistMaxLocSad ;  ///< =10.0, for supressing sadle-point,  not max loc in a neighboorhoud
        int               mDistRectInt;     ///< = 20,  insideness of points  for seed detection
        size_t            mMaxNbSP_ML0 ;   ///< = 30000  Max number of best point  saddle points, before MaxLoc
        size_t            mMaxNbSP_ML1  ;   ///< = 2000   Max number of best point  saddle points, after  MaxLoc
        cPt2di            mPtLimCalcSadle;  ///< =(2,1)  limit point for calc sadle neighbour , included, 

        // ---------------- Thresholds for Symetry  criteria --------------------
        tREAL8            mThresholdSym  ;  ///< = 0.5,  threshlod for symetry criteria
        tREAL8            mDistCalcSym0  ;  ///< = 8.0  distance for evaluating symetry criteria
        tREAL8            mDistDivSym    ;  ///< = 2.0  maximal distance to initial value in symetry opt

	int               mNumDebug;
        // =========== Internal param ============
        tIm                   mImIn;        ///< Input global image
        cPt2di                mSzIm;        ///< Size of image
	tDIm *                mDImIn;       ///< Data input image 
        tIm                   mImBlur;      ///< Blurred image, used in pre-detetction
	tDIm *                mDImBlur;     ///< Data input image 
	bool                  mHasMasqTest; ///< Do we have a test image 4 debuf (with masq)
	cIm2D<tU_INT1>        mMasqTest;    ///< Possible image of mas 4 debug, print info ...
        cIm2D<tU_INT1>        mImLabel;     ///< Image storing labels of centers
	cDataIm2D<tU_INT1> *  mDImLabel;    ///< Data Image of label
        cIm2D<tU_INT1>        mImTmp;       ///< Temporary image for connected components
	cDataIm2D<tU_INT1> *  mDImTmp;      ///< Data Image of "mImTmp"

        std::vector<cCdSadle> mVCdtSad;     ///< Candidate  that are selected as local max of saddle criteria
        std::vector<int>      mNbSads;      ///< For info, number of sadle points at different step
        std::vector<cCdSym>   mVCdtSym;     ///< Candidate that are selected on the symetry criteria
};


/* *************************************************** */
/*                                                     */
/*              cAppliCheckBoardTargetExtract          */
/*                                                     */
/* *************************************************** */

cAppliCheckBoardTargetExtract::cAppliCheckBoardTargetExtract(const std::vector<std::string> & aVArgs,const cSpecMMVII_Appli & aSpec) :
   cMMVII_Appli     (aVArgs,aSpec),
   mPhProj          (*this),
   mTimeSegm        (this),
   mThickness       (1.0),
   mOptimSegByRadiom (false),
   mLengtSInit       (5.0),
   mLengtProlong     (20.0),
   mStepSeg          (0.5),
   mMaxCostCorrIm    (0.1),
   mNbBlur1          (4),
   mDistMaxLocSad    (10.0),
   mDistRectInt      (20),
   mMaxNbSP_ML0      (30000),
   mMaxNbSP_ML1      (2000),
   mPtLimCalcSadle   (2,1),
   mThresholdSym     (0.5),
   mDistCalcSym0     (8.0),
   mDistDivSym       (2.0),
   mNumDebug         (-1),
   mImIn             (cPt2di(1,1)),
   mDImIn            (nullptr),
   mImBlur           (cPt2di(1,1)),
   mDImBlur          (nullptr),
   mHasMasqTest      (false),
   mMasqTest         (cPt2di(1,1)),
   mImLabel          (cPt2di(1,1)),
   mDImLabel         (nullptr),
   mImTmp            (cPt2di(1,1)),
   mDImTmp           (nullptr)
{
}



cCollecSpecArg2007 & cAppliCheckBoardTargetExtract::ArgObl(cCollecSpecArg2007 & anArgObl)
{
   return  anArgObl
             <<   Arg2007(mNameIm,"Name of image (first one)",{{eTA2007::MPatFile,"0"},eTA2007::FileImage})
             <<   Arg2007(mNameSpecif,"Name of target file")
   ;
}


cCollecSpecArg2007 & cAppliCheckBoardTargetExtract::ArgOpt(cCollecSpecArg2007 & anArgOpt)
{
   return
	        anArgOpt
             <<  mPhProj.DPMask().ArgDirInOpt("TestMask","Mask for selecting point used in detailed mesg/output")
             <<  AOpt2007(mThickness,"Thickness","Thickness for modelizaing line-blur in fine radiom model",{eTA2007::HDV})
             <<  AOpt2007(mOptimSegByRadiom,"OSBR","Optimize segement by radiometry",{eTA2007::HDV})
             <<  AOpt2007(mNumDebug,"NumDebug","Num marq target for debug",{eTA2007::Tuning})
   ;
}


void cAppliCheckBoardTargetExtract::MakeImageSaddlePoints(const tDIm & aDIm,const cDataIm2D<tU_INT1> & aDMasq) const
{
    cRGBImage  aRGB = RGBImFromGray<tElem>(aDIm);

    for (const auto & aPix : cRect2(aDIm.Dilate(-1)))
    {
       if (aDMasq.GetV(aPix) >= (int) eTopoMaxLoc)
       {
          cPt3di  aCoul = cRGBImage::Yellow;
	  if (aDMasq.GetV(aPix)== eFilterSym) aCoul = cRGBImage::Green;
	  if (aDMasq.GetV(aPix)== eFilterRadiom) aCoul = cRGBImage::Red;
          aRGB.SetRGBPix(aPix,aCoul);
       }
    }
    aRGB.ToFile("Saddles.tif");
}

bool cAppliCheckBoardTargetExtract::IsPtTest(const cPt2dr & aPt) const
{
   return mHasMasqTest && (mMasqTest.DIm().GetV(ToI(aPt)) != 0);
}


bool DebugCB = false;

/*  
 *
 *  (cos(T) U + sin(T) V)^2  =>  1 + 2 cos(T)sin(T) U.V = 1 + sin(2T) U.V, ValMin  -> 1 -U.V
 *
 */

cCdRadiom cAppliCheckBoardTargetExtract::TestBinarization(cScoreTetaLine & aSTL,const cCdSym & aCdSym,tREAL8 aThickness)
{
    bool IsMarqed = IsPtTest(aCdSym.mC);
    static int aCptGlob=0 ; aCptGlob++;
    static int aCptMarq=0 ; if (IsMarqed) aCptMarq++;
    DebugCB = (aCptMarq == mNumDebug) && IsMarqed;

    auto aPairTeta = aSTL.Tetas_CheckBoard(aCdSym.mC,0.1,1e-3);
    tREAL8 aLength = aSTL.Prolongate(mLengtProlong,aPairTeta,true);

    auto [aTeta0,aTeta1] = aPairTeta;

    cCdRadiom aCdRadiom(aCdSym,*mDImIn,aTeta0,aTeta1,aLength,aThickness);

    if (mOptimSegByRadiom)
    {
       aCdRadiom.OptimSegIm(*(aSTL.DIm()),aSTL.LengthCur());
    }

    if (IsMarqed)
    {
          aCdRadiom.ShowDetail(aCptMarq,aSTL,mNameIm,*mDImTmp);
    }

    return aCdRadiom;
}


void cAppliCheckBoardTargetExtract::ReadImagesAndBlurr()
{
    /* [0]    Initialise : read image and mask */

    cAutoTimerSegm aTSInit(mTimeSegm,"0-Init");

	// [0.0]   read image
    mImIn =  tIm::FromFile(mNameIm);
    mDImIn = &mImIn.DIm() ;
    mSzIm = mDImIn->Sz();

	// [0.1]   initialize labeling image 
    mDImLabel =  &(mImLabel.DIm());
    mDImLabel->Resize(mSzIm);
    mDImLabel->InitCste(eNone);

    mDImTmp = &(mImTmp.DIm() );
    mDImTmp->Resize(mSzIm);
    mDImTmp->InitCste(0);

    // [0.2]   Generate potential mask for test points
    mHasMasqTest = mPhProj.ImageHasMask(mNameIm);
    if (mHasMasqTest)
       mMasqTest =  mPhProj.MaskOfImage(mNameIm,*mDImIn);

    /* [1]   Compute a blurred image => less noise, less low level saddle */

    cAutoTimerSegm aTSBlur(mTimeSegm,"1-Blurr");

    mImBlur  = mImIn.Dup(); // create image blurred with less noise
    mDImBlur = &(mImBlur.DIm());

    SquareAvgFilter(*mDImBlur,mNbBlur1,1,1); // 1,1 => Nbx,Nby
}

void cAppliCheckBoardTargetExtract::ComputeTopoSadles()
{
    cAutoTimerSegm aTSTopoSad(mTimeSegm,"2.0-TopoSad");
    cRect2 aRectInt = mDImIn->Dilate(-mDistRectInt); // Rectangle excluding point too close to border

         // 2.1  point with criteria on conexity of point > in neighoor

    for (const auto & aPix : aRectInt)
    {
        if (FlagSup8Neigh(*mDImBlur,aPix).NbConComp() >=4)
	{
            mDImLabel->SetV(aPix,eTopo0);
	}
    }

         // 2.2  as often there 2 "touching" point with this criteria
	 // select 1 point in conected component

    cAutoTimerSegm aTSMaxCC(mTimeSegm,"2.1-MaxCCSad");
    int aNbCCSad=0;
    std::vector<cPt2di>  aVCC;
    const std::vector<cPt2di> & aV8 = Alloc8Neighbourhood();

    for (const auto& aPix : *mDImLabel)
    {
         if (mDImLabel->GetV(aPix)==eTopo0)
	 {
             aNbCCSad++;
             ConnectedComponent(aVCC,*mDImLabel,aV8,aPix,eTopo0,eTopoTmpCC);
	     cWhichMax<cPt2di,tREAL8> aBestPInCC;
	     for (const auto & aPixCC : aVCC)
	     {
                 aBestPInCC.Add(aPixCC,CriterionTopoSadle(*mDImBlur,aPixCC));
	     }

	     cPt2di aPCC = aBestPInCC.IndexExtre();
	     mDImLabel->SetV(aPCC,eTopoMaxOfCC);
	 }
    }
}

/**  The saddle criteria is defined by fitting a quadratic function on the image. Having computed the eigen value of quadratic function :
 *
 *      - this criteria is 0 if they have same sign
 *      - else it is the smallest eigen value
 *
 *   This fitting is done a smpothed version of the image :
 *      - it seem more "natural" for fitting a smooth model
 *      - it limits the effect of delocalization
 *      - it (should be) is not a problem as long as the kernel is smaller than the smallest checkbord we want to detect
 *
 *    As it is used on a purely relative criteria, we dont have to bother how it change the value.
 *     
 */
void cAppliCheckBoardTargetExtract::SaddleCritFiler() 
{
    cAutoTimerSegm aTSCritSad(mTimeSegm,"3.0-CritSad");

    cCalcSaddle  aCalcSBlur(Norm2(mPtLimCalcSadle)+0.001,1.0); // structure for computing saddle criteria

       // [3.1]  compute for each point the saddle criteria
    for (const auto& aPix : *mDImLabel)
    {
         if (mDImLabel->GetV(aPix)==eTopoMaxOfCC)
	 {
             tREAL8 aCritS = aCalcSBlur.CalcSaddleCrit(*mDImBlur,aPix);
             mVCdtSad.push_back(cCdSadle(ToR(aPix),aCritS));
	 }
    }
    mNbSads.push_back(mVCdtSad.size()); // memo size for info 

    //   [3.2]    sort by decreasing criteria of saddles => "-"  aCdt.mSadCrit + limit size
    cAutoTimerSegm aTSMaxLoc(mTimeSegm,"3.1-MaxLoc");

    SortOnCriteria(mVCdtSad,[](const auto & aCdt){return - aCdt.mSadCrit;});
    ResizeDown(mVCdtSad,mMaxNbSP_ML0);   
    mNbSads.push_back(mVCdtSad.size()); // memo size for info 


    //   [3.3]  select  MaxLocal
    mVCdtSad = FilterMaxLoc((cPt2dr*)nullptr,mVCdtSad,[](const auto & aCdt) {return aCdt.mC;}, mDistMaxLocSad);
    mNbSads.push_back(mVCdtSad.size()); // memo size for info 

    //   [3.3]  select KBest + MaxLocal
    //  limit the number of point , a bit rough but first experiment show that sadle criterion is almost perfect on good images
    // mVCdtSad.resize(std::min(mVCdtSad.size(),size_t(mMaxNbMLS)));
    ResizeDown(mVCdtSad,mMaxNbSP_ML1);
    mNbSads.push_back(mVCdtSad.size()); // memo size for info 

    for (const auto & aCdt : mVCdtSad)
        mDImLabel->SetV(ToI(aCdt.mC),eTopoMaxLoc);
}

void cAppliCheckBoardTargetExtract::SymetryFiler()
{
    cAutoTimerSegm aTSSym(mTimeSegm,"4-SYM");
    cFilterDCT<tREAL4> * aFSym = cFilterDCT<tREAL4>::AllocSym(mImIn,0.0,mDistCalcSym0,1.0);
    cOptimByStep<2> aOptimSym(*aFSym,true,mDistDivSym);

    for (auto & aCdtSad : mVCdtSad)
    {
        auto [aValSym,aNewP] = aOptimSym.Optim(aCdtSad.mC,1.0,0.01);  // Pos Init, Step Init, Step Lim
        aCdtSad.mC = aNewP;

        if (aValSym < mThresholdSym)
        {
           mVCdtSym.push_back(cCdSym(aCdtSad,aValSym));
           mDImLabel->SetV(ToI(aNewP),eFilterSym);
        }
    }

    delete aFSym;
}

void cAppliCheckBoardTargetExtract::DoOneImage() 
{ 
    /* [0]    Initialise : read image ,  mask + Blurr */
    ReadImagesAndBlurr();
    /* [2]  Compute "topological" saddle point */
    ComputeTopoSadles();
    /* [3]  Compute point that are max local of  saddle point criteria */
    SaddleCritFiler();
    /* [4]  Calc Symetry criterion */
    SymetryFiler();

    /* [5]  Compute lines, radiom model & correlation */
    std::vector<cCdRadiom> aVCdtRad;
    cAutoTimerSegm aTSRadiom(mTimeSegm,"Radiom");
    {
        cCubicInterpolator aCubI(-0.5);
        cScoreTetaLine  aSTL(*mDImIn,aCubI,mLengtSInit,mStepSeg);
        for (const auto & aCdtSym : mVCdtSym)
        {
            cCdRadiom aCdRad = TestBinarization(aSTL,aCdtSym,mThickness);
	    if (aCdRad.mCostCorrel <= mMaxCostCorrIm)
	    {
               aVCdtRad.push_back(aCdRad);
	       mDImLabel->SetV(ToI(aCdRad.mC),eFilterRadiom);
	    }
        }
    }


    cAutoTimerSegm aTSMakeIm(mTimeSegm,"OTHERS");
    MakeImageSaddlePoints(*mDImIn,*mDImLabel);

    StdOut()  << "NB Cd,  SAD: " << mNbSads
	      << " SYM:" << mVCdtSym.size() 
	      << " Radiom:" << aVCdtRad.size() << "\n";
}

/*
 *  Dist= sqrt(5)
 *  T=6.3751
 *  2 sqrt->6.42483
 */


int  cAppliCheckBoardTargetExtract::Exe()
{
   mPhProj.FinishInit();

   if (RunMultiSet(0,0))
   {
       return ResultMultiSet();
   }

   DoOneImage();

   return EXIT_SUCCESS;
}

/* =============================================== */
/*                                                 */
/*                       ::                        */
/*                                                 */
/* =============================================== */

tMMVII_UnikPApli Alloc_CheckBoardCodedTarget(const std::vector<std::string> &  aVArgs,const cSpecMMVII_Appli & aSpec)
{
   return tMMVII_UnikPApli(new cAppliCheckBoardTargetExtract(aVArgs,aSpec));
}

cSpecMMVII_Appli  TheSpecExtractCheckBoardTarget
(
     "CodedTargetCheckBoardExtract",
      Alloc_CheckBoardCodedTarget,
      "Extract coded target from images",
      {eApF::CodedTarget,eApF::ImProc},
      {eApDT::Image,eApDT::Xml},
      {eApDT::Image,eApDT::Xml},
      __FILE__
);


};
