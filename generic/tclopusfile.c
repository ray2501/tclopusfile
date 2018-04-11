/*
 * tclopus.c
 *
 *      Copyright (C) Danilo Chang 2016-2017
 *
 ********************************************************************/

/*
 * For C++ compilers, use extern "C"
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <tcl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <opusfile.h>

extern DLLEXPORT int    Opusfile_Init(Tcl_Interp * interp);

/*
 * end block for C++
 */

#ifdef __cplusplus
}
#endif

typedef struct OpusData OpusData;

struct OpusData {
  OggOpusFile *file;
  Tcl_Interp *interp;
  opus_int16 *buffer;
  int bits;
  int bitrate;
  long samplerate;
  int channels;
  int length;
  int buffersize;
  int buff_init;
};

TCL_DECLARE_MUTEX(myMutex);

void split(char **array, char *str, const char *del) {
   char *s = strtok(str, del);

   while(s != NULL) {
     *array++ = s;
     s = strtok(NULL, del);
   }
}


static int OpusObjCmd(void *cd, Tcl_Interp *interp, int objc,Tcl_Obj *const*objv){
  OpusData *pOpus = (OpusData *) cd;
  int choice;
  int rc = TCL_OK;

  static const char *OPUS_strs[] = {
    "buffersize",
    "read",
    "seek",
    "getTags",
    "close", 
    0
  };

  enum OPUS_enum {
    OPUS_BUFFERSIZE,
    OPUS_READ,
    OPUS_SEEK,
    OPUS_GETTAGS,
    OPUS_CLOSE,
  };

  if( objc < 2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "SUBCOMMAND ...");
    return TCL_ERROR;
  }

  if( Tcl_GetIndexFromObj(interp, objv[1], OPUS_strs, "option", 0, &choice) ){
    return TCL_ERROR;
  }

  switch( (enum OPUS_enum)choice ){

    case OPUS_BUFFERSIZE: {
      int buffersize = 0;

      if( objc != 3 ){
        Tcl_WrongNumArgs(interp, 2, objv, "size");
        return TCL_ERROR;
      }

      if(Tcl_GetIntFromObj(interp, objv[2], &buffersize) != TCL_OK) {
         return TCL_ERROR;
      }

      if(buffersize <= 0) {
         Tcl_AppendResult(interp, "Error: buffersize needs > 0", (char*)0);
         return TCL_ERROR;
      }

      Tcl_MutexLock(&myMutex);
      if(pOpus->buff_init == 0) {
        pOpus->buffersize = buffersize;
        pOpus->buff_init = 1;
      }
      Tcl_MutexUnlock(&myMutex);

      break;
    }

    case OPUS_READ: {
      Tcl_Obj *return_obj = NULL;
      long second_count = pOpus->samplerate * pOpus->channels;
      int result = 0;

      if( objc != 2 ){
        Tcl_WrongNumArgs(interp, 2, objv, 0);
        return TCL_ERROR;
      }

      // It is still 0 -> setup the value
      if(pOpus->buffersize == 0) {
         Tcl_MutexLock(&myMutex);
         pOpus->buffersize = second_count;
         pOpus->buff_init = 1;
         Tcl_MutexUnlock(&myMutex);
      }

      if(pOpus->buffer == NULL) {
         pOpus->buffer = (opus_int16 *) malloc (pOpus->buffersize * sizeof(opus_int16));
         if( pOpus->buffer==0 ){
           Tcl_SetResult(interp, (char *)"malloc failed", TCL_STATIC);
           return TCL_ERROR;
         }
      }

      /*
       * Reads more samples from the stream.
       * The sample number is almost the same -> max 960.
       */
tryagain:
      result = op_read(pOpus->file, pOpus->buffer, pOpus->buffersize, NULL);
      if(result <= 0 ) {
         if (result == OP_HOLE) goto tryagain;             
         return TCL_ERROR;
      } else {
         return_obj = Tcl_NewStringObj((char *) pOpus->buffer,
                             result * pOpus->channels * sizeof(opus_int16));

         Tcl_SetObjResult(interp, return_obj);
      }

      break;
    }

    case OPUS_SEEK: {
      Tcl_Obj *return_obj = NULL;
      Tcl_WideInt location = 0;
      int result = 0;

      if( objc != 3 ){
        Tcl_WrongNumArgs(interp, 2, objv,
          "location"
        );
        return TCL_ERROR;
      }

      if(Tcl_GetWideIntFromObj(interp, objv[2], &location) != TCL_OK) {
          return TCL_ERROR;
      }

      if(location < 0) {
          return TCL_ERROR;
      }

      result = op_pcm_seek(pOpus->file, (opus_int64) location);

      if(result < 0) {
          return_obj = Tcl_NewBooleanObj(0);
      } else {
          return_obj = Tcl_NewBooleanObj(1);
      }

      Tcl_SetObjResult(interp, return_obj);
      break;
    }

    case OPUS_GETTAGS: {
      Tcl_Obj *return_obj = NULL;
      const OpusTags *tags;
      int count = 0;

      if( objc != 2 ){
        Tcl_WrongNumArgs(interp, 2, objv, 0);
        return TCL_ERROR;
      }

      tags = op_tags(pOpus->file, -1);
      if(tags == NULL) {
        return TCL_ERROR;
      }

      return_obj = Tcl_NewListObj(0, NULL);
      for(count = 0; count < tags->comments; count++) {
          char *array[2];
          const char *del = "=";
          split(array, tags->user_comments[count], del);

          Tcl_ListObjAppendElement(interp, return_obj,
                 Tcl_NewStringObj(array[0],
                 strlen(array[0])));

          if(array[1]==NULL) {
              Tcl_ListObjAppendElement(interp, return_obj,
                     Tcl_NewStringObj("",
                     -1));
          } else {
              Tcl_ListObjAppendElement(interp, return_obj,
                     Tcl_NewStringObj(array[1],
                     strlen(array[1])));
          }
      }

      Tcl_SetObjResult(interp, return_obj);

      break;
    }

    case OPUS_CLOSE: {
      Tcl_Obj *return_obj = NULL;

      if( objc != 2 ){
        Tcl_WrongNumArgs(interp, 2, objv, 0);
        return TCL_ERROR;
      }

      op_free(pOpus->file);

      if(pOpus->buffer) free(pOpus->buffer);
      Tcl_Free((char *)pOpus);
      pOpus = NULL;

      Tcl_DeleteCommand(interp, Tcl_GetStringFromObj(objv[0], 0));

      return_obj = Tcl_NewBooleanObj(1);
      Tcl_SetObjResult(interp, return_obj);
      break;
    }

  } /* End of the SWITCH statement */

  return rc;
}


static int OpusMain(void *cd, Tcl_Interp *interp, int objc,Tcl_Obj *const*objv){
  OpusData *p;
  const char *zArg = NULL;
  const char *zFile = NULL;
  int buffersize = 0;
  Tcl_DString translatedFilename;
  Tcl_Obj *pResultStr = NULL;
  int len;
  int i = 0;
  int error = 0;

  if( objc < 3 || (objc&1)!=1 ){
    Tcl_WrongNumArgs(interp, 1, objv,
      "HANDLE path ?-buffersize size? "
    );
    return TCL_ERROR;
  }

  p = (OpusData *)Tcl_Alloc( sizeof(*p) );
  if( p==0 ){
    Tcl_SetResult(interp, (char *)"malloc failed", TCL_STATIC);
    return TCL_ERROR;
  }

  memset(p, 0, sizeof(*p));
  p->interp = interp;

  zFile = Tcl_GetStringFromObj(objv[2], &len);
  if( !zFile || len < 1 ){
    Tcl_Free((char *)p);
    return TCL_ERROR;
  }

  for(i=3; i+1<objc; i+=2){
    zArg = Tcl_GetStringFromObj(objv[i], 0);

    if( strcmp(zArg, "-buffersize")==0 ){
      if(Tcl_GetIntFromObj(interp, objv[i+1], &buffersize) != TCL_OK) {
         Tcl_Free((char *)p);
         return TCL_ERROR;
      }

      if(buffersize <= 0) {
         Tcl_Free((char *)p);
         Tcl_AppendResult(interp, "Error: buffersize needs > 0", (char*)0);
         return TCL_ERROR;
      }

      Tcl_MutexLock(&myMutex);
      p->buffersize = buffersize;
      p->buff_init = 1;
      Tcl_MutexUnlock(&myMutex);
    }
  }

  zFile = Tcl_TranslateFileName(interp, zFile, &translatedFilename);
  p->file = op_open_file(zFile, &error);
  Tcl_DStringFree(&translatedFilename);

  if(p->file == NULL) {
      Tcl_Free((char *)p);  //open fail, so we need free our memory
      p = NULL;

      return TCL_ERROR;
  }

  p->buffer = NULL;
  p->samplerate = 48000;
  p->channels = op_channel_count(p->file, -1);
  p->bits = 16;
  p->bitrate = 	op_bitrate(p->file, -1) / 1000;
  p->length = (op_pcm_total(p->file, -1) / 48000);

  zArg = Tcl_GetStringFromObj(objv[1], 0);
  Tcl_CreateObjCommand(interp, zArg, OpusObjCmd, (char*)p, (Tcl_CmdDeleteProc *)NULL);

  pResultStr = Tcl_NewListObj(0, NULL);
  Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewStringObj("length", -1));
  Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewIntObj(p->length));
  Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewStringObj("bits", -1));
  Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewIntObj(p->bits));
  Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewStringObj("bitrate", -1));
  Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewIntObj(p->bitrate));
  Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewStringObj("samplerate", -1));
  Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewIntObj(p->samplerate));
  Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewStringObj("channels", -1));
  Tcl_ListObjAppendElement(interp, pResultStr, Tcl_NewIntObj(p->channels));

  Tcl_SetObjResult(interp, pResultStr);
  return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * Opusfile_Init --
 *
 *	Initialize the new package.
 *
 * Results:
 *	A standard Tcl result
 *
 * Side effects:
 *	The Opus package is created.
 *
 *----------------------------------------------------------------------
 */

int
Opusfile_Init(Tcl_Interp *interp)
{
    if (Tcl_InitStubs(interp, "8.4", 0) == NULL) {
	return TCL_ERROR;
    }
    if (Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION) != TCL_OK) {
	return TCL_ERROR;
    }

    Tcl_CreateObjCommand(interp, "opusfile", (Tcl_ObjCmdProc *) OpusMain,
        (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

    return TCL_OK;
}
