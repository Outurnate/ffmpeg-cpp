#include "Sources/RawVideoDataSource.h"

#include "ffmpeg.h"
#include "FFmpegException.h"
#include "FrameSinks/VideoFrameSink.h"

namespace ffmpegcpp
{

	RawVideoDataSource::RawVideoDataSource(int width, int height, AVPixelFormat pixelFormat, AVRational framesPerSecond, VideoFrameSink* output)
		: RawVideoDataSource(width, height, pixelFormat, pixelFormat, framesPerSecond, output)
	{

	}

	RawVideoDataSource::RawVideoDataSource(int width, int height, AVPixelFormat sourcePixelFormat, AVPixelFormat targetPixelFormat, AVRational framesPerSecond, VideoFrameSink* output)
	{
		Init(width, height, sourcePixelFormat, targetPixelFormat, framesPerSecond, output);
	}

	void RawVideoDataSource::Init(int width, int height, AVPixelFormat sourcePixelFormat, AVPixelFormat targetPixelFormat, AVRational framesPerSecond, VideoFrameSink* output)
	{
		this->output = output;
		this->sourcePixelFormat = sourcePixelFormat;

		// set up the time base
		this->timeBase.num = framesPerSecond.den;
		this->timeBase.den = framesPerSecond.num;

		// create the frame
		int ret;

		frame = MakeFFmpegResource<AVFrame>(av_frame_alloc());
		if (!frame)
		{
			throw FFmpegException("Could not allocate video frame");
		}

		frame->format = targetPixelFormat;
		frame->width = width;
		frame->height = height;

		/* allocate the buffers for the frame data */
		ret = av_frame_get_buffer(frame.get(), 32);
		if (ret < 0)
		{
			throw FFmpegException("Could not allocate the video frame data", ret);
		}
	}

	void RawVideoDataSource::WriteFrame(void* data, int bytesPerRow)
	{
		// make sure the frame data is writable
		int ret = av_frame_make_writable(frame.get());
		if (ret < 0)
		{
			throw FFmpegException("Error making frame writable", ret);
		}

		const int in_linesize[1] = { bytesPerRow };

		// if the source and target pixel format are the same, we don't do any conversions, we just copy
		// but we use sws_scale anyway because we need to convert to the internal line_size format of frame
		auto newSwsContext = sws_getCachedContext(swsContext.get(),
			frame->width, frame->height, sourcePixelFormat,
			frame->width, frame->height, (AVPixelFormat)frame->format,
			0, nullptr, nullptr, nullptr);
    if (swsContext.get() != newSwsContext)
    {
      swsContext.release();
      swsContext.reset(newSwsContext);
    }
		sws_scale(swsContext.get(), (const uint8_t * const *)&data, in_linesize, 0,
			frame->height, frame->data, frame->linesize);

		// send to the output
		output->WriteFrame(frame.get(), &this->timeBase);
	}

	void RawVideoDataSource::Close()
	{
		output->Close();
	}

	int RawVideoDataSource::GetWidth() const
	{
		return frame->width;
	}

	int RawVideoDataSource::GetHeight() const
	{
		return frame->height;
	}

	bool RawVideoDataSource::IsPrimed() const
	{
		return output->IsPrimed();
	}
}
