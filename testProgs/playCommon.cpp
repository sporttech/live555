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
59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
**********/
// Copyright (c) 1996-2003, Live Networks, Inc.  All rights reserved
// A common framework, used for the "openRTSP" and "playSIP" applications
// Implementation

#include "playCommon.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"

#if defined(__WIN32__) || defined(_WIN32)
#define snprintf _snprintf
#else
#include <signal.h>
#define USE_SIGNALS 1
#endif

// Forward function definitions:
void setupStreams();
void startPlayingStreams();
void tearDownStreams();
void closeMediaSinks();
void subsessionAfterPlaying(void* clientData);
void subsessionByeHandler(void* clientData);
void sessionAfterPlaying(void* clientData = NULL);
void sessionTimerHandler(void* clientData);
void shutdown(int exitCode = 1);
void signalHandlerShutdown(int sig);
void checkForPacketArrival(void* clientData);
Boolean setupDestinationRTSPServer();

char const* progName;
UsageEnvironment* env;
Medium* ourClient = NULL;
MediaSession* session = NULL;
TaskToken currentTimerTask = NULL;
Boolean createReceivers = True;
Boolean outputQuickTimeFile = False;
QuickTimeFileSink* qtOut = NULL;
Boolean audioOnly = False;
Boolean videoOnly = False;
char const* singleMedium = NULL;
int verbosityLevel = 0;
float endTime = 0;
float endTimeSlop = 5.0; // extra seconds to delay
Boolean playContinuously = False;
int simpleRTPoffsetArg = -1;
Boolean notifyOnPacketArrival = False;
Boolean streamUsingTCP = False;
char* username = NULL;
char* password = NULL;
char* proxyServerName = NULL;
unsigned short proxyServerPortNum = 0;
unsigned char desiredAudioRTPPayloadFormat = 0;
unsigned short movieWidth = 240;
unsigned short movieHeight = 180;
unsigned movieFPS = 15;
Boolean packetLossCompensate = False;
Boolean syncStreams = False;
Boolean generateHintTracks = False;
char* destRTSPURL = NULL;

#ifdef BSD
static struct timezone Idunno;
#else
static int Idunno;
#endif
struct timeval startTime;

void usage() {
  fprintf(stderr, "Usage: %s [-p <startPortNum>] [-r|-q] [-a|-v] [-V] [-e <endTime>] [-c] [-s <offset>] [-n]%s [-u <username> <password>%s]%s [-w <width> -h <height>] [-f <frames-per-second>] [-y] [-H] <url>\n",
	  progName,
	  controlConnectionUsesTCP ? " [-t]" : "",
	  allowProxyServers
	  ? " [<proxy-server> [<proxy-server-port>]]" : "",
	  supportCodecSelection
	  ? " [-A <audio-codec-rtp-payload-format-code>]" : "");
  //##### Add "-D <dest-rtsp-url>" #####
  shutdown();
}

