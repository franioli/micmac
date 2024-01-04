#include "ctopoobs.h"
#include "MMVII_PhgrDist.h"
#include "MMVII_2Include_Serial_Tpl.h"
#include <memory>
#include "ctopoobsset.h"
#include "ctopopoint.h"
#include "MMVII_SysSurR.h"
#include "Topo.h"

namespace MMVII
{

cTopoObs::cTopoObs(cTopoObsSet* set, eTopoObsType type, const std::vector<std::string> &pts, const std::vector<tREAL8> & vals, const cResidualWeighterExplicit<tREAL8> &aWeights):
    mSet(set), mType(type), mPts(pts), mVals(vals), mWeights(aWeights)
{
    if (!mSet)
    {
        MMVII_INTERNAL_ERROR("Obs: no set given")
        return; //just to please the compiler
    }
    switch (mType) {
    case eTopoObsType::eDist:
        MMVII_INTERNAL_ASSERT_strong(mSet->getType()==eTopoObsSetType::eSimple, "Obs: incorrect set type")
        MMVII_INTERNAL_ASSERT_strong(pts.size()==2, "Obs: incorrect number of points")
        MMVII_INTERNAL_ASSERT_strong(vals.size()==1, "Obs: 1 value should be given")
        MMVII_INTERNAL_ASSERT_strong(aWeights.size()==1, "Obs: 1 weight should be given")
        break;
    case eTopoObsType::eSubFrame:
        MMVII_INTERNAL_ASSERT_strong(mSet->getType()==eTopoObsSetType::eSubFrame, "Obs: incorrect set type")
        MMVII_INTERNAL_ASSERT_strong(pts.size()==2, "Obs: incorrect number of points")
        MMVII_INTERNAL_ASSERT_strong(vals.size()==3, "Obs: 3 values should be given")
        MMVII_INTERNAL_ASSERT_strong(aWeights.size()==3, "Obs: 3 weights should be given")
        break;
    case eTopoObsType::eDistParam:
        MMVII_INTERNAL_ASSERT_strong(mSet->getType()==eTopoObsSetType::eDistParam, "Obs: incorrect set type")
        MMVII_INTERNAL_ASSERT_strong(pts.size()==2, "Obs: incorrect number of points")
        MMVII_INTERNAL_ASSERT_strong(vals.empty(), "Obs: value should not be given")
        MMVII_INTERNAL_ASSERT_strong(aWeights.size()==1, "Obs: 1 weight should be given")
        break;
    default:
        MMVII_INTERNAL_ERROR("unknown obs set type")
    }
    std::cout<<"DEBUG: create cTopoObs "<<toString()<<"\n";

}

void cTopoObs::AddData(const  cAuxAr2007 & anAuxInit)
{
    std::cout<<"Add data obs '"<<toString()<<"'"<<std::endl;
    cAuxAr2007 anAux("TopoObs",anAuxInit);

    MMVII::EnumAddData(anAux,mType,"Type");
    MMVII::AddData(cAuxAr2007("Pts",anAux),mPts);
    MMVII::AddData(cAuxAr2007("Vals",anAux),mVals);
}

void AddData(const cAuxAr2007 & anAux, cTopoObs *aTopoObs)
{
     aTopoObs->AddData(anAux);
}

std::string cTopoObs::toString() const
{
    std::ostringstream oss;
    oss<<"TopoObs "<<E2Str(mType)<<" ";
    for (auto & pt: mPts)
        oss<<pt<<" ";
    oss<<"values: ";
    for (auto & val: mVals)
        oss<<val<<" ";
    return oss.str();
}

std::vector<int> cTopoObs::getIndices(cBA_Topo *aBA_Topo) const
{
    std::vector<int> indices;
    switch (mType) {
    case eTopoObsType::eDist:
    case eTopoObsType::eDistParam:
    case eTopoObsType::eSubFrame:
        {
            // 2 points
            std::string nameFrom = mPts[0];
            std::string nameTo = mPts[1];
            auto & [ptFromUK, ptFrom3d] = aBA_Topo->getPointWithUK(nameFrom);
            auto & [ptToUK, ptTo3d] = aBA_Topo->getPointWithUK(nameTo);
            auto paramsIndices = mSet->getParamIndices();
            indices.insert(std::end(indices), std::begin(paramsIndices), std::end(paramsIndices)); // TODO: use PushIndexes
            ptFromUK->PushIndexes(indices, *ptFrom3d);
            ptToUK->PushIndexes(indices, *ptTo3d);
        }
        break;
    default:
        MMVII_INTERNAL_ERROR("unknown obs type")
    }
    return indices;
}

std::vector<tREAL8> cTopoObs::getVals() const
{
    std::vector<tREAL8> vals;
    switch (mType) {
    case eTopoObsType::eDist:
    case eTopoObsType::eDistParam:
        //just send measurments
        vals = mVals;
        break;
    case eTopoObsType::eSubFrame:
    {
        //add rotations to measurments
        cTopoObsSetSubFrame* set = dynamic_cast<cTopoObsSetSubFrame*>(mSet);
        if (!set)
        {
            MMVII_INTERNAL_ERROR("error set type")
            return {}; //just to please the compiler
        }
        vals = set->getRot();
        vals.insert(std::end(vals), std::begin(mVals), std::end(mVals));
        break;
    }
    default:
        MMVII_INTERNAL_ERROR("unknown obs type")
    }
    return vals;
}

cResidualWeighterExplicit<tREAL8> &cTopoObs::getWeights()
{
    return mWeights;
}

/*std::vector<tREAL8> cTopoObs::getResiduals(const cTopoComp *comp) const
{
    auto eq = comp->getEquation(getType());
    std::vector<int> indices = getIndices();
    std::vector<tREAL8> vals = getVals();
    std::vector<tREAL8> vUkVal;
    std::for_each(indices.begin(), indices.end(), [&](int i) { vUkVal.push_back(comp->getSys()->CurGlobSol()(i)); });
    std::vector<tREAL8> eval = eq->DoOneEval(vUkVal, vals);
    // select only residuals from formula eval
    std::vector<tREAL8> residuals;
    int valPerSubObs = eval.size() / mWeights.size();
    for (unsigned int i = 0; i < eval.size(); i += valPerSubObs)
        residuals.push_back(eval[i]);
    return residuals;
}*/

};
