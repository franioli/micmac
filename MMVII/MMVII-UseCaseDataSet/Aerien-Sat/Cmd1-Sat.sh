set -e
#MMVII  ImportInitExtSens  IMG_PNEO3_202309051055014_PAN_SEN_PWOI_000129104_1_1_F_1_P_R1C1.tif [IMG_PNEO3_202309051055014_PAN_SEN_PWOI_000129104_1_1_F_1_P_R1C1.tif,../PNEO/000129104_1_1_STD_A/IMG_01_PNEO3_MS-FS/RPC_PNEO3_202309051055014_MS-FS_SEN_PWOI_000129104_1_1_F_1.XML] WGS84 ExtPatFile=0


#MMVII  ImportInitExtSens  IMG_PNEO3_202309051055194_PAN_SEN_PWOI_000129105_1_1_F_1_P_R1C1.tif [IMG_PNEO3_202309051055194_PAN_SEN_PWOI_000129105_1_1_F_1_P_R1C1.tif,../PNEO/000129105_1_1_STD_A/IMG_01_PNEO3_MS-FS/RPC_PNEO3_202309051055194_MS-FS_SEN_PWOI_000129105_1_1_F_1.XML] WGS84 ExtPatFile=0

#MMVII  ImportInitExtSens  IMG_PNEO4_202309081102245_PAN_SEN_PWOI_000129106_1_1_F_1_P_R1C1.tif [IMG_PNEO4_202309081102245_PAN_SEN_PWOI_000129106_1_1_F_1_P_R1C1.tif,../PNEO/000129106_1_1_STD_A/IMG_01_PNEO4_MS-FS/RPC_PNEO4_202309081102245_MS-FS_SEN_PWOI_000129106_1_1_F_1.XML] WGS84 ExtPatFile=0

#MMVII  ImportInitExtSens  IMG_PNEO4_202309081102495_PAN_SEN_PWOI_000129107_1_1_F_1_P_R1C1.tif [IMG_PNEO4_202309081102495_PAN_SEN_PWOI_000129107_1_1_F_1_P_R1C1.tif,../PNEO/000129107_1_1_STD_A/IMG_01_PNEO4_MS-FS/RPC_PNEO4_202309081102495_MS-FS_SEN_PWOI_000129107_1_1_F_1.XML] WGS84 ExtPatFile=0

#RPC_PNEO3_202309051055014_PAN_SEN_PWOI_000129104_1_1_F_1.XML
#IMG_PNEO3_202309051055014_PAN_SEN_PWOI_000129104_1_1_F_1_P_R1C1.tif


#   Create  Sets 
MMVII EditSet ImPNEO.xml += IMG_PNEO3_202309051055014_PAN_SEN_PWOI_000129104_1_1_F_1_P_R1C1.tif ExtPatFile=0
MMVII EditSet ImPNEO.xml += IMG_PNEO3_202309051055194_PAN_SEN_PWOI_000129105_1_1_F_1_P_R1C1.tif ExtPatFile=0
MMVII EditSet ImPNEO.xml += IMG_PNEO4_202309081102245_PAN_SEN_PWOI_000129106_1_1_F_1_P_R1C1.tif ExtPatFile=0
MMVII EditSet ImPNEO.xml += IMG_PNEO4_202309081102495_PAN_SEN_PWOI_000129107_1_1_F_1_P_R1C1.tif ExtPatFile=0



#  Import Sat data
MMVII ImportInitExtSens  ImPNEO.xml  ['IMG(.*)_P_R1C1.tif','ExternalData/RPC$1.XML'] SatWGS84
MMVII ImportORGI ExternalData/ORGI_LNR/ 

#  Check import + initial residual
MMVII TestSensor ImPNEO.xml SatWGS84 InPointsMeasure=ORGI
MMVII ReportGCP  ImPNEO.xml ORGI SatWGS84

#     RTL SENS
MMVII SysCoCreateRTL ImPNEO.xml RTL InOri=SatWGS84 z0=0
MMVII GCPChSysCo RTL ORGI RTLSat SysIn=Geog
MMVII OriChSysCo  ImPNEO.xml RTL   SatWGS84 RTLFix


# ==========================================

MMVII OriParametrizeSensor ImPNEO.xml RTLFix RTLD0 0
MMVII OriParametrizeSensor ImPNEO.xml RTLFix RTLD1 1
MMVII OriParametrizeSensor ImPNEO.xml RTLFix RTLD2 2

MMVII OriBundleAdj ImPNEO.xml RTLD2  TestAdjSat GCPDir=RTLSat   GCPW=[1,1] TiePWeight=[1,1] TPDir=ORGI
MMVII ReportGCP  ImPNEO.xml RTLSat TestAdjSat




