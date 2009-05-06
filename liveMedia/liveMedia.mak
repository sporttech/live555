INCLUDES = -Iinclude -I../UsageEnvironment/include -I../groupsock/include
##### Change the following for your environment: 
# Comment out the following line to produce Makefiles that generate debuggable code:
NODEBUG=1

# The following definition ensures that we link with "wsock32.lib"
# rather than "ws2_32.lib".  For some reason, the standard Berkeley
# socket calls for multicast don't work properly with "ws2_32.lib".
TARGETOS = WIN95

!include    <ntwin32.mak>

UI_OPTS =		$(guilflags) $(guilibsdll)
# Use the following to get a console (e.g., for debugging):
CONSOLE_UI_OPTS =		$(conlflags) $(conlibsdll)
CPU=i386

TOOLS32	=		c:\Program Files\DevStudio\Vc
COMPILE_OPTS =		$(INCLUDES) $(cdebug) $(cflags) $(cvarsdll) -I. -I"$(TOOLS32)\include"
C =			c
C_COMPILER =		"$(TOOLS32)\bin\cl"
C_FLAGS =		$(COMPILE_OPTS)
CPP =			cpp
CPLUSPLUS_COMPILER =	$(C_COMPILER)
CPLUSPLUS_FLAGS =	$(COMPILE_OPTS)
OBJ =			obj
LINK =			$(link) -out:
LIBRARY_LINK =		lib -out:
LINK_OPTS_0 =		$(linkdebug) msvcirt.lib
LIBRARY_LINK_OPTS =	
LINK_OPTS =		$(LINK_OPTS_0) $(UI_OPTS)
CONSOLE_LINK_OPTS =	$(LINK_OPTS_0) $(CONSOLE_UI_OPTS)
SERVICE_LINK_OPTS =     kernel32.lib advapi32.lib shell32.lib -subsystem:console,$(APPVER)
LIB_SUFFIX =		lib
LIBS_FOR_CONSOLE_APPLICATION =
LIBS_FOR_GUI_APPLICATION =
MULTIMEDIA_LIBS =	winmm.lib
EXE =			.exe

rc32 = "$(TOOLS32)\bin\rc"
.rc.res:
	$(rc32) $<
##### End of variables to change

LIVEMEDIA_LIB = libliveMedia.$(LIB_SUFFIX)
ALL = $(LIVEMEDIA_LIB)
all:	$(ALL)

.$(C).$(OBJ):
	$(C_COMPILER) -c $(C_FLAGS) $<       

.$(CPP).$(OBJ):
	$(CPLUSPLUS_COMPILER) -c $(CPLUSPLUS_FLAGS) $<

MP3_SOURCE_OBJS = MP3FileSource.$(OBJ) MP3HTTPSource.$(OBJ) MP3Transcoder.$(OBJ) MP3ADU.$(OBJ) MP3ADUdescriptor.$(OBJ) MP3ADUinterleaving.$(OBJ) MP3ADUTranscoder.$(OBJ) MP3StreamState.$(OBJ) MP3Internals.$(OBJ) MP3InternalsHuffman.$(OBJ) MP3InternalsHuffmanTable.$(OBJ) MP3ADURTPSource.$(OBJ)
MPEG_SOURCE_OBJS = MPEGDemux.$(OBJ) MPEGDemuxedElementaryStream.$(OBJ) MPEGVideoStreamFramer.$(OBJ) MPEGAudioStreamFramer.$(OBJ) MPEGAudioRTPSource.$(OBJ) MPEG4LATMAudioRTPSource.$(OBJ) $(MP3_SOURCE_OBJS) MPEGVideoRTPSource.$(OBJ)
MP3_SINK_OBJS = MP3ADURTPSink.$(OBJ)
MPEG_SINK_OBJS = MPEGAudioRTPSink.$(OBJ) $(MP3_SINK_OBJS) MPEGVideoRTPSink.$(OBJ) MPEGVideoHTTPSink.$(OBJ)

MISC_SOURCE_OBJS = MediaSource.$(OBJ) FramedSource.$(OBJ) FramedFileSource.$(OBJ) FramedFilter.$(OBJ) ByteStreamFileSource.$(OBJ) BasicUDPSource.$(OBJ) DeviceSource.$(OBJ) $(MPEG_SOURCE_OBJS)
MISC_SINK_OBJS = MediaSink.$(OBJ) FileSink.$(OBJ) HTTPSink.$(OBJ) $(MPEG_SINK_OBJS) GSMAudioRTPSink.$(OBJ) H263plusVideoRTPSink.$(OBJ) SimpleRTPSink.$(OBJ)

