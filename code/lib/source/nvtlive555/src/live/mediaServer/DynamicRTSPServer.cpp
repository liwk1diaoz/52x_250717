/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// Copyright (c) 1996-2013, Live Networks, Inc.  All rights reserved
// A subclass of "RTSPServer" that creates "ServerMediaSession"s on demand,
// based on whether or not the specified stream name exists as a file
// Implementation

#include "DynamicRTSPServer.hh"
#include <liveMedia.hh>
#include <string.h>

//NVT_MODIFIED
DynamicRTSPServer*
DynamicRTSPServer::createNew(UsageEnvironment& env,
			     UserAuthenticationDatabase* authDatabase,
			     unsigned reclamationTestSeconds) {
  NvtMgr *pNvtMgr = NvtMgr_GetHandle();
  Port ourPort(pNvtMgr->Get_Port());
  int ourSocket = setUpOurSocket(env, ourPort);
  if (ourSocket == -1) return NULL;

  return new DynamicRTSPServer(env, ourSocket, ourPort, authDatabase, reclamationTestSeconds);
}

DynamicRTSPServer::DynamicRTSPServer(UsageEnvironment& env, int ourSocket,
				     Port ourPort,
				     UserAuthenticationDatabase* authDatabase, unsigned reclamationTestSeconds)
  : RTSPServerSupportingHTTPStreaming(env, ourSocket, ourPort, authDatabase, reclamationTestSeconds) {
}

DynamicRTSPServer::~DynamicRTSPServer() {
}

static ServerMediaSession* createNewSMS(UsageEnvironment& env,
					char const* fileName, FILE* fid); // forward

ServerMediaSession*
DynamicRTSPServer::lookupServerMediaSession(char const* streamName,int isForceClean, char const* url) { //NVT_MODIFIED, in order to clean sms at each rtsp-desc command
  // First, check whether the specified "streamName" exists as a local file:
  //NVT_MODIFIED
  NvtMgr *pNvtMgr = NvtMgr_GetHandle();
  NVTLIVE555_URL_INFO Info = { 0 };
  if(pNvtMgr->Trans_UrlToInfo(&Info, url)!=0)  {
      return NULL;
  }

  Boolean fileExists = True;

  // Next, check whether we already have a "ServerMediaSession" for this file:
  ServerMediaSession* sms = RTSPServer::lookupServerMediaSession(streamName);

  //NVT_MODIFIED
  //we set to isForceClean=1 at rtsp command description to clean sms and sps,pps.
  if(sms!=NULL && isForceClean==1)
  {
      removeServerMediaSession(sms);
      sms=NULL;
  }

  Boolean smsExists = sms != NULL;

  // Handle the four possibilities for "fileExists" and "smsExists":
  if (!fileExists) {
    if (smsExists) {
      // "sms" was created for a file that no longer exists. Remove it:
      removeServerMediaSession(sms);
    }
    return NULL;
  } else {
    if (!smsExists) {
      // Create a new "ServerMediaSession" object for streaming from the named file.
      sms = createNewSMS(envir(), streamName, (FILE*)&Info); //NVT_MODIFIED, reduce times of url to info
	  if (sms) {
		  addServerMediaSession(sms);
	  } else {
		  return NULL;
	  }
    }
    return sms;
  }
}


#if !defined(__ECOS) && !defined(__FREERTOS)
// Special code for handling Matroska files:
static char newMatroskaDemuxWatchVariable;
static MatroskaFileServerDemux* demux;
static void onMatroskaDemuxCreation(MatroskaFileServerDemux* newDemux, void* /*clientData*/) {
  demux = newDemux;
  newMatroskaDemuxWatchVariable = 1;
}
// END Special code for handling Matroska files:
#endif

#define NEW_SMS(description) do {\
char const* descStr = description\
    ", streamed by the LIVE555 Media Server";\
sms = ServerMediaSession::createNew(env, fileName, fileName, descStr);\
} while(0)

