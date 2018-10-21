#define  INCL_WINSHELLDATA
#define  INCL_VIO
#define  INCL_DOS
#define  INCL_DOSFILEMGR
#define  INCL_DOSMEMMGR
#define  INCL_DOSMISC
#define  INCL_DOSNMPIPES
#define  INCL_ERRORS
#define  INCL_REXXSAA
#define  _DLL
#define  _MT
#include <os2.h>
#include <rexxsaa.h>
#include <memory.h>
#include <malloc.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <engine\engapi.h>
#include <engsbapi\engsbapi.h>
#include <pcmmgr\pcmapi.h>

/*********************************************************************/
/*  Various definitions used by various functions.                   */
/*********************************************************************/

#define  MAX_DIGITS     9          /* maximum digits in numeric arg  */
#define  MAX            256        /* temporary buffer length        */
#define  IBUF_LEN       4096       /* Input buffer length            */
#define  AllocFlag      PAG_COMMIT | PAG_WRITE  /* for DosAllocMem   */

#define MAXBUFFER 1000

int result;
int count;

ProductBaseID pbID;
ProductBaseIteratorID pbitID;

ProductLineID plID;
ProductLineIteratorID plitID;

SystemID sysID;
SystemIteratorID sysitID;

ComponentClassID ccID;
ComponentClassIteratorID ccitID;

DataTypeID dtID;
DataTypeIteratorID dtitID;

ResourceID resID;
ResourceIteratorID resitID;

ProdCompMapID pcmID;
ProdCompMapIteratorID pcmitID;

ProdCompMapInstanceID pcmiID;
ProdCompMapInstanceIteratorID pcmiitID;

ProductID prodID;
long proditID;

char buf[MAXBUFFER];
char myresult[MAXBUFFER];
int len = MAXBUFFER;

INT systemInitialized = FALSE;
INT systemBundled = FALSE;
INT productlineInitialized = FALSE;
INT pcmLoaded = FALSE;
INT pbLoaded = FALSE;

const char CompiledFileName[13] = "INFILE.ASC\0";
const char pcmPrefix[13] = "bundling\0";
const char PBName[13] = "Power\0";
const char PBVersion[4] = "1.0\0";

/*********************************************************************/
/*  Declare all exported functions as REXX functions.                */
/*********************************************************************/
RexxFunctionHandler SBLoadParsePB;
RexxFunctionHandler SBLoadCompiledPB;
RexxFunctionHandler SBSaveParsePB;
RexxFunctionHandler SBSaveCompiledPB;
RexxFunctionHandler SBVersion;
RexxFunctionHandler SBLoadFuncs;
RexxFunctionHandler SBDropFuncs;
RexxFunctionHandler SBCountAllPB;
RexxFunctionHandler SBConfigureSystem;
RexxFunctionHandler SBGetAllClasses;
RexxFunctionHandler SBGetAllCompsAndDesc;
RexxFunctionHandler SBGetAllDataTypes;
RexxFunctionHandler SBGetAllProductLines;
RexxFunctionHandler SBGetAllResources;
RexxFunctionHandler SBRequestComponent;
RexxFunctionHandler SBRequestResource;
RexxFunctionHandler SBNewSystem;
RexxFunctionHandler SBBundleSystem;
RexxFunctionHandler SBLoadParsePCM;
RexxFunctionHandler SBLoadCompiledPCM;
RexxFunctionHandler SBGetAllBOMData;

RexxFunctionHandler SBGetProductLineProducts;

RexxFunctionHandler SBPCMVersion;
RexxFunctionHandler SBGetAllProducts;
RexxFunctionHandler SBGetAllProductsWithDesc;
RexxFunctionHandler SBGetProductProductLines;

RexxFunctionHandler SBBundledResults;
RexxFunctionHandler SBSQLQuery;
RexxFunctionHandler SBBundledResultsExpanded;

RexxFunctionHandler SBReconfigureSystem;

RexxFunctionHandler SBGetImplicitRequests;
RexxFunctionHandler SBGetExplicitRequests;


/*********************************************************************/
/* RxStemData                                                        */
/*   Structure which describes as generic                            */
/*   stem variable.                                                  */
/*********************************************************************/

typedef struct RxStemData {
    SHVBLOCK shvb;                     /* Request block for RxVar    */
    CHAR ibuf[IBUF_LEN];               /* Input buffer               */
    CHAR varname[MAX];                 /* Buffer for the variable    */
                                       /* name                       */
    CHAR stemname[MAX];                /* Buffer for the variable    */
                                       /* name                       */
    ULONG stemlen;                     /* Length of stem.            */
    ULONG vlen;                        /* Length of variable value   */
    ULONG j;                           /* Temp counter               */
    ULONG tlong;                       /* Temp counter               */
    ULONG count;                       /* Number of elements         */
                                       /* processed                  */
} RXSTEMDATA;

/*********************************************************************/
/* RxFncTable                                                        */
/*   Array of names of the SBREXX functions.                         */
/*   This list is used for registration and deregistration.          */
/*********************************************************************/

static PSZ  RxFncTable[] =
   {
      "SBLoadParsePB",
      "SBLoadCompiledPB",
      "SBSaveParsePB",
      "SBSaveCompiledPB",
      "SBVersion",
      "SBLoadFuncs",
      "SBDropFuncs",
      "SBCountAllPB",
      "SBConfigureSystem",
      "SBGetAllClasses",
      "SBGetAllDataTypes",
      "SBGetAllProductLines",
      "SBGetAllResources",
      "SBRequestComponent",
      "SBRequestResource",
      "SBNewSystem",
      "SBBundleSystem",
      "SBLoadParsePCM",
      "SBLoadCompiledPCM",
      "SBGetAllBOMData",
      "SBPCMVersion",
      "SBGetAllProducts",
      "SBGetAllCompsAndDesc",
      "SBGetAllProductsWithDesc",
      "SBGetProductProductLines",
      "SBGetProductLineProducts",
      "SBBundledResults",
      "SBSQLQuery",
      "SBBundledResultsExpanded",
      "SBReconfigureSystem",
      "SBGetImplicitRequests",
      "SBGetExplicitRequests"
   };

/*********************************************************************/
/* Numeric Error Return Strings                                      */
/*********************************************************************/

#define  NO_UTIL_ERROR    "0"          /* No error whatsoever        */
#define  ERROR_NOMEM      "2"          /* Insufficient memory        */
#define  ERROR_FILEOPEN   "3"          /* Error opening text file    */

/*********************************************************************/
/* Alpha Numeric Return Strings                                      */
/*********************************************************************/

#define  ERROR_RETSTR   "ERROR:"

/*********************************************************************/
/* Numeric Return calls                                              */
/*********************************************************************/

#define  INVALID_ROUTINE 40            /* Raise Rexx error           */
#define  VALID_ROUTINE    0            /* Successful completion      */

/*********************************************************************/
/* Some useful macros                                                */
/*********************************************************************/

#define BUILDRXSTRING(t, s) { \
  strcpy((t)->strptr,(s));\
  (t)->strlength = strlen((s)); \
}

/********************************************************************
* Function:  string2long(string, number)                            *
*                                                                   *
* Purpose:   Validates and converts an ASCII-Z string from string   *
*            form to an unsigned long.  Returns FALSE if the number *
*            is not valid, TRUE if the number was successfully      *
*            converted.                                             *
*                                                                   *
* RC:        TRUE - Good number converted                           *
*            FALSE - Invalid number supplied.                       *
*********************************************************************/

BOOL string2long(PSZ string, LONG *number)
{
  ULONG    accumulator;                /* converted number           */
  INT      length;                     /* length of number           */
  INT      sign;                       /* sign of number             */

  sign = 1;                            /* set default sign           */
  if (*string == '-') {                /* negative?
    sign = -1;                         /* change sign                */
    string++;                          /* step past sign             */
  }

  length = strlen(string);             /* get length of string       */
  if (length == 0 ||                   /* if null string             */
      length > MAX_DIGITS)             /* or too long                */
    return FALSE;                      /* not valid                  */

  accumulator = 0;                     /* start with zero            */

  while (length) {                     /* while more digits          */
    if (!isdigit(*string))             /* not a digit?               */
      return FALSE;                    /* tell caller                */
                                       /* add to accumulator         */
    accumulator = accumulator *10 + (*string - '0');
    length--;                          /* reduce length              */
    string++;                          /* step pointer               */
  }
  *number = accumulator * sign;        /* return the value           */
  return TRUE;                         /* good number                */
}


/*************************************************************************
* Function:  SBDropFuncs                                                 *
*                                                                        *
* Syntax:    call SysDropFuncs                                           *
*                                                                        *
* Return:    NO_UTIL_ERROR - Successful.                                 *
*************************************************************************/

ULONG SBDropFuncs(CHAR *name, ULONG numargs, RXSTRING args[],
                          CHAR *queuename, RXSTRING *retstr)
{
  INT     entries;                     /* Num of entries             */
  INT     j;                           /* Counter                    */

  if (numargs != 0)                    /* no arguments for this      */
    return INVALID_ROUTINE;            /* raise an error             */

  retstr->strlength = 0;               /* return a null string result*/

  entries = sizeof(RxFncTable)/sizeof(PSZ);

  for (j = 0; j < entries; j++)
    RexxDeregisterFunction(RxFncTable[j]);

  return VALID_ROUTINE;                /* no error on call           */
}


/*************************************************************************
* Function:  SBLoadFuncs                                                 *
*                                                                        *
* Syntax:    call SBLoadFuncs [option]                                   *
*                                                                        *
* Params:    none                                                        *
*                                                                        *
* Return:    null string                                                 *
*************************************************************************/

ULONG SBLoadFuncs(CHAR *name, ULONG numargs, RXSTRING args[],
                           CHAR *queuename, RXSTRING *retstr)
{
  INT    entries;                      /* Num of entries             */
  INT    j;                            /* Counter                    */

  retstr->strlength = 0;               /* set return value           */
                                       /* check arguments            */
  if (numargs > 0)
    return INVALID_ROUTINE;

  entries = sizeof(RxFncTable)/sizeof(PSZ);

  for (j = 0; j < entries; j++) {
    RexxRegisterFunctionDll(RxFncTable[j],
          "SBREXX", RxFncTable[j]);
  }
  return VALID_ROUTINE;
}

/*************************************************************************
* Function:  SBLoadParsePB                                               *
*                                                                        *
* Syntax:    call SBLoadParsePB pbFileName                               *
*                                                                        *
* Params:    pbFileName - Product Base File to load                      *
*                                                                        *
* Return:    NO_UTIL_ERROR                                               *
*            Return code from sb_LoadProductBase                         *
*************************************************************************/

ULONG SBLoadParsePB(CHAR *name, ULONG numargs, RXSTRING args[],
                       CHAR *queuename, RXSTRING *retstr)
{
 char drive[_MAX_DRIVE],dir[_MAX_DIR],fname[_MAX_FNAME],ext[_MAX_EXT],prsfile[255];
 ULONG  rc;                           /* Ret code of func           */
 FILE *stream;

 sprintf((CHAR *)&myresult,"FAILURE");
 _splitpath(args[0].strptr,(CHAR *)&drive,(CHAR *)&dir,(CHAR *)&fname,(CHAR *)&ext);

 if (numargs != 1)
                                       /* If no args, then its an    */
                                       /* incorrect call             */
 return INVALID_ROUTINE;

 if ((strcmpi(ext,".PRS") != 0) && (strcmpi(ext,".PRB") != 0))
    return INVALID_ROUTINE;


  if (pbLoaded) {
        result = sb_FreeProductBase(pbID);
        pbLoaded = FALSE;
  }

  result = sb_NewProductBase(&pbID);

  if (result == SB_SUCCESS) {
     if (strcmpi(ext,".PRS") == 0) {
        result = sb_ParseFile(args[0].strptr, pbID);
        if (result == SB_SUCCESS) {
           sprintf((CHAR *)&myresult, "%s  %s %s", "SUCCESS", args[0].strptr, "read Successfully.");
           pbLoaded = TRUE;
        }
        else {
           result = sb_FreeProductBase(pbID);
           sprintf((CHAR *)&myresult,"FAILURE Unable to parse input file.");
        }
     }
     else {   /* Read PRB file */
        if (NULL == (stream = fopen(args[0].strptr, "r")))
            sprintf((CHAR *)&myresult,"FAILURE Unable to open PRB file.");
        else {
            for(;((fscanf(stream,"%s\n",prsfile) != EOF) && (result != SB_FAILURE));) {
               printf("Reading file: %s\n",prsfile);
               result = sb_ParseFile(prsfile, pbID);
            }
            fclose(stream);
            if (result == SB_FAILURE) {
               sprintf((CHAR *)&myresult,"FAILURE Unable to parse file %s",prsfile);
            }
            else {
               sprintf((CHAR *)&myresult,"%s  %s %s","SUCCESS", args[0].strptr, "read Successfully.");
               pbLoaded = TRUE;
            }
        }
        /*  End Read PRB file */
     }
  }
  else sprintf((CHAR *)&myresult,"FAILURE Unable to allocate a new product base.");

  sprintf(retstr->strptr, "%s", myresult);   /* result is return code      */
  retstr->strlength = strlen(retstr->strptr);
  return VALID_ROUTINE;
}

