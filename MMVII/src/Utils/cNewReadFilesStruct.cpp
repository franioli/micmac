#include "cMMVII_Appli.h"
#include "MMVII_ReadFileStruct.h"
#include "MMVII_Bench.h"



/**
   \file cConvCalib.cpp  testgit

   \brief file for conversion between calibration (change format, change model) and tests
*/


namespace MMVII
{

/* ***************************************************************************************** */
/*                                                                                           */
/*                                cNewReadFilesStruct                                        */
/*                                                                                           */
/* ***************************************************************************************** */




void cNewReadFilesStruct::Check(std::map<std::string,size_t> & aMap1,std::map<std::string,size_t> & aMap2,bool RefIsI1)
{
     for (const auto & [aStr1,aCpt1] :  aMap1)
     {
	 if ( (TypeOfName(aStr1) ==  eRFS_TypeField::eUnused) || (TypeOfName(aStr1) ==  eRFS_TypeField::eBla))
	 {
            // There is no sens to have skip in specif
            if (RefIsI1)
            {
                  MMVII_INTERNAL_ERROR("Skip in specif !!");
            }
	    else
	    {
		    // Everyrhing OK we can have as many skipped as wanted
	    }
	 }
	 else
	 {
              size_t aCpt2 = aMap2[aStr1];

	      if (aCpt1 > aCpt2)
	      {
                   if (( aCpt1==StaredNumber)  && (RefIsI1))
		   {
			   // special cas , we scpecifed any and 1 is ref
		   }
		   else
		   {
                      MMVII_UserError
                      (
		          eTyUEr::eUnClassedError,
			  "dif bewteen specif & format for token="+aStr1 + " Cpt1=" + ToStr(aCpt1) + " Cpt2=" + ToStr(aCpt2)
                     );
		   }
	      }
	 }
     }
}

eRFS_TypeField cNewReadFilesStruct::TypeOfName(const std::string & aName)
{
     if (aName== "?")
        return eRFS_TypeField::eUnused;

     if (aName== "Bla")
        return eRFS_TypeField::eBla;

     std::string aBeginF = "XYZWPKFS";
     std::string aBeginS = "NI";
     std::string aBeginI = "E";

     if (contains(aBeginF,aName.at(0))  )
        return eRFS_TypeField::eFloat;

     if (contains(aBeginS,aName.at(0))  )
        return eRFS_TypeField::eString;

     if (contains(aBeginI,aName.at(0))  )
        return eRFS_TypeField::eInt;

     MMVII_INTERNAL_ERROR("Cannot compile type of : " + aName);
     return eRFS_TypeField::eUnused;
}


void  cNewReadFilesStruct::ParseFormat(bool isSpec,const std::string & aFormat,std::map<std::string,size_t>  & aMap,std::vector<std::string> & aVec)
{
    const char * aC = aFormat.c_str();

    while (*aC)
    {
        //  skip white spaces
        while (isspace(*aC) || (*aC==',')  || (*aC=='/'))  aC++;

	// extract the token
        const char * aC0 = aC;
        if (isupper(*aC))  // if begib with upper case, go on while minus & digit
        {
           aC++;
           while ( (*aC) && (!isupper(*aC)) && (isalnum(*aC))  )
               aC++;
        }
	else if (*aC=='?')  // if begib with upper case, go on while minus & digit
        {
            aC++;
	}
	else if (*aC==0) // if we reach end of car whil skeeping white : end of game
	{
             return;
	}
	else
	{
             // token must begin with uper case
             MMVII_UserError(eTyUEr::eUnClassedError,"Format is not reconized  :" +std::string(aC));
	}
	std::string aNameField(aC0,size_t(aC-aC0));

	bool  isStared = (*aC=='*');
        // StdOut() << "========== [" << aNameField << "]\n";
	if (isStared)
	{
            aC++;
	    aMap[aNameField] = StaredNumber;
	    if (!isSpec)
                MMVII_UserError(eTyUEr::eUnClassedError,"No star allowed in format");
	}
	else 
	   aMap[aNameField] ++;


	aVec.push_back(aNameField);
    }
}

cNewReadFilesStruct::cNewReadFilesStruct(const std::string & aFormat,const std::string & aSpecifFMand,const std::string & aSpecifTot) :
    mFormat  (aFormat),
    mDebug   (false)
{
   ParseFormat(false,mFormat,mCptFields,mNameFields);
   for (const auto & aStr: mNameFields)
   {
        mTypes.push_back(TypeOfName(aStr));
	if (mTypes.back() == eRFS_TypeField::eBla && (mTypes.size()!=mNameFields.size()))
	{
           MMVII_UserError(eTyUEr::eUnClassedError,"\"Bla\" specification can only happen at end");
	}
   }


   // Check that all the field of Format are in spec tot
   {
      std::map<std::string,size_t>    aCptSpecTot;
      std::vector<std::string>        aNameSpecTot;
      ParseFormat(true,aSpecifTot,aCptSpecTot,aNameSpecTot);

      // StdOut() << "LLLL " << __LINE__ << "\n";
      Check(mCptFields,aCptSpecTot,false);
   }
   // Check that all the field of spec mandatory are in Format 
   {
      std::map<std::string,size_t>    aCptSpecMand;
      std::vector<std::string>        aNameSpecMand;
      ParseFormat(true,aSpecifFMand,aCptSpecMand,aNameSpecMand);

      // dOut() << "LLLL " << __LINE__ << "\n";
      Check(aCptSpecMand,mCptFields,true);
   }
}

void cNewReadFilesStruct::ReadFile(const std::string & aNameFile,int aL0,int aNumLastL ,int aComment)
{
	 // std::map<std::string,std::vector<int> >          mMapInt;
	 // std::map<std::string,std::vector<tREAL8> >       mMapFloat;
	 // std::map<std::string,std::vector<std::string> >  mMapString;
    mMapInt.clear();
    mMapFloat.clear();
    mMapString.clear();

    if (aNumLastL<=0)  
       aNumLastL = 1e9;

    mNameFile = aNameFile;
    if (! ExistFile(mNameFile))
    {
       MMVII_UserError(eTyUEr::eOpenFile,std::string("For file ") + mNameFile);
    }
    std::ifstream infile(mNameFile);

    std::string line;
    // mNbLineRead = 0;
    int aNumL = 0;
    mNbLineRead = 0;
    bool  endByBla = (mTypes.back() == eRFS_TypeField::eBla);

    while (std::getline(infile, line))
    {
         if ((aNumL>=aL0) && (aNumL< aNumLastL))
	 {
             const char * aC = line.c_str();
             bool  GoOn = true;
	     size_t aNbToken = 0;
	     while (GoOn)
	     {
                 while ( (isspace(*aC))  || (*aC==',')   ) aC++;
                 if ((*aC==aComment) || (*aC==0))
		 {
		     GoOn= false;
		     if ((aNbToken!=0) &&  (aNbToken != mTypes.size()))
		     {
                         MMVII_UserError(eTyUEr::eUnClassedError,"Bad nb token at line " + ToStr(aNumL) + " of file " + mNameFile);
		     }
		 }
		 else 
                 {
                      if (aNbToken>= mNameFields.size())
		      {
                         StdOut() << "\n\n    *  In Line [" << line  << "]\n";
                         StdOut() << "\n\n    *  stil to process Line [" << std::string(aC)  << "]\n";
                         MMVII_UserError(eTyUEr::eUnClassedError,"Too much token ");
		      }
                      const char * aC0 = aC;
		      while (*aC && (!isspace(*aC)) && (*aC!=aComment)  && (*aC!=','))
                            aC++;

	              std::string aToken(aC0,size_t(aC-aC0));
		      std::string aNameField = mNameFields.at(aNbToken);
		      switch (mTypes.at(aNbToken))
		      {
		  	      case  eRFS_TypeField::eInt  : 
                                    AddVal(aNameField,aToken,mMapInt);
                              break;

		  	      case  eRFS_TypeField::eFloat  : 
                                    AddVal(aNameField,aToken,mMapFloat);
                              break;

		  	      case  eRFS_TypeField::eString  : 
                                    AddVal(aNameField,aToken,mMapString);
                              break;


			      default :
                              break;
		      }
	              aNbToken ++;
		      if (endByBla && (aNbToken+1 == mNameFields.size()))
		      {
			  GoOn = false;
		      }

		 }
	     }
	     if (aNbToken>0)
	        mNbLineRead++;
	 }
	 if (mDebug)
	    StdOut() << "-------------------------------------------\n";
         aNumL++;
    }
}

/* ***************************************************************************************** */
/*                                                                                           */
/*                              Different Bench on  cNewReadFilesStruct                      */
/*                                                                                           */
/* ***************************************************************************************** */

class cBenchcNewReadFilesStruct
{
      public :
         cBenchcNewReadFilesStruct(int aNum=1);

