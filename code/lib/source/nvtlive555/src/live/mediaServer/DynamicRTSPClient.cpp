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

#include "DynamicRTSPClient.hh"
#include <liveMedia.hh>
#include <string.h>

// By default, we request that the server stream its data using RTP/UDP.
// If, instead, you want to request that the server stream via RTP-over-TCP, change the following to True:
#define REQUEST_STREAMING_OVER_TCP False

static LIVE555CLI_CONFIG g_NvtConfig={0};

// RTSP 'response handlers':
static void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString);
static void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString);
static void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString);

// Other event handler functions:
static void subsessionAfterPlaying(void* clientData); // called when a stream's subsession (e.g., audio or video substream) ends
static void subsessionByeHandler(void* clientData); // called when a RTCP "BYE" is received for a subsession
static void streamTimerHandler(void* clientData);

// Used to iterate through each stream's 'subsessions', setting up each one:
static void setupNextSubsession(RTSPClient* rtspClient);


// A function that outputs a string that identifies each stream (for debugging output).  Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment& env, const RTSPClient& rtspClient) {
    return env << "[URL:\"" << rtspClient.url() << "\"]: ";
}

// A function that outputs a string that identifies each subsession (for debugging output).  Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment& env, const MediaSubsession& subsession) {
    return env << subsession.mediumName() << "/" << subsession.codecName();
}

void usage(UsageEnvironment& env, char const* progName) {
    env << "Usage: " << progName << " <rtsp-url-1> ... <rtsp-url-N>\n";
    env << "\t(where each <rtsp-url-i> is a \"rtsp://\" URL)\n";
}

void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString) {
    do {
        UsageEnvironment& env = rtspClient->envir(); // alias
        StreamClientState& scs = ((DynamicRTSPClient*)rtspClient)->scs; // alias

        if (resultCode != 0) {
            env << *rtspClient << "Failed to get a SDP description: " << resultString << "\n";
            delete[] resultString;
            break;
        }

#if (CFG_CHECK_SERVER_VENDOR)
        if(strstr(resultString,"Nvt RTSP")==0)
        {
            env << *rtspClient << "Failed to recognize RTSP Server.\n";
            delete[] resultString;
            break;
        }
#endif

        char* const sdpDescription = resultString;
        env << *rtspClient << "Got a SDP description:\n" << sdpDescription << "\n";

        // Create a media session object from this SDP description:
        scs.session = MediaSession::createNew(env, sdpDescription);
        delete[] sdpDescription; // because we don't need it anymore
        if (scs.session == NULL) {
            env << *rtspClient << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
            break;
        } else if (!scs.session->hasSubsessions()) {
            env << *rtspClient << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
            break;
        }

        // Then, create and set up our data source objects for the session.  We do this by iterating over the session's 'subsessions',
        // calling "MediaSubsession::initiate()", and then sending a RTSP "SETUP" command, on each one.
        // (Each 'subsession' will have its own data source.)
        scs.iter = new MediaSubsessionIterator(*scs.session);
        setupNextSubsession(rtspClient);

        return;
    } while (0);

    // An unrecoverable error occurred with this stream.
    shutdownStream(rtspClient);
}

void setupNextSubsession(RTSPClient* rtspClient) {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((DynamicRTSPClient*)rtspClient)->scs; // alias
    NvtMediaCliData* pNvData = ((DynamicRTSPClient*)rtspClient)->Get_NvtData();

    scs.subsession = scs.iter->next();
    if (scs.subsession != NULL) {
        if((pNvData->Get_MediaType(scs.subsession->mediumName())==NVT_MEDIA_TYPE_VIDEO && !pNvData->Get_Config()->uiDisableVideo)
            || (pNvData->Get_MediaType(scs.subsession->mediumName())==NVT_MEDIA_TYPE_AUDIO && !pNvData->Get_Config()->uiDisableAudio))
        {
            if (!scs.subsession->initiate()) {
                env << *rtspClient << "Failed to initiate the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
                setupNextSubsession(rtspClient); // give up on this subsession; go to the next one
            } else {
                env << *rtspClient << "Initiated the \"" << *scs.subsession
                    << "\" subsession (client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1 << ")\n";            
                // Continue setting up this subsession, by sending a RTSP "SETUP" command:
                rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP, False, REQUEST_STREAMING_OVER_TCP);               
            }                       
        }
        else
        {
            env << *rtspClient << "ignore meta setup" << "\n";
            setupNextSubsession(rtspClient); // give up on this subsession; go to the next one
        }     
        return;
    }

    // We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
    if(pNvData->OpenStream()!=0)
    {
        shutdownStream(rtspClient);
    }
    if (scs.session->absStartTime() != NULL) {
        // Special case: The stream is indexed by 'absolute' time, so send an appropriate "PLAY" command:
        rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY, scs.session->absStartTime(), scs.session->absEndTime());
    } else {
        scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
        rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY);
    }
}


