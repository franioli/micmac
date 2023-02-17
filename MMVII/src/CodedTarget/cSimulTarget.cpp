#include "CodedTarget.h"
#include "MMVII_2Include_Serial_Tpl.h"
#include "MMVII_Tpl_Images.h"
#include "MMVII_Interpolators.h"


// Test git branch

namespace MMVII
{

namespace  cNS_CodedTarget
{

/*  *********************************************************** */
/*                                                              */
/*                   cGeomSimDCT                                */
/*                                                              */
/*  *********************************************************** */

bool cGeomSimDCT::Intersect(const cGeomSimDCT &  aG2) const
{
     return  Norm2(mC-aG2.mC) < mR2+aG2.mR2 ;
}
cGeomSimDCT::cGeomSimDCT() :
    mResExtr (nullptr)
{
}

void cGeomSimDCT::Translate(const cPt2dr & aTr)
{
    mC += aTr;       
    mCornEl1 += aTr; 
    mCornEl2 += aTr;
}


cGeomSimDCT::cGeomSimDCT(int aNum,const  cPt2dr& aC,const double& aR1,const double& aR2):
    mResExtr (nullptr),
    mNum (aNum),
    mC   (aC),
    mR1  (aR1),
    mR2  (aR2)
{
}

void AddData(const  cAuxAr2007 & anAux,cGeomSimDCT & aGSD)
{
   MMVII::AddData(cAuxAr2007("Num",anAux),aGSD.mNum);
   MMVII::AddData(cAuxAr2007("Name",anAux),aGSD.name);
   MMVII::AddData(cAuxAr2007("Center",anAux),aGSD.mC);
   MMVII::AddData(cAuxAr2007("CornEl1",anAux),aGSD.mCornEl1);
   MMVII::AddData(cAuxAr2007("CornEl2",anAux),aGSD.mCornEl2);
   MMVII::AddData(cAuxAr2007("R1",anAux),aGSD.mR1);
   MMVII::AddData(cAuxAr2007("R2",anAux),aGSD.mR2);
}

/*  *********************************************************** */
/*                                                              */
/*                      cResSimul                               */
/*                                                              */
/*  *********************************************************** */

cResSimul::cResSimul() :
    mRayMinMax (15.0,60.0),
    mBorder    (1.0),
    mRatioMax  (3.0)
{
}

double cResSimul::BorderGlob() const
{
    return mBorder * mRayMinMax.y();
}

void AddData(const  cAuxAr2007 & anAux,cResSimul & aRS)
{
   MMVII::AddData(cAuxAr2007("Com",anAux),aRS.mCom);
   MMVII::AddData(cAuxAr2007("RayMinMax",anAux),aRS.mRayMinMax);
   MMVII::AddData(cAuxAr2007("RatioMax",anAux),aRS.mRatioMax);
   MMVII::AddData(cAuxAr2007("Geoms",anAux),aRS.mVG);
}

cResSimul  cResSimul::FromFile(const std::string& aName)
{
   cResSimul aRes;

   ReadFromFile(aRes,aName);
   return aRes;
}



/*  *********************************************************** */
/*                                                              */
/*              cAppliSimulCodeTarget                           */
/*                                                              */
/*  *********************************************************** */


class cAppliSimulCodeTarget : public cMMVII_Appli
{
     public :
        typedef tREAL4            tElem;
        typedef cIm2D<tElem>      tIm;
        typedef cDataIm2D<tElem>  tDIm;
        typedef cAffin2D<tREAL8>  tAffMap;


        cAppliSimulCodeTarget(const std::vector<std::string> & aVArgs,const cSpecMMVII_Appli & aSpec);

     private :
        // =========== overridding cMMVII_Appli::methods ============
        int Exe() override;
        cCollecSpecArg2007 & ArgObl(cCollecSpecArg2007 & anArgObl) override ;
        cCollecSpecArg2007 & ArgOpt(cCollecSpecArg2007 & anArgOpt) override ;


        // =========== other methods ============

        void  AddPosTarget(int aNum);  ///< Add the position of the target, don insert it
        void  IncrustTarget(cGeomSimDCT & aGSD);

	double RandomRay() const;


        // =========== Mandatory args ============
	std::string mNameIm;
	std::string mNameTarget;

        // =========== Optionnal args ============
	cResSimul           mRS;
	int                 mPerN;
        double              mSzKernel;

                //  --
	double              mDownScale;
	double              mAttenGray;
	double              mPropSysLin;
	double              mAmplWhiteNoise;

