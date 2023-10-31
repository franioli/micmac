#include "MMVII_PCSens.h"
#include "MMVII_MMV1Compat.h"
#include "MMVII_DeclareCste.h"
#include "MMVII_BundleAdj.h"

/**
   \file cConvCalib.cpp  testgit

   \brief file for conversion between calibration (change format, change model) and tests
*/


namespace MMVII
{


   /* ********************************************************** */
   /*                                                            */
   /*                 cAppli_ImportOri                           */
   /*                                                            */
   /* ********************************************************** */

class cAppli_ImportOri : public cMMVII_Appli
{
     public :
        cAppli_ImportOri(const std::vector<std::string> & aVArgs,const cSpecMMVII_Appli & aSpec);
        int Exe() override;
        cCollecSpecArg2007 & ArgObl(cCollecSpecArg2007 & anArgObl) override ;
        cCollecSpecArg2007 & ArgOpt(cCollecSpecArg2007 & anArgOpt) override ;

        std::vector<std::string>  Samples() const override;
     private :

	cPhotogrammetricProject  mPhProj;

	// Mandatory Arg
	std::string              mNameFile;
	std::string              mFormat;


	// Optionall Arg
	int                      mL0;
	int                      mLLast;
	char                     mComment;
	eTyUnitAngle             mAngleUnit;
	std::string              mRepIJK;
	bool                     mRepIJDir;
	std::vector<std::string> mChgName;
};

cAppli_ImportOri::cAppli_ImportOri(const std::vector<std::string> & aVArgs,const cSpecMMVII_Appli & aSpec) :
   cMMVII_Appli  (aVArgs,aSpec),
   mPhProj       (*this),
   mL0           (0),
   mLLast        (-1),
   mComment      ('#'),
   mAngleUnit    (eTyUnitAngle::eUA_radian),
   mRepIJK       ("ijk"),
   mRepIJDir     (false)
{
}

cCollecSpecArg2007 & cAppli_ImportOri::ArgObl(cCollecSpecArg2007 & anArgObl) 
{
    return anArgObl
	      <<  Arg2007(mNameFile ,"Name of Input File")
	      <<  Arg2007(mFormat   ,"Format of file as for ex \"SNSXYZSS\" ")
              <<  mPhProj.DPOrient().ArgDirInMand("Ori folder to extract calibration")
              <<  mPhProj.DPOrient().ArgDirOutMand()
           ;
}

cCollecSpecArg2007 & cAppli_ImportOri::ArgOpt(cCollecSpecArg2007 & anArgObl) 
{
    
    return anArgObl
       << AOpt2007(mL0,"NumL0","Num of first line to read",{eTA2007::HDV})
       << AOpt2007(mLLast,"NumLast","Num of last line to read (-1 if at end of file)",{eTA2007::HDV})
       << AOpt2007(mComment,"Com","Carac for commentary",{eTA2007::HDV})
       << AOpt2007(mAngleUnit,"AngU","Unity for angles",{{eTA2007::HDV},{AC_ListVal<eTyUnitAngle>()}})
       << AOpt2007(mRepIJK,"Rep","Repair coded (relative  to MMVII convention)  ",{{eTA2007::HDV}})
       << AOpt2007(mRepIJDir,"KIsUp","Corespond to repair \"i-j-k\" ",{{eTA2007::HDV}})
       << AOpt2007(mChgName,"ChgN","Change name [Pat,Name], for ex \"[(.*),IMU_\\$0]\"  add prefix \"IMU_\" ",{{eTA2007::ISizeV,"[2,2]"}})
    ;
}

std::vector<std::string>  cAppli_ImportOri::Samples() const
{
   return {
              "MMVII ImportOri trajectographie_tif.opk NSSXYZWPKS Calib InitUP AngU=degree KIsUp=true ChgN=[\".*\",\"Traj_\\$0\"]"
	};
}

// "NSSXYZWPKS"

int cAppli_ImportOri::Exe()
{
    mPhProj.FinishInit();
    std::vector<std::vector<std::string>> aVNames;
    std::vector<std::vector<double>> aVNums;
    std::vector<cPt3dr> aVXYZ,aVWKP;

    if (mRepIJDir)
       mRepIJK = "i-j-k";

    cRotation3D<tREAL8>  aRotAfter = cRotation3D<tREAL8>::RotFromCanonicalAxes(mRepIJK);

    MMVII_INTERNAL_ASSERT_tiny(CptSameOccur(mFormat,"XYZN")==1,"Bad format vs NXYZ");

    ReadFilesStruct
    (
        mNameFile, mFormat,
        mL0, mLLast, mComment,
        aVNames,aVXYZ,aVWKP,aVNums
    );

    tREAL8  aAngDiv = AngleInRad(mAngleUnit);

    for (size_t aK=0 ; aK<aVXYZ.size() ; aK++)
    {
         std::string aNameIm = aVNames.at(aK).at(0);

	 ChgName(mChgName,aNameIm);
	 cPt3dr aCenter = aVXYZ.at(aK);
	 cPt3dr aWPK = aVWKP.at(aK) / aAngDiv ;

	 cPerspCamIntrCalib *  aCalib = mPhProj.InternalCalibFromStdName(aNameIm);

	 cRotation3D<tREAL8>  aRot =  cRotation3D<tREAL8>::RotFromWPK(aWPK);
	 aRot = aRot * aRotAfter;

	 cIsometry3D aPose(aCenter,aRot);
	 cSensorCamPC  aCam(aNameIm,aPose,aCalib);

	 mPhProj.SaveCamPC(aCam);


	 StdOut() << "NAME =" <<   aNameIm << " xyz=" << aVXYZ.at(aK)  << " WPK=" << aWPK  << " F=" << aCalib->F() << "\n";
	 // getchar();
    }


    return EXIT_SUCCESS;
}




tMMVII_UnikPApli Alloc_ImportOri(const std::vector<std::string> & aVArgs,const cSpecMMVII_Appli & aSpec)
{
   return tMMVII_UnikPApli(new cAppli_ImportOri(aVArgs,aSpec));
}

cSpecMMVII_Appli  TheSpec_ImportOri
(
     "ImportOri",
      Alloc_ImportOri,
      "Import/Convert basic Orient file in MMVII format",
      {eApF::Ori},
      {eApDT::Ori},
      {eApDT::Ori},
      __FILE__
);


}; // MMVII