void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) {
    do {
        UsageEnvironment& env = rtspClient->envir(); // alias
        StreamClientState& scs = ((DynamicRTSPClient*)rtspClient)->scs; // alias
        NvtMediaCliData* pNvData = ((DynamicRTSPClient*)rtspClient)->Get_NvtData();

        if (resultCode != 0) {
            env << *rtspClient << "Failed to set up the \"" << *scs.subsession << "\" subsession: " << resultString << "\n";
            break;
        }

        env << *rtspClient << "Set up the \"" << *scs.subsession
            << "\" subsession (client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1 << ")\n";
     
        // Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
        // (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
        // after we've sent a RTSP "PLAY" command.)

        scs.subsession->sink = DummySink::createNew(env, pNvData, *scs.subsession, scs.subsession->codecName()); //NVT_MODIFIED to indicate video or audio
        // perhaps use your own custom "MediaSink" subclass instead
        if (scs.subsession->sink == NULL) {
            env << *rtspClient << "Failed to create a data sink for the \"" << *scs.subsession
                << "\" subsession: " << env.getResultMsg() << "\n";
            break;
        }

        env << *rtspClient << "Created a data sink for the \"" << *scs.subsession << "\" subsession\n";
        scs.subsession->miscPtr = rtspClient; // a hack to let subsession handle functions get the "RTSPClient" from the subsession
        scs.subsession->sink->startPlaying(*(scs.subsession->readSource()),
            subsessionAfterPlaying, scs.subsession);
        // Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
        if (scs.subsession->rtcpInstance() != NULL) {
            scs.subsession->rtcpInstance()->setByeHandler(subsessionByeHandler, scs.subsession);
        }
    } while (0);
    delete[] resultString;

    // Set up the next subsession, if any:
    setupNextSubsession(rtspClient);
}


void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString) {
    Boolean success = False;

    do {
        UsageEnvironment& env = rtspClient->envir(); // alias
        StreamClientState& scs = ((DynamicRTSPClient*)rtspClient)->scs; // alias

        if (resultCode != 0) {
            env << *rtspClient << "Failed to start playing session: " << resultString << "\n";
            break;
        }

        // Set a timer to be handled at the end of the stream's expected duration (if the stream does not already signal its end
        // using a RTCP "BYE").  This is optional.  If, instead, you want to keep the stream active - e.g., so you can later
        // 'seek' back within it and do another RTSP "PLAY" - then you can omit this code.
        // (Alternatively, if you don't want to receive the entire stream, you could set this timer for some shorter value.)
        if (scs.duration > 0) {
            unsigned const delaySlop = 2; // number of seconds extra to delay, after the stream's expected duration.  (This is optional.)
            scs.duration += delaySlop;
            unsigned uSecsToDelay = (unsigned)(scs.duration*1000000);
            scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)streamTimerHandler, rtspClient);
        }

        env << *rtspClient << "Started playing session";
        if (scs.duration > 0) {
            env << " (for up to " << scs.duration << " seconds)";
        }
        env << "...\n";

        success = True;
    } while (0);
    delete[] resultString;

    if (!success)
    {
        // An unrecoverable error occurred with this stream.
        shutdownStream(rtspClient);
    }
}


// Implementation of the other event handlers:

void subsessionAfterPlaying(void* clientData) {
    MediaSubsession* subsession = (MediaSubsession*)clientData;
    RTSPClient* rtspClient = (RTSPClient*)(subsession->miscPtr);

    // Begin by closing this subsession's stream:
    Medium::close(subsession->sink);
    subsession->sink = NULL;

    // Next, check whether *all* subsessions' streams have now been closed:
    MediaSession& session = subsession->parentSession();
    MediaSubsessionIterator iter(session);
    while ((subsession = iter.next()) != NULL) {
        if (subsession->sink != NULL) return; // this subsession is still active
    }

    // All subsessions' streams have now been closed, so shutdown the client:
    shutdownStream(rtspClient);
}

void subsessionByeHandler(void* clientData) {
    MediaSubsession* subsession = (MediaSubsession*)clientData;
    RTSPClient* rtspClient = (RTSPClient*)subsession->miscPtr;
    UsageEnvironment& env = rtspClient->envir(); // alias

    env << *rtspClient << "Received RTCP \"BYE\" on \"" << *subsession << "\" subsession\n";

    // Now act as if the subsession had closed:
    subsessionAfterPlaying(subsession);
}

void streamTimerHandler(void* clientData) {
    DynamicRTSPClient* rtspClient = (DynamicRTSPClient*)clientData;
    StreamClientState& scs = rtspClient->scs; // alias

    scs.streamTimerTask = NULL;

    // Shut down the stream:
    shutdownStream(rtspClient);
}

void shutdownStream(RTSPClient* rtspClient, int exitCode) {
    //UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((DynamicRTSPClient*)rtspClient)->scs; // alias
    NvtMediaCliData* pNvData = ((DynamicRTSPClient*)rtspClient)->Get_NvtData();

    Boolean b_is_stream_opened = (scs.session != NULL)?True:False;

    // First, check whether any subsessions have still to be closed:
    if (scs.session != NULL) {
        Boolean someSubsessionsWereActive = False;
        MediaSubsessionIterator iter(*scs.session);
        MediaSubsession* subsession;

        while ((subsession = iter.next()) != NULL) {
            if (subsession->sink != NULL) {
                Medium::close(subsession->sink);
                subsession->sink = NULL;

                if (subsession->rtcpInstance() != NULL) {
                    subsession->rtcpInstance()->setByeHandler(NULL, NULL); // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
                }

                someSubsessionsWereActive = True;
            }
        }

        if (someSubsessionsWereActive) {
            // Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
            // Don't bother handling the response to the "TEARDOWN".
            rtspClient->sendTeardownCommand(*scs.session, NULL);
        }
    }

    //don't close rtspClient here, because it may be occurred in schedule loop.
    //, which cause memory damage when free scheduler pointer.
    //env << *rtspClient << "Closing the stream.\n";
    //Medium::close(rtspClient);

    // Note that this will also cause this stream's "StreamClientState" structure to get reclaimed.
    pNvData->SetRTSPClientPointer(NULL);

    if(b_is_stream_opened)
    {
        pNvData->CloseStream();
    }
}

// Implementation of "StreamClientState":
StreamClientState::StreamClientState()
    : iter(NULL), session(NULL), subsession(NULL), streamTimerTask(NULL), duration(0.0) {
}

StreamClientState::~StreamClientState() {
    delete iter;
    if (session != NULL) {
        // We also need to delete "session", and unschedule "streamTimerTask" (if set)
        UsageEnvironment& env = session->envir(); // alias

        env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
        Medium::close(session);
    }
}

DynamicRTSPClient*  DynamicRTSPClient::createNew(UsageEnvironment& env, NvtMediaCliData* pNvtData,
    int verbosityLevel,
    char const* applicationName,
    portNumBits tunnelOverHTTPPortNum) 
{
    if(pNvtData->Set_Config(&g_NvtConfig)!=0) 
        return NULL;

    return new DynamicRTSPClient(env, pNvtData, pNvtData->Get_Url(), verbosityLevel, applicationName, tunnelOverHTTPPortNum);
}

DynamicRTSPClient::DynamicRTSPClient(UsageEnvironment& env, NvtMediaCliData* pNvtData, char const* rtspURL,
	int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum)
	: RTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, -1)
    , m_pNvtData (pNvtData)
{
    
}

DynamicRTSPClient::~DynamicRTSPClient() 
{
}

unsigned DynamicRTSPClient::SendDescribeCommand()
{
    return sendDescribeCommand(continueAfterDESCRIBE);
}

// Implementation of "DummySink":

