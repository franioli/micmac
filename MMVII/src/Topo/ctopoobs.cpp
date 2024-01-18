#include "ctopoobs.h"
#include "MMVII_PhgrDist.h"
#include <memory>
#include "ctopoobsset.h"
#include "ctopopoint.h"
#include "MMVII_SysSurR.h"

namespace MMVII
{

cTopoObs::cTopoObs(cTopoObsSet* set, TopoObsType type, const std::vector<cTopoPoint*> & pts, const std::vector<tREAL8> & vals, const cResidualWeighterExplicit<tREAL8> &aWeights):
    mSet(set), mType(type), mPts(pts), mVals(vals)//, mWeights(aWeights)
{
    MMVII_INTERNAL_ASSERT_strong(mSet, "Obs: no set given")
    switch (mType) {
    case TopoObsType::dist:
        MMVII_INTERNAL_ASSERT_strong(mSet->getType()==TopoObsSetType::simple, "Obs: incorrect set type")
        MMVII_INTERNAL_ASSERT_strong(pts.size()==2, "Obs: incorrect number of points")
        MMVII_INTERNAL_ASSERT_strong(vals.size()==1, "Obs: 1 value should be given")
        MMVII_INTERNAL_ASSERT_strong(aWeights.size()==1, "Obs: 1 weight should be given")
        break;
    case TopoObsType::subFrame:
        MMVII_INTERNAL_ASSERT_strong(mSet->getType()==TopoObsSetType::subFrame, "Obs: incorrect set type")
        MMVII_INTERNAL_ASSERT_strong(pts.size()==2, "Obs: incorrect number of points")
        MMVII_INTERNAL_ASSERT_strong(vals.size()==3, "Obs: 3 values should be given")
        MMVII_INTERNAL_ASSERT_strong(aWeights.size()==3, "Obs: 3 weights should be given")
        break;
    case TopoObsType::distParam:
        MMVII_INTERNAL_ASSERT_strong(mSet->getType()==TopoObsSetType::distParam, "Obs: incorrect set type")
        MMVII_INTERNAL_ASSERT_strong(pts.size()==2, "Obs: incorrect number of points")
        MMVII_INTERNAL_ASSERT_strong(vals.empty(), "Obs: value should not be given")
        MMVII_INTERNAL_ASSERT_strong(aWeights.size()==1, "Obs: 1 weight should be given")
        break;
    default:
        MMVII_INTERNAL_ERROR("unknown obs set type")
    }
    set->addObs(*this);
}

std::string cTopoObs::type2string() const
{
    switch (mType) {
    case TopoObsType::dist:
        return "dist";
    case TopoObsType::distParam:
        return "distParam";
    case TopoObsType::subFrame:
        return "subFrame";
    default:
        MMVII_INTERNAL_ERROR("unknown obs type")
        return "?";
    }
}

std::string cTopoObs::toString() const
{
    std::ostringstream oss;
    oss<<"TopoObs "<<type2string()<<" ";
    for (auto & pt: mPts)
        oss<<pt->getName()<<" ";
    oss<<"values: ";
    for (auto & val: mVals)
        oss<<val<<" ";
    return oss.str();
}

std::vector<int> cTopoObs::getIndices() const
{
    std::vector<int> indices;
    switch (mType) {
    case TopoObsType::dist:
    case TopoObsType::distParam:
    case TopoObsType::subFrame:
        {
        // 2 points
            auto paramsIndices = mSet->getParamIndices();
            indices.insert(std::end(indices), std::begin(paramsIndices), std::end(paramsIndices));
            auto fromIndices = mPts[0]->getIndices();
            indices.insert(std::end(indices), std::begin(fromIndices), std::end(fromIndices));
            auto toIndices = mPts[1]->getIndices();
            indices.insert(std::end(indices), std::begin(toIndices), std::end(toIndices));
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
    case TopoObsType::dist:
    case TopoObsType::distParam:
        //just send measurments
        vals = mVals;
        break;
    case TopoObsType::subFrame:
    {
        //add rotations to measurments
        cTopoObsSetSubFrame* set = dynamic_cast<cTopoObsSetSubFrame*>(mSet);
        if (!set) MMVII_INTERNAL_ERROR("error set type")
        vals = set->getRot();
        vals.insert(std::end(vals), std::begin(mVals), std::end(mVals));
        break;
    }
    default:
        MMVII_INTERNAL_ERROR("unknown obs type")
    }
    return vals;
}

/*cResidualWeighterExplicit<tREAL8> &cTopoObs::getWeights()
{
    return mWeights;
}*/

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
