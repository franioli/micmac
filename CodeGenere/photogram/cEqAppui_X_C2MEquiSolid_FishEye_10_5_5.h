// File Automatically generated by eLiSe
#include "general/all.h"
#include "private/all.h"


class cEqAppui_X_C2MEquiSolid_FishEye_10_5_5: public cElCompiledFonc
{
   public :

      cEqAppui_X_C2MEquiSolid_FishEye_10_5_5();
      void ComputeVal();
      void ComputeValDeriv();
      void ComputeValDerivHessian();
      double * AdrVarLocFromString(const std::string &);
      void SetEquiSolid_FishEye_10_5_5_State_0_0(double);
      void SetXIm(double);
      void SetXTer(double);
      void SetYIm(double);
      void SetYTer(double);
      void SetZTer(double);


      static cAutoAddEntry  mTheAuto;
      static cElCompiledFonc *  Alloc();
   private :

      double mLocEquiSolid_FishEye_10_5_5_State_0_0;
      double mLocXIm;
      double mLocXTer;
      double mLocYIm;
      double mLocYTer;
      double mLocZTer;
};