RTP_SOURCE_OBJS = RTPSource.$(OBJ) MultiFramedRTPSource.$(OBJ) SimpleRTPSource.$(OBJ) PrioritizedRTPStreamSelector.$(OBJ) H263plusVideoRTPSource.$(OBJ) QCELPAudioRTPSource.$(OBJ)
RTP_SINK_OBJS = RTPSink.$(OBJ) MultiFramedRTPSink.$(OBJ)
RTP_INTERFACE_OBJS = RTPInterface.$(OBJ)
RTP_OBJS = $(RTP_SOURCE_OBJS) $(RTP_SINK_OBJS) $(RTP_INTERFACE_OBJS)

RTCP_OBJS = RTCP.$(OBJ) rtcp_from_spec.$(OBJ)
RTSP_OBJS = RTSPServer.$(OBJ) RTSPClient.$(OBJ)
SIP_OBJS = SIPClient.$(OBJ)

SESSION_OBJS = MediaSession.$(OBJ) ServerMediaSession.$(OBJ) PassiveServerMediaSession.$(OBJ)

QUICKTIME_OBJS = QuickTimeFileSink.$(OBJ) QuickTimeGenericRTPSource.$(OBJ)

MISC_OBJS = BitVector.$(OBJ) StreamParser.$(OBJ) our_md5.$(OBJ) our_md5hl.$(OBJ)

LIVEMEDIA_LIB_OBJS = Media.$(OBJ) $(MISC_SOURCE_OBJS) $(MISC_SINK_OBJS) $(RTP_OBJS) $(RTCP_OBJS) $(RTSP_OBJS) $(SIP_OBJS) $(SESSION_OBJS) $(QUICKTIME_OBJS) $(MISC_OBJS)

$(LIVEMEDIA_LIB): $(LIVEMEDIA_LIB_OBJS) \
    $(PLATFORM_SPECIFIC_LIB_OBJS)
	$(LIBRARY_LINK)$@ $(LIBRARY_LINK_OPTS) \
		$(LIVEMEDIA_LIB_OBJS)

