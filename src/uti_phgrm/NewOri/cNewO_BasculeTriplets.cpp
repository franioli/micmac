/*Header-MicMac-eLiSe-25/06/2007

    MicMac : Multi Image Correspondances par Methodes Automatiques de Correlation
    eLiSe  : ELements of an Image Software Environnement

    www.micmac.ign.fr


    Copyright : Institut Geographique National
    Author : Marc Pierrot Deseilligny
    Contributors : Gregoire Maillet, Didier Boldo.

[1] M. Pierrot-Deseilligny, N. Paparoditis.
    "A multiresolution and optimization-based image matching approach:
    An application to surface reconstruction from SPOT5-HRS stereo imagery."
    In IAPRS vol XXXVI-1/W41 in ISPRS Workshop On Topographic Mapping From Space
    (With Special Emphasis on Small Satellites), Ankara, Turquie, 02-2006.

[2] M. Pierrot-Deseilligny, "MicMac, un lociel de mise en correspondance
    d'images, adapte au contexte geograhique" to appears in
    Bulletin d'information de l'Institut Geographique National, 2007.

Francais :

    MicMac est un logiciel de mise en correspondance d'image adapte
    au contexte de recherche en information geographique. Il s'appuie sur
    la bibliotheque de manipulation d'image eLiSe. Il est distibue sous la
    licences Cecill-B.  Voir en bas de fichier et  http://www.cecill.info.


English :

    MicMac is an open source software specialized in image matching
    for research in geographic information. MicMac is built on the
    eLiSe image library. MicMac is governed by the  "Cecill-B licence".
    See below and http://www.cecill.info.

Header-MicMac-eLiSe-25/06/2007*/

#include "cNewO_BasculeTriplets.h"
#include <array>
#include <complex>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include "XML_GEN/SuperposImage.h"
#include "algo_geom/cMesh3D.h"
#include "general/bitm.h"
#include "general/exemple_basculement.h"
#include "general/ptxd.h"
#include <cmath>
#include <cfloat>

cTriplet::cTriplet(cOriBascule* s1, cOriBascule* s2, cOriBascule* s3,
             cXml_Ori3ImInit& xml, std::array<int, 3>& m, bool dir)
    : m(m),
      direct(dir),
      mR2on1(Xml2El(xml.Ori2On1())),
      mR3on1(Xml2El(xml.Ori3On1())) {
    mSoms[0] = s1;
    mSoms[1] = s2;
    mSoms[2] = s3;
}

/******************************
  Start cAppliBasculeTriplets
*******************************/
void cAppliBasculeTriplets::InitOneDir(const std::string& aPat, bool aD1) {

    cElemAppliSetFile anEASF(aPat);
    std::string aDirNKS = "";  // si NKS-Set-OfFile contient le nom absolut

    const std::vector<std::string>* aVN = anEASF.SetIm();
    std::cout << "For pattern \"" << aPat;
    std::cout << "\", found ori: \n";
    for (size_t aK = 0; aK < aVN->size(); aK++) {
        std::string aNameOri = (*aVN)[aK];
        std::cout << "  - " << aNameOri << "\n";
        CamStenope* aCS = CamOrientGenFromFile(aNameOri, anEASF.mICNM);

        SplitDirAndFile(aDirNKS, aNameOri, aNameOri);
        cOriBascule& aOri = mAllOris[mapping[aNameOri]];
        aOri.mName = mapping[aNameOri];
        std::cout << aOri.mName << std::endl;
        aOri.mNameFull = anEASF.mDir + aDirNKS + aNameOri;
        aOri.mCam = aCS;
        if (aD1) {
            block1.insert(aOri.mName);
        } else {
            block2.insert(aOri.mName);
        }
    }
}

size_t cAppliBasculeTriplets::InitTriplets(bool aModeBin)
{
    cXml_TopoTriplet aXml3 =
        StdGetFromSI(mNM->NameTopoTriplet(true), Xml_TopoTriplet);

    std::vector<cXml_OneTriplet> triplets(aXml3.Triplets().begin(),
                                          aXml3.Triplets().end());
    for (auto& it3 : aXml3.Triplets()) {
        std::string aN3 = mNM->NameOriOptimTriplet(aModeBin, it3.Name1(),
                                                   it3.Name2(), it3.Name3());

        cXml_Ori3ImInit aXml3Ori = StdGetFromSI(aN3, Xml_Ori3ImInit);

        std::string names[] = {it3.Name1(), it3.Name2(), it3.Name3()};
        int in1 = 0, in2 = 0;
        std::array<short, 3> mask;
        for (int i : {0, 1 , 2}) {
            if (block1.count(names[i])) {
                in1++;
                mask[i] = 1;
            }
            if (block2.count(names[i])) {
                in2++;
                mask[i] = 2;
            }
        }
        if (!in1 || !in2 || in1 + in2 != 3) {
            continue;
        }

        bool direct = in1 == 2;
        short selector = (direct) ? 1 : 2;
        std::array<int, 3> map;
        for (int i : {0, 1, 2}) {
            if (mask[i] != selector)
                map[2] = i;
        }
         map[1] = (map[2] + 2) % 3;
         map[0] = (map[2] + 1) % 3;

        //std::cout << in1 << " " << in2 << std::endl;
        //std::cout << map[0] << " " << map[1] << " " << map[2] << " " << std::endl;
        /*
        cTriplet b(&mAllOris[names[(1 + destIndex) % 3]],
                   &mAllOris[names[(2 + destIndex) % 3]],
                   &mAllOris[names[destIndex]], aXml3Ori, mask, direct);*/
        cTriplet b(&mAllOris[it3.Name1()],
                   &mAllOris[it3.Name2()],
                   &mAllOris[it3.Name3()], aXml3Ori, map, direct);
        b.cost = 1. - aXml3Ori.ResiduTriplet();
        std::cout << "Residue triplet: " << 1. - aXml3Ori.ResiduTriplet() << std::endl;

        ts.emplace_back(std::move(b));
        cTriplet* t = &ts.back();
        for (int i : {0, 1, 2}) {
            mAllOris[names[i]].ts.push_back(t);
        }
    }
    std::cout << "Found " << ts.size() << " correlated triplets." << std::endl;
    return ts.size();
}
// Tester de comparer le meme nombre de campari que l'approche hierarchique
//
// Poser sur du papier les equations sur les triangles entre deux bloc avec N >
// 2 triangles
// Rotation avant -> puis translation et facteur d'echelle


