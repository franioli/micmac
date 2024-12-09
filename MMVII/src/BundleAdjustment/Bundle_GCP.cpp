#include "BundleAdjustment.h"


namespace MMVII
{

/* -------------------------------------------------------------- */
/*                cBA_GCP                                         */
/* -------------------------------------------------------------- */

cBA_GCP::cBA_GCP() :
    mMesGCP           (nullptr),
    mSigmaGCP         (-1)
{
}

cBA_GCP::~cBA_GCP() 
{
    DeleteAllAndClear(mGCP_UK);
    delete mMesGCP;
}

/* -------------------------------------------------------------- */
/*                cMMVII_BundleAdj::GCP                           */
/* -------------------------------------------------------------- */

void cMMVII_BundleAdj::AddGCP(const std::string & aName,tREAL8 aSigmaGCP,const  cStdWeighterResidual & aWeighter, cSetMesGndPt *  aMesGCP, bool verbose)
{
    if (!mGCP)
        mGCP = new cBA_GCP;

    mGCP->mName           = aName;
    mGCP->mMesGCP         = aMesGCP;
    mGCP->mSigmaGCP       = aSigmaGCP;
    mGCP->mGCPIm_Weighter = aWeighter;

    //  mMesGCP = aMesGCP;
    //  mSigmaGCP = aSigmaGCP;
    //  mGCPIm_Weighter = aWeighter;

    if (verbose && mVerbose)
    {
        StdOut()<<  "MESIM=" << mGCP->mMesGCP->MesImOfPt().size() << " MesGCP=" << mGCP->mMesGCP->MesGCP().size()  << std::endl;
    }
}

void cMMVII_BundleAdj::InitItereGCP()
{
    for (auto & aPtr_BA_GCP : mGCP)
    {
        //  This should no longer exist with new GCP handling
        MMVII_INTERNAL_ASSERT_strong(aPtr_BA_GCP->mMesGCP!=nullptr,"aPtr_BA_GCP->mMesGCP");
        if (aPtr_BA_GCP->mSigmaGCP>0)
        {
            for (const auto & aGCP : aPtr_BA_GCP->mMesGCP->MesGCP())
            {
                cPt3dr_UK * aPtrUK = new cPt3dr_UK(aGCP.mPt,aGCP.mNamePt);
                aPtr_BA_GCP->mGCP_UK.push_back(aPtrUK);
                mSetIntervUK.AddOneObj(aPtrUK);
            }
        }
    }
/*
    if (
            (mMesGCP!=nullptr)   //  if GCP where initialized
         && (mSigmaGCP > 0)  // is GGP are unknown
       )
    {
        for (const auto & aGCP : mMesGCP->MesGCP())
	{
            cPt3dr_UK * aPtrUK = new cPt3dr_UK(aGCP.mPt);
            mGCP_UK.push_back(aPtrUK);
	    mSetIntervUK.AddOneObj(aPtrUK);
	}
    }
*/
}


void cMMVII_BundleAdj::OneItere_OnePackGCP(cBA_GCP & aBA, bool verbose)
{
    const cSetMesGndPt   * aSet           = aBA.mMesGCP;
    const tREAL8 & aSigmaGCP              = aBA.mSigmaGCP;
    cSetMesGndPt&   aNewGCP               = aBA.mNewGCP;
    std::vector<cPt3dr_UK*> & aGCP_UK     = aBA.mGCP_UK;
    cStdWeighterResidual& aGCPIm_Weighter = aBA.mGCPIm_Weighter;

    

    // MMVII_DEV_WARNING("std::cout.precision(15) JkUiiuoiu");
    // std::cout.precision(15);
    if (aSet==nullptr) return;
    //   W>0  obs is an unknown "like others"
    //   W=0 , obs is fix , use schurr subst and fix the variables
    //   W<0 , obs is substitued
    const std::vector<cMes1Gnd3D> &      aVMesGCP = aSet->MesGCP();
    const std::vector<cMultipleImPt> & aVMesIm  = aSet->MesImOfPt() ;
    const std::vector<cSensorImage*> & aVSens   = aSet->VSens() ;

    // StdOut() << "GCP " << aVMesGCP.size() << " " << aVMesIm.size() << " " << aVSens.size() << std::endl;

    size_t aNbGCP = aVMesGCP.size();

    bool  aGcpUk = (aSigmaGCP>0);  // are GCP unknowns
    bool  aGcpFix = (aSigmaGCP==0);  // is GCP just an obervation

    int aNbGCPVis = 0;
    int aAvgVis = 0;
    int aAvgNonVis = 0;
    if (verbose && mVerbose)
    {
        StdOut() << "  * " <<  aBA.mName << " : Gcp0=" << aSet->AvgSqResidual() ;
        if (aGcpUk)
        {
            aNewGCP = *aSet;
            for (size_t aK=0 ; aK< aNbGCP ; aK++)
            {
                // cPt3dr aDif = mNewGCP.MesGCP()[aK].mPt -  aGCP_UK[aK]->Pt();
                // StdOut() << " DIFF=" << aDif  << " DDD= "  << (aDif.x()==0)  <<" " << (aDif.y()==0)  <<" " << (aDif.z()==0)  <<" " << "\n";
                aNewGCP.MesGCP()[aK].mPt = aGCP_UK[aK]->Pt();
            }
            StdOut() << " , GcpNew=" << aNewGCP.AvgSqResidual() ; // getchar();
        }
    }

    // MMVII_INTERNAL_ASSERT_tiny(!aGcpUk,"Dont handle GCP UK 4 Now");

    //  Three temporary unknowns for x-y-z of the 3d point
    std::vector<int> aVIndGround = {-1,-2,-3};
    std::vector<int> aVIndFix = (aGcpFix ? aVIndGround : std::vector<int>());

    //  Parse all GCP
    for (size_t aKp=0 ; aKp < aNbGCP ; aKp++)
    {
        const cPt3dr & aPGr = aVMesGCP.at(aKp).mPt;
        const cPt3dr & aPtSigmas = aVMesGCP.at(aKp).SigmasXYZ();
        cPt3dr_UK * aPtrGcpUk =  aGcpUk ? aGCP_UK[aKp] : nullptr;
        cSetIORSNL_SameTmp<tREAL8>  aStrSubst(aPGr.ToStdVector(),aVIndFix);

        const std::vector<cPt2dr> & aVPIm  = aVMesIm.at(aKp).VMeasures();
        const std::vector<int> &  aVIndIm  = aVMesIm.at(aKp).VImages();

        int aNbImVis  = 0;
        // Parse all image having a measure with this GCP
        for (size_t aKIm=0 ; aKIm<aVPIm.size() ; aKIm++)
        {
            int aIndIm = aVIndIm.at(aKIm);
            cSensorImage * aSens = aVSens.at(aIndIm);
            const cPt2dr & aPIm = aVPIm.at(aKIm);
            //StdOut() << "aSensaSensaSens " << aSens->NameImage() << " " << aVIndIm << "\n";

            // compute indexe of unknown, if GCp are !UK we have fix index for temporary
            std::vector<int> aVIndGlob = aGcpUk ? (std::vector<int>()) : aVIndGround;
            if (aGcpUk)  // if GCP are UK, we have to fill with its index
            {
                aPtrGcpUk->PushIndexes(aVIndGlob);
            }
            //  Add index of sensor (Pose+Calib for PC Cam)
            for (auto & anObj : aSens->GetAllUK())
                anObj->PushIndexes(aVIndGlob);

            /*StdOut() << "VISSSS " << aSens->IsVisibleOnImFrame(aPIm)
                << " " << aPGr
                << " "<< aSens->IsVisible(aPGr)
            << "\n";*/

            // Do something only if GCP is visible
            if (aSens->IsVisibleOnImFrame(aPIm) && (aSens->IsVisible(aPGr)))
            {
                aNbImVis++;
                cPt2dr aResidual = aPIm - aSens->Ground2Image(aPGr);
                tREAL8 aWeightImage =   aGCPIm_Weighter.SingleWOfResidual(aResidual);
                cCalculator<double> * anEqColin =  aSens->GetEqColinearity();
                // the "obs" are made of 2 point and, possibily, current rotation (for PC cams)
                std::vector<double> aVObs = aPIm.ToStdVector();

                aSens->PushOwnObsColinearity(aVObs,aPGr);

                if (aGcpUk)  // Case Uknown, we just add the equation
                {
                    mSys->R_CalcAndAddObs(anEqColin,aVIndGlob,aVObs,aWeightImage);
                }
                else  // Case to subst by schur compl,we accumulate in aStrSubst
                {
                    mSys->R_AddEq2Subst(aStrSubst,anEqColin,aVIndGlob,aVObs,aWeightImage);
                }
            }
        }
        aAvgVis += aNbImVis;
        aAvgNonVis += aVPIm.size() -aNbImVis;
        aNbGCPVis += (aNbImVis !=0);

        // bool  aGcpUk = (aSigmaGCP>0);  // are GCP unknowns
        // bool  aGcpFix = (aSigmaGCP==0);  // is GCP just an obervation
        if (aVMesGCP.at(aKp).isFree())
            continue;

        cPt3dr aWeightGroundXYZ(1., 1., 1.);
        if (!aGcpFix)
            aWeightGroundXYZ = DivCByC( {1., 1., 1.}, Square(aSigmaGCP)* MulCByC(aPtSigmas,aPtSigmas));

        if (! aGcpUk) // case  subst,  we now can make schurr commpl and subst aSigmaGCP<=0
        {
            if (! aGcpFix)  // if GCP is not hard fix, we must add obs on ground
            {
                for (auto i = 0; i < 3; ++i)
                {
                    aStrSubst.AddFixCurVarTmp(aVIndGround[i],aWeightGroundXYZ[i]);
                }
            }
            mSys->R_AddObsWithTmpUK(aStrSubst);  // finnaly add obs accummulated
        }
        else  // aSigmaGCP >0
        {
            //  Add observation fixing GCP  aPGr
            // mR8_Sys->AddEqFixCurVar(*aPtrGcpUk,aPtrGcpUk->Pt(),aWeightGround);
            // FIX TO GCP INIT NOT TO LAST ESTIMATION
            for (auto i = 0; i < 3; ++i)
            {

                mR8_Sys->AddEqFixNewVal(*aPtrGcpUk,aPtrGcpUk->Pt()[i],aPGr[i],aWeightGroundXYZ[i]);
            }
            //previously: mR8_Sys->AddEqFixNewVal(*aPtrGcpUk,aPtrGcpUk->Pt(),aPGr,1/1000);
        }
    }

     if (verbose && mVerbose)
     {
        StdOut() << " PropVis1Im=" << aNbGCPVis /double(aNbGCP)  
		<< " AvgVis=" << aAvgVis/double(aNbGCP) 
		<< " NonVis=" << aAvgNonVis/double(aNbGCP) 
        ;
        StdOut() << std::endl;
     }
}


void cMMVII_BundleAdj::OneItere_GCP(bool verbose)
{
     for (const auto & aBA_GCP_Ptr : mGCP)
     {
         MMVII_INTERNAL_ASSERT_strong(aBA_GCP_Ptr->mMesGCP!=nullptr,"aPtr_BA_GCP->mMesGCP");
         OneItere_OnePackGCP(*aBA_GCP_Ptr, verbose);
     }
}

void cMMVII_BundleAdj::Save_newGCP()
{
    if (mGCP.size()>1)
       MMVII_DEV_WARNING("For now dont handle save of multiple GCP");

    if ( mGCP.empty())
       return;
    // for (const auto & aBA_GCP_Ptr : mVGCP)
    const auto & aBA_GCP_Ptr = mGCP.at(0);
    {
        if (aBA_GCP_Ptr->mMesGCP  && mPhProj->DPGndPt3D().DirOutIsInit())
        {
            mPhProj->SaveGCP3D(aBA_GCP_Ptr->mNewGCP.ExtractSetGCP("NewGCP"));
            for (const auto & aMes1Im : aBA_GCP_Ptr->mMesGCP->MesImInit())
                 mPhProj->SaveMeasureIm(aMes1Im);
            mPhProj->SaveCurSysCoGCP(mPhProj->CurSysCo(mPhProj->DPGndPt3D()));
        }
    }
}

    /* ---------------------------------------- */
    /*            cPt3dr_UK                     */
    /* ---------------------------------------- */

template <const int Dim> cPtxdr_UK<Dim>::cPtxdr_UK(const tPt & aPt,const std::string& aName)  :
    mPt    (aPt),
    mName  (aName)
{
}

std::vector<std::string> VNameCoordsPt = {"x","y","z","t"};

template <const int Dim>  void cPtxdr_UK<Dim>::GetAdrInfoParam(cGetAdrInfoParam<tREAL8> & aGAIP)
{
    for (int aD=0 ; aD<Dim ; aD++)
    {
        aGAIP.TestParam(this,&mPt[aD],VNameCoordsPt.at(aD));
    }
    aGAIP.SetNameType("GCP");
    aGAIP.SetIdObj(mName);
}

template <const int Dim>  cPtxdr_UK<Dim>::~cPtxdr_UK()
{
        OUK_Reset();
}

template <const int Dim> void cPtxdr_UK<Dim>::PutUknowsInSetInterval()
{
    mSetInterv->AddOneInterv(mPt);
}

template <const int Dim>  const cPtxd<tREAL8,Dim> & cPtxdr_UK<Dim>::Pt() const {return mPt;}
template <const int Dim>  cPtxd<tREAL8,Dim> & cPtxdr_UK<Dim>::Pt() {return mPt;}

template class cPtxdr_UK<2>;
template class cPtxdr_UK<3>;

};



