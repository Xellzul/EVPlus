/* media.cpp -- media decoding for the movies and songs (movie.cpp) and QuickTime-compressed PICTs, on FFmpeg (LGPL, DLLs).
 *
 * MediaFile: the container and the video decoder of one movie, used only from the game thread (MovieDraw).
 * MediaAudio: a second demuxer + decoder of the same file for the audio, used only from the audio output thread
 *          (under its lock), so the two never share an AVFormatContext.
 * MediaDecodeImage: one picture of a QuickTime-compressed PICT (burgerpict.cpp), decoded without a container.
 */
#include <stdlib.h>
#include <string.h>
extern "C" {                        /* FFmpeg is C */
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
}
#include "media.h"
#include "platform.h"

#define kNoTime INT64_MIN

struct MediaFile {
    AVFormatContext *fmt;
    int vstream;                 /* -1: no decodable video */
    int astream;                 /* -1: no audio */
    AVCodecContext *vdec;
    AVPacket *pkt;
    AVFrame *cur;                /* the frame on screen at curMs */
    AVFrame *next;               /* decoded ahead, shown from nextMs */
    int64_t curMs, nextMs, drawnMs;
    int haveCur, haveNext, eof;
    int width, height;
    int64_t durationMs;
    struct SwsContext *sws;
    uint8_t *scratch;            /* swscale writes past dstW on some widths: it scales here first */
    int scratchStride, scratchRows;
};

struct MediaAudio {
    AVFormatContext *fmt;
    int stream;
    AVCodecContext *dec;
    SwrContext *swr;
    AVPacket *pkt;
    AVFrame *frame;
    int16_t *buf;                /* converted samples, interleaved stereo */
    int bufCap, bufLen, bufPos;  /* in frames */
    int eof, flushed;
};

static AVCodecContext *OpenDecoder(AVStream *st)
{
    const AVCodec *codec = avcodec_find_decoder(st->codecpar->codec_id);
    AVCodecContext *c;

    if (codec == NULL)
        return NULL;
    c = avcodec_alloc_context3(codec);
    if (c == NULL)
        return NULL;
    if (avcodec_parameters_to_context(c, st->codecpar) < 0) {
        avcodec_free_context(&c);
        return NULL;
    }
    c->pkt_timebase = st->time_base;
    c->thread_count = 1;
    if (avcodec_open2(c, codec, NULL) < 0) {
        avcodec_free_context(&c);
        return NULL;
    }
    return c;
}

static int64_t FrameMs(const AVFrame *f, AVRational tb)
{
    int64_t pts = f->best_effort_timestamp;

    if (pts == AV_NOPTS_VALUE)
        pts = f->pts;
    if (pts == AV_NOPTS_VALUE)
        return 0;
    return av_rescale_q(pts, tb, (AVRational){ 1, 1000 });
}

/* the stream that is a real picture track: not ID3 cover art or a poster */
static int FindVideo(AVFormatContext *fmt)
{
    unsigned i;

    for (i = 0; i < fmt->nb_streams; i++) {
        AVStream *st = fmt->streams[i];
        if (st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO &&
            (st->disposition & AV_DISPOSITION_ATTACHED_PIC) == 0)
            return (int)i;
    }
    return -1;
}

static int FindAudio(AVFormatContext *fmt)
{
    unsigned i;

    for (i = 0; i < fmt->nb_streams; i++)
        if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
            return (int)i;
    return -1;
}

static int OpenInput(AVFormatContext **fmt, const char *path)
{
    av_log_set_level(AV_LOG_QUIET);
    return avformat_open_input(fmt, path, NULL, NULL);      /* the game's paths are UTF-8, as FFmpeg's */
}