// Ajustement dans martini avant campari sur les 5 points au lieux des triangles
//
static std::array<ElRotation3D, 3> EstimAllRt(cTriplet* aLnk, bool inv = false) {
    const cOriBascule* aS1 = aLnk->of(0);
    const cOriBascule* aS2 = aLnk->of(1);
    const cOriBascule* aS3 = aLnk->of(2);

    // Get current R,t of the mother pair
    const ElRotation3D aC1ToM = aS1->mCam->Orient().inv();//TODO get current rot
    const ElRotation3D aC2ToM = aS2->mCam->Orient().inv();

    // Get rij,tij of the triplet sommets
    const ElRotation3D aC1ToL = aLnk->RotOfSom(aS1);
    const ElRotation3D aC2ToL = aLnk->RotOfSom(aS2);
    const ElRotation3D aC3ToL = aLnk->RotOfSom(aS3);

    // Propagate R,t according to:
    // aC1ToM.tr() = T0 + aRL2M * aC1ToL.tr() * Lambda
    //
    // 1-R
    ElMatrix<double> aRL2Mprim = aC1ToM.Mat() * aC1ToL.Mat().transpose();
    ElMatrix<double> aRL2Mbis = aC2ToM.Mat() * aC2ToL.Mat().transpose();
    ElMatrix<double> aRL2M = NearestRotation((aRL2Mprim + aRL2Mbis) * 0.5);

    // 2-Lambda
    double d12L = euclid(aC2ToL.tr() - aC1ToL.tr());
    double d12M = euclid(aC2ToM.tr() - aC1ToM.tr());
    double Lambda = d12M / ElMax(d12L, 1e-20);

    // 3-t
    Pt3dr aC1ToLRot = aRL2M * aC1ToL.tr();
    Pt3dr aC2ToLRot = aRL2M * aC2ToL.tr();

    Pt3dr T0prim = aC1ToM.tr() - aC1ToLRot * Lambda;
    Pt3dr T0bis = aC2ToM.tr() - aC2ToLRot * Lambda;
    Pt3dr T0 = (T0prim + T0bis) / 2.0;

    Pt3dr aT1 = T0 + aRL2M * aC1ToL.tr() * Lambda;
    Pt3dr aT2 = T0 + aRL2M * aC2ToL.tr() * Lambda;
    Pt3dr aT3 = T0 + aRL2M * aC3ToL.tr() * Lambda;


    // 4- return R{1,2,3}, t{1,2,3}
    return {ElRotation3D(aT1, aRL2M * aC1ToL.Mat(), true),
            ElRotation3D(aT2, aRL2M * aC2ToL.Mat(), true),
            ElRotation3D(aT3, aRL2M * aC3ToL.Mat(), true)};
}

/*
static std::array<ElRotation3D, 2> EstimBadAllRt(cTriplet* aLnk) {
    const cOriBascule* aS1 = aLnk->mSoms[0];
    const cOriBascule* aS3 = aLnk->mSoms[2];

    // Get current R,t of the mother pair
    const ElRotation3D aC1ToM = aS1->mCam->Orient();//TODO get current rot

    // Get rij,tij of the triplet sommets
    const ElRotation3D aC1ToL = aLnk->RotOfSom(aS1);
    const ElRotation3D aC3ToL = aLnk->RotOfSom(aS3);

    // Propagate R,t according to:
    // aC1ToM.tr() = T0 + aRL2M * aC1ToL.tr() * Lambda
    //
    // 1-R
    ElMatrix<double> aRL2Mprim = aC1ToM.Mat() * aC1ToL.Mat().transpose();
    ElMatrix<double> aRL2M = NearestRotation(aRL2Mprim);

    // 2-Lambda
    double d12L = euclid(aC1ToL.tr());
    double d12M = euclid(aC1ToM.tr());
    double Lambda = d12M / ElMax(d12L, 1e-20);

    // 3-t
    Pt3dr aC1ToLRot = aRL2M * aC1ToL.tr();

    Pt3dr T0prim = aC1ToM.tr() - aC1ToLRot * Lambda;
    Pt3dr T0 = T0prim;

    Pt3dr aT1 = T0 + aRL2M * aC1ToL.tr() * Lambda;
    Pt3dr aT3 = T0 + aRL2M * aC3ToL.tr() * Lambda;


    // 4- return R{1,2,3}, t{1,2,3}
    return {ElRotation3D(aT1, aRL2M * aC1ToL.Mat(), true),
            ElRotation3D(aT3, aRL2M * aC3ToL.Mat(), true)};
}
*/