Media.$(CPP):		include/Media.hh
include/Media.hh:	include/liveMedia_version.hh
MediaSource.$(CPP):	include/MediaSource.hh
include/MediaSource.hh:		include/Media.hh
FramedSource.$(CPP):	include/FramedSource.hh
include/FramedSource.hh:	include/MediaSource.hh
FramedFileSource.$(CPP): include/FramedFileSource.hh
include/FramedFileSource.hh:	include/FramedSource.hh
FramedFilter.$(CPP):	include/FramedFilter.hh
include/FramedFilter.hh:	include/FramedSource.hh
RTPSource.$(CPP):	include/RTPSource.hh
include/RTPSource.hh:		include/FramedSource.hh include/RTPInterface.hh
include/RTPInterface.hh:	include/Media.hh
MultiFramedRTPSource.$(CPP):	include/MultiFramedRTPSource.hh
include/MultiFramedRTPSource.hh:	include/RTPSource.hh
SimpleRTPSource.$(CPP):	include/SimpleRTPSource.hh
include/SimpleRTPSource.hh:	include/MultiFramedRTPSource.hh
include/PrioritizedRTPStreamSelector.hh:        include/RTCP.hh
PrioritizedRTPStreamSelector.$(CPP):    include/PrioritizedRTPStreamSelector.hh
H263plusVideoRTPSource.$(CPP):	include/H263plusVideoRTPSource.hh
include/H263plusVideoRTPSource.hh:	include/MultiFramedRTPSource.hh
QCELPAudioRTPSource.$(CPP):	include/QCELPAudioRTPSource.hh include/MultiFramedRTPSource.hh include/FramedFilter.hh
include/QCELPAudioRTPSource.hh:		include/RTPSource.hh
ByteStreamFileSource.$(CPP):	include/ByteStreamFileSource.hh
include/ByteStreamFileSource.hh:	include/FramedFileSource.hh
BasicUDPSource.$(CPP):		include/BasicUDPSource.hh
include/BasicUDPSource.hh:	include/FramedSource.hh
DeviceSource.$(CPP):	include/DeviceSource.hh include/FramedSource.hh
include/DeviceSource.hh:	include/FramedSource.hh
MPEGDemux.$(CPP):	include/MPEGDemux.hh include/MPEGDemuxedElementaryStream.hh StreamParser.hh
include/MPEGDemux.hh:		include/FramedSource.hh
include/MPEGDemuxedElementaryStream.hh:	include/MPEGDemux.hh
StreamParser.hh:	include/FramedSource.hh
MPEGDemuxedElementaryStream.$(CPP):	include/MPEGDemuxedElementaryStream.hh
MPEGVideoStreamFramer.$(CPP):	include/MPEGVideoStreamFramer.hh StreamParser.hh
include/MPEGVideoStreamFramer.hh:	include/FramedFilter.hh
MPEGAudioStreamFramer.$(CPP):	include/MPEGAudioStreamFramer.hh StreamParser.hh MP3Internals.hh
include/MPEGAudioStreamFramer.hh:	include/FramedFilter.hh
MPEGAudioRTPSource.$(CPP):	include/MPEGAudioRTPSource.hh
include/MPEGAudioRTPSource.hh:	include/MultiFramedRTPSource.hh
MPEG4LATMAudioRTPSource.$(CPP):	include/MPEG4LATMAudioRTPSource.hh
include/MPEG4LATMAudioRTPSource.hh:	include/MultiFramedRTPSource.hh
MP3FileSource.$(CPP):	include/MP3FileSource.hh MP3StreamState.hh
include/MP3FileSource.hh:	include/FramedFileSource.hh
MP3StreamState.hh:	MP3Internals.hh
MP3Internals.hh:	BitVector.hh
MP3HTTPSource.$(CPP):	include/MP3HTTPSource.hh MP3StreamState.hh
include/MP3HTTPSource.hh:	include/MP3FileSource.hh
MP3Transcoder.$(CPP):	include/MP3ADU.hh include/MP3Transcoder.hh
include/MP3ADU.hh:		include/FramedFilter.hh
include/MP3Transcoder.hh:	include/MP3ADU.hh include/MP3ADUTranscoder.hh
include/MP3ADUTranscoder.hh:	include/FramedFilter.hh
MP3ADU.$(CPP):		include/MP3ADU.hh MP3ADUdescriptor.hh MP3Internals.hh
MP3ADUdescriptor.$(CPP):	MP3ADUdescriptor.hh
MP3ADUinterleaving.$(CPP):	include/MP3ADUinterleaving.hh MP3ADUdescriptor.hh
include/MP3ADUinterleaving.hh:	include/FramedFilter.hh
MP3ADUTranscoder.$(CPP):	include/MP3ADUTranscoder.hh MP3Internals.hh
MP3StreamState.$(CPP):	MP3StreamState.hh
MP3Internals.$(CPP):	MP3InternalsHuffman.hh
MP3InternalsHuffman.hh:	MP3Internals.hh
MP3InternalsHuffman.$(CPP):	MP3InternalsHuffman.hh
MP3InternalsHuffmanTable.$(CPP):	MP3InternalsHuffman.hh
MP3ADURTPSource.$(CPP):	include/MP3ADURTPSource.hh MP3ADUdescriptor.hh
include/MP3ADURTPSource.hh:	include/MultiFramedRTPSource.hh
MPEGVideoRTPSource.$(CPP):	include/MPEGVideoRTPSource.hh
include/MPEGVideoRTPSource.hh:	include/MultiFramedRTPSource.hh
MediaSink.$(CPP):	include/MediaSink.hh
include/MediaSink.hh:		include/FramedSource.hh
FileSink.$(CPP):	include/FileSink.hh
include/FileSink.hh:		include/MediaSink.hh
HTTPSink.$(CPP):	include/HTTPSink.hh
include/HTTPSink.hh:		include/MediaSink.hh
RTPSink.$(CPP):		include/RTPSink.hh
include/RTPSink.hh:		include/MediaSink.hh include/RTPInterface.hh
MultiFramedRTPSink.$(CPP):	include/MultiFramedRTPSink.hh
include/MultiFramedRTPSink.hh:		include/RTPSink.hh
RTPInterface.$(CPP):		include/RTPInterface.hh
MPEGAudioRTPSink.$(CPP):	include/MPEGAudioRTPSink.hh
include/MPEGAudioRTPSink.hh:	include/MultiFramedRTPSink.hh
MP3ADURTPSink.$(CPP):	include/MP3ADURTPSink.hh
include/MP3ADURTPSink.hh:	include/MultiFramedRTPSink.hh
MPEGVideoRTPSink.$(CPP):	include/MPEGVideoRTPSink.hh include/MPEGVideoStreamFramer.hh
include/MPEGVideoRTPSink.hh:	include/MultiFramedRTPSink.hh
MPEGVideoHTTPSink.$(CPP):	include/MPEGVideoHTTPSink.hh
include/MPEGVideoHTTPSink.hh:	include/HTTPSink.hh
GSMAudioRTPSink.$(CPP):		include/GSMAudioRTPSink.hh
include/GSMAudioRTPSink.hh:	include/MultiFramedRTPSink.hh
H263plusVideoRTPSink.$(CPP):	include/H263plusVideoRTPSink.hh
include/H263plusVideoRTPSink.hh:	include/MultiFramedRTPSink.hh
SimpleRTPSink.$(CPP):		include/SimpleRTPSink.hh
include/SimpleRTPSink.hh:	include/MultiFramedRTPSink.hh
RTCP.$(CPP):		include/RTCP.hh rtcp_from_spec.h
include/RTCP.hh:		include/RTPSink.hh include/RTPSource.hh
rtcp_from_spec.$(C):	rtcp_from_spec.h
RTSPServer.$(CPP):	include/RTSPServer.hh
include/RTSPServer.hh:		include/ServerMediaSession.hh
include/ServerMediaSession.hh:	include/RTPSink.hh
RTSPClient.$(CPP):	include/RTSPClient.hh our_md5.h
include/RTSPClient.hh:		include/MediaSession.hh
SIPClient.$(CPP):	include/SIPClient.hh our_md5.h
include/SIPClient.hh:		include/MediaSession.hh include/RTSPClient.hh
MediaSession.$(CPP):	include/liveMedia.hh
include/MediaSession.hh:	include/RTCP.hh
ServerMediaSession.$(CPP):	include/ServerMediaSession.hh
PassiveServerMediaSession.$(CPP):	include/PassiveServerMediaSession.hh
include/PassiveServerMediaSession.hh:	include/ServerMediaSession.hh
QuickTimeFileSink.$(CPP):	include/QuickTimeFileSink.hh include/H263plusVideoRTPSource.hh
include/QuickTimeFileSink.hh:	include/MediaSession.hh
QuickTimeGenericRTPSource.$(CPP):	include/QuickTimeGenericRTPSource.hh
include/QuickTimeGenericRTPSource.hh:	include/MultiFramedRTPSource.hh
BitVector.$(CPP):	BitVector.hh
StreamParser.$(CPP):	StreamParser.hh
our_md5.$(C):		our_md5.h
our_md5hl.$(C):		our_md5.h

include/liveMedia.hh:	include/MPEGAudioRTPSink.hh include/MP3ADURTPSink.hh include/FileSink.hh include/MPEGVideoHTTPSink.hh include/GSMAudioRTPSink.hh include/H263plusVideoRTPSink.hh include/SimpleRTPSink.hh include/ByteStreamFileSource.hh include/MPEGAudioRTPSource.hh include/MP3ADURTPSource.hh include/QCELPAudioRTPSource.hh include/MPEGVideoRTPSource.hh include/H263plusVideoRTPSource.hh include/MP3HTTPSource.hh include/MP3ADU.hh include/MP3ADUinterleaving.hh include/MP3Transcoder.hh include/MPEGDemuxedElementaryStream.hh include/MPEGAudioStreamFramer.hh include/MPEGVideoStreamFramer.hh include/DeviceSource.hh include/PrioritizedRTPStreamSelector.hh include/RTSPServer.hh include/RTSPClient.hh include/SIPClient.hh include/MediaSession.hh include/QuickTimeFileSink.hh include/QuickTimeGenericRTPSource.hh include/PassiveServerMediaSession.hh

clean:
	-rm -rf *.$(OBJ) $(ALL) core *.core *~ include/*~

##### Any additional, platform-specific rules come here:
