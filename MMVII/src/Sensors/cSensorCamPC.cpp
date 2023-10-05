#include "MMVII_2Include_Serial_Tpl.h"
#include "MMVII_PCSens.h"
#include "MMVII_Geom2D.h"


/**
   \file cSensorCamPC.cpp  

   \brief file for implementing class sensor for perspective central
*/


namespace MMVII
{

/* ******************************************************* */
/*                                                         */
/*                   cSensorCamPC                          */
/*                                                         */
/* ******************************************************* */

cSensorCamPC::cSensorCamPC(const std::string & aNameImage,const tPose & aPose,cPerspCamIntrCalib * aCalib) :
   cSensorImage     (aNameImage),
   mPose            (aPose),
   mInternalCalib   (aCalib),
   mOmega           (0,0,0)
{
}

std::vector<cObjWithUnkowns<tREAL8> *>  cSensorCamPC::GetAllUK() 
{
    return std::vector<cObjWithUnkowns<tREAL8> *> {this,mInternalCalib};
}


void cSensorCamPC::PutUknowsInSetInterval()
{
    mSetInterv->AddOneInterv(mPose.Tr());
    mSetInterv->AddOneInterv(mOmega);
}

cPt2dr cSensorCamPC::Ground2Image(const cPt3dr & aP) const
{
     return mInternalCalib->Value(P_W2L(aP));
}


        //  Local(0,0,0) = Center, then mPose Cam->Word, then we use Inverse, BTW Inverse is as efficient as direct
cPt3dr cSensorCamPC::P_W2L(const cPt3dr & aP) const { return       mPose.Inverse(aP); }
cPt3dr cSensorCamPC::V_W2L(const cPt3dr & aP) const { return mPose.Rot().Inverse(aP); }

cPt3dr cSensorCamPC::P_L2W(const cPt3dr & aP) const { return       mPose.Value(aP); }
cPt3dr cSensorCamPC::V_L2W(const cPt3dr & aP) const { return mPose.Rot().Value(aP); }




double cSensorCamPC::DegreeVisibility(const cPt3dr & aP) const
{
     return mInternalCalib->DegreeVisibility(mPose.Inverse(aP));
}

double cSensorCamPC::DegreeVisibilityOnImFrame(const cPt2dr & aP) const
{
     return mInternalCalib->DegreeVisibilityOnImFrame(aP);
}

cPt3dr cSensorCamPC::Ground2ImageAndDepth(const cPt3dr & aP) const
{
    cPt3dr aPCam = mPose.Inverse(aP);  // P in camera coordinate
    cPt2dr aPIm = mInternalCalib->Value(aPCam);

    return cPt3dr(aPIm.x(),aPIm.y(),aPCam.z());
}

const cPt2di & cSensorCamPC::SzPix() const {return  mInternalCalib->SzPix();}

cPt3dr cSensorCamPC::ImageAndDepth2Ground(const cPt3dr & aPImAndD) const
{
    cPt2dr aPIm = Proj(aPImAndD);
    cPt3dr aPCam = mInternalCalib->DirBundle(aPIm);
    cPt3dr aRes =  mPose.Value(aPCam * (aPImAndD.z() / aPCam.z()));
    return aRes;
}

tSeg3dr  cSensorCamPC::Image2Bundle(const cPt2dr & aPIm) const 
{
   return  tSeg3dr(Center(),static_cast<const cSensorImage*>(this)->ImageAndDepth2Ground(aPIm,1.0));
}


const cPt3dr * cSensorCamPC::CenterOfPC() { return  & Center(); }
         /// Return the calculator, adapted to the type, for computing colinearity equation
cCalculator<double> * cSensorCamPC::EqColinearity(bool WithDerives,int aSzBuf,bool ReUse) 
{
   return mInternalCalib->EqColinearity(WithDerives,aSzBuf,ReUse);
}

void cSensorCamPC::PushOwnObsColinearity(std::vector<double> & aVObs)
{
     Pose().Rot().Mat().PushByCol(aVObs);
}

const cPixelDomain & cSensorCamPC::PixelDomain() const 
{
	return mInternalCalib->PixelDomain();
}



/*
size_t  cSensorCamPC::NumXCenter() const
{
   return IndOfVal(&(mPose.Tr().x()));
}
*/

cPerspCamIntrCalib * cSensorCamPC::InternalCalib() {return mInternalCalib;}

const cPt3dr & cSensorCamPC::Center() const {return mPose.Tr();}
const cPt3dr & cSensorCamPC::Omega()  const {return mOmega;}
cPt3dr cSensorCamPC::AxeI()   const {return mPose.Rot().AxeI();}
cPt3dr cSensorCamPC::AxeJ()   const {return mPose.Rot().AxeJ();}
cPt3dr cSensorCamPC::AxeK()   const {return mPose.Rot().AxeK();}
const cIsometry3D<tREAL8> & cSensorCamPC::Pose() const {return mPose;}

cIsometry3D<tREAL8>  cSensorCamPC::RelativePose(const cSensorCamPC& aCam2) const
{
	return mPose.MapInverse()*aCam2.mPose;
}


/*   Let R be the rotation of pose  P=(C,P= : Cam-> Word, what is optimized in colinearity for a ground point G
 *   is Word->Cam  :
 *
 *          tR(G-C)
 *
 * So the optimal rotation R' with get satisfy the equation :
 *
 *          (1+^W) tR0 =tR'   
 *
 * The we have the formula :
 *
 *          --->   R'=R0 t(1+^W)
 *
 *  And note that for axiators :
 *
 *       t Axiator(W) = Axiator(-W)
 *
 */

void cSensorCamPC::OnUpdate()
{
	//  used above formula to modify  rotation
     mPose.SetRotation(mPose.Rot() * cRotation3D<tREAL8>::RotFromAxiator(-mOmega));
        // now this have modify rotation, the "delta" is void :
     mOmega = cPt3dr(0,0,0);
}


//
tREAL8 cSensorCamPC::AngularProjResiudal(const cPair2D3D& aPair) const
{
    cPt2dr aPIm =  aPair.mP2;
    cPt3dr aDirBundleIm = ImageAndDepth2Ground(cPt3dr(aPIm.x(),aPIm.y(),1.0)) - Center();
    cPt3dr aDirProj =  aPair.mP3 - Center();              // direction of projection

    tREAL8 aRes = Norm2(VUnit(aDirBundleIm)-VUnit(aDirProj));  // equivalent to angular distance

    return aRes;
}

tREAL8  cSensorCamPC::AvgAngularProjResiudal(const cSet2D3D& aSet) const
{
   cWeightAv<tREAL8> aWA;

   for (const auto & aPair : aSet.Pairs())
   {
       aWA.Add(1.0,AngularProjResiudal(aPair));
   }

   return aWA.Average();

}