/*************************************************************************
* Function:  SBLoadCompiledPB                                            *
*                                                                        *
* Syntax:    call SBLoadCompiledPB                                       *
*                                                                        *
*                                                                        *
* Return:    NO_UTIL_ERROR                                               *
*                                                                        *
*************************************************************************/

ULONG SBLoadCompiledPB(CHAR *name, ULONG numargs, RXSTRING args[],
                       CHAR *queuename, RXSTRING *retstr)
{
 ULONG  rc;                           /* Ret code of func           */
 sprintf((CHAR *)&myresult,"");
 if (numargs != 0)
                                       /* If no args, then its an    */
                                       /* incorrect call             */
 return INVALID_ROUTINE;

 if (pbLoaded) {
       result = sb_FreeProductBase(pbID);
       pbLoaded = FALSE;
 }

 result = sb_NewProductBase(&pbID);

 if (result == SB_SUCCESS) {
    result = sb_LoadProductBase(CompiledFileName,&pbID,1);
    if (result == SB_SUCCESS) {
       sprintf((CHAR *)&myresult, "%s  %s %s", "SUCCESS", CompiledFileName, "read Successfully.");
       pbLoaded = TRUE;
    }
    else {
       result = sb_FreeProductBase(pbID);
       sprintf((CHAR *)&myresult,"%s %s","FAILURE Unable to read", CompiledFileName);
    }
 }
 else sprintf((CHAR *)&myresult,"FAILURE Unable to allocate a new product base.");

 sprintf(retstr->strptr, "%s", myresult);   /* result is return code      */
 retstr->strlength = strlen(retstr->strptr);
 return VALID_ROUTINE;
}


/*************************************************************************
* Function:  SBSaveParsePB                                               *
*                                                                        *
* Syntax:    call SBSaveParsePB                                          *
*                                                                        *
*                                                                        *
* Return:    NO_UTIL_ERROR                                               *
*                                                                        *
*************************************************************************/

ULONG SBSaveParsePB(CHAR *name, ULONG numargs, RXSTRING args[],
                       CHAR *queuename, RXSTRING *retstr)
{
 ULONG  rc;                           /* Ret code of func           */
 sprintf((CHAR *)&myresult,"FAILURE");
 if (numargs != 1)
                                       /* If no args, then its an    */
                                       /* incorrect call             */
 return INVALID_ROUTINE;

 if (pbLoaded) {
    result = sb_WriteParseFile(pbID, args[0].strptr);
    if (result == SB_SUCCESS) {
       sprintf((CHAR *)&myresult,"%s  %s %s","SUCCESS", args[0].strptr, "saved Successfully.");
    }
 }
 else sprintf((CHAR *)&myresult,"FAILURE  No product base loaded.");

 sprintf(retstr->strptr, "%s", myresult);   /* result is return code      */
 retstr->strlength = strlen(retstr->strptr);
 return VALID_ROUTINE;
}


/*************************************************************************
* Function:  SBSaveCompiledPB                                            *
*                                                                        *
* Syntax:    call SBSaveCompiledPB                                       *
*                                                                        *
*                                                                        *
* Return:    NO_UTIL_ERROR                                               *
*                                                                        *
*************************************************************************/

ULONG SBSaveCompiledPB(CHAR *name, ULONG numargs, RXSTRING args[],
                       CHAR *queuename, RXSTRING *retstr)
{
 ULONG  rc;                           /* Ret code of func           */
 sprintf((CHAR *)&myresult,"FAILURE");
 if (numargs != 0)
                                       /* If no args, then its an    */
                                       /* incorrect call             */
 return INVALID_ROUTINE;

 if (pbLoaded) {
    result = sb_SaveProductBase(pbID, CompiledFileName, 1);
    if (result == SB_SUCCESS) {
       sprintf((CHAR *)&myresult,"%s  %s %s","SUCCESS", CompiledFileName, "saved Successfully.");
    }
 }
 else sprintf((CHAR *)&myresult,"FAILURE  No product base loaded.");


 sprintf(retstr->strptr, "%s", myresult);   /* result is return code      */
 retstr->strlength = strlen(retstr->strptr);
 return VALID_ROUTINE;
}


/*************************************************************************
* Function:  SBVersion                                                   *
*                                                                        *
* Syntax:    call SBVersion                                              *
*                                                                        *
* Return:    SalesBUILDER Version String                                 *
*************************************************************************/

ULONG SBVersion(CHAR *name, ULONG numargs, RXSTRING args[],
                        CHAR *queuename, RXSTRING *retstr)
{
  ULONG  rc = 0;                       /* Return code of this func   */

  if (numargs != 0)                    /* validate arg count         */
    return INVALID_ROUTINE;

  result = sb_GetVersionString(buf,MAXBUFFER);
  sprintf(retstr->strptr, "%s", buf);
  retstr->strlength = strlen(retstr->strptr);
  return VALID_ROUTINE;
}

/*************************************************************************
* Function:  SBPCMVersion                                                *
*                                                                        *
* Syntax:    call SBPCMVersion                                           *
*                                                                        *
* Return:    SalesBUILDER PCM Version String                             *
*************************************************************************/

ULONG SBPCMVersion(CHAR *name, ULONG numargs, RXSTRING args[],
                        CHAR *queuename, RXSTRING *retstr)
{
  ULONG  rc = 0;                       /* Return code of this func   */

  if (numargs != 0)                    /* validate arg count         */
    return INVALID_ROUTINE;

  result = sbpcm_GetVersionString(buf,MAXBUFFER);
  sprintf(retstr->strptr, "%s", buf);
  retstr->strlength = strlen(retstr->strptr);
  return VALID_ROUTINE;
}

/*************************************************************************
* Function:  SBCountAllPB                                                *
*                                                                        *
* Syntax:    call SBCountAllPB                                           *
*                                                                        *
* Params:                                                                *
*                                                                        *
* Return:    NO_UTIL_ERROR                                               *
*                                                                        *
*************************************************************************/

ULONG SBCountAllPB(CHAR *name, ULONG numargs, RXSTRING args[],
                       CHAR *queuename, RXSTRING *retstr)
{
  ULONG  rc = 0;                           /* Ret code of func           */

  if (numargs != 0)
   return INVALID_ROUTINE;

  sprintf((CHAR *)&myresult,"");
  result = sb_GetAllProductBases(&pbitID);
  if (result == SB_SUCCESS) {
     result = sb_Count(pbitID,&count);
     if (result == SB_SUCCESS) {sprintf((CHAR *)&myresult,"%u Product Bases found.",count);}
  }
  result = sb_FreeIterator(pbitID);

  sprintf(retstr->strptr, "%s", myresult);   /* result is string      */
  retstr->strlength = strlen(retstr->strptr);
  return VALID_ROUTINE;

}


/*************************************************************************
* Function:  SBConfigureSystem                                           *
*                                                                        *
* Syntax:    call SBConfigureSystem                                      *
*                                                                        *
* Params:                                                                *
*                                                                        *
* Return:    NO_UTIL_ERROR                                               *
*                                                                        *
*************************************************************************/

ULONG SBConfigureSystem(CHAR *name, ULONG numargs, RXSTRING args[],
                       CHAR *queuename, RXSTRING *retstr)
{
  ULONG  rc = 0;                           /* Ret code of func           */

  if (numargs != 0)
   return INVALID_ROUTINE;

  sprintf((CHAR *)&myresult,"FAILURE");
  if (systemInitialized) {
     result = sb_ConfigureSystem(sysID);
     if (result == SB_SUCCESS) sprintf((CHAR *)&myresult,"SUCCESS  SBConfigureSystem");
     else sprintf((CHAR *)&myresult,"FAILURE  SBConfigureSystem:  Call to sb_ConfigureSystem failed");
  }
  else sprintf((CHAR *)&myresult,"FAILURE  SBConfigureSystem: No system initialized");

  sprintf(retstr->strptr, "%s", myresult);   /* result is string      */
  retstr->strlength = strlen(retstr->strptr);
  return VALID_ROUTINE;

}


/*************************************************************************
* Function:  SBGetAllClasses                                             *
*                                                                        *
* Syntax:    call SBGetAllClasses                                        *
*                                                                        *
* Params:                                                                *
*                                                                        *
* Return:    NO_UTIL_ERROR                                               *
*                                                                        *
*************************************************************************/

ULONG SBGetAllClasses(CHAR *name, ULONG numargs, RXSTRING args[],
                       CHAR *queuename, RXSTRING *retstr)
{
  int i;
  RXSTEMDATA ldp;
  ULONG len;
  ULONG  rc = 0;                           /* Ret code of func           */

  if (numargs != 1)
   return INVALID_ROUTINE;
                                       /* Initialize data area       */
  ldp.count = 0;
  strcpy(ldp.varname, args[0].strptr);
  ldp.stemlen = args[0].strlength;
  strupr(ldp.varname);                 /* uppercase the name         */

  if (ldp.varname[ldp.stemlen-1] != '.')
    ldp.varname[ldp.stemlen++] = '.';

  sprintf((CHAR *)&myresult,"FAILURE");
  if (pbLoaded) {
     result = sb_GetAllClasses(pbID,&ccitID);
     if (result == SB_SUCCESS) {
        result = sb_Count(ccitID,&count);
        if (result == SB_SUCCESS) {
           for (i=1; i<= count; i++) {
              result = sb_Next(ccitID,&ccID);
              if (result == SB_SUCCESS) {
                 result = sb_GetClassName(ccID,(CHAR *)&buf,MAXBUFFER);
                 len = strlen(buf);
                 memcpy(ldp.ibuf, buf, len);
                 ldp.vlen = len;
                 ldp.count++;
                 sprintf(ldp.varname+ldp.stemlen, "%d", ldp.count);
                 if (ldp.ibuf[ldp.vlen-1] == '\n')
                    ldp.vlen--;
                 ldp.shvb.shvnext = NULL;
                 ldp.shvb.shvname.strptr = ldp.varname;
                 ldp.shvb.shvname.strlength = strlen(ldp.varname);
                 ldp.shvb.shvnamelen = ldp.shvb.shvname.strlength;
                 ldp.shvb.shvvalue.strptr = ldp.ibuf;
                 ldp.shvb.shvvalue.strlength = ldp.vlen;
                 ldp.shvb.shvvaluelen = ldp.vlen;
                 ldp.shvb.shvcode = RXSHV_SET;
                 ldp.shvb.shvret = 0;
                 if (RexxVariablePool(&ldp.shvb) == RXSHV_BADN)
                    return INVALID_ROUTINE;      /* error on non-zero          */
              }
           }
           sprintf((CHAR *)&myresult,"SUCCESS  SBGetAllClasses");
        }
     }
  }
  else sprintf((CHAR *)&myresult,"FAILURE  SBGetAllClasses");

  result = sb_FreeIterator(ccitID);


  sprintf(ldp.ibuf, "%d", ldp.count);
  ldp.varname[ldp.stemlen] = '0';
  ldp.varname[ldp.stemlen+1] = 0;
  ldp.shvb.shvnext = NULL;
  ldp.shvb.shvname.strptr = ldp.varname;
  ldp.shvb.shvname.strlength = ldp.stemlen+1;
  ldp.shvb.shvnamelen = ldp.stemlen+1;
  ldp.shvb.shvvalue.strptr = ldp.ibuf;
  ldp.shvb.shvvalue.strlength = strlen(ldp.ibuf);
  ldp.shvb.shvvaluelen = ldp.shvb.shvvalue.strlength;
  ldp.shvb.shvcode = RXSHV_SET;
  ldp.shvb.shvret = 0;
  if (RexxVariablePool(&ldp.shvb) == RXSHV_BADN)
    return INVALID_ROUTINE;            /* error on non-zero          */

 sprintf(retstr->strptr, "%s", myresult);   /* result is string      */
 retstr->strlength = strlen(retstr->strptr);
 return VALID_ROUTINE;

}


/*************************************************************************
* Function:  SBGetAllDataTypes                                           *
*                                                                        *
* Syntax:    call SBGetAllDataTypes                                      *
*                                                                        *
* Params:                                                                *
*                                                                        *
* Return:    NO_UTIL_ERROR                                               *
*                                                                        *
*************************************************************************/