	 /// make a test for 1 config
	 void OneTest
              (
                   const std::string& aFormat,   // format of the fields to read in the file
		   const std::string& aSpecMand, // specification of mandtory fields
		   const std::string& aSpecOpt,  // specification of all fields (mandatory+optionnal)
		   int aFlagFields               // specify the fields that must be read
              );
         void ReadFile(cNewReadFilesStruct & aNRFS) { aNRFS.ReadFile(mNameFile,mL0,mLLast ,mComment);}

      private :
          std::string mNameFile;  ///<  name of file in MMVII test folders
	  int         mL0;        ///< first line to read
	  int         mLLast;     ///< last line to read
	  int         mComment;   ///< comment char

};



cBenchcNewReadFilesStruct::cBenchcNewReadFilesStruct(int aNum) :
     mNameFile  (cMMVII_Appli::InputDirTestMMVII() + "TestParseFile"+ToStr(aNum) + ".txt"),
     mL0        (2),
     mLLast     (21),
     mComment   ('#')
{
}


static   std::string TheFormatAll("El1NameFpiFl1Fl2Fc1Fc2Fc3Fl3El2");

static const int FlagEL1 =  1<<0;
static const int FlagName = 1<<1;
static const int FlagPi =   1<<2;
static const int FlagFl1 =  1<<3;
static const int FlagFl2 =  1<<4;
static const int FlagFc1 =  1<<5;
static const int FlagFc2 =  1<<6;
static const int FlagFc3 =  1<<7;
static const int FlagFl3 =  1<<8;
static const int FlagEL2 =  1<<9;



void cBenchcNewReadFilesStruct::OneTest
     (
          const std::string& aFormat,
	  const std::string& aSpecMand,
	  const std::string& aSpecOpt,
	  int aFlagT
     )
{
    // Test that we can create the  file, read the format, and test the number of line read
    cNewReadFilesStruct aNRFS(aFormat,aSpecMand,aSpecOpt);
    aNRFS.ReadFile(mNameFile,mL0,mLLast ,mComment);
    MMVII_INTERNAL_ASSERT_bench(aNRFS.NbLineRead()==3,"Bad nb lines in cBenchcNewReadFilesStruct");

    std::vector<std::string> aVNames={"Zero","One","Two"};  // "truth" to check the string values 

    for (size_t aKLine=0 ; aKLine<aNRFS.NbLineRead() ; aKLine++)  // parse all lines
    {
        // for each of the 10 values, check, if it must be read, the value is as expected
        if (aFlagT & FlagEL1)
           MMVII_INTERNAL_ASSERT_bench(aNRFS.GetValue<int>("El1",aKLine)==(int)aKLine,"BenchcNewReadF El0");

        if (aFlagT & FlagName)
           MMVII_INTERNAL_ASSERT_bench(aNRFS.GetValue<std::string>("Name",aKLine)==aVNames.at(aKLine),"BenchcNewReadF Names");

        if (aFlagT & FlagPi)
           MMVII_INTERNAL_ASSERT_bench(std::abs(aNRFS.GetValue<tREAL8>("Fpi",aKLine)-3.14)<1e-10,"BenchcNewReadF Pi");

        if (aFlagT & FlagFl1)
           MMVII_INTERNAL_ASSERT_bench(aNRFS.GetValue<tREAL8>("Fl1",aKLine)==aKLine,"BenchcNewReadF FL1");
        MMVII_INTERNAL_ASSERT_bench(aNRFS.FieldIsKnow("Fl1")==((aFlagT & FlagFl1)!=0),"BenchcNewReadF FielIsKnos Fl1 ");


        if (aFlagT & FlagFl2)
           MMVII_INTERNAL_ASSERT_bench(aNRFS.GetValue<tREAL8>("Fl2",aKLine)==aKLine,"BenchcNewReadF FL2");

        if (aFlagT & FlagFc1)
           MMVII_INTERNAL_ASSERT_bench(aNRFS.GetValue<tREAL8>("Fc1",aKLine)==1,"BenchcNewReadF Fc1");

        if (aFlagT & FlagFc2)
           MMVII_INTERNAL_ASSERT_bench(aNRFS.GetValue<tREAL8>("Fc2",aKLine)==2,"BenchcNewReadF Fc1");
        MMVII_INTERNAL_ASSERT_bench(aNRFS.FieldIsKnow("Fc2")==((aFlagT & FlagFc2)!=0),"BenchcNewReadF FielIsKnos Fc2 ");

        if (aFlagT & FlagFc3)
           MMVII_INTERNAL_ASSERT_bench(aNRFS.GetValue<tREAL8>("Fc3",aKLine)==3,"BenchcNewReadF Fc1");

        if (aFlagT & FlagFl3)
           MMVII_INTERNAL_ASSERT_bench(aNRFS.GetValue<tREAL8>("Fl3",aKLine)==aKLine,"BenchcNewReadF Fc1");

        if (aFlagT & FlagEL2)
           MMVII_INTERNAL_ASSERT_bench(aNRFS.GetValue<int>("El2",aKLine)==(int)aKLine,"BenchcNewReadF Fc1");
    }

}

void BenchcNewReadFilesStruct(cParamExeBench & aParam)
{
    if (! aParam.NewBench("NewFileStruct")) return;

    // Full specif  Integer-Line(1)   Name-Line  Pi Float-Line(1,2)   Float-Col(1,2,3) Integer-Line(2)

    cBenchcNewReadFilesStruct  aBRNF;

   // [1.a]  Basic test
        // [1.a]   format=spec mandatory = spec total;  all the field must be present, we set all flags to 1
    aBRNF.OneTest(TheFormatAll,TheFormatAll,TheFormatAll,0xFFFFFF);

        // [1.b]  Idem with some variation on syntax
    std::string aFormAllBis= "El1 Name,Fpi,Fl1Fl2,Fc1 Fc2 Fc3 Fl3 El2";
    aBRNF.OneTest(aFormAllBis,TheFormatAll,aFormAllBis,0xFFFFFF);


     // [2] Case where ther field in the file that are not in the specif, we must skip them
    std::string aFSkip= "El1NameFpi ?????   Fl3El2";
    std::string aFPart1= "El1NameFpi  Fl3El2";

        // [2.1]  the 2 specif contains only the 5 fields
    aBRNF.OneTest(aFSkip,aFPart1,aFPart1,FlagEL1|FlagName|FlagPi|FlagFl3|FlagEL2);

        // [2.2]  the optionall specif contains the 10 fields
    aBRNF.OneTest(aFSkip,aFPart1,aFormAllBis,FlagEL1|FlagName|FlagPi|FlagFl3|FlagEL2);
        // [2.3]  the format and optionall specif contains the 10 fields,  while all specif mand contains only 5
    aBRNF.OneTest(TheFormatAll,aFPart1,TheFormatAll,0xFFFFFF);

    // [3]  Now format using field with multiple arity

    std::string aFormatXStar   ("X* Name  Fc1 Fc2 Fc3 ");
    std::string aFormatXStarOpt  =  aFormatXStar + "Xtoto";
    {
        // A Format where X stand for El,Pi ... 
        std::string aFormatX6   ("X Name X XX Fc1 Fc2 Fc3  XX");

        cNewReadFilesStruct aNRFSX6(aFormatX6,aFormatX6,aFormatXStarOpt);
        aBRNF.ReadFile(aNRFSX6);
        MMVII_INTERNAL_ASSERT_bench(aNRFSX6.NbLineRead()==3,"Bad nb lines in cBenchcNewReadFilesStruct");
        // Check the function "Arrity" 
        MMVII_INTERNAL_ASSERT_bench(aNRFSX6.ArrityField("X")==6,"Bad arrity X in cBenchcNewReadFilesStruct");
        MMVII_INTERNAL_ASSERT_bench(aNRFSX6.ArrityField("Name")==1,"Bad arrity Name in cBenchcNewReadFilesStruct");
        MMVII_INTERNAL_ASSERT_bench(aNRFSX6.ArrityField("Y")==0,"Bad arrity Y in cBenchcNewReadFilesStruct");

        cNewReadFilesStruct aNRFSAll(TheFormatAll,TheFormatAll,TheFormatAll);
        aBRNF.ReadFile(aNRFSAll);
        MMVII_INTERNAL_ASSERT_bench(aNRFSX6.NbLineRead()==3,"Bad nb lines in cBenchcNewReadFilesStruct");

	//  Now Test the extension "Bla"
        cBenchcNewReadFilesStruct  aBRNF2(2);  // Acces to second file
        std::string aFormatXBla   ("X Name X ?? Fc1 Fc2 Fc3  XX Bla");
	cNewReadFilesStruct aNRFSBla(aFormatXBla,aFormatXStar,aFormatXStarOpt);
	aBRNF2.ReadFile(aNRFSBla);
        MMVII_INTERNAL_ASSERT_bench(aNRFSBla.NbLineRead()==3,"Bad nb lines in cBenchcNewReadFilesStruct");


	// Check for some fields that the value obtain by X[]  aFormatX6 are the same the corresponding fields in 
        for (size_t aKL=0 ; aKL<aNRFSX6.NbLineRead() ; aKL++)
        {
            MMVII_INTERNAL_ASSERT_bench((aNRFSAll.GetValue<int>("El1",aKL)==aNRFSX6.GetKthValue<tREAL8>("X",aKL,0)),"BenchcNewReadF El0");
            MMVII_INTERNAL_ASSERT_bench((aNRFSAll.GetValue<tREAL8>("Fpi",aKL)==aNRFSX6.GetKthValue<tREAL8>("X",aKL,1)),"BenchcNewReadF El0");
            MMVII_INTERNAL_ASSERT_bench((aNRFSAll.GetValue<int>("El2",aKL)==aNRFSX6.GetKthValue<tREAL8>("X",aKL,5)),"BenchcNewReadF El0");

            MMVII_INTERNAL_ASSERT_bench((aNRFSAll.GetValue<tREAL8>("Fpi",aKL)==aNRFSBla.GetKthValue<tREAL8>("X",aKL,1)),"BenchcNewReadF El0");
        }
    }
    aParam.EndBench();
}

};  // MMVII 
 



