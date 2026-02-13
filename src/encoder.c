#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include "logger.h"

typedef struct {
  AVCodecContext *codec_ctx;
  AVFrame *frame;
  AVPacket *pkt;
  struct SwsContext *sws_ctx;
  FILE *out_file;
} EncoderState;

EncoderState g_enc = {0};

void init_encoder(const char *filename, int width, int height) {
    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    g_enc.codec_ctx = avcodec_alloc_context3(codec);

    g_enc.codec_ctx->width = width;
    g_enc.codec_ctx->height = height;
    g_enc.codec_ctx->time_base = (AVRational){1, 60}; // 60 FPS
    g_enc.codec_ctx->framerate = (AVRational){60, 1};
    g_enc.codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;

    avcodec_open2(g_enc.codec_ctx, codec, NULL);

    g_enc.frame = av_frame_alloc();
    g_enc.frame->format = g_enc.codec_ctx->pix_fmt;
    g_enc.frame->width = width;
    g_enc.frame->height = height;
    av_frame_get_buffer(g_enc.frame, 0);

    g_enc.pkt = av_packet_alloc();

    // Setup BGRA to YUV420P converter
    g_enc.sws_ctx = sws_getContext(width, height, AV_PIX_FMT_BGRA,
                                   width, height, AV_PIX_FMT_YUV420P,
                                   SWS_BICUBIC, NULL, NULL, NULL);

    g_enc.out_file = fopen(filename, "wb");
    log_info("Encoder initialized: %s", filename);
}

void encode_frame(unsigned char *bgra_data) {
    const uint8_t *src_data[1] = { bgra_data };
    int src_linesize[1] = { 4 * g_enc.codec_ctx->width };
    sws_scale(g_enc.sws_ctx, src_data, src_linesize, 0, g_enc.codec_ctx->height,
              g_enc.frame->data, g_enc.frame->linesize);

    avcodec_send_frame(g_enc.codec_ctx, g_enc.frame);
    
    while (avcodec_receive_packet(g_enc.codec_ctx, g_enc.pkt) == 0) {
        fwrite(g_enc.pkt->data, 1, g_enc.pkt->size, g_enc.out_file);
        av_packet_unref(g_enc.pkt);
    }
}