ULONG SBGetAllDataTypes(CHAR *name, ULONG numargs, RXSTRING args[],
                       CHAR *queuename, RXSTRING *retstr)
{
  int i;
  RXSTEMDATA ldp;
  ULONG len;
  ULONG  rc = 0;                           /* Ret code of func           */

  if (numargs != 1)
   return INVALID_ROUTINE;
                                       /* Initialize data area       */
  ldp.count = 0;
  strcpy(ldp.varname, args[0].strptr);
  ldp.stemlen = args[0].strlength;
  strupr(ldp.varname);                 /* uppercase the name         */

  if (ldp.varname[ldp.stemlen-1] != '.')
    ldp.varname[ldp.stemlen++] = '.';

  sprintf((CHAR *)&myresult,"FAILURE");
  result = sb_GetAllDataTypes(pbID,&dtitID);
  if (result == SB_SUCCESS) {
     result = sb_Count(dtitID,&count);
     if (result == SB_SUCCESS) {
        for (i=1; i<= count; i++) {
           result = sb_Next(dtitID,&dtID);
           if (result == SB_SUCCESS) {
              result = sb_GetDataTypeName(dtID,(CHAR *)&buf,MAXBUFFER);
              len = strlen(buf);
              memcpy(ldp.ibuf, buf, len);
              ldp.vlen = len;
              ldp.count++;
              sprintf(ldp.varname+ldp.stemlen, "%d", ldp.count);
              if (ldp.ibuf[ldp.vlen-1] == '\n')
                 ldp.vlen--;
              ldp.shvb.shvnext = NULL;
              ldp.shvb.shvname.strptr = ldp.varname;
              ldp.shvb.shvname.strlength = strlen(ldp.varname);
              ldp.shvb.shvnamelen = ldp.shvb.shvname.strlength;
              ldp.shvb.shvvalue.strptr = ldp.ibuf;
              ldp.shvb.shvvalue.strlength = ldp.vlen;
              ldp.shvb.shvvaluelen = ldp.vlen;
              ldp.shvb.shvcode = RXSHV_SET;
              ldp.shvb.shvret = 0;
              if (RexxVariablePool(&ldp.shvb) == RXSHV_BADN)
                 return INVALID_ROUTINE;      /* error on non-zero          */
           }
        }
     }
  }
  result = sb_FreeIterator(dtitID);
  sprintf((CHAR *)&myresult,"SUCCESS");

  sprintf(ldp.ibuf, "%d", ldp.count);
  ldp.varname[ldp.stemlen] = '0';
  ldp.varname[ldp.stemlen+1] = 0;
  ldp.shvb.shvnext = NULL;
  ldp.shvb.shvname.strptr = ldp.varname;
  ldp.shvb.shvname.strlength = ldp.stemlen+1;
  ldp.shvb.shvnamelen = ldp.stemlen+1;
  ldp.shvb.shvvalue.strptr = ldp.ibuf;
  ldp.shvb.shvvalue.strlength = strlen(ldp.ibuf);
  ldp.shvb.shvvaluelen = ldp.shvb.shvvalue.strlength;
  ldp.shvb.shvcode = RXSHV_SET;
  ldp.shvb.shvret = 0;
  if (RexxVariablePool(&ldp.shvb) == RXSHV_BADN)
    return INVALID_ROUTINE;            /* error on non-zero          */

 sprintf(retstr->strptr, "%s", myresult);   /* result is string      */
 retstr->strlength = strlen(retstr->strptr);
 return VALID_ROUTINE;

}


/*************************************************************************
* Function:  SBGetAllProductLines                                        *
*                                                                        *
* Syntax:    call SBGetAllProductLines                                   *
*                                                                        *
* Params:                                                                *
*                                                                        *
* Return:    NO_UTIL_ERROR                                               *
*                                                                        *
*************************************************************************/

ULONG SBGetAllProductLines(CHAR *name, ULONG numargs, RXSTRING args[],
                       CHAR *queuename, RXSTRING *retstr)
{
  int i;
  RXSTEMDATA ldp;
  ULONG len;
  ULONG  rc = 0;                           /* Ret code of func           */

  if (numargs != 1)
   return INVALID_ROUTINE;
                                       /* Initialize data area       */
  ldp.count = 0;
  strcpy(ldp.varname, args[0].strptr);
  ldp.stemlen = args[0].strlength;
  strupr(ldp.varname);                 /* uppercase the name         */

  if (ldp.varname[ldp.stemlen-1] != '.')
    ldp.varname[ldp.stemlen++] = '.';

  sprintf((CHAR *)&myresult,"FAILURE");
  result = sb_GetAllProductLines(pbID,&plitID);
  if (result == SB_SUCCESS) {
     result = sb_Count(plitID,&count);
     if (result == SB_SUCCESS) {
        for (i=1; i<= count; i++) {
           result = sb_Next(plitID,&plID);
           if (result == SB_SUCCESS) {
              result = sb_GetProductLineName(plID,(CHAR *)&buf,MAXBUFFER);
              len = strlen(buf);
              memcpy(ldp.ibuf, buf, len);
              ldp.vlen = len;
              ldp.count++;
              sprintf(ldp.varname+ldp.stemlen, "%d", ldp.count);
              if (ldp.ibuf[ldp.vlen-1] == '\n')
                 ldp.vlen--;
              ldp.shvb.shvnext = NULL;
              ldp.shvb.shvname.strptr = ldp.varname;
              ldp.shvb.shvname.strlength = strlen(ldp.varname);
              ldp.shvb.shvnamelen = ldp.shvb.shvname.strlength;
              ldp.shvb.shvvalue.strptr = ldp.ibuf;
              ldp.shvb.shvvalue.strlength = ldp.vlen;
              ldp.shvb.shvvaluelen = ldp.vlen;
              ldp.shvb.shvcode = RXSHV_SET;
              ldp.shvb.shvret = 0;
              if (RexxVariablePool(&ldp.shvb) == RXSHV_BADN)
                 return INVALID_ROUTINE;      /* error on non-zero          */
           }
        }
     }
  }
  result = sb_FreeIterator(plitID);
  sprintf((CHAR *)&myresult,"SUCCESS");

  sprintf(ldp.ibuf, "%d", ldp.count);
  ldp.varname[ldp.stemlen] = '0';
  ldp.varname[ldp.stemlen+1] = 0;
  ldp.shvb.shvnext = NULL;
  ldp.shvb.shvname.strptr = ldp.varname;
  ldp.shvb.shvname.strlength = ldp.stemlen+1;
  ldp.shvb.shvnamelen = ldp.stemlen+1;
  ldp.shvb.shvvalue.strptr = ldp.ibuf;
  ldp.shvb.shvvalue.strlength = strlen(ldp.ibuf);
  ldp.shvb.shvvaluelen = ldp.shvb.shvvalue.strlength;
  ldp.shvb.shvcode = RXSHV_SET;
  ldp.shvb.shvret = 0;
  if (RexxVariablePool(&ldp.shvb) == RXSHV_BADN)
    return INVALID_ROUTINE;            /* error on non-zero          */

 sprintf(retstr->strptr, "%s", myresult);   /* result is string      */
 retstr->strlength = strlen(retstr->strptr);
 return VALID_ROUTINE;

}


/*************************************************************************
* Function:  SBGetAllResources                                           *
*                                                                        *
* Syntax:    call SBGetAllResources                                      *
*                                                                        *
* Params:                                                                *
*                                                                        *
* Return:    NO_UTIL_ERROR                                               *
*                                                                        *
*************************************************************************/

ULONG SBGetAllResources(CHAR *name, ULONG numargs, RXSTRING args[],
                       CHAR *queuename, RXSTRING *retstr)
{
  int i;
  RXSTEMDATA ldp;
  ULONG len;
  ULONG  rc = 0;                           /* Ret code of func           */

  if (numargs != 1)
   return INVALID_ROUTINE;
                                       /* Initialize data area       */
  ldp.count = 0;
  strcpy(ldp.varname, args[0].strptr);
  ldp.stemlen = args[0].strlength;
  strupr(ldp.varname);                 /* uppercase the name         */

  if (ldp.varname[ldp.stemlen-1] != '.')
    ldp.varname[ldp.stemlen++] = '.';

  sprintf((CHAR *)&myresult,"FAILURE");
  result = sb_GetAllResources(pbID,&resitID);
  if (result == SB_SUCCESS) {
     result = sb_Count(resitID,&count);
     if (result == SB_SUCCESS) {
        for (i=1; i<= count; i++) {
           result = sb_Next(resitID,&resID);
           if (result == SB_SUCCESS) {
              result = sb_GetResourceName(resID,(CHAR *)&buf,MAXBUFFER);
              len = strlen(buf);
              memcpy(ldp.ibuf, buf, len);
              ldp.vlen = len;
              ldp.count++;
              sprintf(ldp.varname+ldp.stemlen, "%d", ldp.count);
              if (ldp.ibuf[ldp.vlen-1] == '\n')
                 ldp.vlen--;
              ldp.shvb.shvnext = NULL;
              ldp.shvb.shvname.strptr = ldp.varname;
              ldp.shvb.shvname.strlength = strlen(ldp.varname);
              ldp.shvb.shvnamelen = ldp.shvb.shvname.strlength;
              ldp.shvb.shvvalue.strptr = ldp.ibuf;
              ldp.shvb.shvvalue.strlength = ldp.vlen;
              ldp.shvb.shvvaluelen = ldp.vlen;
              ldp.shvb.shvcode = RXSHV_SET;
              ldp.shvb.shvret = 0;
              if (RexxVariablePool(&ldp.shvb) == RXSHV_BADN)
                 return INVALID_ROUTINE;      /* error on non-zero          */
           }
        }
     }
  }
  result = sb_FreeIterator(resitID);
  sprintf((CHAR *)&myresult,"SUCCESS");

  sprintf(ldp.ibuf, "%d", ldp.count);
  ldp.varname[ldp.stemlen] = '0';
  ldp.varname[ldp.stemlen+1] = 0;
  ldp.shvb.shvnext = NULL;
  ldp.shvb.shvname.strptr = ldp.varname;
  ldp.shvb.shvname.strlength = ldp.stemlen+1;
  ldp.shvb.shvnamelen = ldp.stemlen+1;
  ldp.shvb.shvvalue.strptr = ldp.ibuf;
  ldp.shvb.shvvalue.strlength = strlen(ldp.ibuf);
  ldp.shvb.shvvaluelen = ldp.shvb.shvvalue.strlength;
  ldp.shvb.shvcode = RXSHV_SET;
  ldp.shvb.shvret = 0;
  if (RexxVariablePool(&ldp.shvb) == RXSHV_BADN)
    return INVALID_ROUTINE;            /* error on non-zero          */

 sprintf(retstr->strptr, "%s", myresult);   /* result is string      */
 retstr->strlength = strlen(retstr->strptr);
 return VALID_ROUTINE;

}


/*************************************************************************
* Function:  SBRequestComponent                                          *
*                                                                        *
* Syntax:    call SBRequestComponent                                     *
*                                                                        *
* Params:    componentName, qty, priority                                *
*                                                                        *
* Return:    NO_UTIL_ERROR                                               *
*                                                                        *
*************************************************************************/

ULONG SBRequestComponent(CHAR *name, ULONG numargs, RXSTRING args[],
                       CHAR *queuename, RXSTRING *retstr)
{
  ULONG  rc = 0;                           /* Ret code of func           */
  CHAR componentName[256];
  LONG qty, priority;

  if (numargs != 3)
   return INVALID_ROUTINE;

  strcpy((CHAR *)&componentName,args[0].strptr);
  string2long(args[1].strptr,&qty);
  string2long(args[2].strptr,&priority);

  if (pbLoaded) {
     if (systemInitialized) {
        result = sb_RequestComponentByName(sysID,componentName,qty,priority,0L,0L);
        if (result == SB_SUCCESS) {
           sprintf((CHAR *)&myresult, "SUCCESS  SBRequestComponent: %s requested",componentName);
        }
        else {
           sprintf((CHAR *)&myresult,"FAILURE  SBRequestComponent: %s not requested",componentName);
        }
     }
     else sprintf((CHAR *)&myresult,"FAILURE  SBRequestComponent: System not initialized");
  }


  sprintf(retstr->strptr, "%s", myresult);   /* result is string      */
  retstr->strlength = strlen(retstr->strptr);
  return VALID_ROUTINE;

}

/*************************************************************************
* Function:  SBRequestResource                                           *
*                                                                        *
* Syntax:    call SBRequestResource                                      *
*                                                                        *
* Params:    resourceName, qty, priority                                 *
*                                                                        *
* Return:    NO_UTIL_ERROR                                               *
*                                                                        *
*************************************************************************/

ULONG SBRequestResource(CHAR *name, ULONG numargs, RXSTRING args[],
                       CHAR *queuename, RXSTRING *retstr)
{
  ULONG  rc = 0;                           /* Ret code of func           */
  CHAR componentClassName[256],resourceName[256];
  float qty;
  LONG priority;
  ResAllocType allocationStrategy;

  if (numargs != 5)
   return INVALID_ROUTINE;

  strcpy((CHAR *)&componentClassName,args[0].strptr);
  strcpy((CHAR *)&resourceName,args[1].strptr);
  string2long(args[3].strptr,&priority);

  if (pbLoaded) {
     if (systemInitialized) {
        result = sb_RequestResourceByName(sysID,componentClassName,resourceName,qty,allocationStrategy,priority,0L);
        if (result == SB_SUCCESS) {
           sprintf((CHAR *)&myresult, "SUCCESS  SBRequestResource: %s requested",resourceName);
        }
        else {
           sprintf((CHAR *)&myresult,"FAILURE  SBRequestResource: %s not requested",resourceName);
        }
     }
     else sprintf((CHAR *)&myresult,"FAILURE  SBRequestResource: System not initialized");
  }

  sprintf(retstr->strptr, "%s", myresult);   /* result is string      */
  retstr->strlength = strlen(retstr->strptr);
  return VALID_ROUTINE;

}