        // =========== Internal param ============
        tIm                        mImIn;
        cParamCodedTarget          mPCT;
	std::string                mDirTarget;
	std::string                mPrefixOut;
};


/* *************************************************** */
/*                                                     */
/*              cAppliSimulCodeTarget                  */
/*                                                     */
/* *************************************************** */

cAppliSimulCodeTarget::cAppliSimulCodeTarget(const std::vector<std::string> & aVArgs,const cSpecMMVII_Appli & aSpec) :
   cMMVII_Appli     (aVArgs,aSpec),
   mPerN            (1),
   mSzKernel        (2.0),
   mDownScale       (3.0),
   mAttenGray       (0.2),
   mPropSysLin      (0.2),
   mAmplWhiteNoise  (0.1),
   mImIn            (cPt2di(1,1))
{
}

cCollecSpecArg2007 & cAppliSimulCodeTarget::ArgObl(cCollecSpecArg2007 & anArgObl)
{
   return  anArgObl
             <<   Arg2007(mNameIm,"Name of image (first one)",{{eTA2007::MPatFile,"0"},eTA2007::FileImage})
             <<   Arg2007(mNameTarget,"Name of target file")
   ;
}


cCollecSpecArg2007 & cAppliSimulCodeTarget::ArgOpt(cCollecSpecArg2007 & anArgOpt)
{
   return
	        anArgOpt
             <<   AOpt2007(mRS.mRayMinMax,"Rays","Min/Max ray for gen target",{eTA2007::HDV})
             <<   AOpt2007(mSzKernel,"SzK","Sz of Kernel for interpol",{eTA2007::HDV})
             <<   AOpt2007(mRS.mBorder,"Border","Border w/o target, prop to R Max",{eTA2007::HDV})

             <<   AOpt2007(mPerN,"PerN","Period for target, to doc quick in test",{eTA2007::HDV,eTA2007::Tuning})
   ;
}

double cAppliSimulCodeTarget::RandomRay() const { return RandInInterval(mRS.mRayMinMax.x(),mRS.mRayMinMax.y());}



void   cAppliSimulCodeTarget::AddPosTarget(int aNum)
{
     cBox2dr aBoxGenerate = mImIn.DIm().ToR().Dilate(-mRS.BorderGlob());
     for (int aK=0 ; aK< 200 ; aK++)
     {
        cPt2dr  aC = aBoxGenerate.GeneratePointInside();
	// StdOut() << "HHH " << aC << " K=" << aK << "\n";
        //  Compute two random ray in the given interval
        double  aR1 = RandomRay() ;
        double  aR2 = RandomRay() ;
	OrderMinMax(aR1,aR2);  //   assure  aR1 <= aR2
	if (aR2/aR1 > mRS.mRatioMax)  // assure that  R2/R1 <= RatioMax
	{
            double aR = sqrt(aR1*aR2);
	    aR1 = aR / sqrt(mRS.mRatioMax);
	    aR2 = aR * sqrt(mRS.mRatioMax);
	}
        cGeomSimDCT aGSD(aNum,aC,aR1,aR2);
	bool GotClose = false;
	for (const auto& aG2 : mRS.mVG)
            GotClose = GotClose || aG2.Intersect(aGSD);
	if (! GotClose)
	{
            mRS.mVG.push_back(aGSD);
	    return;
	}
     }
}

void  cAppliSimulCodeTarget::IncrustTarget(cGeomSimDCT & aGSD)
{
    aGSD.name = mPCT.NameOfNum(aGSD.mNum);
    std::string aName = mDirTarget + mPCT.NameFileOfNum(aGSD.mNum);
    tIm aImT =  tIm::FromFile(aName).GaussDeZoom(mDownScale,5);
    tDIm & aDImT = aImT.DIm();
    cPt2dr aSz = ToR(aDImT.Sz());
    cPt2dr aC0 = mPCT.mCenterF/mDownScale;

    cPt2dr aDirModif = FromPolar(1.0,M_PI*RandUnif_C());
    double aDiag = Norm2(aC0);
    double aAtten = mAttenGray * RandUnif_0_1();
    double aAttenLin  = mPropSysLin * RandUnif_0_1();
    for (const auto & aPix : aDImT)
    {
         double aVal = aDImT.GetV(aPix);
	 aVal =  128  + (aVal-128) * (1-aAtten)   ;
         double aScal = Scal(ToR(aPix)-aC0,aDirModif) / aDiag;
	 aVal =  128  + (aVal-128) * (1-aAttenLin)  + aAttenLin * aScal * 128;
	 //
	 aDImT.SetV(aPix,aVal);
    }



    tAffMap aMap0 =  tAffMap::Translation(-aC0);
    tAffMap aMap1 =  tAffMap::Rotation(M_PI*RandUnif_C());
    tAffMap aMap2 =  tAffMap::HomotXY(aGSD.mR1/aSz.x(),aGSD.mR2/aSz.y());
    tAffMap aMap3 =  tAffMap::Rotation(M_PI*RandUnif_C());
    tAffMap aMap4 =  tAffMap::Translation(aGSD.mC);

    tAffMap aMapT2Im =  aMap4 * aMap3 * aMap2 * aMap1 * aMap0;
    tAffMap aMapIm2T =  aMapT2Im.MapInverse();


    cBox2di aBoxIm = ImageOfBox(aMapT2Im,aDImT.ToR()).Dilate(mSzKernel+2).ToI();

    tDIm & aDImIn = mImIn.DIm();
    for (const auto & aPix : cRect2(aBoxIm))
    {
        if ( aDImIn.Inside(aPix))
	{
            cRessampleWeigth  aRW = cRessampleWeigth::GaussBiCub(ToR(aPix),aMapIm2T,mSzKernel);
	    const std::vector<cPt2di>  & aVPts = aRW.mVPts;
	    if (!aVPts.empty())
	    {
                double aSomW = 0.0;
                double aSomVW = 0.0;
	        for (int aK=0; aK<int(aVPts.size()) ; aK++)
	        {
                    if (aDImT.Inside(aVPts[aK]))
                    {
                       double aW = aRW.mVWeight[aK];
		       aSomW  += aW;
		       aSomVW += aW * aDImT.GetV(aVPts[aK]);
                    }
	        }
		aSomVW =  aSomVW * (1- mAmplWhiteNoise) +  mAmplWhiteNoise * 128 * RandUnif_C() * aSomW;
	        double aVal = aSomVW + (1-aSomW)*aDImIn.GetV(aPix);
	        aDImIn.SetV(aPix,aVal);
	    }
	}
    }
    aGSD.mCornEl1 = aMapT2Im.Value(mPCT.mCornEl1/mDownScale);
    aGSD.mCornEl2 = aMapT2Im.Value(mPCT.mCornEl2/mDownScale);

#if 0  // Marking point specific, do it only for tuning
    {
        for (const auto & aDec : cRect2::BoxWindow(0))
        {
             aDImIn.SetV(ToI(aGSD.mCornEl1)+aDec,128);
             aDImIn.SetV(ToI(aGSD.mCornEl2)+aDec,128);
        }
     }
#endif
    
    StdOut() << "NNN= " << aName << " C0=" << aC0 <<  aBoxIm.Sz() <<  " " << aGSD.mR2/aGSD.mR1 << "\n";
}

int  cAppliSimulCodeTarget::Exe()
{
   mPrefixOut =  "SimulTarget_" + LastPrefix(mNameIm);
   mRS.mCom = CommandOfMain();
   mPCT.InitFromFile(mNameTarget);
   mDirTarget =  DirOfPath(mNameTarget);

   mImIn = tIm::FromFile(mNameIm);


   for (int aNum = 0 ; aNum<mPCT.NbCodeAvalaible() ; aNum+=mPerN)
   {
        AddPosTarget(aNum);
        StdOut() <<  "Ccc=" << mRS.mVG.back().mC << "\n";
   }

   for (auto  & aG : mRS.mVG)
   {
       IncrustTarget(aG);
   }
   SaveInFile(mRS,mPrefixOut +"_GroundTruth.xml");

   mImIn.DIm().ToFile(mPrefixOut+".tif",eTyNums::eTN_U_INT1);



   return EXIT_SUCCESS;
}
};


/* =============================================== */
/*                                                 */
/*                       ::                        */
/*                                                 */
/* =============================================== */
using namespace  cNS_CodedTarget;

tMMVII_UnikPApli Alloc_SimulCodedTarget(const std::vector<std::string> &  aVArgs,const cSpecMMVII_Appli & aSpec)
{
   return tMMVII_UnikPApli(new cAppliSimulCodeTarget(aVArgs,aSpec));
}

cSpecMMVII_Appli  TheSpecSimulCodedTarget
(
     "CodedTargetSimul",
      Alloc_SimulCodedTarget,
      "Extract coded target from images",
      {eApF::CodedTarget,eApF::ImProc},
      {eApDT::Image,eApDT::Xml},
      {eApDT::Image,eApDT::Xml},
      __FILE__
);


};