static void  ScTr2to1
      (
             const std::vector<ElRotation3D> & aVR1 ,
             const std::vector<ElRotation3D> & aVR2,
             std::vector<Pt3dr> &              aVP1,
             std::vector<Pt3dr> &              aVP2,
             const ElMatrix<double> &          aRM2toM1 ,
             double &                          aSc2to1,
             Pt3dr &                           aTr2to1
      )
{
    aVP1.clear();
    aVP2.clear();
    Pt3dr aCdg1(0,0,0);
    Pt3dr aCdg2(0,0,0);
    for (int aK = 0 ; aK<int(aVR1.size()) ; aK++)
    {
            Pt3dr aP1 =  aVR1[aK].ImAff(Pt3dr(0,0,0));
            Pt3dr aP2 =  aRM2toM1 * aVR2[aK].ImAff(Pt3dr(0,0,0));

            aVP1.push_back(aP1);
            aVP2.push_back(aP2);

            aCdg1 = aCdg1 + aP1;
            aCdg2 = aCdg2 + aP2;
    }

    aCdg1  = aCdg1 / double(aVR1.size());
    aCdg2  = aCdg2 / double(aVR1.size());

    double aSomD1 = 0;
    double aSomD2 = 0;

    for (int aK = 0 ; aK<int(aVR1.size()) ; aK++)
    {
        double aD1 = euclid(aVP1[aK]-aCdg1);
        double aD2 = euclid(aVP2[aK]-aCdg2);

        aSomD1 += aD1;
        aSomD2 += aD2;
    }
    aSomD1 /= aVR1.size();
    aSomD2 /= aVR1.size();

    aSc2to1 = aSomD1 / aSomD2;
    aTr2to1 = aCdg1  - aCdg2 * aSc2to1;
}

static ElMatrix<double> RotM2toM1(const std::vector<ElRotation3D>& aVR1,
                                  const std::vector<ElRotation3D>& aVR2) {
    ElMatrix<double> aRes(3, 3, 0.0);
    //  Cam -> Monde
    for (int aK = 0; aK < int(aVR1.size()); aK++) {
        ElRotation3D aLocM2toM1 = aVR1[aK] * aVR2[aK].inv();
        aRes = aRes + aLocM2toM1.Mat();
        auto mRM2toM1 = aLocM2toM1.Mat();
    }
    aRes = aRes * (1.0 / double(aVR1.size()));
    return NearestRotation(aRes);
}

static cSolBasculeRig  BascFromVRot
                (
                     const std::vector<ElRotation3D> & aVR1,
                     const std::vector<ElRotation3D> & aVR2,
                     const std::vector<double> & aCosts,
                     std::vector<Pt3dr> &              aVP1,
                     std::vector<Pt3dr> &              aVP2
                )
{

    ElMatrix<double>  aRM2toM1 = RotM2toM1(aVR1,aVR2);

    double aSc2to1 = 1;
    Pt3dr  aTr2to1(0,0,0);

    ScTr2to1
        (
         aVR1,aVR2,
         aVP1,aVP2,
         aRM2toM1, aSc2to1,aTr2to1
        );

    cRansacBasculementRigide aBasc(false); // false  => Pas de vitesse
    for (size_t aKP=0 ; aKP<aVP1.size(); aKP++)
    {
        aBasc.AddExemple(aVP1[aKP],aVP2[aKP],0,"");
    }
    aBasc.CloseWithTrGlob(false);

    aBasc.ExploreAllRansac(100);
    cSolBasculeRig aSBR = aBasc.BestSol();
    return aSBR;

    /*
    if (aBasc.SolIsInit()) {

        cSetEqFormelles aSetEq (cNameSpaceEqF::eSysPlein);
        cL2EqObsBascult  * aL2Basc = aSetEq.NewEqObsBascult(aSBR,false);
        aSetEq.SetClosed();

        for (int aKEt=0 ; aKEt<5 ; aKEt++)
        {
            aSetEq.SetPhaseEquation();
            for (int aKP = 0; aKP < int(aVP1.size()); aKP++) {
                aL2Basc->AddObservation(aVP1[aKP], aVP2[aKP], aCosts[aKP]);
            }
            aSetEq.SolveResetUpdate();
            aSBR=aL2Basc->CurSol();
        }

        return aSBR;
    }
    */

    //return cSolBasculeRig::StdSolFromPts(aVP1, aVP2, &aCosts, 1000000000, 5);
    /*cRansacBasculementRigide basc(false);
    for (size_t i = 0; i < aVP1.size(); i++) {
        std::string name = std::to_string(i);
        basc.AddExemple(aVP1[i], aVP2[i], 0, name);
    }
    basc.CloseWithTrGlob(true);
    basc.EstimLambda();
    basc.ExploreAllRansac();
    //return basc.BestSol();*/
    return cSolBasculeRig::SBRFromElems(aTr2to1,aRM2toM1,aSc2to1);
}

cSolBasculeRig cAppliBasculeTriplets::ComputeBascule() {
    std::vector<ElRotation3D> mVR1;
    std::vector<ElRotation3D> mVR2;
    std::vector<double> mCosts;

    std::vector<ElRotation3D> mOVR1;
    std::vector<ElRotation3D> mOVR2;

    //Load same orientations
    for (auto t : ts) {
        auto r = EstimAllRt(&t);
        ElRotation3D pA = (t.direct) ? r[2].inv() : t.of(2)->mCam->Orient();
        ElRotation3D pB = (t.direct) ? t.of(2)->mCam->Orient() : r[2].inv();

        mOVR1.push_back(pA);
        mOVR2.push_back(pB);

        mVR1.push_back(pB.inv());
        mVR2.push_back(pA.inv());
        mCosts.push_back(t.cost);
    }

    std::vector<Pt3dr> mVP1;
    std::vector<Pt3dr> mVP2;

    cSolBasculeRig aSol = BascFromVRot(mVR1, mVR2, mCosts, mVP1, mVP2);

    ElMatrix<double> mRM2toM1 = aSol.Rot();
    double mSc2to1 = aSol.Lambda();
    Pt3dr mTr2to1 = aSol.Tr();

    std::cout << "Lambda= " << mSc2to1 << "\n"
              << "Tr    = " << mTr2to1 << "\n"
              << "R     = " << mRM2toM1(0, 0) << "  " << mRM2toM1(0, 1) << " "
              << mRM2toM1(0, 2) << "\n"
              << "        " << mRM2toM1(1, 0) << "  " << mRM2toM1(1, 1) << " "
              << mRM2toM1(1, 2) << "\n"
              << "        " << mRM2toM1(2, 0) << "  " << mRM2toM1(2, 1) << " "
              << mRM2toM1(2, 2) << "\n";



    if (std::isnan(mSc2to1) || std::isnan(mTr2to1.x) || std::isnan(mTr2to1.y) || std::isnan(mTr2to1.z)) {
        return cSolBasculeRig::Id();
    }

    std::vector<cTriplet*> mTrip;
    for (int aK = 0; aK < int(mVP1.size()); aK++) {
        ElRotation3D aRM1toCam = mOVR1[aK];
        ElRotation3D aRM2toCam = mOVR2[aK];
        Pt3dr aC1 = mVP1[aK];
        Pt3dr aC2 = mVP2[aK];  //  Pt3dr aP2 =  aRM2toM1 *
                               //  mVDpl[aK]->mCam2->PseudoOpticalCenter();

                               //  => M1toC = M2toC * aM2toM1-1
        ElMatrix<double> aDifM =
            aRM2toCam.Mat() * mRM2toM1.transpose() - aRM1toCam.Mat();
        Pt3dr aDifP = aC1 - (aC2 * mSc2to1 + mTr2to1);
        auto r = aDifM.L2();
        std::cout << "RESIDU R= " << r << " " << euclid(aDifP) << "\n";

        if (maxR < 0 || r < maxR) {
            mTrip.push_back(&ts[aK]);
        }
    }
    mVR1.clear();
    mVR2.clear();
    mVP1.clear();
    mVP2.clear();

    for (auto t : mTrip) {
        auto r = EstimAllRt(t);
        ElRotation3D pA = (t->direct) ? r[2].inv() : t->of(2)->mCam->Orient();
        ElRotation3D pB = (t->direct) ? t->of(2)->mCam->Orient() : r[2].inv();
        mOVR1.push_back(pA);
        mOVR2.push_back(pB);

        mVR1.push_back(pB.inv());
        mVR2.push_back(pA.inv());
    }


    //aSol = BascFromVRot(mVR1, mVR2, mVP1, mVP2);

    mRM2toM1 = aSol.Rot();
    mSc2to1 = aSol.Lambda();
    mTr2to1 = aSol.Tr();

    std::cout << "Lambda= " << mSc2to1 << "\n"
              << "Tr    = " << mTr2to1 << "\n"
              << "R     = " << mRM2toM1(0, 0) << "  " << mRM2toM1(0, 1) << " "
              << mRM2toM1(0, 2) << "\n"
              << "        " << mRM2toM1(1, 0) << "  " << mRM2toM1(1, 1) << " "
              << mRM2toM1(1, 2) << "\n"
              << "        " << mRM2toM1(2, 0) << "  " << mRM2toM1(2, 1) << " "
              << mRM2toM1(2, 2) << "\n";

    std::cout << "Basculed" << std::endl;
    return aSol;
}

void cAppliBasculeTriplets::ApplyBascule(const cSolBasculeRig& aSol) {

    ElMatrix<double> mRM2toM1 = aSol.Rot();
    double mSc2to1 = aSol.Lambda();
    Pt3dr mTr2to1 = aSol.Tr();

    for (auto name : block2) {
        CamStenope* aCam = mAllOris[name].mCam;
        ElRotation3D aRM2toCam = aCam->Orient();
        ElMatrix<double> aRM1toCam = aRM2toCam.Mat() * mRM2toM1.transpose();
        Pt3dr aC2 = aRM2toCam.ImRecAff(Pt3dr(0, 0, 0));

        Pt3dr aC1 = (mRM2toM1 * aC2) * mSc2to1 + mTr2to1;

        ElRotation3D aCamToM1(aC1, aRM1toCam.transpose(), true);

        aCam->SetOrientation(aCamToM1.inv());
    }
}

void cAppliBasculeTriplets::Sauv()
{
    std::cout << "Output: " << mDirOutGlob << std::endl;
    ELISE_fp::MkDir(mDirOutGlob);
    std::vector<std::string> images;
    images.insert( images.end(), block1.begin(), block1.end() );
    images.insert( images.end(), block2.begin(), block2.end() );

    for (auto name : images) {
        cOriBascule& anOM = mAllOris[name];
        CamStenope* aCam = anOM.mCam;
        cOrientationConique anOC = aCam->StdExportCalibGlob();
        cOrientationConique anOCInit =
            StdGetFromPCP(anOM.mNameFull, OrientationConique);

        anOCInit.Externe() = anOC.Externe();
        anOCInit.Verif() = anOC.Verif();

        if (anOCInit.FileInterne().IsInit()) {
            anOCInit.FileInterne().SetVal(
                mDirOutLoc + NameWithoutDir(anOCInit.FileInterne().Val()));
        }

        std::string aName = mDirOutGlob + anOM.mXmlFile;

        MakeFileXML(anOCInit, aName);
    }

    SauvCalib(mOri1);
    SauvCalib(mOri2);
}

void cAppliBasculeTriplets::SauvCalib(const std::string & anOri)
{
    std::string anOriTmp = anOri;

    /* Afin de gerer NKS-Set-OfFile */
    cElRegex anAutom("NKS-Set-OfFile.*",10);
    bool ISNKS = anAutom.Match(anOri);
    if (ISNKS)
    {
        std::string aDirNKS, aNameOri;

        cElemAppliSetFile anEASF(anOri);
        aNameOri = (*anEASF.SetIm())[0];
        SplitDirAndFile(aDirNKS,aNameOri,aNameOri);
        anOriTmp = anEASF.mDir + aDirNKS + aNameOri;

    }

    ELISE_fp::CpFile
    (
          DirOfFile(anOriTmp) + "AutoCal*.xml",
          mDirOutGlob
    );
}


cAppliBasculeTriplets::cAppliBasculeTriplets(int argc,char ** argv) :
    mRatioOutlier(0)
{
    NRrandom3InitOfTime();

    std::string aDir;
    bool aModeBin = true;
    std::string mFullPat;

    ElInitArgMain
        (
         argc,argv,
         LArgMain()
                    << EAMC(mFullPat, "Pattern")
                    << EAMC(mOri1,"First set of image", eSAM_IsPatFile)
                    << EAMC(mOri2,"Second set of image", eSAM_IsPatFile)
                    << EAMC(mOriOut,"Orientation Dir"),
         LArgMain() << EAM(aModeBin,"Bin",true,"Binaries file, def = true",eSAM_IsBool)
                    << EAM(maxR, "R", true, "Max Residue for selected triplet")
                    << ArgCMA()
        );

    mEASF.Init(mFullPat);
    mNM = new cNewO_NameManager(mExtName, mPrefHom, mQuick, mEASF.mDir,
                                mNameOriCalib, "dat");
    const cInterfChantierNameManipulateur::tSet * aVIm = mEASF.SetIm();
    for (unsigned aKIm = 0; aKIm < aVIm->size(); aKIm++) {
        std::string xmlfile = "Orientation-" + (*aVIm)[aKIm] + ".xml";
        mapping[xmlfile] = (*aVIm)[aKIm];
        mAllOris[(*aVIm)[aKIm]] = cOriBascule((*aVIm)[aKIm]);
    }

    InitOneDir(mOri1, true);
    InitOneDir(mOri2, false);

    mDirOutLoc =  "Ori-" + mOriOut + "/";
    mDirOutGlob = Dir() + mDirOutLoc;

    StdCorrecNameOrient(mNameOriCalib, aDir);
    // std::cout << mNM->Dir3P() << std::endl;
    size_t n = InitTriplets(aModeBin);
    if (n < 3) {
        exit(1);
        return;
    }

    auto bascule = ComputeBascule();

    ApplyBascule(bascule);

    Sauv();
    std::cout << "(";
    for (auto i : block1) {
        std::cout << i << "|";
    }
    for (auto i : block2) {
        std::cout << i << "|";
    }
    std::cout << ")"<<std::endl;

    /*
            std::pair<ElRotation3D, ElRotation3D> aPair =
                mNM->OriRelTripletFromExisting(InOri, it3.Name1(), it3.Name2(),
                                               it3.Name3(), Ok);

            std::string aNameSauveXml = mNM->NameOriOptimTriplet(
                false, it3.Name1(), it3.Name2(), it3.Name3(), false);
            std::string aNameSauveBin = mNM->NameOriOptimTriplet(
                true, it3.Name1(), it3.Name2(), it3.Name3(), false);
            //aXml.Sigma() = {cur_sigma_t, cur_sigma_r};
            //------------
                aXml.Ori2On1() = El2Xml(
                    ElRotation3D(aPair.first.tr(), aPair.first.Mat(), true));
                aXml.Ori3On1() = El2Xml(
                    ElRotation3D(aPair.second.tr(), aPair.second.Mat(), true));
                aXml.Ori2On1() = El2Xml(RandView(
                    ElRotation3D(aPair.first.tr(), aPair.first.Mat(), true),
                    cur_sigma_t, cur_sigma_r));

            //------------
            MakeFileXML(aXml, aNameSauveXml);
            MakeFileXML(aXml, aNameSauveBin);
            */
}


////////////////////////// Mains //////////////////////////

int BasculeTriplet_main(int argc,char ** argv)
{
    cAppliBasculeTriplets aAppBascTri(argc,argv);

    return EXIT_SUCCESS;
}