MediaFile *MediaOpen(const char *path)
{
    MediaFile *m = (MediaFile *)calloc(1, sizeof *m);
    AVStream *st;

    if (m == NULL)
        return NULL;
    if (OpenInput(&m->fmt, path) < 0) {
        free(m);
        return NULL;
    }
    if (avformat_find_stream_info(m->fmt, NULL) < 0) {
        avformat_close_input(&m->fmt);
        free(m);
        return NULL;
    }
    m->vstream = FindVideo(m->fmt);
    m->astream = FindAudio(m->fmt);
    if (m->vstream < 0 && m->astream < 0) {
        avformat_close_input(&m->fmt);
        free(m);
        return NULL;
    }
    if (m->fmt->duration != AV_NOPTS_VALUE && m->fmt->duration > 0)
        m->durationMs = av_rescale(m->fmt->duration, 1000, AV_TIME_BASE);
    if (m->vstream >= 0) {
        st = m->fmt->streams[m->vstream];
        m->width = st->codecpar->width;
        m->height = st->codecpar->height;
        /* QuickTime's movie lasts as long as its longest track: the container's figure, unless the video
           track says longer */
        if (st->duration != AV_NOPTS_VALUE && st->duration > 0) {
            int64_t v = av_rescale_q(st->duration, st->time_base, (AVRational){ 1, 1000 });
            if (v > m->durationMs)
                m->durationMs = v;
        }
        /* a codec FFmpeg lacks: the movie still opens and plays its length, drawing nothing (QuickTime's
           behaviour with a missing decompressor) */
        m->vdec = OpenDecoder(st);
    }
    m->pkt = av_packet_alloc();
    m->cur = av_frame_alloc();
    m->next = av_frame_alloc();
    m->drawnMs = kNoTime;
    return m;
}

void MediaClose(MediaFile *m)
{
    if (m == NULL)
        return;
    sws_freeContext(m->sws);
    av_free(m->scratch);
    av_frame_free(&m->cur);
    av_frame_free(&m->next);
    av_packet_free(&m->pkt);
    avcodec_free_context(&m->vdec);
    avformat_close_input(&m->fmt);
    free(m);
}

int MediaHasVideo(const MediaFile *m) { return m != NULL && m->vstream >= 0; }
int MediaHasAudio(const MediaFile *m) { return m != NULL && m->astream >= 0; }
int64_t MediaDurationMs(const MediaFile *m) { return m != NULL ? m->durationMs : 0; }

void MediaVideoSize(const MediaFile *m, int *w, int *h)
{
    *w = m != NULL ? m->width : 0;
    *h = m != NULL ? m->height : 0;
}

/* decodes the next video frame into f; 0 at the end of the stream */
static int DecodeNext(MediaFile *m, AVFrame *f)
{
    int r;

    for (;;) {
        r = avcodec_receive_frame(m->vdec, f);
        if (r == 0)
            return 1;
        if (r == AVERROR_EOF)
            return 0;
        if (r != AVERROR(EAGAIN))
            return 0;
        if (m->eof)
            return 0;
        for (;;) {
            r = av_read_frame(m->fmt, m->pkt);
            if (r < 0) {
                m->eof = 1;
                avcodec_send_packet(m->vdec, NULL);
                break;
            }
            if (m->pkt->stream_index == m->vstream) {
                avcodec_send_packet(m->vdec, m->pkt);
                av_packet_unref(m->pkt);
                break;
            }
            av_packet_unref(m->pkt);
        }
    }
}