     // =================  READ/WRITE on files ===================

void cSensorCamPC::AddData(const cAuxAr2007 & anAux0)
{
     cAuxAr2007 anAux("CameraPose",anAux0);
     std::string aNameImage = NameImage();
     cPt3dr aC = Center();
     cPt3dr aI = AxeI();
     cPt3dr aJ = AxeJ();
     cPt3dr aK = AxeK();
     cPtxd<tREAL8,4>  aQuat =  MatrRot2Quat(mPose.Rot().Mat());
     std::string      aNameCalib = (anAux.Input() ? "" : mInternalCalib->Name());


     MMVII::AddData(cAuxAr2007("NameImage",anAux),aNameImage);
     MMVII::AddData(cAuxAr2007("NameInternalCalib",anAux),aNameCalib);
     MMVII::AddData(cAuxAr2007("Center",anAux),aC);

    {
        cAuxAr2007 aAuxRot("RotMatrix",anAux);
        MMVII::AddData(cAuxAr2007("AxeI",aAuxRot),aI);
        MMVII::AddData(cAuxAr2007("AxeJ",aAuxRot),aJ);
        MMVII::AddData(cAuxAr2007("AxeK",aAuxRot),aK);
    }
    MMVII::AddData(cAuxAr2007("EQ",anAux),aQuat);
    anAux.Ar().AddComment("EigenQuaternion, for information");

    cPt3dr aWPK = mPose.Rot().ToWPK() *  (180.0/M_PI);
    MMVII::AddData(cAuxAr2007("WPK",anAux),aWPK);
    anAux.Ar().AddComment("Omega Phi Kapa in degree, for information");


    cPt3dr aYPR = mPose.Rot().ToYPR() *  (180.0/M_PI);
    MMVII::AddData(cAuxAr2007("YPR",anAux),aYPR);
    anAux.Ar().AddComment("Yaw Pitch Roll in degree, for information");



    if (anAux.Input())
    {
         SetNameImage(aNameImage);
         mPose = tPose(aC,cRotation3D<tREAL8>(MatFromCols(aI,aJ,aK),false));
	 mTmpNameCalib = aNameCalib;
	 mOmega = cPt3dr(0,0,0);
    }
}

void AddData(const cAuxAr2007 & anAux,cSensorCamPC & aPC)
{
    aPC.AddData(anAux);
}

void cSensorCamPC::ToFile(const std::string & aNameFile) const
{
    SaveInFile(const_cast<cSensorCamPC &>(*this),aNameFile);
    std::string aNameCalib = DirOfPath(aNameFile) + mInternalCalib->Name() + "." + GlobNameDefSerial();
    mInternalCalib->ToFileIfFirstime(aNameCalib);
}

cSensorCamPC * cSensorCamPC::FromFile(const std::string & aFile,bool Remanent)
{
   // Cannot use RemanentObjectFromFile because construction is not standard
   static std::map<std::string,cSensorCamPC*> TheMapRes;
   cSensorCamPC * & anExistingCam = TheMapRes[aFile];

   if (Remanent && (anExistingCam!= nullptr))
      return anExistingCam;


   cSensorCamPC * aPC = new cSensorCamPC("NONE",tPose::Identity(),nullptr);
   ReadFromFile(*aPC,aFile);

   aPC->mInternalCalib =  cPerspCamIntrCalib::FromFile(DirOfPath(aFile) + aPC->mTmpNameCalib + "." + GlobNameDefSerial());
   aPC->mTmpNameCalib = "";

   anExistingCam = aPC;
   return aPC;
}

std::string  cSensorCamPC::NameOri_From_Image(const std::string & aNameImage)
{
   return cSensorImage::NameOri_From_PrefixAndImage(PrefixName(),aNameImage);
}

std::vector<cPt2dr>  cSensorCamPC::PtsSampledOnSensor(int aNbByDim) const 
{
     return  mInternalCalib->PtsSampledOnSensor(aNbByDim,true);
}


std::string  cSensorCamPC::V_PrefixName() const { return PrefixName() ; }
std::string  cSensorCamPC::PrefixName()  { return "PerspCentral";}

void  cSensorCamPC::GetAdrInfoParam(cGetAdrInfoParam<tREAL8> & aGAIP)
{
   aGAIP.TestParam(this, &( mPose.Tr().x()),"Cx");
   aGAIP.TestParam(this, &( mPose.Tr().y()),"Cy");
   aGAIP.TestParam(this, &( mPose.Tr().z()),"Cz");

   aGAIP.TestParam(this, &( mOmega.x())    ,"Wx");
   aGAIP.TestParam(this, &( mOmega.y())    ,"Wy");
   aGAIP.TestParam(this, &( mOmega.z())    ,"Wz");
}
     // =================  becnh ===================

void cSensorCamPC::Bench()
{
   cSet2D3D  aSet32 =  SyntheticsCorresp3D2D(20,3,1.0,10.0) ;
   tREAL8 aRes = AvgAngularProjResiudal(aSet32);

   MMVII_INTERNAL_ASSERT_bench(aRes<1e-8,"Avg res ang");
}

void cSensorCamPC::BenchOneCalib(cPerspCamIntrCalib * aCalib)
{
   cIsometry3D<tREAL8> aPose = cIsometry3D<tREAL8>::RandomIsom3D(10.0);

    cSensorCamPC aCam("BenchCam",aPose,aCalib);
    aCam.Bench();
}


}; // MMVII