static ServerMediaSession* createNewSMS(UsageEnvironment& env,
					char const* fileName, FILE* pInfo) {
  //trans string to nvt format
  if(pInfo == NULL) {
      return NULL;
  }

  NVTLIVE555_URL_INFO Info = *(NVTLIVE555_URL_INFO *)pInfo;
  // Use the file name extension to determine the type of "ServerMediaSession":
  char const* extension = strrchr(fileName, '.');

  if (extension == NULL) { //return NULL; //NVT_MODIFIED
      extension = ".mp4";
  }

  ServerMediaSession* sms = NULL;
#if !defined(__ECOS) && !defined(__FREERTOS)
  Boolean const reuseSource = False;
  if (strcmp(extension, ".aac") == 0) {
    // Assumed to be an AAC Audio (ADTS format) file:
    NEW_SMS("AAC Audio");
    sms->addSubsession(ADTSAudioFileServerMediaSubsession::createNew(env, fileName, reuseSource));
  } else if (strcmp(extension, ".amr") == 0) {
    // Assumed to be an AMR Audio file:
    NEW_SMS("AMR Audio");
    sms->addSubsession(AMRAudioFileServerMediaSubsession::createNew(env, fileName, reuseSource));
  } else if (strcmp(extension, ".ac3") == 0) {
    // Assumed to be an AC-3 Audio file:
    NEW_SMS("AC-3 Audio");
    sms->addSubsession(AC3AudioFileServerMediaSubsession::createNew(env, fileName, reuseSource));
  } else if (strcmp(extension, ".m4e") == 0) {
    // Assumed to be a MPEG-4 Video Elementary Stream file:
    NEW_SMS("MPEG-4 Video");
    sms->addSubsession(MPEG4VideoFileServerMediaSubsession::createNew(env, fileName, reuseSource));
  } else if (strcmp(extension, ".264") == 0) {
    // Assumed to be a H.264 Video Elementary Stream file:
    NEW_SMS("H.264 Video");
    OutPacketBuffer::maxSize = 300000; //NVT_MODIFIED
    sms->addSubsession(H264VideoFileServerMediaSubsession::createNew(env, fileName, reuseSource));
  } else if (strcmp(extension, ".mp3") == 0) {
    // Assumed to be a MPEG-1 or 2 Audio file:
    NEW_SMS("MPEG-1 or 2 Audio");
    // To stream using 'ADUs' rather than raw MP3 frames, uncomment the following:
//#define STREAM_USING_ADUS 1
    // To also reorder ADUs before streaming, uncomment the following:
//#define INTERLEAVE_ADUS 1
    // (For more information about ADUs and interleaving,
    //  see <http://www.live555.com/rtp-mp3/>)
    Boolean useADUs = False;
    Interleaving* interleaving = NULL;
#ifdef STREAM_USING_ADUS
    useADUs = True;
#ifdef INTERLEAVE_ADUS
    unsigned char interleaveCycle[] = {0,2,1,3}; // or choose your own...
    unsigned const interleaveCycleSize
      = (sizeof interleaveCycle)/(sizeof (unsigned char));
    interleaving = new Interleaving(interleaveCycleSize, interleaveCycle);
#endif
#endif
    sms->addSubsession(MP3AudioFileServerMediaSubsession::createNew(env, fileName, reuseSource, useADUs, interleaving));
  } else if (strcmp(extension, ".mpg") == 0) {
    // Assumed to be a MPEG-1 or 2 Program Stream (audio+video) file:
    NEW_SMS("MPEG-1 or 2 Program Stream");
    MPEG1or2FileServerDemux* demux
      = MPEG1or2FileServerDemux::createNew(env, fileName, reuseSource);
    sms->addSubsession(demux->newVideoServerMediaSubsession());
    sms->addSubsession(demux->newAudioServerMediaSubsession());
  } else if (strcmp(extension, ".vob") == 0) {
    // Assumed to be a VOB (MPEG-2 Program Stream, with AC-3 audio) file:
    NEW_SMS("VOB (MPEG-2 video with AC-3 audio)");
    MPEG1or2FileServerDemux* demux
      = MPEG1or2FileServerDemux::createNew(env, fileName, reuseSource);
    sms->addSubsession(demux->newVideoServerMediaSubsession());
    sms->addSubsession(demux->newAC3AudioServerMediaSubsession());
  } else if (strcmp(extension, ".ts") == 0) {
    // Assumed to be a MPEG Transport Stream file:
    // Use an index file name that's the same as the TS file name, except with ".tsx":
    unsigned indexFileNameLen = strlen(fileName) + 2; // allow for trailing "x\0"
    char* indexFileName = new char[indexFileNameLen];
    sprintf(indexFileName, "%sx", fileName);
    NEW_SMS("MPEG Transport Stream");
    sms->addSubsession(MPEG2TransportFileServerMediaSubsession::createNew(env, fileName, indexFileName, reuseSource));
    delete[] indexFileName;
  } else if (strcmp(extension, ".wav") == 0) {
    // Assumed to be a WAV Audio file:
    NEW_SMS("WAV Audio Stream");
    // To convert 16-bit PCM data to 8-bit u-law, prior to streaming,
    // change the following to True:
    Boolean convertToULaw = False;
    sms->addSubsession(WAVAudioFileServerMediaSubsession::createNew(env, fileName, reuseSource, convertToULaw));
  } else if (strcmp(extension, ".dv") == 0) {
    // Assumed to be a DV Video file
    // First, make sure that the RTPSinks' buffers will be large enough to handle the huge size of DV frames (as big as 288000).
    OutPacketBuffer::maxSize = 300000;

    NEW_SMS("DV Video");
    sms->addSubsession(DVVideoFileServerMediaSubsession::createNew(env, fileName, reuseSource));
  } else if (strcmp(extension, ".mkv") == 0 || strcmp(extension, ".webm") == 0) {
    // Assumed to be a Matroska file (note that WebM ('.webm') files are also Matroska files)
    NEW_SMS("Matroska video+audio+(optional)subtitles");

    // Create a Matroska file server demultiplexor for the specified file.  (We enter the event loop to wait for this to complete.)
    newMatroskaDemuxWatchVariable = 0;
    MatroskaFileServerDemux::createNew(env, fileName, onMatroskaDemuxCreation, NULL);
    env.taskScheduler().doEventLoop(&newMatroskaDemuxWatchVariable);

    ServerMediaSubsession* smss;
    while ((smss = demux->newServerMediaSubsession()) != NULL) {
      sms->addSubsession(smss);
    }
  }
  else
#endif
  //NVT_MODIFIED
  if (strcmp(extension, ".mov") == 0||strcmp(extension, ".MOV") == 0
      || strcmp(extension, ".mp4") == 0||strcmp(extension, ".MP4") == 0 ) { //NVT_MODIFIED
	  INT32 hr_v = -1, hr_a = -1;
      // Assumed to be a H.264 Video Elementary Stream file:
      // OutPacketBuffer::maxSize = 300000;
	  NvtMgr *pNvtMgr = NvtMgr_GetHandle();
	  NvtServerMediaSubsession::INIT_DATA InitData = { 0 };
	  ISTRM_CB* pCb = pNvtMgr->Get_StrmCb();
	  INT32 hStrm = pCb->OpenStrm(Info.channel_id, NVT_MEDIA_TYPE_VIDEO, NULL);
	  if (hStrm == 0) {
		  printf("failed to open stream.\r\n");
		  return NULL;
	  }
	  if ((hr_v = pCb->GetVideoInfo(hStrm, Info.channel_id, 5000, &InitData.VideoInfo)) != 0) {
		  printf("live555: failed to GetVideoInfo stream. er = %d\r\n", (int)hr_v);
		  return NULL;
	  }
	  pCb->CloseStrm(hStrm);
	  if (pNvtMgr->IsSupportAudio()) {
		  //set timeout=100ms for the case without audio (if timeout too long, cause VLC timeout the connection.
#if defined(__ECOS) || defined(__LINUX510) || defined(__LINUX680)
		  hStrm = pCb->OpenStrm(7, NVT_MEDIA_TYPE_AUDIO, NULL);
		  if (hStrm == 0) {
			  printf("failed to open audio stream.\r\n");
		  }
		  else if ((hr_a = pCb->GetAudioInfo(hStrm, 7, 100, &InitData.AudioInfo)) != 0) {
			  printf("live555: no audio. er = %d\r\n", (int)hr_a);
		  }
		  pCb->CloseStrm(hStrm);
#else
		  hStrm = pCb->OpenStrm(Info.channel_id, NVT_MEDIA_TYPE_AUDIO, NULL);
		  if (hStrm == 0) {
			  printf("failed to open audio stream.\r\n");
		  } else if ((hr_a = pCb->GetAudioInfo(hStrm, Info.channel_id, 100, &InitData.AudioInfo)) != 0) {
			  printf("live555: no audio. er = %d\r\n", (int)hr_a);
		  }
		  if (hStrm) {
			  pCb->CloseStrm(hStrm);
		  }
#endif
	  } else {
		  hr_a = 0;
	  }

	  NEW_SMS("Nvt RTSP");
	  InitData.env = &env;
	  InitData.FileName = fileName;
	  InitData.MediaType = NVT_MEDIA_TYPE_VIDEO;
	  InitData.uiMediaSrcID = Info.channel_id;
	  InitData.UrlInfo = Info;
	  NvtServerMediaSubsession* pSmsVideo = NvtServerMediaSubsession::createNew(&InitData);
	  sms->addSubsession(pSmsVideo);

	  if (pNvtMgr->IsSupportAudio() && hr_a==0) {
		  InitData.MediaType = NVT_MEDIA_TYPE_AUDIO;
		  NvtServerMediaSubsession* pSmsAudio = NvtServerMediaSubsession::createNew(&InitData);
		  sms->addSubsession(pSmsAudio);
		  pSmsVideo->SetPartnerSms(pSmsAudio);
		  pSmsAudio->SetPartnerSms(pSmsVideo);
	  }
  }
  if (strcmp(extension, ".movna") == 0||strcmp(extension, ".MOVNA") == 0
      || strcmp(extension, ".mp4na") == 0||strcmp(extension, ".MP4NA") == 0 ) { //NVT_MODIFIED
#if 0
          // Assumed to be a H.264 Video Elementary Stream file:
          // OutPacketBuffer::maxSize = 300000;
          u_int32_t hr;
          u_int32_t uiStrmID=0;
          if((hr=g_NvtData->Lock_StrmID(uiMediaSrcID,&uiStrmID)) != LIVE555_ER_STRMID_NO_RESOURCE)
          {
              NEW_SMS("Nvt RTSP No Audio");

              NvtServerMediaSubsession::INIT_DATA InitData = {0};
              InitData.env = &env;
              InitData.FileName = fileName;
              InitData.pNvtData = g_NvtData;
              InitData.pStreamQueue = g_NvtData->Get_CurrVideo(uiMediaSrcID);
              InitData.bSupportStrmID = (hr==LIVE555_ER_STRMID_ONE_CONNECTION_ONLY)?0:1;
              InitData.uiStrmID = (hr==LIVE555_ER_STRMID_ONE_CONNECTION_ONLY)?0:uiStrmID;
              InitData.uiMediaSrcID = uiMediaSrcID;
              sms->addSubsession(NvtServerMediaSubsession::createNew(&InitData));
              if(g_NvtData->Get_CurrVideo(uiMediaSrcID)->GetType(uiMediaSrcID)!=NVT_CODEC_TYPE_MJPG)
              {
                g_NvtData->RequireKeyFrame(uiStrmID);
              }
          }
#endif
  }
  return sms;
}