/*************************************************************************
* Function:  SBNewSystem                                                 *
*                                                                        *
* Syntax:    call SBNewSystem                                            *
*                                                                        *
* Return:                                                                *
*************************************************************************/

ULONG SBNewSystem(CHAR *name, ULONG numargs, RXSTRING args[],
                        CHAR *queuename, RXSTRING *retstr)
{
  ULONG  rc = 0;                       /* Return code of this func   */
  CHAR productLineName[256];

  if (numargs != 1)                    /* validate arg count         */
    return INVALID_ROUTINE;

  strcpy((CHAR *)&productLineName,args[0].strptr);
  sprintf((CHAR *)&myresult,"FAILURE");

  if (systemInitialized) {
     result = sb_FreeSystem(sysID);
  }

  if (pbLoaded) {
     result = sb_GetProductLineID(pbID,productLineName,&plID);
     if (result == SB_SUCCESS) {
        result = sb_NewSystem(plID,&sysID);
        sprintf((CHAR *)&myresult,"SUCCESS  SBNewSystem: New System Created");
        systemInitialized = TRUE;
     }
     else sprintf((CHAR *)&myresult,"FAILURE  SBNewSystem: Unrecognized product line");
  }
  else sprintf((CHAR *)&myresult,"FAILURE  SBNewSystem: No product base loaded");


  sprintf(retstr->strptr, "%s", myresult);   /* result is string      */
  retstr->strlength = strlen(retstr->strptr);
  return VALID_ROUTINE;
}


/*************************************************************************
* Function:  SBBundleSystem                                              *
*                                                                        *
* Syntax:    call SBBundleSystem                                         *
*                                                                        *
* Return:                                                                *
*************************************************************************/

ULONG SBBundleSystem(CHAR *name, ULONG numargs, RXSTRING args[],
                        CHAR *queuename, RXSTRING *retstr)
{
  ULONG  rc = 0;                       /* Return code of this func   */
  CHAR productLineName[256];

  if (numargs != 0)                    /* validate arg count         */
    return INVALID_ROUTINE;

  sprintf((CHAR *)&myresult,"FAILURE  SBBundleSystem: General Failure");

  if (pbLoaded) {
     if (pcmLoaded) {
        if (systemInitialized) {
           result = sbpcm_BundleSystem(sysID,pcmID,0,&pcmiID);
           if (result == SB_SUCCESS) {
              sprintf((CHAR *)&myresult,"SUCCESS  SBBundleSystem");
              systemBundled = TRUE;
           }
           else sprintf((CHAR *)&myresult,"FAILURE  SBBundleSystem: Call to sbpcm_BundleSystem failed.");
        }
        else sprintf((CHAR *)&myresult,"FAILURE  SBBundleSystem: No system configured.");
     }
     else sprintf((CHAR *)&myresult,"FAILURE  SBBundleSystem: No product component mapping loaded.");
  }
  else sprintf((CHAR *)&myresult,"FAILURE  SBBundleSystem: No product base loaded.");

  sprintf(retstr->strptr, "%s", myresult);   /* result is string      */
  retstr->strlength = strlen(retstr->strptr);
  return VALID_ROUTINE;
}


/*************************************************************************
* Function:  SBLoadCompiledPCM                                           *
*                                                                        *
* Syntax:    call SBLoadCompiledPCM                                      *
*                                                                        *
*                                                                        *
* Return:    NO_UTIL_ERROR                                               *
*                                                                        *
*************************************************************************/

ULONG SBLoadCompiledPCM(CHAR *name, ULONG numargs, RXSTRING args[],
                       CHAR *queuename, RXSTRING *retstr)
{
 ULONG  rc;                           /* Ret code of func           */
 sprintf((CHAR *)&myresult,"");
 if (numargs != 0)
                                       /* If no args, then its an    */
                                       /* incorrect call             */
 return INVALID_ROUTINE;

 if (pbLoaded) {
    if (pcmLoaded) {
          result = sbpcm_FreeProdCompMap(pcmID);
          pcmLoaded = FALSE;
    }

    result = sbpcm_NewProdCompMap(pbID,1,&pcmID);

    if (result == SB_SUCCESS) {
       result = sbpcm_LoadProdCompMap(pbID,pcmPrefix,&pcmID);
       if (result == SB_SUCCESS) {
          sprintf((CHAR *)&myresult, "SUCCESS  SBLoadCompiledPCM: %s loaded",pcmPrefix);
          pcmLoaded = TRUE;
       }
       else {
          result = sbpcm_FreeProdCompMap(pcmID);
          sprintf((CHAR *)&myresult,"FAILURE  SBLoadCompiledPCM: %s",pcmPrefix);
       }
    }
    else sprintf((CHAR *)&myresult,"FAILURE  SBLoadCompiledPCM: Unable to allocate new product component mapping");
 }
 else sprintf((CHAR *)&myresult,"FAILURE  SBLoadCompiledPCM: No product base loaded");


 sprintf(retstr->strptr, "%s", myresult);   /* result is return code      */
 retstr->strlength = strlen(retstr->strptr);
 return VALID_ROUTINE;
}


/*************************************************************************
* Function:  SBLoadParsePCM                                              *
*                                                                        *
* Syntax:    call SBLoadParsePCM                                         *
*                                                                        *
*                                                                        *
* Return:    NO_UTIL_ERROR                                               *
*                                                                        *
*************************************************************************/

ULONG SBLoadParsePCM(CHAR *name, ULONG numargs, RXSTRING args[],
                       CHAR *queuename, RXSTRING *retstr)
{
 ULONG  rc;                           /* Ret code of func           */
 CHAR pcmParseFile[80];
 if (numargs != 1)
                                       /* If no args, then its an    */
                                       /* incorrect call             */
 return INVALID_ROUTINE;

 strcpy(pcmParseFile,args[0].strptr);

 if (pbLoaded) {
    if (pcmLoaded) {
          result = sbpcm_FreeProdCompMap(pcmID);
          pcmLoaded = FALSE;
    }

    result = sbpcm_NewProdCompMap(pbID,1,&pcmID);

    if (result == SB_SUCCESS) {
       result = sbpcm_ParseFile(pcmParseFile,pcmID,1);
       if (result == SB_SUCCESS) {
          sprintf((CHAR *)&myresult, "SUCCESS  SBLoadParsePCM: %s loaded",pcmParseFile);
          pcmLoaded = TRUE;
       }
       else {
          result = sbpcm_FreeProdCompMap(pcmID);
          sprintf((CHAR *)&myresult,"FAILURE  SBLoadParsePCM: %s not loaded",pcmParseFile);
       }
    }
    else sprintf((CHAR *)&myresult,"FAILURE  SBLoadParsePCM: Unable to allocate new product component mapping");
 }
 else sprintf((CHAR *)&myresult,"FAILURE  SBLoadParsePCM: No product base loaded");


 sprintf(retstr->strptr, "%s", myresult);   /* result is return code      */
 retstr->strlength = strlen(retstr->strptr);
 return VALID_ROUTINE;
}


/*************************************************************************
* Function:  SBGetAllBOMData                                             *
*                                                                        *
* Syntax:    call SBGetAllBOMData                                        *
*                                                                        *
* Params:                                                                *
*                                                                        *
* Return:    NO_UTIL_ERROR                                               *
*                                                                        *
*************************************************************************/

typedef struct {
   float quantity;
   CHAR partNum[256];
   CHAR description[256];
} BOMDataType;

ULONG SBGetAllBOMData(CHAR *name, ULONG numargs, RXSTRING args[],
                       CHAR *queuename, RXSTRING *retstr)
{
  RXSTEMDATA ldp;
  ULONG  rc = 0;                           /* Ret code of func           */
  ULONG len;
  float occs;
  InstanceID inst;
  InstanceIteratorID instIt;
  AttributeID attrID;
  AttributeValueID attrValueID;
  INT attrType = 0;
  BOMDataType BOMData;
  CHAR outputline[512];

  if (numargs != 1)
   return INVALID_ROUTINE;
                                       /* Initialize data area       */
  ldp.count = 0;
  strcpy(ldp.varname, args[0].strptr);
  ldp.stemlen = args[0].strlength;
  strupr(ldp.varname);                 /* uppercase the name         */

  if (ldp.varname[ldp.stemlen-1] != '.')
    ldp.varname[ldp.stemlen++] = '.';

  sprintf((CHAR *)&myresult,"FAILURE  SBGetAllBOMData: General Failure");
  if (pbLoaded) {
     if (systemInitialized) {
        result = sb_GetInstancesWithPartNumbers(sysID,&instIt);
        if (result == SB_SUCCESS) {
           result = sb_GetAttributeID(pbID,"sb_Quantity",&attrID);
           if (result == SB_SUCCESS) result = sb_GetAttributeType(attrID, &attrType);
              else attrID = 0;
           while (sb_Next(instIt, &inst)) {
              sprintf((CHAR *)&myresult, "SUCCESS  SBGetAllBOMData");
              result = sb_GetInstancePartNumber(inst,BOMData.partNum,256);
              result = sb_GetInstanceDescriptionOrName(inst,BOMData.description,256);
              if (attrID) {
                 result = sb_GetInstanceAttrValue(inst,attrID,&attrValueID);
                 if (result == SB_SUCCESS) {
                    sb_GetAttrValueFloat(attrValueID, &occs);
                    sb_FreeAttrValue(attrValueID);
                 }
              }
              else occs = 1.0;
              BOMData.quantity = occs;
              sprintf(outputline,"%f,\"%s\",\"%s\"\n",BOMData.quantity,BOMData.partNum,BOMData.description);
              len = strlen(outputline);
              memcpy(ldp.ibuf, outputline, len);
              ldp.vlen = len;
              ldp.count++;
              sprintf(ldp.varname+ldp.stemlen, "%d", ldp.count);
              if (ldp.ibuf[ldp.vlen-1] == '\n')
                 ldp.vlen--;
              ldp.shvb.shvnext = NULL;
              ldp.shvb.shvname.strptr = ldp.varname;
              ldp.shvb.shvname.strlength = strlen(ldp.varname);
              ldp.shvb.shvnamelen = ldp.shvb.shvname.strlength;
              ldp.shvb.shvvalue.strptr = ldp.ibuf;
              ldp.shvb.shvvalue.strlength = ldp.vlen;
              ldp.shvb.shvvaluelen = ldp.vlen;
              ldp.shvb.shvcode = RXSHV_SET;
              ldp.shvb.shvret = 0;
              if (RexxVariablePool(&ldp.shvb) == RXSHV_BADN)
                 return INVALID_ROUTINE;      /* error on non-zero          */
           }
        }
        else sprintf((CHAR *)&myresult,"FAILURE  SBGetAllBOMData: sb_GetInstancesWithPartNumbers");
     }
     else sprintf((CHAR *)&myresult,"FAILURE  SBGetAllBOMData: System not initialized");
  }
  else sprintf((CHAR *)&myresult,"FAILURE  SBGetAllBOMData: No product base loaded");

  sprintf(ldp.ibuf, "%d", ldp.count);
  ldp.varname[ldp.stemlen] = '0';
  ldp.varname[ldp.stemlen+1] = 0;
  ldp.shvb.shvnext = NULL;
  ldp.shvb.shvname.strptr = ldp.varname;
  ldp.shvb.shvname.strlength = ldp.stemlen+1;
  ldp.shvb.shvnamelen = ldp.stemlen+1;
  ldp.shvb.shvvalue.strptr = ldp.ibuf;
  ldp.shvb.shvvalue.strlength = strlen(ldp.ibuf);
  ldp.shvb.shvvaluelen = ldp.shvb.shvvalue.strlength;
  ldp.shvb.shvcode = RXSHV_SET;
  ldp.shvb.shvret = 0;
  if (RexxVariablePool(&ldp.shvb) == RXSHV_BADN)
    return INVALID_ROUTINE;            /* error on non-zero          */

 sprintf(retstr->strptr, "%s", myresult);   /* result is string      */
 retstr->strlength = strlen(retstr->strptr);
 return VALID_ROUTINE;

}


/*************************************************************************
* Function:  SBGetAllProducts                                            *
*                                                                        *
* Syntax:    call SBGetAllProducts                                       *
*                                                                        *
* Params:                                                                *
*                                                                        *
* Return:    NO_UTIL_ERROR                                               *
*                                                                        *
*************************************************************************/

ULONG SBGetAllProducts(CHAR *name, ULONG numargs, RXSTRING args[],
                       CHAR *queuename, RXSTRING *retstr)
{
  int i;
  RXSTEMDATA ldp;
  ULONG len;
  ULONG  rc = 0;                           /* Ret code of func           */

  if (numargs != 1)
   return INVALID_ROUTINE;
                                       /* Initialize data area       */
  ldp.count = 0;
  strcpy(ldp.varname, args[0].strptr);
  ldp.stemlen = args[0].strlength;
  strupr(ldp.varname);                 /* uppercase the name         */

  if (ldp.varname[ldp.stemlen-1] != '.')
    ldp.varname[ldp.stemlen++] = '.';

  sprintf((CHAR *)&myresult,"FAILURE");
  if (pcmLoaded) {
     result = sbpcm_GetAllProducts(pcmID,&proditID,1);
     if (result == SB_SUCCESS) {
        result = sbpcm_Count(proditID,&count);
        if (result == SB_SUCCESS) {
           for (i=1; i<= count; i++) {
              result = sbpcm_Next(proditID,&prodID);
              if (result == SB_SUCCESS) {
                 result = sbpcm_GetProductProductNumber(prodID,(CHAR *)&buf,MAXBUFFER);
                 len = strlen(buf);
                 memcpy(ldp.ibuf, buf, len);
                 ldp.vlen = len;
                 ldp.count++;
                 sprintf(ldp.varname+ldp.stemlen, "%d", ldp.count);
                 if (ldp.ibuf[ldp.vlen-1] == '\n')
                    ldp.vlen--;
                 ldp.shvb.shvnext = NULL;
                 ldp.shvb.shvname.strptr = ldp.varname;
                 ldp.shvb.shvname.strlength = strlen(ldp.varname);
                 ldp.shvb.shvnamelen = ldp.shvb.shvname.strlength;
                 ldp.shvb.shvvalue.strptr = ldp.ibuf;
                 ldp.shvb.shvvalue.strlength = ldp.vlen;
                 ldp.shvb.shvvaluelen = ldp.vlen;
                 ldp.shvb.shvcode = RXSHV_SET;
                 ldp.shvb.shvret = 0;
                 if (RexxVariablePool(&ldp.shvb) == RXSHV_BADN)
                    return INVALID_ROUTINE;      /* error on non-zero          */
              }
           }
           sprintf((CHAR *)&myresult,"SUCCESS  SBGetAllProducts");
        }
     }
     else sprintf((CHAR *)&myresult,"FAILURE  SBGetAllProducts: unable to obtain product iterator");
  }
  else sprintf((CHAR *)&myresult,"FAILURE  SBGetAllProducts: pcm not loaded");

  result = sb_FreeIterator(proditID);


  sprintf(ldp.ibuf, "%d", ldp.count);
  ldp.varname[ldp.stemlen] = '0';
  ldp.varname[ldp.stemlen+1] = 0;
  ldp.shvb.shvnext = NULL;
  ldp.shvb.shvname.strptr = ldp.varname;
  ldp.shvb.shvname.strlength = ldp.stemlen+1;
  ldp.shvb.shvnamelen = ldp.stemlen+1;
  ldp.shvb.shvvalue.strptr = ldp.ibuf;
  ldp.shvb.shvvalue.strlength = strlen(ldp.ibuf);
  ldp.shvb.shvvaluelen = ldp.shvb.shvvalue.strlength;
  ldp.shvb.shvcode = RXSHV_SET;
  ldp.shvb.shvret = 0;
  if (RexxVariablePool(&ldp.shvb) == RXSHV_BADN)
    return INVALID_ROUTINE;            /* error on non-zero          */

 sprintf(retstr->strptr, "%s", myresult);   /* result is string      */
 retstr->strlength = strlen(retstr->strptr);
 return VALID_ROUTINE;

}


/*************************************************************************
* Function:  SBGetAllProductsWithDesc                                    *
*                                                                        *
* Syntax:    call SBGetAllProductsWithDesc                               *
*                                                                        *
* Params:                                                                *
*                                                                        *
* Return:    NO_UTIL_ERROR                                               *
*                                                                        *
*************************************************************************/

typedef struct {
   CHAR partNum[256];
   CHAR description[256];
} ProductDataType;


ULONG SBGetAllProductsWithDesc(CHAR *name, ULONG numargs, RXSTRING args[],
                       CHAR *queuename, RXSTRING *retstr)
{
  int i;
  RXSTEMDATA ldp;
  ProductDataType ProductData;
  CHAR outputline[512];
  ULONG len;
  ULONG  rc = 0;                           /* Ret code of func           */

  if (numargs != 1)
   return INVALID_ROUTINE;
                                       /* Initialize data area       */
  ldp.count = 0;
  strcpy(ldp.varname, args[0].strptr);
  ldp.stemlen = args[0].strlength;
  strupr(ldp.varname);                 /* uppercase the name         */

  if (ldp.varname[ldp.stemlen-1] != '.')
    ldp.varname[ldp.stemlen++] = '.';

  sprintf((CHAR *)&myresult,"FAILURE");
  if (pcmLoaded) {
     result = sbpcm_GetAllProducts(pcmID,&proditID,1);
     if (result == SB_SUCCESS) {
        result = sbpcm_Count(proditID,&count);
        if (result == SB_SUCCESS) {
           for (i=1; i<= count; i++) {
              result = sbpcm_Next(proditID,&prodID);
              if (result == SB_SUCCESS) {
                 sprintf((CHAR *)&myresult, "SUCCESS  SBGetAllProductsWithDesc");
                 result = sbpcm_GetProductProductNumber(prodID,ProductData.partNum,256);
                 result = sbpcm_GetProductDescription(prodID,ProductData.description,256);
                 sprintf(outputline,"\"%s\",\"%s\"\n",ProductData.partNum,ProductData.description);
                 len = strlen(outputline);
                 memcpy(ldp.ibuf, outputline, len);
                 ldp.vlen = len;
                 ldp.count++;
                 sprintf(ldp.varname+ldp.stemlen, "%d", ldp.count);
                 if (ldp.ibuf[ldp.vlen-1] == '\n')
                    ldp.vlen--;
                 ldp.shvb.shvnext = NULL;
                 ldp.shvb.shvname.strptr = ldp.varname;
                 ldp.shvb.shvname.strlength = strlen(ldp.varname);
                 ldp.shvb.shvnamelen = ldp.shvb.shvname.strlength;
                 ldp.shvb.shvvalue.strptr = ldp.ibuf;
                 ldp.shvb.shvvalue.strlength = ldp.vlen;
                 ldp.shvb.shvvaluelen = ldp.vlen;
                 ldp.shvb.shvcode = RXSHV_SET;
                 ldp.shvb.shvret = 0;
                 if (RexxVariablePool(&ldp.shvb) == RXSHV_BADN)
                    return INVALID_ROUTINE;      /* error on non-zero          */
              }
           }
           sprintf((CHAR *)&myresult,"SUCCESS  SBGetAllProductsWithDesc");
        }
     }
     else sprintf((CHAR *)&myresult,"FAILURE  SBGetAllProductsWithDesc: unable to obtain product iterator");
  }
  else sprintf((CHAR *)&myresult,"FAILURE  SBGetAllProductsWithDesc: pcm not loaded");

  result = sb_FreeIterator(proditID);


  sprintf(ldp.ibuf, "%d", ldp.count);
  ldp.varname[ldp.stemlen] = '0';
  ldp.varname[ldp.stemlen+1] = 0;
  ldp.shvb.shvnext = NULL;
  ldp.shvb.shvname.strptr = ldp.varname;
  ldp.shvb.shvname.strlength = ldp.stemlen+1;
  ldp.shvb.shvnamelen = ldp.stemlen+1;
  ldp.shvb.shvvalue.strptr = ldp.ibuf;
  ldp.shvb.shvvalue.strlength = strlen(ldp.ibuf);
  ldp.shvb.shvvaluelen = ldp.shvb.shvvalue.strlength;
  ldp.shvb.shvcode = RXSHV_SET;
  ldp.shvb.shvret = 0;
  if (RexxVariablePool(&ldp.shvb) == RXSHV_BADN)
    return INVALID_ROUTINE;            /* error on non-zero          */

 sprintf(retstr->strptr, "%s", myresult);   /* result is string      */
 retstr->strlength = strlen(retstr->strptr);
 return VALID_ROUTINE;

}


/*************************************************************************
* Function:  SBGetProductProductLines                                    *
*                                                                        *
* Syntax:    call SBGetProductProductLines                               *
*                                                                        *
* Params:                                                                *
*                                                                        *
* Return:    NO_UTIL_ERROR                                               *
*                                                                        *
*************************************************************************/

ULONG SBGetProductProductLines(CHAR *name, ULONG numargs, RXSTRING args[],
                               CHAR *queuename, RXSTRING *retstr)
{
  int i;
  RXSTEMDATA ldp;
  ULONG len;
  ULONG  rc = 0;                           /* Ret code of func           */
  PartNumberID pnID;
  ProductLineID defplID;
  CHAR ProductPartNumber[256];

  if (numargs != 2)
   return INVALID_ROUTINE;
                                       /* Initialize data area       */
  ldp.count = 0;
  strcpy(ldp.varname, args[0].strptr);
  ldp.stemlen = args[0].strlength;
  strupr(ldp.varname);                 /* uppercase the name         */

  if (ldp.varname[ldp.stemlen-1] != '.')
    ldp.varname[ldp.stemlen++] = '.';

  strcpy((CHAR *)&ProductPartNumber,args[1].strptr);

  sprintf((CHAR *)&myresult,"FAILURE");
  if (pcmLoaded) {
     printf("SBGetProductProductLines -- ProductPartNumber: %s\n",ProductPartNumber);
     result = sbpcm_GetPartNumber(pcmID,ProductPartNumber,&pnID);
     if (result == SB_SUCCESS) {
       printf("SBGetProductProductLines -- Part Number ID: %d\n",pnID);
       result = sb_GetProductLineID(pbID,"The-Default-ProductLine",&defplID);
       if (result == SB_SUCCESS) {
         printf("SBGetProductProductLines -- Default Product Line ID: %d\n",defplID);
         result = sbpcm_GetProductID(pcmID,pnID,defplID,&prodID);
         if (result == SB_SUCCESS) {
           printf("SBGetProductProductLines -- Product ID: %d\n",prodID);
           result = sbpcm_GetProductProductLines(prodID,&plitID);
           if (result == SB_SUCCESS) {
              printf("SBGetProductProductLines -- Product Line Iterator ID: %d\n",plitID);
              result = sb_Count(plitID,&count);
              if (result == SB_SUCCESS) {
                 printf("SBGetProductProductLines -- count: %d\n",count);
                 for (i=1; i<= count; i++) {
                    result = sb_Next(plitID,&plID);
                    if (result == SB_SUCCESS) {
                       result = sb_GetProductLineName(plID,(CHAR *)&buf,MAXBUFFER);
                       len = strlen(buf);
                       memcpy(ldp.ibuf, buf, len);
                       ldp.vlen = len;
                       ldp.count++;
                       sprintf(ldp.varname+ldp.stemlen, "%d", ldp.count);
                       if (ldp.ibuf[ldp.vlen-1] == '\n')
                          ldp.vlen--;
                       ldp.shvb.shvnext = NULL;
                       ldp.shvb.shvname.strptr = ldp.varname;
                       ldp.shvb.shvname.strlength = strlen(ldp.varname);
                       ldp.shvb.shvnamelen = ldp.shvb.shvname.strlength;
                       ldp.shvb.shvvalue.strptr = ldp.ibuf;
                       ldp.shvb.shvvalue.strlength = ldp.vlen;
                       ldp.shvb.shvvaluelen = ldp.vlen;
                       ldp.shvb.shvcode = RXSHV_SET;
                       ldp.shvb.shvret = 0;
                       if (RexxVariablePool(&ldp.shvb) == RXSHV_BADN)
                          return INVALID_ROUTINE;      /* error on non-zero          */
                    }
                 }
                 sprintf((CHAR *)&myresult,"SUCCESS  SBGetProductProductLines");
              }
              else sprintf((CHAR *)&myresult,"FAILURE  SBGetProductProductLines: SBCount Failed");
           }
           else sprintf((CHAR *)&myresult,"FAILURE  SBGetProductProductLines: Unable to obtain product product lines iterator");
        }
        else sprintf((CHAR *)&myresult,"FAILURE  SBGetProductProductLines: Unable to obtain product ID");
      }
      else sprintf((CHAR *)&myresult,"FAILURE  SBGetProductProductLines: Unable to obtain product line ID of The-Default-Product-Line");
    }
    else sprintf((CHAR *)&myresult,"FAILURE  SBGetProductProductLines: Unable to obtain part number ID");
  }
  else sprintf((CHAR *)&myresult,"FAILURE  SBGetProductProductLines: PCM is not loaded");

  result = sb_FreeIterator(plitID);


  sprintf(ldp.ibuf, "%d", ldp.count);
  ldp.varname[ldp.stemlen] = '0';
  ldp.varname[ldp.stemlen+1] = 0;
  ldp.shvb.shvnext = NULL;
  ldp.shvb.shvname.strptr = ldp.varname;
  ldp.shvb.shvname.strlength = ldp.stemlen+1;
  ldp.shvb.shvnamelen = ldp.stemlen+1;
  ldp.shvb.shvvalue.strptr = ldp.ibuf;
  ldp.shvb.shvvalue.strlength = strlen(ldp.ibuf);
  ldp.shvb.shvvaluelen = ldp.shvb.shvvalue.strlength;
  ldp.shvb.shvcode = RXSHV_SET;
  ldp.shvb.shvret = 0;
  if (RexxVariablePool(&ldp.shvb) == RXSHV_BADN)
    return INVALID_ROUTINE;            /* error on non-zero          */

 sprintf(retstr->strptr, "%s", myresult);   /* result is string      */
 retstr->strlength = strlen(retstr->strptr);
 return VALID_ROUTINE;

}

/*************************************************************************
* Function:  SBGetProductLineProducts                                    *
*                                                                        *
* Syntax:    call SBGetProductLineProducts                               *
*                                                                        *
* Params:                                                                *
*                                                                        *
* Return:    NO_UTIL_ERROR                                               *
*                                                                        *
*************************************************************************/

ULONG SBGetProductLineProducts(CHAR *name, ULONG numargs, RXSTRING args[],
                               CHAR *queuename, RXSTRING *retstr)
{
  int i;
  RXSTEMDATA ldp;
  ULONG len;
  ULONG  rc = 0;                           /* Ret code of func           */
  ProductLineID localplID;
  CHAR ProductLineName[256];

  if (numargs != 2)
   return INVALID_ROUTINE;
                                       /* Initialize data area       */
  ldp.count = 0;
  strcpy(ldp.varname, args[0].strptr);
  ldp.stemlen = args[0].strlength;
  strupr(ldp.varname);                 /* uppercase the name         */

  if (ldp.varname[ldp.stemlen-1] != '.')
    ldp.varname[ldp.stemlen++] = '.';

  strcpy((CHAR *)&ProductLineName,args[1].strptr);

  sprintf((CHAR *)&myresult,"FAILURE");
  if (pcmLoaded) {
     printf("SBGetProductLineProducts -- ProductLineName: %s\n",ProductLineName);
     result = sb_GetProductLineID(pbID,ProductLineName,&localplID);
     if (result == SB_SUCCESS) {
       printf("SBGetProductLineProducts -- Product Line ID: %d\n",localplID);
       result = sbpcm_GetProductLineProducts(pcmID,localplID,&proditID);
       if (result == SB_SUCCESS) {
          printf("SBGetProductLineProducts -- Product Iterator ID: %d\n",proditID);
          result = sb_Count(proditID,&count);
          if (result == SB_SUCCESS) {
             printf("SBGetProductLineProducts -- count: %d\n",count);
             for (i=1; i<= count; i++) {
                result = sb_Next(proditID,&prodID);
                if (result == SB_SUCCESS) {
                   result = sbpcm_GetProductProductNumber(prodID,(CHAR *)&buf,MAXBUFFER);
                   len = strlen(buf);
                   memcpy(ldp.ibuf, buf, len);
                   ldp.vlen = len;
                   ldp.count++;
                   sprintf(ldp.varname+ldp.stemlen, "%d", ldp.count);
                   if (ldp.ibuf[ldp.vlen-1] == '\n')
                      ldp.vlen--;
                   ldp.shvb.shvnext = NULL;
                   ldp.shvb.shvname.strptr = ldp.varname;
                   ldp.shvb.shvname.strlength = strlen(ldp.varname);
                   ldp.shvb.shvnamelen = ldp.shvb.shvname.strlength;
                   ldp.shvb.shvvalue.strptr = ldp.ibuf;
                   ldp.shvb.shvvalue.strlength = ldp.vlen;
                   ldp.shvb.shvvaluelen = ldp.vlen;
                   ldp.shvb.shvcode = RXSHV_SET;
                   ldp.shvb.shvret = 0;
                   if (RexxVariablePool(&ldp.shvb) == RXSHV_BADN)
                      return INVALID_ROUTINE;      /* error on non-zero          */
                }
             }
             sprintf((CHAR *)&myresult,"SUCCESS  SBGetProductLineProducts");
          }
          else sprintf((CHAR *)&myresult,"FAILURE  SBGetProductLineProducts: SBCount Failed");
      }
      else sprintf((CHAR *)&myresult,"FAILURE  SBGetProductLineProducts: Unable to obtain product iterator");
    }
    else sprintf((CHAR *)&myresult,"FAILURE  SBGetProductLineProducts: Unable to obtain product line ID");
  }
  else sprintf((CHAR *)&myresult,"FAILURE  SBGetProductLineProducts: PCM is not loaded");

  result = sb_FreeIterator(proditID);


  sprintf(ldp.ibuf, "%d", ldp.count);
  ldp.varname[ldp.stemlen] = '0';
  ldp.varname[ldp.stemlen+1] = 0;
  ldp.shvb.shvnext = NULL;
  ldp.shvb.shvname.strptr = ldp.varname;
  ldp.shvb.shvname.strlength = ldp.stemlen+1;
  ldp.shvb.shvnamelen = ldp.stemlen+1;
  ldp.shvb.shvvalue.strptr = ldp.ibuf;
  ldp.shvb.shvvalue.strlength = strlen(ldp.ibuf);
  ldp.shvb.shvvaluelen = ldp.shvb.shvvalue.strlength;
  ldp.shvb.shvcode = RXSHV_SET;
  ldp.shvb.shvret = 0;
  if (RexxVariablePool(&ldp.shvb) == RXSHV_BADN)
    return INVALID_ROUTINE;            /* error on non-zero          */

 sprintf(retstr->strptr, "%s", myresult);   /* result is string      */
 retstr->strlength = strlen(retstr->strptr);
 return VALID_ROUTINE;

}


/*************************************************************************
* Function:  SBBundledResults                                            *
*                                                                        *
* Syntax:    call SBBundledResults                                       *
*                                                                        *
* Params:                                                                *
*                                                                        *
* Return:    NO_UTIL_ERROR                                               *
*                                                                        *
*************************************************************************/

typedef struct {
   long quantity;
   CHAR partNum[256];
   CHAR description[256];
} BundledResultsDataType;

ULONG SBBundledResults(CHAR *name, ULONG numargs, RXSTRING args[],
                       CHAR *queuename, RXSTRING *retstr)
{
  RXSTEMDATA ldp;
  ULONG  rc = 0;                           /* Ret code of func           */
  ULONG len;
  ProductInstanceID prodinst;
  ProductInstanceIteratorID prodinstIt;
  BundledResultsDataType BundledData;
  CHAR outputline[512];

  if (numargs != 1)
   return INVALID_ROUTINE;
                                       /* Initialize data area       */
  ldp.count = 0;
  strcpy(ldp.varname, args[0].strptr);
  ldp.stemlen = args[0].strlength;
  strupr(ldp.varname);                 /* uppercase the name         */

  if (ldp.varname[ldp.stemlen-1] != '.')
    ldp.varname[ldp.stemlen++] = '.';

  sprintf((CHAR *)&myresult,"FAILURE  SBBundledResults: General Failure");
  if (pbLoaded) {
     if (systemInitialized) {
        if (systemBundled) {
           result = sbpcm_GetUniqueProdInstances(pcmiID,&prodinstIt);
           if (result == SB_SUCCESS) {
              while (sb_Next(prodinstIt, &prodinst)) {
                 sprintf((CHAR *)&myresult, "SUCCESS  SBBundledResults");
                 result = sbpcm_GetProdInstanceOccurrences(prodinst,&(BundledData.quantity));
                 result = sbpcm_GetProdInstanceModelNumber(prodinst,BundledData.partNum,256);
                 result = sbpcm_GetProdInstanceDescription(prodinst,BundledData.description,256);
                 sprintf(outputline,"%d,\"%s\",\"%s\"\n",BundledData.quantity,BundledData.partNum,BundledData.description);
                 len = strlen(outputline);
                 memcpy(ldp.ibuf, outputline, len);
                 ldp.vlen = len;
                 ldp.count++;
                 sprintf(ldp.varname+ldp.stemlen, "%d", ldp.count);
                 if (ldp.ibuf[ldp.vlen-1] == '\n')
                    ldp.vlen--;
                 ldp.shvb.shvnext = NULL;
                 ldp.shvb.shvname.strptr = ldp.varname;
                 ldp.shvb.shvname.strlength = strlen(ldp.varname);
                 ldp.shvb.shvnamelen = ldp.shvb.shvname.strlength;
                 ldp.shvb.shvvalue.strptr = ldp.ibuf;
                 ldp.shvb.shvvalue.strlength = ldp.vlen;
                 ldp.shvb.shvvaluelen = ldp.vlen;
                 ldp.shvb.shvcode = RXSHV_SET;
                 ldp.shvb.shvret = 0;
                 if (RexxVariablePool(&ldp.shvb) == RXSHV_BADN)
                    return INVALID_ROUTINE;      /* error on non-zero          */
              }
           }
           else sprintf((CHAR *)&myresult,"FAILURE  SBBundledResults: sb_GetInstancesWithPartNumbers");
        }
        else sprintf((CHAR *)&myresult,"FAILURE  SBBundledResults: System not bundled");
     }
     else sprintf((CHAR *)&myresult,"FAILURE  SBBundledResults: System not initialized");
  }
  else sprintf((CHAR *)&myresult,"FAILURE  SBBundledResults: No product base loaded");

  sprintf(ldp.ibuf, "%d", ldp.count);
  ldp.varname[ldp.stemlen] = '0';
  ldp.varname[ldp.stemlen+1] = 0;
  ldp.shvb.shvnext = NULL;
  ldp.shvb.shvname.strptr = ldp.varname;
  ldp.shvb.shvname.strlength = ldp.stemlen+1;
  ldp.shvb.shvnamelen = ldp.stemlen+1;
  ldp.shvb.shvvalue.strptr = ldp.ibuf;
  ldp.shvb.shvvalue.strlength = strlen(ldp.ibuf);
  ldp.shvb.shvvaluelen = ldp.shvb.shvvalue.strlength;
  ldp.shvb.shvcode = RXSHV_SET;
  ldp.shvb.shvret = 0;
  if (RexxVariablePool(&ldp.shvb) == RXSHV_BADN)
    return INVALID_ROUTINE;            /* error on non-zero          */

 sprintf(retstr->strptr, "%s", myresult);   /* result is string      */
 retstr->strlength = strlen(retstr->strptr);
 return VALID_ROUTINE;

}


/*************************************************************************
* Function:  SBSQLQuery                                                  *
*                                                                        *
* Syntax:    call SBSQLQuery                                             *
*                                                                        *
* Params:                                                                *
*                                                                        *
* Return:    NO_UTIL_ERROR                                               *
*                                                                        *
*************************************************************************/


ULONG SBSQLQuery(CHAR *name, ULONG numargs, RXSTRING args[],
                       CHAR *queuename, RXSTRING *retstr)
{
  RXSTEMDATA ldp;
  ULONG  rc = 0;                           /* Ret code of func           */
  ULONG len;
  CHAR outputline[512];
  CHAR Query[512];
  ObjectID QueryContext;
  CHAR ResultBuf[MAXBUFFER];
  BlockIteratorID BlockItID;
  long ResultCount;
  IteratorID itID;
  ObjectID obj;


  if (numargs != 3)
   return INVALID_ROUTINE;
                                       /* Initialize data area       */
  ldp.count = 0;
  strcpy(ldp.varname, args[0].strptr);
  ldp.stemlen = args[0].strlength;
  strupr(ldp.varname);                 /* uppercase the name         */

  if (ldp.varname[ldp.stemlen-1] != '.')
    ldp.varname[ldp.stemlen++] = '.';

  sprintf((CHAR *)&myresult,"FAILURE  SBSQLQuery: Query Context can only be 'productbase' or 'system'.");
  if (strcmpi(args[1].strptr,"productbase")==0) {
        printf("pbID = %d\n",pbID);
        QueryContext = pbID;
        }
     else if (strcmpi(args[1].strptr,"system")==0) {
        printf("sysID = %d\n",sysID);
        QueryContext = sysID;
        }
     else return INVALID_ROUTINE;
  printf("args[1] = %s\n",args[1].strptr);
  printf("QueryContext = %d\n",QueryContext);

  sprintf((CHAR *)&myresult,"FAILURE  SBSQLQuery: Query too long.");
  strcpy((CHAR *)&Query,args[2].strptr);
  printf("args[2] = %s\n",args[2].strptr);
  printf("Query = %s\n",Query);

  sprintf((CHAR *)&myresult,"FAILURE  SBSQLQuery: General Failure");
  if (pbLoaded) {
     if ((QueryContext == pbID) || (systemInitialized)) {
        result = sb_SQLQuery(QueryContext,Query,(unsigned char *)&ResultBuf,MAXBUFFER,&BlockItID,&ResultCount,&itID);
        if (result == SB_SUCCESS) {
           sprintf((CHAR *)&myresult, "SUCCESS  SBSQLQuery");
           printf("ResultBuf = %s\n",ResultBuf);
           printf("itID = %d\n", itID);
/*         while (sb_Next(itIt, &obj)) {
                 result = sbpcm_GetProdInstanceOccurrences(prodinst,&(BundledData.quantity));
                 result = sbpcm_GetProdInstanceModelNumber(prodinst,BundledData.partNum,256);
                 result = sbpcm_GetProdInstanceDescription(prodinst,BundledData.description,256);
                 sprintf(outputline,"%d,\"%s\",\"%s\"\n",BundledData.quantity,BundledData.partNum,BundledData.description);
                 len = strlen(outputline);
                 memcpy(ldp.ibuf, outputline, len);
                 ldp.vlen = len;
                 ldp.count++;
                 sprintf(ldp.varname+ldp.stemlen, "%d", ldp.count);
                 if (ldp.ibuf[ldp.vlen-1] == '\n')
                    ldp.vlen--;
                 ldp.shvb.shvnext = NULL;
                 ldp.shvb.shvname.strptr = ldp.varname;
                 ldp.shvb.shvname.strlength = strlen(ldp.varname);
                 ldp.shvb.shvnamelen = ldp.shvb.shvname.strlength;
                 ldp.shvb.shvvalue.strptr = ldp.ibuf;
                 ldp.shvb.shvvalue.strlength = ldp.vlen;
                 ldp.shvb.shvvaluelen = ldp.vlen;
                 ldp.shvb.shvcode = RXSHV_SET;
                 ldp.shvb.shvret = 0;
                 if (RexxVariablePool(&ldp.shvb) == RXSHV_BADN)
                    return INVALID_ROUTINE;
              } */
        }
        else sprintf((CHAR *)&myresult,"FAILURE  SBSQLQuery: Query Failed");
     }
     else sprintf((CHAR *)&myresult,"FAILURE  SBSQLQuery: Unable to determine query context");
  }
  else sprintf((CHAR *)&myresult,"FAILURE  SBSQLQuery: No product base loaded");

  sprintf(ldp.ibuf, "%d", ldp.count);
  ldp.varname[ldp.stemlen] = '0';
  ldp.varname[ldp.stemlen+1] = 0;
  ldp.shvb.shvnext = NULL;
  ldp.shvb.shvname.strptr = ldp.varname;
  ldp.shvb.shvname.strlength = ldp.stemlen+1;
  ldp.shvb.shvnamelen = ldp.stemlen+1;
  ldp.shvb.shvvalue.strptr = ldp.ibuf;
  ldp.shvb.shvvalue.strlength = strlen(ldp.ibuf);
  ldp.shvb.shvvaluelen = ldp.shvb.shvvalue.strlength;
  ldp.shvb.shvcode = RXSHV_SET;
  ldp.shvb.shvret = 0;
  if (RexxVariablePool(&ldp.shvb) == RXSHV_BADN)
    return INVALID_ROUTINE;            /* error on non-zero          */

 sprintf(retstr->strptr, "%s", myresult);   /* result is string      */
 retstr->strlength = strlen(retstr->strptr);
 return VALID_ROUTINE;

}


/*************************************************************************
* Function:  SBGetAllComponentsAndDesc                                   *
*                                                                        *
* Syntax:    call SBGetAllComponentsAndDesc                              *
*                                                                        *
* Params:                                                                *
*                                                                        *
* Return:    NO_UTIL_ERROR                                               *
*                                                                        *
*************************************************************************/


typedef struct {
   CHAR Name[256];
   CHAR PartNum[256];
   CHAR Description[256];
   CHAR Label[256];
} ComponentDataType;


ULONG SBGetAllCompsAndDesc(CHAR *name, ULONG numargs, RXSTRING args[],
                          CHAR *queuename, RXSTRING *retstr)
{
  ComponentDataType ComponentData;
  ProductLineID defplID;
  ComponentIteratorID compitID;
  ComponentID compID;
  ComponentClassID classID;
  CHAR outputline[1024];
  int i;
  RXSTEMDATA ldp;
  ULONG len;
  ULONG  rc = 0;                           /* Ret code of func           */

  if (numargs != 1)
   return INVALID_ROUTINE;
                                       /* Initialize data area       */
  ldp.count = 0;
  strcpy(ldp.varname, args[0].strptr);
  ldp.stemlen = args[0].strlength;
  strupr(ldp.varname);                 /* uppercase the name         */

  if (ldp.varname[ldp.stemlen-1] != '.')
    ldp.varname[ldp.stemlen++] = '.';

  sprintf((CHAR *)&myresult,"FAILURE: GetAllComponentsAndDesc");
  if (pbLoaded) {
     result = sb_GetProductLineID(pbID,"The-Default-ProductLine",&defplID);
     if (result == SB_SUCCESS) {
       result = sb_GetClassID(pbID,"Component",&classID);
       if (result == SB_SUCCESS) {
         result = sb_GetClassComponents(defplID,classID,&compitID);
         if (result == SB_SUCCESS) {
            result = sb_Count(compitID,&count);
            if (result == SB_SUCCESS) {
               for (i=1; i<= count; i++) {
                  result = sb_Next(compitID,&compID);
                  if (result == SB_SUCCESS) {
                     result = sb_GetComponentName(compID,ComponentData.Name,256);
                     result = sb_GetComponentPartNumber(compID,ComponentData.PartNum,256);
                     result = sb_GetComponentDescription(compID,ComponentData.Description,256);
                     result = sb_GetComponentLabel(compID,ComponentData.Label,256);
                     sprintf(outputline,"\"%s\",\"%s\",\"%s\",\"%s\"\n",ComponentData.Name,ComponentData.PartNum,ComponentData.Description,ComponentData.Label);
                     len = strlen(outputline);
                     memcpy(ldp.ibuf, outputline, len);
                     ldp.vlen = len;
                     ldp.count++;
                     sprintf(ldp.varname+ldp.stemlen, "%d", ldp.count);
                     if (ldp.ibuf[ldp.vlen-1] == '\n')
                        ldp.vlen--;
                     ldp.shvb.shvnext = NULL;
                     ldp.shvb.shvname.strptr = ldp.varname;
                     ldp.shvb.shvname.strlength = strlen(ldp.varname);
                     ldp.shvb.shvnamelen = ldp.shvb.shvname.strlength;
                     ldp.shvb.shvvalue.strptr = ldp.ibuf;
                     ldp.shvb.shvvalue.strlength = ldp.vlen;
                     ldp.shvb.shvvaluelen = ldp.vlen;
                     ldp.shvb.shvcode = RXSHV_SET;
                     ldp.shvb.shvret = 0;
                     if (RexxVariablePool(&ldp.shvb) == RXSHV_BADN)
                        return INVALID_ROUTINE;      /* error on non-zero          */
                  }
               }
               sprintf((CHAR *)&myresult,"SUCCESS  SBGetAllComponentsAndDesc");
            }
         }
      }
    }
  }
  else sprintf((CHAR *)&myresult,"FAILURE  SBGetAllComponentsAndDesc");

  result = sb_FreeIterator(compitID);


  sprintf(ldp.ibuf, "%d", ldp.count);
  ldp.varname[ldp.stemlen] = '0';
  ldp.varname[ldp.stemlen+1] = 0;
  ldp.shvb.shvnext = NULL;
  ldp.shvb.shvname.strptr = ldp.varname;
  ldp.shvb.shvname.strlength = ldp.stemlen+1;
  ldp.shvb.shvnamelen = ldp.stemlen+1;
  ldp.shvb.shvvalue.strptr = ldp.ibuf;
  ldp.shvb.shvvalue.strlength = strlen(ldp.ibuf);
  ldp.shvb.shvvaluelen = ldp.shvb.shvvalue.strlength;
  ldp.shvb.shvcode = RXSHV_SET;
  ldp.shvb.shvret = 0;
  if (RexxVariablePool(&ldp.shvb) == RXSHV_BADN)
    return INVALID_ROUTINE;            /* error on non-zero          */

 sprintf(retstr->strptr, "%s", myresult);   /* result is string      */
 retstr->strlength = strlen(retstr->strptr);
 return VALID_ROUTINE;

}

/*************************************************************************
* Function:  SBBundledResultsExpanded                                    *
*                                                                        *
* Syntax:    call SBBundledResultsExpanded                               *
*                                                                        *
* Params:                                                                *
*                                                                        *
* Return:    NO_UTIL_ERROR                                               *
*                                                                        *
*************************************************************************/

void ExpandResults(ProductInstanceID prodinst, RXSTEMDATA* ldp, ULONG BOMLevel)
{
  ULONG len;
  ProductID prodID;

  ProductOptionIteratorID poIt;
  ProductOptionID poID;

  ProductInstanceID expandinst;
  ProductInstanceIteratorID expandinstIt;

  BundledResultsDataType BundledData;
  CHAR outputline[512];

/*  result = sbpcm_GetProdInstanceProductID(prodinst, &prodID);
    result = sbpcm_GetProductRequired(prodID, &poIt);
 */

  BOMLevel++;
  result = sbpcm_ExpandUniqueProdInstance(prodinst, &expandinstIt);
  if (result==SB_SUCCESS) {
     while(sb_Next(expandinstIt, &expandinst)) {
        result = sbpcm_GetProdInstanceOccurrences(expandinst,&(BundledData.quantity));
        result = sbpcm_GetProdInstanceModelNumber(expandinst,BundledData.partNum,256);
                 result = sbpcm_GetProdInstanceDescription(expandinst,BundledData.description,256);
        sprintf(outputline,"%d,%d,\"%s\",\"%s\"\n",BundledData.quantity,BOMLevel,BundledData.partNum,BundledData.description);
        printf("%s\n",outputline);
        len = strlen(outputline);
        memcpy(ldp->ibuf, outputline, len);
        ldp->vlen = len;
        ldp->count++;
        sprintf(ldp->varname+ldp->stemlen, "%d", ldp->count);
        if (ldp->ibuf[ldp->vlen-1] == '\n')
        ldp->vlen--;
        ldp->shvb.shvnext = NULL;
        ldp->shvb.shvname.strptr = ldp->varname;
        ldp->shvb.shvname.strlength = strlen(ldp->varname);
        ldp->shvb.shvnamelen = ldp->shvb.shvname.strlength;
        ldp->shvb.shvvalue.strptr = ldp->ibuf;
        ldp->shvb.shvvalue.strlength = ldp->vlen;
        ldp->shvb.shvvaluelen = ldp->vlen;
        ldp->shvb.shvcode = RXSHV_SET;
        ldp->shvb.shvret = 0;
        if (RexxVariablePool(&ldp->shvb) == RXSHV_BADN)
           return INVALID_ROUTINE;      /* error on non-zero          */
        ExpandResults(expandinst,ldp,BOMLevel);
     }
  }
}


ULONG SBBundledResultsExpanded(CHAR *name, ULONG numargs, RXSTRING args[],
                               CHAR *queuename, RXSTRING *retstr)
{
  RXSTEMDATA ldp;
  ULONG  rc = 0;                           /* Ret code of func           */
  ULONG len;
  ULONG BOMLevel = 0;
  ProductInstanceID prodinst, expandinst;
  ProductInstanceIteratorID prodinstIt, expandinstIt;
  BundledResultsDataType BundledData;
  CHAR outputline[512];

  if (numargs != 1)
   return INVALID_ROUTINE;
                                       /* Initialize data area       */
  ldp.count = 0;
  strcpy(ldp.varname, args[0].strptr);
  ldp.stemlen = args[0].strlength;
  strupr(ldp.varname);                 /* uppercase the name         */

  if (ldp.varname[ldp.stemlen-1] != '.')
    ldp.varname[ldp.stemlen++] = '.';

  sprintf((CHAR *)&myresult,"FAILURE  SBBundledResultsExpanded: General Failure");
  if (pbLoaded) {
     if (systemInitialized) {
        if (systemBundled) {
           result = sbpcm_GetUniqueProdInstances(pcmiID,&prodinstIt);
           if (result == SB_SUCCESS) {
              while (sb_Next(prodinstIt, &prodinst)) {
                 sprintf((CHAR *)&myresult, "SUCCESS  SBBundledResultsExpanded");
                 result = sbpcm_GetProdInstanceOccurrences(prodinst,&(BundledData.quantity));
                 result = sbpcm_GetProdInstanceModelNumber(prodinst,BundledData.partNum,256);
                 result = sbpcm_GetProdInstanceDescription(prodinst,BundledData.description,256);
                 sprintf(outputline,"%d,%d,\"%s\",\"%s\"\n",BundledData.quantity,BOMLevel,BundledData.partNum,BundledData.description);
                 len = strlen(outputline);
                 memcpy(ldp.ibuf, outputline, len);
                 ldp.vlen = len;
                 ldp.count++;
                 sprintf(ldp.varname+ldp.stemlen, "%d", ldp.count);
                 if (ldp.ibuf[ldp.vlen-1] == '\n')
                    ldp.vlen--;
                 ldp.shvb.shvnext = NULL;
                 ldp.shvb.shvname.strptr = ldp.varname;
                 ldp.shvb.shvname.strlength = strlen(ldp.varname);
                 ldp.shvb.shvnamelen = ldp.shvb.shvname.strlength;
                 ldp.shvb.shvvalue.strptr = ldp.ibuf;
                 ldp.shvb.shvvalue.strlength = ldp.vlen;
                 ldp.shvb.shvvaluelen = ldp.vlen;
                 ldp.shvb.shvcode = RXSHV_SET;
                 ldp.shvb.shvret = 0;
                 if (RexxVariablePool(&ldp.shvb) == RXSHV_BADN)
                    return INVALID_ROUTINE;      /* error on non-zero          */
                 ExpandResults(prodinst,&ldp,BOMLevel);
              }
           }
           else sprintf((CHAR *)&myresult,"FAILURE  SBBundledResults: sb_GetInstancesWithPartNumbers");
        }
        else sprintf((CHAR *)&myresult,"FAILURE  SBBundledResults: System not bundled");
     }
     else sprintf((CHAR *)&myresult,"FAILURE  SBBundledResults: System not initialized");
  }
  else sprintf((CHAR *)&myresult,"FAILURE  SBBundledResults: No product base loaded");

  sprintf(ldp.ibuf, "%d", ldp.count);
  ldp.varname[ldp.stemlen] = '0';
  ldp.varname[ldp.stemlen+1] = 0;
  ldp.shvb.shvnext = NULL;
  ldp.shvb.shvname.strptr = ldp.varname;
  ldp.shvb.shvname.strlength = ldp.stemlen+1;
  ldp.shvb.shvnamelen = ldp.stemlen+1;
  ldp.shvb.shvvalue.strptr = ldp.ibuf;
  ldp.shvb.shvvalue.strlength = strlen(ldp.ibuf);
  ldp.shvb.shvvaluelen = ldp.shvb.shvvalue.strlength;
  ldp.shvb.shvcode = RXSHV_SET;
  ldp.shvb.shvret = 0;
  if (RexxVariablePool(&ldp.shvb) == RXSHV_BADN)
    return INVALID_ROUTINE;            /* error on non-zero          */

 sprintf(retstr->strptr, "%s", myresult);   /* result is string      */
 retstr->strlength = strlen(retstr->strptr);
 return VALID_ROUTINE;

}


