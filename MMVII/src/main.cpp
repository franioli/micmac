#include "cMMVII_Appli.h"
#include "MMVII_DeclareCste.h"
#include <clocale>
#include "MMVII_Sys.h"
/*

Delaunay/delaunator : Copyright (c) 2018 Volodymyr Bilonenko  (MIT Licence)
Ply/happly  Copyright (c) 2018 Nick Sharp
eigen ...

*/


using namespace MMVII;



int main(int argc, char ** argv)
{


   std::setlocale(LC_ALL, "C");
   // std::setlocale(LC_ALL, "en_US.UTF-8");

   cMMVII_Appli::InitMMVIIDirs(MMVII_CanonicalRootDirFromExec());
   // Debug, print command
#if 0
   {
       StdOut() << "==========COMM=====   " << std::endl;
       for (int aK=0 ; aK<argc ; aK++)
       {
            if (aK) StdOut() << " ";
            StdOut() << argv[aK];
       }
       StdOut() << std::endl;
   }
#endif
    
   if (argc>1)
   {
      std::string aNameCom = argv[1];

      // Recherche la specif correspondant au nom de commande
      cSpecMMVII_Appli*  aSpec = cSpecMMVII_Appli::SpecOfName(aNameCom,true);

      // Execute si match
      if (aSpec)
      {
         std::vector<std::string> aVArgs;
         for (int aK=0 ; aK<argc; aK++)
             aVArgs.push_back(argv[aK]);
         int aRes =  aSpec->AllocExecuteDestruct(aVArgs);
         return aRes;
      }
   }

   // Affiche toutes les commandes
   for (const auto & aSpec : cSpecMMVII_Appli::VecAll())
   {
      StdOut()  << aSpec->Name() << " => " << aSpec->Comment() << std::endl;
   }
   return EXIT_SUCCESS;
}



