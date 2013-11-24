/*
 * Copyright 2012 Michael Chen <omxcodec@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SOFT_FFMPEGAUDIO_H_
#define SOFT_FFMPEGAUDIO_H_

#include "SimpleSoftOMXComponent.h"

#ifndef __GNUC__
#error "__GNUC__ cflags should be enabled"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <math.h>
#include <limits.h> /* INT_MAX */

#include "config.h"
#include "libavutil/avstring.h"
#include "libavutil/colorspace.h"
#include "libavutil/mathematics.h"
#include "libavutil/pixdesc.h"
#include "libavutil/imgutils.h"
#include "libavutil/parseutils.h"
#include "libavutil/avassert.h"
#include "libavutil/intreadwrite.h"
#include "libavutil/opt.h"
#include "libavutil/internal.h"
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#include "libavcodec/avfft.h"
#include "libswresample/swresample.h"

#ifdef __cplusplus
}
#endif

#include "utils/ffmpeg_utils.h"

const int AVCODEC_MAX_AUDIO_FRAME_SIZE = 192000;

namespace android {

struct SoftFFmpegAudio : public SimpleSoftOMXComponent {

    SoftFFmpegAudio(const char *name, const OMX_CALLBACKTYPE *callbacks, OMX_PTR appData,
                                                                         OMX_COMPONENTTYPE **component);

protected:
    virtual ~SoftFFmpegAudio();

    virtual OMX_ERRORTYPE internalGetParameter(OMX_INDEXTYPE index, OMX_PTR params);
    virtual OMX_ERRORTYPE internalSetParameter(OMX_INDEXTYPE index, const OMX_PTR params);

    virtual void onQueueFilled(OMX_U32 portIndex);
    virtual void onPortFlushCompleted(OMX_U32 portIndex);
    virtual void onPortEnableCompleted(OMX_U32 portIndex, bool enabled);

private:
    enum {
        kInputPortIndex   = 0,
        kOutputPortIndex  = 1,
        kNumInputBuffers  = 4,
        kNumOutputBuffers = 4,
        kOutputBufferSize = 4608 * 2
    };

    enum {
        MODE_NONE,
        MODE_AAC,
        MODE_MPEG,
        MODE_VORBIS,
        MODE_WMA,
        MODE_RA,
        MODE_FLAC,
        MODE_MPEGL1,
        MODE_MPEGL2,
        MODE_AC3,
        MODE_APE,
        MODE_DTS,
        MODE_HEURISTIC
    } mMode;

    enum EOSStatus {
        INPUT_DATA_AVAILABLE,
        INPUT_EOS_SEEN,
        OUTPUT_FRAMES_FLUSHED,
    };

    enum {
        ERR_NO_FRM              = 2,
        ERR_FLUSHED             = 1,
        ERR_OK                  = 0,
        ERR_OOM                 = -1,
        ERR_INVALID_PARAM       = -2,
        ERR_CODEC_NOT_FOUND     = -3,
        ERR_DECODER_OPEN_FAILED = -4,
        ERR_SWR_INIT_FAILED     = -5,
        ERR_RESAMPLE_FAILED     = -6
    };

    bool mFFmpegAlreadyInited;
    bool mCodecAlreadyOpened;
    bool mExtradataReady;
    bool mIgnoreExtradata;
    AVCodecContext *mCtx;
    struct SwrContext *mSwrCtx;
    AVFrame *mFrame;

    uint8_t *mVorbisHeaderStart[3];
    int mVorbisHeaderLen[3];

    EOSStatus mEOSStatus;

    bool mSignalledError;

    int64_t mAudioClock;
    int32_t mInputBufferSize;

    DECLARE_ALIGNED(16, uint8_t, mAudioBuffer)[AVCODEC_MAX_AUDIO_FRAME_SIZE * 4];

    uint8_t mSilenceBuffer[kOutputBufferSize];
    uint8_t *mResampledData;
    int32_t mResampledDataSize;

    int mAudioSrcFreq;
    int mAudioTgtFreq;
    int mAudioSrcChannels;
    int mAudioTgtChannels;
    int64_t mAudioSrcChannelLayout;
    int64_t mAudioTgtChannelLayout;
    enum AVSampleFormat mAudioSrcFmt;
    enum AVSampleFormat mAudioTgtFmt;

    enum {
        NONE,
        AWAITING_DISABLED,
        AWAITING_ENABLED
    } mOutputPortSettingsChange;

    void setMode(const char *name);
    void initInputFormat(uint32_t mode, OMX_PARAM_PORTDEFINITIONTYPE &def);
    void setDefaultCtx(AVCodecContext *avctx, const AVCodec *codec);
    void resetCtx();
    OMX_ERRORTYPE isRoleSupported(const OMX_PARAM_COMPONENTROLETYPE *roleParams);
    void adjustAudioParams();

    void initPorts();
    status_t initDecoder();
    void deInitDecoder();

    void    initVorbisHdr();
    void    deinitVorbisHdr();
    int32_t handleExtradata();
    int32_t handleVorbisExtradata(OMX_BUFFERHEADERTYPE *inHeader);
    int32_t openDecoder();
    void    updateTimeStamp(OMX_BUFFERHEADERTYPE *inHeader);
    void    initPacket(AVPacket *pkt, OMX_BUFFERHEADERTYPE *inHeader);
    int32_t decodeAudio();
    int32_t resampleAudio();
    void    drainOneOutputBuffer();
    void    drainEOSOutputBuffer();
    void    drainAllOutputBuffers();

    DISALLOW_EVIL_CONSTRUCTORS(SoftFFmpegAudio);
};

} // namespace android

#endif  // SOFT_FFMPEGAUDIO_H_