DummySink* DummySink::createNew(UsageEnvironment& env, NvtMediaCliData* pNvtData, MediaSubsession& subsession, char const* streamId) {
    return new DummySink(env, pNvtData, subsession, streamId);
}

DummySink::DummySink(UsageEnvironment& env, NvtMediaCliData* pNvtData, MediaSubsession& subsession, char const* streamId)
    : MediaSink(env)
    , fReceiveBufferSize(0)
	, fSubsession(subsession)
	, m_pNvtData(pNvtData)
{
    fStreamId = strDup(streamId);
    m_MediaType = m_pNvtData->Get_MediaType(fSubsession.mediumName());
    switch(m_MediaType)
    {
    case NVT_MEDIA_TYPE_VIDEO:
        {   
            LIVE555CLI_VIDEO_INFO Info = {0};
            Info.uiCodec = m_pNvtData->Trans_VideoCodec(streamId);

            if(Info.uiCodec == LIVE555CLI_CODEC_VIDEO_H264)
            {            
                unsigned numSPropRecords;
                SPropRecord* sPropRecords = parseSPropParameterSets(fSubsession.fmtp_spropparametersets(), numSPropRecords);
                for (unsigned i = 0; i < numSPropRecords; ++i) 
                {
                    if (sPropRecords[i].sPropLength == 0) continue; // bad data
                    u_int8_t nal_unit_type = (sPropRecords[i].sPropBytes[0])&0x1F;
                    if (nal_unit_type == 7/*SPS*/) 
                    {
                        Info.uiSizeSPS = sPropRecords[i].sPropLength;
                        if(Info.uiSizeSPS > sizeof(Info.pSPS))
                        {
                            printf("live555cli: sps too large to save. (%d)", (int)Info.uiSizeSPS);
                            Info.uiSizeSPS = sizeof(Info.pSPS);
                        }
                        memcpy(Info.pSPS,sPropRecords[i].sPropBytes, Info.uiSizeSPS);
                    } 
                    else if (nal_unit_type == 8/*PPS*/) 
                    {
                        Info.uiSizePPS = sPropRecords[i].sPropLength;
                        if(Info.uiSizePPS > sizeof(Info.pPPS))
                        {
                            printf("live555cli: pps too large to save. (%d)", (int)Info.uiSizePPS);
                            Info.uiSizePPS = sizeof(Info.pPPS);
                        }
                        memcpy(Info.pPPS,sPropRecords[i].sPropBytes, Info.uiSizePPS);
                    }
                }
                delete[] sPropRecords;
            }
			else if (Info.uiCodec == LIVE555CLI_CODEC_VIDEO_H265)
			{
				unsigned numSPropRecords;
				SPropRecord* sPropRecords;
				// VSP
				sPropRecords = parseSPropParameterSets(fSubsession.fmtp_spropvps(), numSPropRecords);
				if (sPropRecords) {
					Info.uiSizeVPS = sPropRecords[0].sPropLength;
					if (Info.uiSizeVPS > sizeof(Info.pVPS))
					{
						printf("live555cli: vps too large to save. (%d)", (int)Info.uiSizeVPS);
						Info.uiSizeVPS = sizeof(Info.pVPS);
					}
					memcpy(Info.pVPS, sPropRecords[0].sPropBytes, Info.uiSizeVPS);
					delete[] sPropRecords;
				}
				// SPS
				sPropRecords = parseSPropParameterSets(fSubsession.fmtp_spropsps(), numSPropRecords);
				if (sPropRecords) {
					Info.uiSizeSPS = sPropRecords[0].sPropLength;
					if (Info.uiSizeSPS > sizeof(Info.pSPS))
					{
						printf("live555cli: sps too large to save. (%d)", (int)Info.uiSizeSPS);
						Info.uiSizeSPS = sizeof(Info.pSPS);
					}
					memcpy(Info.pSPS, sPropRecords[0].sPropBytes, Info.uiSizeSPS);
					delete[] sPropRecords;
				}
				// PPS
				sPropRecords = parseSPropParameterSets(fSubsession.fmtp_sproppps(), numSPropRecords);
				if (sPropRecords) {
					Info.uiSizePPS = sPropRecords[0].sPropLength;
					if (Info.uiSizePPS > sizeof(Info.pPPS))
					{
						printf("live555cli: pps too large to save. (%d)", (int)Info.uiSizePPS);
						Info.uiSizePPS = sizeof(Info.pPPS);
					}
					memcpy(Info.pPPS, sPropRecords[0].sPropBytes, Info.uiSizePPS);
					delete[] sPropRecords;
				}				
			}
            fReceiveBuffer = m_pNvtData->Get_BufVideo(&fReceiveBufferSize);
            m_pNvtData->Set_VideoInfo(&Info);
        } break;
    case NVT_MEDIA_TYPE_AUDIO:
        {        
            LIVE555CLI_AUDIO_INFO Info = {0};
            Info.uiCodec = m_pNvtData->Trans_AudioCodec(streamId);
            Info.uiSamplePerSecond = fSubsession.rtpTimestampFrequency();
            Info.uiChannelCnt = fSubsession.numChannels();
            fReceiveBuffer = m_pNvtData->Get_BufAudio(&fReceiveBufferSize);
            m_pNvtData->Set_AudioInfo(&Info);
        } break;
    case NVT_MEDIA_TYPE_META:
        break;
    default:
        printf("live555cli:DummySink unknown media type:%s",streamId);
        break;;
    }       
}

