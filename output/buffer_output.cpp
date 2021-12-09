/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2021, Raspberry Pi (Trading) Ltd. / Dustin Kerstein
 *
 * buffer_output.cpp
 */

#include "buffer_output.hpp"
#include <thread>
#include <pthread.h>
#include <chrono>
using namespace std::chrono;

// We're going to align the frames within the buffer to friendly byte boundaries
// static constexpr int ALIGN = 16; // power of 2, please

BufferOutput::BufferOutput(VideoOptions const *options) : Output(options), buf_(), framesBuffered_(0), framesWritten_(0)
{
	if (options_->output == "-")
		fp_ = stdout;
	else if (!options_->output.empty())
	{
		fp_ = fopen(options_->output.c_str(), "w");
	}
	if (!fp_)
		throw std::runtime_error("could not open output file");

	std::thread t1(WriterThread, std::ref(*this));
	t1.detach();
}

BufferOutput::~BufferOutput()
{
	// while(framesWritten_ < framesBuffered_)
	// {
	// 	if (fwrite(buf_[framesWritten_], 18677760, 1, fp_) != 1) // NEED TO % 300
	// 		std::cerr << "failed to write output bytes" << std::endl;
	// 	else
	// 	{
	// 		std::cerr << "Frames Written: " << framesWritten_ << ", Frames Buffered: " << framesBuffered_ << std::endl;
	// 		framesWritten_++;
	// 	}
	// }
	CloseFile();
}

void BufferOutput::outputBuffer(void *mem, size_t size, int64_t timestamp_us, uint32_t flags)
{
	framesBuffered_++;
	auto start = high_resolution_clock::now();
	memcpy(&buf_[framesBuffered_ - 1], mem, 18677760); // NEED TO PAD/ALIGN TO 4096
	auto stop = high_resolution_clock::now();
	auto duration = duration_cast<milliseconds>(stop - start);
	std::cerr << "Copy took: " << duration.count() << "ms, Frames Buffered: " << framesBuffered_ << std::endl;
	while (framesBuffered_ == options_->frames &&  framesWritten_ != options_->frames)
	{
		std::cerr << "Waiting 100ms for WriterThread to finish" << std::endl;
		std::this_thread::sleep_for (std::chrono::milliseconds(100));
	}
}

void BufferOutput::CloseFile()
{
	if (fp_ && fp_ != stdout)
		fclose(fp_);
	fp_ = nullptr;
}

void BufferOutput::WriterThread(BufferOutput &obj)
{
	// NOT SURE THIS DOES ANYTHING
	// int policy;
 	// struct sched_param param;
	// pthread_getschedparam(pthread_self(), &policy, &param);
	// param.sched_priority = sched_get_priority_min(policy);
	// pthread_setschedparam(pthread_self(), policy, &param);

	while (obj.fp_ != nullptr)
	{
		// if (obj.framesBuffered_ == 300) 
		// {
			while(obj.framesBuffered_ - obj.framesWritten_ > 0)
			{
				if (fwrite(obj.buf_[obj.framesWritten_], 18677760, 1, obj.fp_) != 1) // NEED TO % 300
					throw std::runtime_error("failed to write output bytes");
				else
					obj.framesWritten_++;
			}
		// }
		// else if (obj.framesBuffered_ - obj.framesWritten_ > 0)
		// {
		// 	if (fwrite(obj.buf_[obj.framesWritten_], 18677760, 1, obj.fp_) != 1) // NEED TO % 300
		// 		throw std::runtime_error("failed to write output bytes");
		// 	else
		// 		obj.framesWritten_++;
		// }
		std::this_thread::sleep_for (std::chrono::milliseconds(100));
	}
	std::cerr << "NULL!!!" << std::endl;
}