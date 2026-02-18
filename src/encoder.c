#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include "logger.h"
#include <Windows.h>

typedef struct {
  AVCodecContext *codec_ctx;
  AVFormatContext *fmt_ctx;
  AVStream *video_stream;
  AVFrame *frame;
  AVPacket *pkt;
  struct SwsContext *sws_ctx;
//   FILE *out_file;
  int64_t frame_count; 
} EncoderState;

EncoderState g_enc = {0};

void init_encoder(const char *filename, int width, int height) {
    HRESULT hr;

    avformat_alloc_output_context2(&g_enc.fmt_ctx, NULL, NULL, filename);
    if (!g_enc.fmt_ctx) {
        log_error("Could not allocate output context");
        return;
    }

    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        log_error("Codec H.264 not found");
        return;
    }

    g_enc.video_stream = avformat_new_stream(g_enc.fmt_ctx, NULL);
    if (!g_enc.video_stream) {
        log_error("Could not create stream");
        return;
    }

    g_enc.codec_ctx = avcodec_alloc_context3(codec);
    g_enc.codec_ctx->width = width;
    g_enc.codec_ctx->height = height;
    g_enc.codec_ctx->time_base = (AVRational){1, 60};
    g_enc.codec_ctx->framerate = (AVRational){60, 1};
    g_enc.codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    g_enc.codec_ctx->gop_size = 12; // Add a GOP size for stability

    if (g_enc.fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        g_enc.codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if (avcodec_open2(g_enc.codec_ctx, codec, NULL) < 0) {
        log_error("Could not open codec");
        return;
    }

    avcodec_parameters_from_context(g_enc.video_stream->codecpar, g_enc.codec_ctx);
    g_enc.video_stream->time_base = g_enc.codec_ctx->time_base;

    if (!(g_enc.fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&g_enc.fmt_ctx->pb, filename, AVIO_FLAG_WRITE) < 0) {
            log_error("Could not open file: %s", filename);
            return;
        }
    }

    if (avformat_write_header(g_enc.fmt_ctx, NULL) < 0) {
        log_error("Error occurred when opening output file");
        return;
    }

    g_enc.frame = av_frame_alloc();
    g_enc.frame->format = g_enc.codec_ctx->pix_fmt;
    g_enc.frame->width = width;
    g_enc.frame->height = height;
    av_frame_get_buffer(g_enc.frame, 0);

    g_enc.pkt = av_packet_alloc();
    g_enc.sws_ctx = sws_getContext(width, height, AV_PIX_FMT_BGRA,
                                   width, height, AV_PIX_FMT_YUV420P,
                                   SWS_BICUBIC, NULL, NULL, NULL);
    g_enc.frame_count = 0;
    
    log_info("Muxer and Encoder initialized: %s", filename);
}

void encode_frame(unsigned char *bgra_data) {
    const uint8_t *src_data[1] = { bgra_data };
    int src_linesize[1] = { 4 * g_enc.codec_ctx->width };
    sws_scale(g_enc.sws_ctx, src_data, src_linesize, 0, g_enc.codec_ctx->height,
              g_enc.frame->data, g_enc.frame->linesize);
    g_enc.frame->pts = g_enc.frame_count++;
    
     if (avcodec_send_frame(g_enc.codec_ctx, g_enc.frame) >= 0) {
        while (avcodec_receive_packet(g_enc.codec_ctx, g_enc.pkt) >= 0) {
            av_packet_rescale_ts(g_enc.pkt, g_enc.codec_ctx->time_base, g_enc.video_stream->time_base);
            g_enc.pkt->stream_index = g_enc.video_stream->index;

            av_interleaved_write_frame(g_enc.fmt_ctx, g_enc.pkt);
            av_packet_unref(g_enc.pkt);
        }
    }
}

void cleanup_encoder() {
    avcodec_send_frame(g_enc.codec_ctx, NULL);
    av_write_trailer(g_enc.fmt_ctx);
    avio_closep(&g_enc.fmt_ctx->pb);
    avformat_free_context(g_enc.fmt_ctx);
}