/*************************************************************************
* Function:  SBReconfigureSystem                                         *
*                                                                        *
* Syntax:    call SBReconfigureSystem                                    *
*                                                                        *
* Params:                                                                *
*                                                                        *
* Return:    NO_UTIL_ERROR                                               *
*                                                                        *
*************************************************************************/

ULONG SBReconfigureSystem(CHAR *name, ULONG numargs, RXSTRING args[],
                       CHAR *queuename, RXSTRING *retstr)
{
  ULONG  rc = 0;                           /* Ret code of func           */

  if (numargs != 0)
   return INVALID_ROUTINE;

  sprintf((CHAR *)&myresult,"FAILURE");
  if (systemInitialized) {
     result = sb_ReconfigureSystem(sysID);
     if (result == SB_SUCCESS) sprintf((CHAR *)&myresult,"SUCCESS  SBReconfigureSystem");
     else sprintf((CHAR *)&myresult,"FAILURE  SBReconfigureSystem:  Call to sb_ReconfigureSystem failed");
  }
  else sprintf((CHAR *)&myresult,"FAILURE  SBReconfigureSystem: No system initialized");

  sprintf(retstr->strptr, "%s", myresult);   /* result is string      */
  retstr->strlength = strlen(retstr->strptr);
  return VALID_ROUTINE;

}


/*************************************************************************
* Function:  SBGetImplicitRequests                                       *
*                                                                        *
* Syntax:    call SBGetImplicitRequests                                  *
*                                                                        *
* Params:                                                                *
*                                                                        *
* Return:    NO_UTIL_ERROR                                               *
*                                                                        *
*************************************************************************/

ULONG SBGetImplicitRequests(CHAR *name, ULONG numargs, RXSTRING args[],
                       CHAR *queuename, RXSTRING *retstr)
{
  RXSTEMDATA ldp;
  ULONG  rc = 0;                           /* Ret code of func           */
  ULONG len;

  BOMDataType BOMData;
  RequestID request;
  RequestIteratorID requestIt;

  InstanceID inst;
  InstanceIteratorID instIt;

  CHAR outputline[512];

  if (numargs != 1)
   return INVALID_ROUTINE;
                                       /* Initialize data area       */
  ldp.count = 0;
  strcpy(ldp.varname, args[0].strptr);
  ldp.stemlen = args[0].strlength;
  strupr(ldp.varname);                 /* uppercase the name         */

  if (ldp.varname[ldp.stemlen-1] != '.')
    ldp.varname[ldp.stemlen++] = '.';

  sprintf((CHAR *)&myresult,"FAILURE  SBGetImplicitRequests: General Failure");
  if (pbLoaded) {
     if (systemInitialized) {
        result = sb_GetAllRequests(sysID,&requestIt);
        if (result == SB_SUCCESS) {
           result = sb_Next(requestIt, &request);
           if (result == SB_SUCCESS) {
              result = sb_GetRequestImplicitInstances(request, &instIt);
              if (result == SB_SUCCESS) {
                sprintf((CHAR *)&myresult, "SUCCESS  SBGetImplicitRequests");
                while (sb_Next(instIt, &inst)) {
                  result = sb_GetInstancePartNumber(inst,BOMData.partNum,256);
                  result = sb_GetInstanceDescriptionOrName(inst,BOMData.description,256);
                  sprintf(outputline,"%d,\"%s\",\"%s\"\n",inst,BOMData.partNum,BOMData.description);
                  len = strlen(outputline);
                  memcpy(ldp.ibuf, outputline, len);
                  ldp.vlen = len;
                  ldp.count++;
                  sprintf(ldp.varname+ldp.stemlen, "%d", ldp.count);
                  if (ldp.ibuf[ldp.vlen-1] == '\n')
                     ldp.vlen--;
                  ldp.shvb.shvnext = NULL;
                  ldp.shvb.shvname.strptr = ldp.varname;
                  ldp.shvb.shvname.strlength = strlen(ldp.varname);
                  ldp.shvb.shvnamelen = ldp.shvb.shvname.strlength;
                  ldp.shvb.shvvalue.strptr = ldp.ibuf;
                  ldp.shvb.shvvalue.strlength = ldp.vlen;
                  ldp.shvb.shvvaluelen = ldp.vlen;
                  ldp.shvb.shvcode = RXSHV_SET;
                  ldp.shvb.shvret = 0;
                  if (RexxVariablePool(&ldp.shvb) == RXSHV_BADN)
                     return INVALID_ROUTINE;      /* error on non-zero          */
                }
              }
              else sprintf((CHAR *)&myresult,"FAILURE  SBGetImplicitRequests: sb_GetRequestImplicitInstances");
           }
           else sprintf((CHAR *)&myresult,"FAILURE  SBGetImplicitRequests: sb_Next (first request)");
        }
        else sprintf((CHAR *)&myresult,"FAILURE  SBGetImplicitRequests: sb_GetAllRequests");
     }
     else sprintf((CHAR *)&myresult,"FAILURE  SBGetImplicitRequests: System not initialized");
  }
  else sprintf((CHAR *)&myresult,"FAILURE  SBGetImplicitRequests: No product base loaded");

  sprintf(ldp.ibuf, "%d", ldp.count);
  ldp.varname[ldp.stemlen] = '0';
  ldp.varname[ldp.stemlen+1] = 0;
  ldp.shvb.shvnext = NULL;
  ldp.shvb.shvname.strptr = ldp.varname;
  ldp.shvb.shvname.strlength = ldp.stemlen+1;
  ldp.shvb.shvnamelen = ldp.stemlen+1;
  ldp.shvb.shvvalue.strptr = ldp.ibuf;
  ldp.shvb.shvvalue.strlength = strlen(ldp.ibuf);
  ldp.shvb.shvvaluelen = ldp.shvb.shvvalue.strlength;
  ldp.shvb.shvcode = RXSHV_SET;
  ldp.shvb.shvret = 0;
  if (RexxVariablePool(&ldp.shvb) == RXSHV_BADN)
    return INVALID_ROUTINE;            /* error on non-zero          */

 sprintf(retstr->strptr, "%s", myresult);   /* result is string      */
 retstr->strlength = strlen(retstr->strptr);
 return VALID_ROUTINE;

}


/*************************************************************************
* Function:  SBGetExplicitRequests                                       *
*                                                                        *
* Syntax:    call SBGetExplicitRequests                                  *
*                                                                        *
* Params:                                                                *
*                                                                        *
* Return:    NO_UTIL_ERROR                                               *
*                                                                        *
*************************************************************************/

ULONG SBGetExplicitRequests(CHAR *name, ULONG numargs, RXSTRING args[],
                       CHAR *queuename, RXSTRING *retstr)
{
  RXSTEMDATA ldp;
  ULONG  rc = 0;                           /* Ret code of func           */
  ULONG len;

  BOMDataType BOMData;
  RequestID request;
  RequestIteratorID requestIt;

  InstanceID inst;
  InstanceIteratorID instIt;

  CHAR outputline[512];

  if (numargs != 1)
   return INVALID_ROUTINE;
                                       /* Initialize data area       */
  ldp.count = 0;
  strcpy(ldp.varname, args[0].strptr);
  ldp.stemlen = args[0].strlength;
  strupr(ldp.varname);                 /* uppercase the name         */

  if (ldp.varname[ldp.stemlen-1] != '.')
    ldp.varname[ldp.stemlen++] = '.';

  sprintf((CHAR *)&myresult,"FAILURE  SBGetExplicitRequests: General Failure");
  if (pbLoaded) {
     if (systemInitialized) {
        result = sb_GetAllRequests(sysID,&requestIt);
        if (result == SB_SUCCESS) {
           result = sb_Next(requestIt, &request);
           if (result == SB_SUCCESS) {
              result = sb_GetRequestExplicitInstances(request, &instIt);
              if (result == SB_SUCCESS) {
                sprintf((CHAR *)&myresult, "SUCCESS  SBGetExplicitRequests");
                while (sb_Next(instIt, &inst)) {
                  result = sb_GetInstancePartNumber(inst,BOMData.partNum,256);
                  result = sb_GetInstanceDescriptionOrName(inst,BOMData.description,256);
                  sprintf(outputline,"%d,\"%s\",\"%s\"\n",inst,BOMData.partNum,BOMData.description);
                  len = strlen(outputline);
                  memcpy(ldp.ibuf, outputline, len);
                  ldp.vlen = len;
                  ldp.count++;
                  sprintf(ldp.varname+ldp.stemlen, "%d", ldp.count);
                  if (ldp.ibuf[ldp.vlen-1] == '\n')
                     ldp.vlen--;
                  ldp.shvb.shvnext = NULL;
                  ldp.shvb.shvname.strptr = ldp.varname;
                  ldp.shvb.shvname.strlength = strlen(ldp.varname);
                  ldp.shvb.shvnamelen = ldp.shvb.shvname.strlength;
                  ldp.shvb.shvvalue.strptr = ldp.ibuf;
                  ldp.shvb.shvvalue.strlength = ldp.vlen;
                  ldp.shvb.shvvaluelen = ldp.vlen;
                  ldp.shvb.shvcode = RXSHV_SET;
                  ldp.shvb.shvret = 0;
                  if (RexxVariablePool(&ldp.shvb) == RXSHV_BADN)
                     return INVALID_ROUTINE;      /* error on non-zero          */
                }
              }
              else sprintf((CHAR *)&myresult,"FAILURE  SBGetExplicitRequests: sb_GetRequestExplicitInstances");
           }
           else sprintf((CHAR *)&myresult,"FAILURE  SBGetExplicitRequests: sb_Next (first request)");
        }
        else sprintf((CHAR *)&myresult,"FAILURE  SBGetExplicitRequests: sb_GetAllRequests");
     }
     else sprintf((CHAR *)&myresult,"FAILURE  SBGetExplicitRequests: System not initialized");
  }
  else sprintf((CHAR *)&myresult,"FAILURE  SBGetExplicitRequests: No product base loaded");

  sprintf(ldp.ibuf, "%d", ldp.count);
  ldp.varname[ldp.stemlen] = '0';
  ldp.varname[ldp.stemlen+1] = 0;
  ldp.shvb.shvnext = NULL;
  ldp.shvb.shvname.strptr = ldp.varname;
  ldp.shvb.shvname.strlength = ldp.stemlen+1;
  ldp.shvb.shvnamelen = ldp.stemlen+1;
  ldp.shvb.shvvalue.strptr = ldp.ibuf;
  ldp.shvb.shvvalue.strlength = strlen(ldp.ibuf);
  ldp.shvb.shvvaluelen = ldp.shvb.shvvalue.strlength;
  ldp.shvb.shvcode = RXSHV_SET;
  ldp.shvb.shvret = 0;
  if (RexxVariablePool(&ldp.shvb) == RXSHV_BADN)
    return INVALID_ROUTINE;            /* error on non-zero          */

 sprintf(retstr->strptr, "%s", myresult);   /* result is string      */
 retstr->strlength = strlen(retstr->strptr);
 return VALID_ROUTINE;

}


