#pragma once

#include "ffmpeg.h"

#include <stdexcept>
#include <string>

namespace ffmpegcpp
{
	class FFmpegException : std::runtime_error
	{

	public:

		FFmpegException(const std::string & error);

		FFmpegException(const std::string & error, int returnValue);

		char const* what() const throw()
		{
			return std::runtime_error::what();
		}

	private:

		char error[AV_ERROR_MAX_STRING_SIZE];
	};
}