DummySink::~DummySink() {
    //delete[] fReceiveBuffer; no need to delete bacause it was provided by NvtMedia
    delete[] fStreamId;
}

void DummySink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes,
struct timeval presentationTime, unsigned durationInMicroseconds) {
    DummySink* sink = (DummySink*)clientData;
    //fwrite(sink->fReceiveBuffer,frameSize,1,xfptr);
    sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

// If you don't want to see debugging output for each received frame, then comment out the following line:
//#define DEBUG_PRINT_EACH_RECEIVED_FRAME 0

void DummySink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
struct timeval presentationTime, unsigned /*durationInMicroseconds*/) {

    LIVE555CLI_FRM Frm = {0};
    Frm.uiFrmCmd = LIVE555CLI_FRM_CMD_FRAME;
    Frm.uiPtsSec = presentationTime.tv_sec;
    Frm.uiPtsUsec = presentationTime.tv_usec;
    Frm.uiAddr = (UINT32)fReceiveBuffer;
    Frm.uiSize = frameSize;

    if(m_MediaType == NVT_MEDIA_TYPE_VIDEO)
    {
        m_pNvtData->SetNextVideoFrm(&Frm);
    }
    else
    {
        m_pNvtData->SetNextAudioFrm(&Frm);
    }
    //if(strcmp(fSubsession.codecName(),"H264")==0) //NVT_MODIFIED
    {
        // We've just received a frame of data.  (Optionally) print out information about it:
#if 0 //#ifdef DEBUG_PRINT_EACH_RECEIVED_FRAME
        if (fStreamId != NULL) envir() << "Stream \"" << fStreamId << "\"; ";
        envir() << fSubsession.mediumName() << "/" << fSubsession.codecName() << ":\tReceived " << frameSize << " bytes";
        if (numTruncatedBytes > 0) envir() << " (with " << numTruncatedBytes << " bytes truncated)";
        char uSecsStr[6+1]; // used to output the 'microseconds' part of the presentation time
        sprintf(uSecsStr, "%06u", (unsigned)presentationTime.tv_usec);
        envir() << ".\tPresentation time: " << (int)presentationTime.tv_sec << "." << uSecsStr;
        if (fSubsession.rtpSource() != NULL && !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP()) {
            envir() << "!"; // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
        }
#ifdef DEBUG_PRINT_NPT
        envir() << "\tNPT: " << fSubsession.getNormalPlayTime(presentationTime);
#endif
        envir() << "\n";
#endif
    }
    // Then continue, to request the next frame of data:
    continuePlaying();
}

Boolean DummySink::continuePlaying() {
    if (fSource == NULL) return False; // sanity check (should not happen)

    // Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
    fSource->getNextFrame(fReceiveBuffer, fReceiveBufferSize,
        afterGettingFrame, this,
        onSourceClosure, this);
   
    return True;
}

extern "C" {

INT32 Live555Cli_SetConfig(LIVE555CLI_CONFIG* pConfig)
{
    g_NvtConfig = *pConfig;
    return 0;
}

}