int main(int argc, char** argv) {
  progName = argv[0];

  gettimeofday(&startTime, &Idunno);

#ifdef USE_SIGNALS
  // Allow ourselves to be shut down gracefully by a SIGHUP or a SIGUSR1:
  signal(SIGHUP, signalHandlerShutdown);
  signal(SIGUSR1, signalHandlerShutdown);
#endif

  unsigned short desiredPortNum = 0;

  // unfortunately we can't use getopt() here, as Windoze doesn't have it
  while (argc > 2) {
    char* const opt = argv[1];
    if (opt[0] != '-') usage();
    switch (opt[1]) {

    case 'p': { // specify start port number
      int portArg;
      if (sscanf(argv[2], "%d", &portArg) != 1) {
	usage();
      }
      if (portArg <= 0 || portArg >= 65536 || portArg&1) {
	fprintf(stderr, "bad port number: %d (must be even, and in the range (0,65536))\n", portArg);
	usage();
      }
      desiredPortNum = (unsigned short)portArg;
      ++argv; --argc;
      break;
    }

    case 'r': { // do not receive data (instead, just 'play' the stream(s))
      createReceivers = False;
      break;
    }

    case 'q': { // output a QuickTime file (to stdout)
      outputQuickTimeFile = True;
      break;
    }

    case 'a': { // receive/record an audio stream only
      audioOnly = True;
      singleMedium = "audio";
      break;
    }

    case 'v': { // receive/record a video stream only
      videoOnly = True;
      singleMedium = "video";
      break;
    }

    case 'V': { // verbose output
      verbosityLevel = 1;
      break;
    }

    case 'e': { // specify end time, or how much to delay after end time
      float arg;
      if (sscanf(argv[2], "%g", &arg) != 1) {
	usage();
      }
      if (argv[2][0] == '-') { // in case argv[2] was "-0"
	// a 'negative' argument was specified; use this for "endTimeSlop":
	endTime = 0; // use whatever's in the SDP
	endTimeSlop = -arg;
      } else {
	endTime = arg;
	endTimeSlop = 0;
      }
      ++argv; --argc;
      break;
    }

    case 'c': { // play continuously
      playContinuously = True;
      break;
    }

    case 's': { // specify an offset to use with "SimpleRTPSource"s
      if (sscanf(argv[2], "%d", &simpleRTPoffsetArg) != 1) {
	usage();
      }
      if (simpleRTPoffsetArg < 0) {
	fprintf(stderr, "offset argument to \"-s\" must be >= 0\n");
	usage();
      }
      ++argv; --argc;
      break;
    }

    case 'n': { // notify the user when the first data packet arrives
      notifyOnPacketArrival = True;
      break;
    }

    case 't': {
      // stream RTP and RTCP over the TCP 'control' connection
      if (controlConnectionUsesTCP) {
	streamUsingTCP = True;
      } else {
	usage();
      }
      break;
    }

    case 'u': { // specify a username and password
      username = argv[2];
      password = argv[3];
      argv+=2; argc-=2;
      if (allowProxyServers && argc > 3 && argv[2][0] != '-') {
	// The next argument is the name of a proxy server:
	proxyServerName = argv[2];
	++argv; --argc;

	if (argc > 3 && argv[2][0] != '-') {
	  // The next argument is the proxy server port number:
	  if (sscanf(argv[2], "%hu", &proxyServerPortNum) != 1) {
	    usage();
	  }
	  ++argv; --argc;
	}
      }
      break;
    }

    case 'A': { // specify a desired audio RTP payload format
      unsigned formatArg;
      if (sscanf(argv[2], "%u", &formatArg) != 1
	  || formatArg >= 96) {
	usage();
      }
      desiredAudioRTPPayloadFormat = (unsigned char)formatArg;
      ++argv; --argc;
      break;
    }

    case 'w': { // specify a width (pixels) for an output QuickTime movie
      if (sscanf(argv[2], "%hu", &movieWidth) != 1) {
	usage();
      }
      ++argv; --argc;
      break;
    }

    case 'h': { // specify a height (pixels) for an output QuickTime movie
      if (sscanf(argv[2], "%hu", &movieHeight) != 1) {
	usage();
      }
      ++argv; --argc;
      break;
    }

    case 'f': { // specify a frame rate (per second) for an output QT movie
      if (sscanf(argv[2], "%u", &movieFPS) != 1) {
	usage();
      }
      ++argv; --argc;
      break;
    }

    // Note: The following option is deprecated, and may someday be removed:
    case 'l': { // try to compensate for packet loss by repeating frames
      packetLossCompensate = True;
      break;
    }

    case 'y': { // synchronize audio and video streams
      syncStreams = True;
      break;
    }

    case 'H': { // generate hint tracks (as well as the regular data tracks)
      generateHintTracks = True;
      break;
    }

    case 'R': { // inject received data into a RTSP server
      destRTSPURL = argv[2];
      ++argv; --argc;
      break;
    }

    default: {
      usage();
      break;
    }
    }

    ++argv; --argc;
  }
  if (argc != 2) usage();
  if (!createReceivers && outputQuickTimeFile) {
    fprintf(stderr, "The -r and -q flags cannot both be used!\n");
    usage();
  }
  if (destRTSPURL != NULL && (!createReceivers || outputQuickTimeFile)) {
    fprintf(stderr, "The -R flag cannot be used with -r or -q!\n");
    usage();
  }
  if (audioOnly && videoOnly) {
    fprintf(stderr, "The -a and -v flags cannot both be used!\n");
    usage();
  }
  if (!createReceivers && notifyOnPacketArrival) {
    fprintf(stderr, "Warning: Because we're not receiving stream data, the -n flag has no effect\n");
  }

  char* url = argv[1];

  // Begin by setting up our usage environment:
  TaskScheduler* scheduler = BasicTaskScheduler::createNew();
  env = BasicUsageEnvironment::createNew(*scheduler);

  // Create our client object:
  ourClient = createClient(*env, verbosityLevel, progName);
  if (ourClient == NULL) {
    fprintf(stderr, "Failed to create %s client: %s\n",
	    clientProtocolName, env->getResultMsg());
    shutdown();
  }

  // Open the URL, to get a SDP description:
  char* sdpDescription
    = getSDPDescriptionFromURL(ourClient, url, username, password,
			       proxyServerName, proxyServerPortNum,
			       desiredPortNum);
  if (sdpDescription == NULL) {
    fprintf(stderr,
	    "Failed to get a SDP description from URL \"%s\": %s\n",
	    url, env->getResultMsg());
    shutdown();
  }

  fprintf(stderr, "Opened URL \"%s\", returning a SDP description:\n%s\n",
	  url, sdpDescription);

  // Create a media session object from this SDP description:
  session = MediaSession::createNew(*env, sdpDescription);
  delete sdpDescription;
  if (session == NULL) {
    fprintf(stderr, "Failed to create a MediaSession object from the SDP description: %s\n", env->getResultMsg());
    shutdown();
  } else if (!session->hasSubsessions()) {
    fprintf(stderr, "This session has no media subsessions (i.e., \"m=\" lines)\n");
    shutdown();
  }

  // Then, setup the "RTPSource"s for the session:
  MediaSubsessionIterator iter(*session);
  MediaSubsession *subsession;
  Boolean madeProgress = False;
  char const* singleMediumToTest = singleMedium;
  while ((subsession = iter.next()) != NULL) {
    // If we've asked to receive only a single medium, then check this now:
    if (singleMediumToTest != NULL) {
      if (strcmp(subsession->mediumName(), singleMediumToTest) != 0) {
	fprintf(stderr, "Ignoring \"%s/%s\" subsession, because we've asked to receive a single %s session only\n",
		subsession->mediumName(), subsession->codecName(),
		singleMedium);
	continue;
      } else {
	// Receive this subsession only
	singleMediumToTest = "xxxxx";
	    // this hack ensures that we get only 1 subsession of this type
      }
    }

    if (desiredPortNum != 0) {
      subsession->setClientPortNum(desiredPortNum);
      desiredPortNum += 2;
    }

    if (createReceivers) {
      if (!subsession->initiate(simpleRTPoffsetArg)) {
	fprintf(stderr, "Unable to create receiver for \"%s/%s\" subsession: %s\n",
		subsession->mediumName(), subsession->codecName(),
		env->getResultMsg());
      } else {
	fprintf(stderr, "Created receiver for \"%s/%s\" subsession (client ports %d-%d)\n",
		subsession->mediumName(), subsession->codecName(),
		subsession->clientPortNum(), subsession->clientPortNum()+1);
	madeProgress = True;

	// Because we're saving the incoming data, rather than playing it
	// in real time, allow an especially large time threshold (1 second)
	// for reordering misordered incoming packets:
	if (subsession->rtpSource() != NULL) {
	  unsigned const thresh = 1000000; // 1 second 
	  subsession->rtpSource()->setPacketReorderingThresholdTime(thresh);
	}
      }
    } else {
      if (subsession->clientPortNum() == 0) {
	fprintf(stderr, "No client port was specified for the \"%s/%s\" subsession.  (Try adding the \"-p <portNum>\" option.)\n",
		subsession->mediumName(), subsession->codecName());
      } else {	
	madeProgress = True;
      }
    }
  }
  if (!madeProgress) shutdown(0);

  // Perform additional 'setup' on each subsession, before playing them:
  setupStreams();

  // Create output files:
  if (createReceivers) {
    if (outputQuickTimeFile) {
      // Create a "QuickTimeFileSink", to write to 'stdout':
      qtOut = QuickTimeFileSink::createNew(*env, *session, "stdout",
					   movieWidth, movieHeight,
					   movieFPS,
					   packetLossCompensate,
					   syncStreams,
					   generateHintTracks);
      if (qtOut == NULL) {
	fprintf(stderr,
		"Failed to create QuickTime file sink for stdout: %s",
		env->getResultMsg());
	shutdown();
      }

      qtOut->startPlaying(sessionAfterPlaying, NULL);
    } else if (destRTSPURL != NULL) {
      // Announce the session into a (separate) RTSP server,
      // and create one or more "RTPTranslator"s to tie the source
      // and destination together:
      if (setupDestinationRTSPServer()) {
	fprintf(stderr,
		"Set up destination RTSP session for \"%s\"\n",
		destRTSPURL);
      } else {
	fprintf(stderr,
		"Failed to set up destination RTSP session for \"%s\": %s\n",
		destRTSPURL, env->getResultMsg());
	shutdown();
      }
    } else {
      // Create and start "FileSink"s for each subsession:
      madeProgress = False;
      iter.reset();
      while ((subsession = iter.next()) != NULL) {
	if (subsession->readSource() == NULL) continue; // was not initiated
	
	// Create an output file for each desired stream:
	char outFileName[1000];
	if (singleMedium == NULL) {
	  // Output file name is "<medium_name>-<codec_name>-<counter>"
	  static unsigned streamCounter = 0;
	  snprintf(outFileName, sizeof outFileName, "%s-%s-%d",
		   subsession->mediumName(), subsession->codecName(),
		   ++streamCounter);
	} else {
	  sprintf(outFileName, "stdout");
	}
	subsession->sink = FileSink::createNew(*env, outFileName);
	if (subsession->sink == NULL) {
	  fprintf(stderr, "Failed to create FileSink for \"%s\": %s\n",
		  outFileName, env->getResultMsg());
	} else {
	  if (singleMedium == NULL) {
	    fprintf(stderr, "Created output file: \"%s\"\n", outFileName);
	  } else {
	    fprintf(stderr, "Outputting data from the \"%s/%s\" subsession to 'stdout'\n",
		    subsession->mediumName(), subsession->codecName());
	  }
	  subsession->sink->startPlaying(*(subsession->readSource()),
					 subsessionAfterPlaying,
					 subsession);
	  
	  // Also set a handler to be called if a RTCP "BYE" arrives
	  // for this subsession:
	  if (subsession->rtcpInstance() != NULL) {
	    subsession->rtcpInstance()->setByeHandler(subsessionByeHandler,
						      subsession);
	  }

	  madeProgress = True;
	}
      }
      if (!madeProgress) shutdown();
    }
  }
    
  // Finally, start playing each subsession, to start the data flow:

  startPlayingStreams();

  env->taskScheduler().doEventLoop(); // does not return

  return 0; // only to prevent compiler warning
}


void setupStreams() {
  MediaSubsessionIterator iter(*session);
  MediaSubsession *subsession;
  Boolean madeProgress = False;

  while ((subsession = iter.next()) != NULL) {
    if (subsession->clientPortNum() == 0) continue; // port # was not set

    if (!clientSetupSubsession(ourClient, subsession, streamUsingTCP)) {
      fprintf(stderr,
              "Failed to setup \"%s/%s\" subsession: %s\n",
              subsession->mediumName(), subsession->codecName(),
              env->getResultMsg());
    } else {
      fprintf(stderr, "Setup \"%s/%s\" subsession (client ports %d-%d)\n",
              subsession->mediumName(), subsession->codecName(),
              subsession->clientPortNum(), subsession->clientPortNum()+1);
      madeProgress = True;
    }
  }
  if (!madeProgress) shutdown();
}

void startPlayingStreams() {
  if (!clientStartPlayingSession(ourClient, session)) {
    fprintf(stderr, "Failed to start playing session: %s\n",
	    env->getResultMsg());
    shutdown();
  } else {
    fprintf(stderr, "Started playing session\n");
  }

  // Figure out how long to delay (if at all) before shutting down, or
  // repeating the playing
  Boolean timerIsBeingUsed = False;
  float totalEndTime = endTime;
  if (endTime == 0) endTime = session->playEndTime(); // use SDP end time
  if (endTime > 0) {
    float const maxDelayTime = (float)( ((unsigned)0x7FFFFFFF)/1000000.0 );
    if (endTime > maxDelayTime) {
      fprintf(stderr, "Warning: specified end time %g exceeds maximum %g; will not do a delayed shutdown\n", endTime, maxDelayTime);
      endTime = 0.0;
    } else {
      timerIsBeingUsed = True;
      totalEndTime = endTime + endTimeSlop;

      int uSecsToDelay = (int)(totalEndTime*1000000.0);
      currentTimerTask = env->taskScheduler().scheduleDelayedTask(
         uSecsToDelay, (TaskFunc*)sessionTimerHandler, (void*)NULL);
    }
  }

  char const* actionString
    = createReceivers? "Receiving streamed data":"Data is being streamed";
  if (timerIsBeingUsed) {
    fprintf(stderr, "%s (for up to %.1f seconds)...\n",
	    actionString, totalEndTime);
  } else {
#ifdef USE_SIGNALS
    pid_t ourPid = getpid();
    fprintf(stderr, "%s (signal with \"kill -HUP %d\" or \"kill -USR1 %d\" to terminate)...\n",
	    actionString, (int)ourPid, (int)ourPid);
#else
    fprintf(stderr, "%s...\n", actionString);
#endif
  }

  // Watch for incoming packets (if desired):
  checkForPacketArrival(NULL);
}

void tearDownStreams() {
  MediaSubsessionIterator iter(*session);
  MediaSubsession* subsession;
  while ((subsession = iter.next()) != NULL) {
    if (subsession->sessionId == NULL) continue; // no PLAY in progress
    
    fprintf(stderr, "Closing \"%s/%s\" subsession\n",
	    subsession->mediumName(), subsession->codecName());
    clientTearDownSubsession(ourClient, subsession);
  }

  clientTearDownSession(ourClient, session);
}

void closeMediaSinks() {
  Medium::close(qtOut);

  MediaSubsessionIterator iter(*session);
  MediaSubsession* subsession;
  while ((subsession = iter.next()) != NULL) {
    Medium::close(subsession->sink);
    subsession->sink = NULL;
  }
}

void subsessionAfterPlaying(void* clientData) {
  // Begin by closing this media subsession:
  MediaSubsession* subsession = (MediaSubsession*)clientData;
  Medium::close(subsession->sink);
  subsession->sink = NULL;

  // Next, check whether *all* subsessions have now been closed:
  MediaSession& session = subsession->parentSession();
  MediaSubsessionIterator iter(session);
  while ((subsession = iter.next()) != NULL) {
    if (subsession->sink != NULL) return; // this subsession is still active
  }

  // All subsessions have now been closed
  sessionAfterPlaying();
}

void subsessionByeHandler(void* clientData) {
  struct timeval timeNow;
  gettimeofday(&timeNow, &Idunno);
  unsigned secsDiff = timeNow.tv_sec - startTime.tv_sec;

  MediaSubsession* subsession = (MediaSubsession*)clientData;
  fprintf(stderr, "Received RTCP \"BYE\" on \"%s/%s\" subsession (after %d seconds)\n",
	  subsession->mediumName(), subsession->codecName(), secsDiff);

  // Act now as if the subsession had closed:
  subsessionAfterPlaying(subsession);
}

void sessionAfterPlaying(void* /*clientData*/) {
  if (!playContinuously) {
    shutdown(0);
  } else {
    // We've been asked to play the stream(s) over again:
    startPlayingStreams();
  }
}

void sessionTimerHandler(void* /*clientData*/) {
  currentTimerTask = NULL;

  sessionAfterPlaying();
}

void shutdown(int exitCode) {
  if (env != NULL) {
    env->taskScheduler().unscheduleDelayedTask(currentTimerTask);
  }

  if (ourClient != NULL) {
    // Teardown any outstanding sessions, and close media sinks:
    if (session != NULL){
      tearDownStreams();
      closeMediaSinks();
    }

    // Then shut down our client:
    Medium::close(ourClient);
  }

  // Shutdown the Media session (and all of the RTP/RTCP subsessions):
  Medium::close(session);

  // Adios...
  exit(exitCode);
}

void signalHandlerShutdown(int /*sig*/) {
  fprintf(stderr, "Got shutdown signal\n");
  shutdown(0);
}

void checkForPacketArrival(void* clientData) {
  if (!notifyOnPacketArrival) return; // we're not checking 

  // Check each subsession, to see whether it has received data packets:
  unsigned numSubsessionsChecked = 0;
  unsigned numSubsessionsWithReceivedData = 0;
  unsigned numSubsessionsThatHaveBeenSynced = 0;

  MediaSubsessionIterator iter(*session);
  MediaSubsession* subsession;
  while ((subsession = iter.next()) != NULL) {
    RTPSource* src = subsession->rtpSource();
    if (src == NULL) continue;
    ++numSubsessionsChecked;

    if (src->receptionStatsDB().numActiveSourcesSinceLastReset() > 0) {
      // At least one data packet has arrived
      ++numSubsessionsWithReceivedData;
    }
    if (src->hasBeenSynchronizedUsingRTCP()) {
      ++numSubsessionsThatHaveBeenSynced;
    }
  }

  unsigned numSubsessionsToCheck = numSubsessionsChecked;
  if (qtOut != NULL) {
    // Special case for "QuickTimeFileSink"s: They might not use all of the
    // input sources:
    numSubsessionsToCheck = qtOut->numActiveSubsessions();
  }

  Boolean notifyTheUser;
  if (!syncStreams) {
    notifyTheUser = numSubsessionsWithReceivedData > 0; // easy case
  } else {
    notifyTheUser = numSubsessionsWithReceivedData >= numSubsessionsToCheck
      && numSubsessionsThatHaveBeenSynced == numSubsessionsChecked;
    // Note: A subsession with no active sources is considered to be synced
  }
  if (notifyTheUser) {
    struct timeval timeNow;
    gettimeofday(&timeNow, &Idunno);
    fprintf(stderr, "%sata packets have begun arriving [%ld%03ld]\007\n",
	    syncStreams ? "Synchronized d" : "D",
	    timeNow.tv_sec, timeNow.tv_usec/1000);
    return;
  }

  // No luck, so reschedule this check again, after a delay:
  int uSecsToDelay = 100000; // 100 ms
  currentTimerTask
    = env->taskScheduler().scheduleDelayedTask(uSecsToDelay,
			       (TaskFunc*)checkForPacketArrival, NULL);
}

// WORK IN PROGRESS #####
class RTPTranslator: public FramedFilter {
public:
  static RTPTranslator* createNew(UsageEnvironment& env,
				  FramedSource* source);

private:
  RTPTranslator(UsageEnvironment& env, FramedSource* source);
  virtual ~RTPTranslator();

  static void afterGettingFrame(void* clientData,
                                unsigned numBytesRead,
                                struct timeval presentationTime);
  void afterGettingFrame1(unsigned numBytesRead,
			  struct timeval presentationTime);

private: // redefined virtual function:
  virtual void doGetNextFrame();

private:
  //unsigned char fBuffer[50000];//##### Later: parameterize
};

RTPTranslator* RTPTranslator::createNew(UsageEnvironment& env,
					FramedSource* source) {
  // Check whether source is a "RTPSource"??? #####
  return new RTPTranslator(env, source);
}

RTPTranslator::RTPTranslator(UsageEnvironment& env, FramedSource* source)
  : FramedFilter(env, source) {
}

RTPTranslator::~RTPTranslator() {
}

void RTPTranslator::doGetNextFrame() {
  // For now, do a direct relay #####
  fInputSource->getNextFrame(fTo, fMaxSize,
			     afterGettingFrame, this,
			     handleClosure, this);
}

void RTPTranslator::afterGettingFrame(void* clientData,
				      unsigned numBytesRead,
				      struct timeval presentationTime) {
  RTPTranslator* rtpTranslator = (RTPTranslator*)clientData;
  rtpTranslator->afterGettingFrame1(numBytesRead, presentationTime);
}

void RTPTranslator::afterGettingFrame1(unsigned numBytesRead,
				       struct timeval presentationTime) {
  fFrameSize = numBytesRead;
  fPresentationTime = presentationTime;
  afterGetting(this);
}

//#####
RTSPClient* rtspClientOutgoing = NULL;
Boolean setupDestinationRTSPServer() {
  do {
    rtspClientOutgoing
      = RTSPClient::createNew(*env, verbosityLevel, progName);
    if (rtspClientOutgoing == NULL) break;

    // Construct the SDP description to announce into the RTSP server:

    // First, get our own IP address, and that of the RTSP server:
    struct in_addr ourIPAddress;
    ourIPAddress.s_addr = ourSourceAddressForMulticast(*env);
    char* ourIPAddressStr = strdup(our_inet_ntoa(ourIPAddress));

    NetAddress serverAddress;
    portNumBits serverPortNum;
    if (!RTSPClient::parseRTSPURL(*env, destRTSPURL,
				  serverAddress, serverPortNum)) break;
    struct in_addr serverIPAddress;
    serverIPAddress.s_addr = *(unsigned*)(serverAddress.data());
    char* serverIPAddressStr = strdup(our_inet_ntoa(serverIPAddress));

    char const* destSDPFmt =
      "v=0\r\n"
      "o=- %u %u IN IP4 %s\r\n"
      "s=RTSP session, relayed through \"%s\"\n"
      "i=relayed RTSP session\n"
      "t=0 0\n"
      "c=IN IP4 %s\n"
      "a=control:*\n"
      "m=audio 0 RTP/AVP %u\n"
      "a=control:trackID=0\n";
    //#####LATER: Support video as well; multiple tracks; other codecs #####
    unsigned destSDPFmtSize = strlen(destSDPFmt)
      + 20 /* max int len */ + 20 + strlen(ourIPAddressStr)
      + strlen(progName)
      + strlen(serverIPAddressStr)
      + 3 /* max char len */;
    char* destSDPDescription = new char[destSDPFmtSize];
    sprintf(destSDPDescription, destSDPFmt,
	    our_random(), our_random(), ourIPAddressStr,
	    progName,
	    serverIPAddressStr,
	    desiredAudioRTPPayloadFormat);
    Boolean announceResult;
    if (username != NULL) {
      announceResult
	= rtspClientOutgoing->announceWithPassword(destRTSPURL,
						   destSDPDescription,
						   username, password);
    } else {
      announceResult
	= rtspClientOutgoing->announceSDPDescription(destRTSPURL,
						     destSDPDescription);
    }
    delete serverIPAddressStr; delete ourIPAddressStr;
    if (!announceResult) break;
    
    // Then, create a "MediaSession" object from this SDP description:
    MediaSession* destSession
      = MediaSession::createNew(*env, destSDPDescription);
    delete destSDPDescription;
    if (destSession == NULL) break;

    // Initiate, setup and play "destSession".
    // ##### TEMP HACK - take advantage of the fact that we have
    // ##### a single audio session only.
    MediaSubsession* destSubsession;
    PrioritizedRTPStreamSelector* multiSource;
    int multiSourceSessionId;
    char const* mimeType
      = desiredAudioRTPPayloadFormat == 0 ? "audio/PCMU" 
      : desiredAudioRTPPayloadFormat == 3 ? "audio/GSM"
      : "audio/???"; //##### FIX
    if (!destSession->initiateByMediaType(mimeType, destSubsession,
					  multiSource,
					  multiSourceSessionId)) break;
    if (!rtspClientOutgoing->setupMediaSubsession(*destSubsession,
						  True, True)) break;
    if (!rtspClientOutgoing->playMediaSubsession(*destSubsession,
						 True/*hackForDSS*/)) break;

    // Next, set up "RTPSink"s for the outgoing packets:
    struct in_addr destAddr; destAddr.s_addr = 0; // because we're using TCP
    Groupsock* destGS = new Groupsock(*env, destAddr, 0/*aud*/, 255);
    if (destGS == NULL) break;
    RTPSink* destRTPSink = NULL;
    if (desiredAudioRTPPayloadFormat == 0) {
      destRTPSink = SimpleRTPSink::createNew(*env, destGS, 0, 8000,
					     "audio", "PCMU");
    } else if (desiredAudioRTPPayloadFormat == 3) {
      destRTPSink = GSMAudioRTPSink::createNew(*env, destGS);
    }
    if (destRTPSink == NULL) break;

    // Tell the sink to stream using TCP:
    destRTPSink->setStreamSocket(rtspClientOutgoing->socketNum(), 0/*aud*/);
    // LATER: set up RTCPInstance also #####

    // Next, set up RTPTranslator(s) between source(s) and destination(s),
    // and start playing them.
    MediaSubsessionIterator iter(*session);
    MediaSubsession *sourceSubsession = NULL;
    while ((sourceSubsession = iter.next()) != NULL) {
      if (strcmp(sourceSubsession->mediumName(), "audio") == 0) break;
    }
    if (sourceSubsession == NULL) break;
    RTPTranslator* rtpTranslator
      = RTPTranslator::createNew(*env, sourceSubsession->readSource());
    if (rtpTranslator == NULL) break;
    destRTPSink->startPlaying(*rtpTranslator,
			      subsessionAfterPlaying, sourceSubsession);
    
    // LATER: delete media on close #####

    return True;
  } while (0);

  return False;
}
