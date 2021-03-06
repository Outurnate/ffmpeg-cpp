#include "FrameSinks/VideoEncoder.h"


#include "Codecs/VideoCodec.h"
#include "ffmpeg.h"
#include "FFmpegException.h"
#include "Muxing/Muxer.h"
#include "Muxing/VideoOutputStream.h"
#include "VideoFormatConverter.h"
#include "OpenCodec.h"

using namespace std;

namespace ffmpegcpp
{

	VideoEncoder::VideoEncoder(VideoCodec* codec, Muxer* muxer)
	{
		this->closedCodec = codec;

		// create an output stream
		this->output = std::make_unique<VideoOutputStream>(muxer, codec);
		muxer->AddOutputStream(output.get());
	}

	VideoEncoder::VideoEncoder(VideoCodec* codec, Muxer* muxer, AVPixelFormat format)
		: VideoEncoder(codec, muxer)
	{
		finalPixelFormat = format;
	}

	VideoEncoder::VideoEncoder(VideoCodec* codec, Muxer* muxer, AVRational frameRate)
		: VideoEncoder(codec, muxer)
	{
		finalFrameRate = frameRate;
		finalFrameRateSet = true;
	}

	VideoEncoder::VideoEncoder(VideoCodec* codec, Muxer* muxer, AVRational frameRate, AVPixelFormat format)
		: VideoEncoder(codec, muxer)
	{
		finalPixelFormat = format;
		finalFrameRate = frameRate;
		finalFrameRateSet = true;
	}

	void VideoEncoder::OpenLazily(AVFrame* frame, AVRational* timeBase)
	{
		// configure the parameters for the codec based on the frame and our preferences
		int width = frame->width;
		int height = frame->height;

		// the format is either the provided format, or the default format if it is not supported
		AVPixelFormat format = finalPixelFormat;
		if (!closedCodec->IsPixelFormatSupported(format)) format = closedCodec->GetDefaultPixelFormat();
		if (format == AV_PIX_FMT_NONE) format = closedCodec->GetDefaultPixelFormat();

		// the frame rate is either the input frame rate, OR the default frame rate if the input frame rate
		// is not supported, OR the explicitly chosen framerate.
		AVRational frameRate;
		frameRate.num = timeBase->den;
		frameRate.den = timeBase->num;
		if (!closedCodec->IsFrameRateSupported(&frameRate)) frameRate = closedCodec->GetClosestSupportedFrameRate(frameRate);
		if (finalFrameRateSet) frameRate = finalFrameRate;

		// open the codec
		codec = closedCodec->Open(width, height, &frameRate, format);
		
		// allocate the packet we'll be using
		pkt = MakeFFmpegResource<AVPacket>(av_packet_alloc());
		if (!pkt)
		{
			throw FFmpegException("Failed to allocate packet");
		}

		// set up the format converter
		try
		{
			formatConverter = std::make_unique<VideoFormatConverter>(codec->GetContext());
		}
		catch (FFmpegException e)
		{
			throw e;
		}
	}

	void VideoEncoder::WriteFrame(AVFrame* frame, AVRational* timeBase)
	{
		// if we haven't opened the codec yet, we do it now!
		if (codec == nullptr)
		{
			OpenLazily(frame, timeBase);
		}

		// convert the frame to a converted_frame
		frame = formatConverter->ConvertFrame(frame);

		if (frame->format != codec->GetContext()->pix_fmt)
		{
			throw FFmpegException("Codec only accepts " + string(av_get_pix_fmt_name(codec->GetContext()->pix_fmt)) + " while frame is in format " + av_get_pix_fmt_name((AVPixelFormat)frame->format));
		}

		frame->pts = frameNumber;
		++frameNumber;

		int ret = avcodec_send_frame(codec->GetContext(), frame);
		if (ret < 0)
		{
			throw FFmpegException("Error sending a frame for encoding", ret);
		}

		PollCodecForPackets();
	}

	void VideoEncoder::Close()
	{
		if (codec == nullptr) return; // can't close if we were never opened

		int ret = avcodec_send_frame(codec->GetContext(), nullptr);
		if (ret < 0)
		{
			throw FFmpegException("Error flushing codec after encoding", ret);
		}

		PollCodecForPackets();
	}

	void VideoEncoder::PollCodecForPackets()
	{
		int ret = 0;
		while (ret >= 0)
		{
			ret = avcodec_receive_packet(codec->GetContext(), pkt.get());
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			{
				return;
			}
			else if (ret < 0)
			{
				throw FFmpegException("Error during encoding", ret);
			}

			//printf("Write packet %3 (size=%5d)\n", data->pkt->pts, data->pkt->size);
			//fwrite(data->pkt->data, 1, data->pkt->size, data->f);
			output->WritePacket(pkt.get(), codec.get());

			av_packet_unref(pkt.get());
		}
	}

	bool VideoEncoder::IsPrimed() const
	{
		return output->IsPrimed();
	}
}