static void Rewind(MediaFile *m)
{
    av_seek_frame(m->fmt, m->vstream, 0, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(m->vdec);
    m->eof = 0;
    m->haveCur = m->haveNext = 0;
    m->drawnMs = kNoTime;
}

int MediaVideoDraw(MediaFile *m, int64_t ms, void *dst, int32_t rowBytes, int dstW, int dstH, int bigEndian,
                int force)
{
    AVRational tb;
    AVFrame *t;
    uint8_t *planes[4];
    int strides[4];

    if (m == NULL || m->vdec == NULL || dst == NULL || dstW <= 0 || dstH <= 0)
        return 0;
    tb = m->fmt->streams[m->vstream]->time_base;
    if (m->haveCur && ms < m->curMs)
        Rewind(m);
    for (;;) {
        if (!m->haveNext) {
            if (!DecodeNext(m, m->next))
                break;
            m->nextMs = FrameMs(m->next, tb);
            m->haveNext = 1;
        }
        if (m->haveCur && m->nextMs > ms)
            break;
        /* the frame due by now (or the very first one) goes on screen; later ones wait */
        t = m->cur;
        m->cur = m->next;
        m->next = t;
        av_frame_unref(m->next);
        m->curMs = m->nextMs;
        m->haveCur = 1;
        m->haveNext = 0;
    }
    if (!m->haveCur)
        return 0;
    if (!force && m->drawnMs == m->curMs)
        return 0;
    m->sws = sws_getCachedContext(m->sws, m->cur->width, m->cur->height, (enum AVPixelFormat)m->cur->format,
                                  dstW, dstH, bigEndian ? AV_PIX_FMT_RGB555BE : AV_PIX_FMT_RGB555LE,
                                  SWS_BILINEAR, NULL, NULL, NULL);
    if (m->sws == NULL)
        return 0;
    /* QuickTime 6 shows these YUV movies as if full range: measured on CE's Race 1.mov against its frames,
       that halves the difference (mean 0.57 of 31 per channel left; QuickTime's own converter is not matched) */
    if ((av_pix_fmt_desc_get((enum AVPixelFormat)m->cur->format)->flags & AV_PIX_FMT_FLAG_RGB) == 0) {
        int *inv, *table, srcRange, dstRange, brightness, contrast, saturation;
        if (sws_getColorspaceDetails(m->sws, &inv, &srcRange, &table, &dstRange, &brightness, &contrast,
                                     &saturation) >= 0)
            sws_setColorspaceDetails(m->sws, inv, 1, table, dstRange, brightness, contrast, saturation);
    }
    /* swscale may write up to a SIMD block past dstW on each row (odd widths, 410p, packed sources): scale into
       a padded buffer and copy exactly dstW pixels, so nothing right of the movie box is touched */
    {
        int stride = ((dstW * 2 + 63) & ~63) + 64, y;
        if (m->scratch == NULL || m->scratchStride != stride || m->scratchRows < dstH) {
            av_free(m->scratch);
            m->scratch = (uint8_t *)av_malloc((size_t)stride * (size_t)(dstH + 1));
            m->scratchStride = stride;
            m->scratchRows = m->scratch != NULL ? dstH : 0;
            if (m->scratch == NULL)
                return 0;
        }
        memset(planes, 0, sizeof planes);
        memset(strides, 0, sizeof strides);
        planes[0] = m->scratch;
        strides[0] = stride;
        sws_scale(m->sws, (const uint8_t *const *)m->cur->data, m->cur->linesize, 0, m->cur->height, planes,
                  strides);
        for (y = 0; y < dstH; y++)
            memcpy((uint8_t *)dst + (ptrdiff_t)y * rowBytes, m->scratch + (size_t)y * stride, (size_t)dstW * 2);
    }
    m->drawnMs = m->curMs;
    return 1;
}

MediaAudio *MediaAudioOpen(const char *path)
{
    MediaAudio *a = (MediaAudio *)calloc(1, sizeof *a);
    AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;

    if (a == NULL)
        return NULL;
    if (OpenInput(&a->fmt, path) < 0)
        goto fail;
    if (avformat_find_stream_info(a->fmt, NULL) < 0)
        goto fail;
    a->stream = FindAudio(a->fmt);
    if (a->stream < 0)
        goto fail;
    a->dec = OpenDecoder(a->fmt->streams[a->stream]);
    if (a->dec == NULL)
        goto fail;
    if (swr_alloc_set_opts2(&a->swr, &stereo, AV_SAMPLE_FMT_S16, MEDIA_AUDIO_RATE, &a->dec->ch_layout,
                            a->dec->sample_fmt, a->dec->sample_rate, 0, NULL) < 0 || swr_init(a->swr) < 0)
        goto fail;
    a->pkt = av_packet_alloc();
    a->frame = av_frame_alloc();
    return a;
fail:
    MediaAudioClose(a);
    return NULL;
}

void MediaAudioClose(MediaAudio *a)
{
    if (a == NULL)
        return;
    av_free(a->buf);
    av_frame_free(&a->frame);
    av_packet_free(&a->pkt);
    swr_free(&a->swr);
    avcodec_free_context(&a->dec);
    avformat_close_input(&a->fmt);
    free(a);
}

void MediaAudioRewind(MediaAudio *a)
{
    if (a == NULL)
        return;
    av_seek_frame(a->fmt, a->stream, 0, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(a->dec);
    swr_init(a->swr);
    a->eof = a->flushed = 0;
    a->bufLen = a->bufPos = 0;
}

static int Reserve(MediaAudio *a, int frames)
{
    if (frames <= a->bufCap)
        return 1;
    av_free(a->buf);
    a->buf = (int16_t *)av_malloc((size_t)frames * 2 * sizeof(int16_t));
    a->bufCap = a->buf != NULL ? frames : 0;
    return a->buf != NULL;
}

/* converts the next decoded frame (or the resampler's tail at the end) into buf; 0 when nothing is left */
static int Refill(MediaAudio *a)
{
    int r, n;
    uint8_t *out[1];

    a->bufLen = a->bufPos = 0;
    for (;;) {
        r = avcodec_receive_frame(a->dec, a->frame);
        if (r == 0) {
            n = swr_get_out_samples(a->swr, a->frame->nb_samples);
            if (!Reserve(a, n))
                return 0;
            out[0] = (uint8_t *)a->buf;
            n = swr_convert(a->swr, out, n, (const uint8_t **)a->frame->extended_data, a->frame->nb_samples);
            av_frame_unref(a->frame);
            if (n > 0) {
                a->bufLen = n;
                return 1;
            }
            continue;
        }
        if (r == AVERROR_EOF || (r != AVERROR(EAGAIN) && r != 0)) {
            if (a->flushed)
                return 0;
            a->flushed = 1;
            n = swr_get_out_samples(a->swr, 0);
            if (n <= 0 || !Reserve(a, n))
                return 0;
            out[0] = (uint8_t *)a->buf;
            n = swr_convert(a->swr, out, n, NULL, 0);
            if (n <= 0)
                return 0;
            a->bufLen = n;
            return 1;
        }
        if (a->eof) {
            avcodec_send_packet(a->dec, NULL);
            continue;
        }
        r = av_read_frame(a->fmt, a->pkt);
        if (r < 0) {
            a->eof = 1;
            avcodec_send_packet(a->dec, NULL);
            continue;
        }
        if (a->pkt->stream_index == a->stream)
            avcodec_send_packet(a->dec, a->pkt);
        av_packet_unref(a->pkt);
    }
}

int MediaAudioRead(MediaAudio *a, int16_t *out, int frames)
{
    int done = 0, n;

    if (a == NULL)
        return 0;
    while (done < frames) {
        if (a->bufPos >= a->bufLen && !Refill(a))
            break;
        n = a->bufLen - a->bufPos;
        if (n > frames - done)
            n = frames - done;
        memcpy(out + done * 2, a->buf + a->bufPos * 2, (size_t)n * 2 * sizeof(int16_t));
        a->bufPos += n;
        done += n;
    }
    return done;
}

static void CopyIndexed(const AVFrame *f, uint8_t *dst, int32_t rowBytes, int w, int h)
{
    int x, y;

    for (y = 0; y < h; y++) {
        const uint8_t *s = f->data[0] + (ptrdiff_t)y * f->linesize[0];
        uint8_t *d = dst + (ptrdiff_t)y * rowBytes;
        if (f->format == AV_PIX_FMT_MONOWHITE || f->format == AV_PIX_FMT_MONOBLACK) {
            /* Mac standard palette: index 0 white, 255 black */
            for (x = 0; x < w; x++) {
                int bit = (s[x >> 3] >> (7 - (x & 7))) & 1;
                if (f->format == AV_PIX_FMT_MONOBLACK)
                    bit ^= 1;
                d[x] = bit ? 0xff : 0x00;
            }
        } else if (f->format == AV_PIX_FMT_GRAY8) {
            /* QuickTime's gray tables run from white at index 0 to black at 255 */
            for (x = 0; x < w; x++)
                d[x] = (uint8_t)(255 - s[x]);
        } else {
            memcpy(d, s, (size_t)w);
        }
    }
}

int MediaDecodeImage(uint32_t cType, int width, int height, int depth, const void *data, int32_t size, void *dst,
                  int32_t rowBytes, int dstBits)
{
    const struct AVCodecTag *tags[2];
    enum AVCodecID id;
    const AVCodec *codec;
    AVCodecContext *c = NULL;
    AVPacket *pkt = NULL;
    AVFrame *f = NULL;
    struct SwsContext *sws = NULL;
    int err = -1;
    uint32_t tag;

    if (size <= 0 || width <= 0 || height <= 0)
        return -1;
    tag = ((cType >> 24) & 0xff) | ((cType >> 8) & 0xff00) | ((cType << 8) & 0xff0000) | (cType << 24);
    tags[0] = avformat_get_mov_video_tags();
    tags[1] = NULL;
    id = av_codec_get_id(tags, tag);
    codec = avcodec_find_decoder(id);
    if (codec == NULL)
        return -1;
    c = avcodec_alloc_context3(codec);
    pkt = av_packet_alloc();
    f = av_frame_alloc();
    if (c == NULL || pkt == NULL || f == NULL)
        goto done;
    c->width = width;
    c->height = height;
    c->bits_per_coded_sample = depth;
    c->codec_tag = tag;
    c->thread_count = 1;
    if (avcodec_open2(c, codec, NULL) < 0)
        goto done;
    if (av_new_packet(pkt, size) < 0)
        goto done;
    memcpy(pkt->data, data, (size_t)size);
    pkt->flags |= AV_PKT_FLAG_KEY;
    if (avcodec_send_packet(c, pkt) < 0)
        goto done;
    avcodec_send_packet(c, NULL);
    if (avcodec_receive_frame(c, f) < 0)
        goto done;
    if (f->width < width || f->height < height)
        goto done;
    if (dstBits == 8) {
        if (f->format != AV_PIX_FMT_PAL8 && f->format != AV_PIX_FMT_GRAY8 &&
            f->format != AV_PIX_FMT_MONOWHITE && f->format != AV_PIX_FMT_MONOBLACK)
            goto done;
        CopyIndexed(f, (uint8_t *)dst, rowBytes, width, height);
    } else {
        uint8_t *planes[4] = { (uint8_t *)dst, NULL, NULL, NULL };
        int strides[4] = { rowBytes, 0, 0, 0 };
        sws = sws_getContext(width, height, (enum AVPixelFormat)f->format, width, height,
                             dstBits == 16 ? AV_PIX_FMT_RGB555BE : AV_PIX_FMT_ARGB, SWS_POINT, NULL, NULL,
                             NULL);
        if (sws == NULL)
            goto done;
        sws_scale(sws, (const uint8_t *const *)f->data, f->linesize, 0, height, planes, strides);
    }
    err = 0;
done:
    sws_freeContext(sws);
    av_frame_free(&f);
    av_packet_free(&pkt);
    avcodec_free_context(&c);
    return err;
}
