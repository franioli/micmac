#include "cMMVII_Appli.h"
#include "MMVII_Image2D.h"
#include "gdal_priv.h"


namespace MMVII
{

class cAppli_ImageMetadata : public cMMVII_Appli
{
public :
    cAppli_ImageMetadata(const std::vector<std::string> & aVArgs,const cSpecMMVII_Appli & aSpec,bool isBasic);
    int Exe() override;
    cCollecSpecArg2007 & ArgObl(cCollecSpecArg2007 & anArgObl) override ;
    cCollecSpecArg2007 & ArgOpt(cCollecSpecArg2007 & anArgOpt) override ;
private :
    void ShowMetadata(const std::string& aName);
    std::string mNameIn;  ///< Input image name
};

cCollecSpecArg2007 & cAppli_ImageMetadata::ArgObl(cCollecSpecArg2007 & anArgObl)
{
    return
        anArgObl
        <<   Arg2007(mNameIn,"Name of input file",{{eTA2007::MPatFile,"0"},eTA2007::FileImage})
        ;
}

cCollecSpecArg2007 & cAppli_ImageMetadata::ArgOpt(cCollecSpecArg2007 & anArgOpt)
{
    return
        anArgOpt
        ;
}

void cAppli_ImageMetadata::ShowMetadata(const std::string &aName)
{
    cDataFileIm2D aFileIn= cDataFileIm2D::Create(aName,eForceGray::No);
    std::cout << aName << " : " << aFileIn.Sz() << std::endl;

    auto aHandle = GDALOpen( aName.c_str(), GA_ReadOnly);
    auto aDataSet = GDALDataset::FromHandle(aHandle);
    if (aDataSet == nullptr)
    {
        MMVII_UserError(eTyUEr::eOpenFile,std::string("Can't open image file: ") + CPLGetLastErrorMsg());
        return; // never happens
    }

    auto MDL = aDataSet->GetMetadataDomainList();
    if (MDL != nullptr) {
        std::cout << "GetMetadataDomainList\n";
        for (char **mdl = MDL; *mdl; mdl++) {
            std::cout << " - '" << *mdl << "'\n";
            auto MD = aDataSet->GetMetadata(*mdl);
            if (MD != nullptr) {
                for (char **md = MD; *md; md++) {
                    std::cout << "    . '" << *md << "'\n";
                }
            }
        }
        CSLDestroy(MDL);
    }
    std::cout << " - Null domain\n";
    auto MD = aDataSet->GetMetadata(nullptr);
    if (MD != nullptr) {
        for (char **md = MD; *md; md++) {
            std::cout << "    . '" << *md << "'\n";
        }
    }

}

int cAppli_ImageMetadata::Exe()
{
#if 0
   if (RunMultiSet(0,0))
   {
       return ResultMultiSet();
   }
    ShowMetadata(mNameIn);

   return EXIT_SUCCESS;
#endif
    for (const auto & aName : VectMainSet(0))
    {
        ShowMetadata(aName);
    }

    return EXIT_SUCCESS;
}

cAppli_ImageMetadata:: cAppli_ImageMetadata(const std::vector<std::string> &  aVArgs,const cSpecMMVII_Appli & aSpec,bool isBasic) :
    cMMVII_Appli (aVArgs,aSpec)
{
}


tMMVII_UnikPApli Alloc_ImageMetadata(const std::vector<std::string> &  aVArgs,const cSpecMMVII_Appli & aSpec)
{
    return tMMVII_UnikPApli(new cAppli_ImageMetadata(aVArgs,aSpec,true));
}

cSpecMMVII_Appli  TheSpec_ImageMetadata
    (
        "ImageMetadata",
        Alloc_ImageMetadata,
        "Display image file metadata",
        {eApF::ImProc},
        {eApDT::Image},
        {eApDT::Xml},
        __FILE__
        );

};
