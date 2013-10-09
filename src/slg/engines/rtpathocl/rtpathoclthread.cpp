/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
 *                                                                         *
 *   This file is part of LuxRays.                                         *
 *                                                                         *
 *   LuxRays is free software; you can redistribute it and/or modify       *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   LuxRays is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 *   LuxRays website: http://www.luxrender.net                             *
 ***************************************************************************/

#if !defined(LUXRAYS_DISABLE_OPENCL)

#include "luxrays/core/oclintersectiondevice.h"

#include "slg/slg.h"
#include "slg/kernels/kernels.h"
#include "slg/engines/rtpathocl/rtpathocl.h"
#include "slg/engines/pathocl/pathocl_datatypes.h"

using namespace std;
using namespace luxrays;
using namespace slg;

//------------------------------------------------------------------------------
// RTPathOCLRenderThread
//------------------------------------------------------------------------------

RTPathOCLRenderThread::RTPathOCLRenderThread(const u_int index,
	OpenCLIntersectionDevice *device, PathOCLRenderEngine *re) : 
	PathOCLRenderThread(index, device, re) {
	clearFBKernel = NULL;
	clearSBKernel = NULL;
	mergeFBKernel = NULL;
	normalizeFBKernel = NULL;
	applyBlurFilterXR1Kernel = NULL;
	applyBlurFilterYR1Kernel = NULL;
	toneMapLinearKernel = NULL;
	updateScreenBufferKernel = NULL;

	tmpFrameBufferBuff = NULL;
	mergedFrameBufferBuff = NULL;
	screenBufferBuff = NULL;
	assignedIters = ((RTPathOCLRenderEngine*)renderEngine)->minIterations;
	frameTime = 0.0;
}

RTPathOCLRenderThread::~RTPathOCLRenderThread() {
	delete clearFBKernel;
	delete clearSBKernel;
	delete mergeFBKernel;
	delete normalizeFBKernel;
	delete applyBlurFilterXR1Kernel;
	delete applyBlurFilterYR1Kernel;
	delete toneMapLinearKernel;
	delete updateScreenBufferKernel;
}

void RTPathOCLRenderThread::AdditionalInit() {
	RTPathOCLRenderEngine *engine = (RTPathOCLRenderEngine *)renderEngine;
	if (engine->displayDeviceIndex == threadIndex)
		InitDisplayThread();

	PathOCLRenderThread::AdditionalInit();
}

string RTPathOCLRenderThread::AdditionalKernelOptions() {
	RTPathOCLRenderEngine *engine = (RTPathOCLRenderEngine *)renderEngine;

	stringstream ss;
	ss.precision(6);
	ss << scientific <<
			PathOCLRenderThread::AdditionalKernelOptions();

	if (engine->film->GetToneMapParams()->GetType() == TONEMAP_LINEAR) {
		const LinearToneMapParams *ltm = (const LinearToneMapParams *)engine->film->GetToneMapParams();
		ss << " -D PARAM_TONEMAP_LINEAR_SCALE=" << ltm->scale << "f";
	} else
		ss << " -D PARAM_TONEMAP_LINEAR_SCALE=1.f";

	ss << " -D PARAM_GAMMA=" << engine->film->GetGamma() << "f";

	return ss.str();
}

string RTPathOCLRenderThread::AdditionalKernelSources() {
	stringstream ss;
	ss << PathOCLRenderThread::AdditionalKernelSources() <<
			slg::ocl::KernelSource_rtpathoclbase_funcs;

	return ss.str();
}

void RTPathOCLRenderThread::CompileAdditionalKernels(cl::Program *program) {
	PathOCLRenderThread::CompileAdditionalKernels(program);

	//--------------------------------------------------------------------------
	// ClearFrameBuffer kernel
	//--------------------------------------------------------------------------

	CompileKernel(program, &clearFBKernel, &clearFBWorkGroupSize, "ClearFrameBuffer");

	//--------------------------------------------------------------------------
	// InitRTFrameBuffer kernel
	//--------------------------------------------------------------------------

	CompileKernel(program, &clearSBKernel, &clearSBWorkGroupSize, "ClearScreenBuffer");

	//--------------------------------------------------------------------------
	// MergeFrameBuffer kernel
	//--------------------------------------------------------------------------

	CompileKernel(program, &mergeFBKernel, &mergeFBWorkGroupSize, "MergeFrameBuffer");

	//--------------------------------------------------------------------------
	// NormalizeFrameBuffer kernel
	//--------------------------------------------------------------------------

	CompileKernel(program, &normalizeFBKernel, &normalizeFBWorkGroupSize, "NormalizeFrameBuffer");

	//--------------------------------------------------------------------------
	// Gaussian blur frame buffer filter kernel
	//--------------------------------------------------------------------------

	CompileKernel(program, &applyBlurFilterXR1Kernel, &applyBlurFilterXR1WorkGroupSize, "ApplyGaussianBlurFilterXR1");
	CompileKernel(program, &applyBlurFilterYR1Kernel, &applyBlurFilterYR1WorkGroupSize, "ApplyGaussianBlurFilterYR1");

	//--------------------------------------------------------------------------
	// ToneMapLinear kernel
	//--------------------------------------------------------------------------

	CompileKernel(program, &toneMapLinearKernel, &toneMapLinearWorkGroupSize, "ToneMapLinear");

	//--------------------------------------------------------------------------
	// UpdateScreenBuffer kernel
	//--------------------------------------------------------------------------

	CompileKernel(program, &updateScreenBufferKernel, &updateScreenBufferWorkGroupSize, "UpdateScreenBuffer");
}

void RTPathOCLRenderThread::Interrupt() {
}

void RTPathOCLRenderThread::Stop() {
	PathOCLRenderThread::Stop();

	FreeOCLBuffer(&tmpFrameBufferBuff);
	FreeOCLBuffer(&mergedFrameBufferBuff);
	FreeOCLBuffer(&screenBufferBuff);
}

void RTPathOCLRenderThread::BeginEdit() {
	// NOTE: this is a huge trick, the LuxRays context is stopped by RenderEngine
	// but the threads are still using the intersection devices in RTPATHOCL.
	// The result is that stuff like geometry edit will not work.
	editMutex.lock();
}

void RTPathOCLRenderThread::EndEdit(const EditActionList &editActions) {
	if (editActions.Has(FILM_EDIT) || editActions.Has(MATERIAL_TYPES_EDIT))
		throw std::runtime_error("RTPATHOCL doesn't support FILM_EDIT or MATERIAL_TYPES_EDIT actions");

	updateActions.AddActions(editActions.GetActions());
	editMutex.unlock();
}

void RTPathOCLRenderThread::InitDisplayThread() {
	const u_int filmBufferPixelCount = threadFilm->GetWidth() * threadFilm->GetHeight();
	AllocOCLBufferRW(&tmpFrameBufferBuff, sizeof(slg::ocl::Pixel) * filmBufferPixelCount, "Tmp FrameBuffer");
	AllocOCLBufferRW(&mergedFrameBufferBuff, sizeof(slg::ocl::Pixel) * filmBufferPixelCount, "Merged FrameBuffer");

	AllocOCLBufferRW(&screenBufferBuff, sizeof(Spectrum) * filmBufferPixelCount, "Screen FrameBuffer");
}

void RTPathOCLRenderThread::SetAdditionalKernelArgs() {
	PathOCLRenderThread::SetAdditionalKernelArgs();

	RTPathOCLRenderEngine *engine = (RTPathOCLRenderEngine *)renderEngine;
	if (engine->displayDeviceIndex == threadIndex) {
		boost::unique_lock<boost::mutex> lock(engine->setKernelArgsMutex);

		u_int argIndex = 0;
		clearFBKernel->setArg(argIndex++, threadFilm->GetWidth());
		clearFBKernel->setArg(argIndex++, threadFilm->GetHeight());
		clearFBKernel->setArg(argIndex++, *mergedFrameBufferBuff);

		argIndex = 0;
		clearSBKernel->setArg(argIndex++, threadFilm->GetWidth());
		clearSBKernel->setArg(argIndex++, threadFilm->GetHeight());
		clearSBKernel->setArg(argIndex, *screenBufferBuff);

		argIndex = 0;
		normalizeFBKernel->setArg(argIndex++, threadFilm->GetWidth());
		normalizeFBKernel->setArg(argIndex++, threadFilm->GetHeight());
		normalizeFBKernel->setArg(argIndex++, *mergedFrameBufferBuff);

		argIndex = 0;
		applyBlurFilterXR1Kernel->setArg(argIndex++, threadFilm->GetWidth());
		applyBlurFilterXR1Kernel->setArg(argIndex++, threadFilm->GetHeight());
		applyBlurFilterXR1Kernel->setArg(argIndex++, *screenBufferBuff);
		applyBlurFilterXR1Kernel->setArg(argIndex++, *tmpFrameBufferBuff);
		argIndex = 0;
		applyBlurFilterYR1Kernel->setArg(argIndex++, threadFilm->GetWidth());
		applyBlurFilterYR1Kernel->setArg(argIndex++, threadFilm->GetHeight());
		applyBlurFilterYR1Kernel->setArg(argIndex++, *tmpFrameBufferBuff);
		applyBlurFilterYR1Kernel->setArg(argIndex++, *screenBufferBuff);

		argIndex = 0;
		toneMapLinearKernel->setArg(argIndex++, threadFilm->GetWidth());
		toneMapLinearKernel->setArg(argIndex++, threadFilm->GetHeight());
		toneMapLinearKernel->setArg(argIndex++, *mergedFrameBufferBuff);

		argIndex = 0;
		updateScreenBufferKernel->setArg(argIndex++, threadFilm->GetWidth());
		updateScreenBufferKernel->setArg(argIndex++, threadFilm->GetHeight());
		updateScreenBufferKernel->setArg(argIndex++, *mergedFrameBufferBuff);
		updateScreenBufferKernel->setArg(argIndex++, *screenBufferBuff);
	}
}

void RTPathOCLRenderThread::UpdateOCLBuffers() {
	editMutex.lock();

	RTPathOCLRenderEngine *engine = (RTPathOCLRenderEngine *)renderEngine;
	const bool amiDisplayThread = (engine->displayDeviceIndex == threadIndex);

	//--------------------------------------------------------------------------
	// Update OpenCL buffers
	//--------------------------------------------------------------------------

	if (updateActions.Has(FILM_EDIT)) {
		// Resize the Frame Buffer
		InitFilm();

		// Display thread initialization
		if (amiDisplayThread)
			InitDisplayThread();
	}

	if (updateActions.Has(CAMERA_EDIT)) {
		// Update Camera
		InitCamera();
	}

	if (updateActions.Has(GEOMETRY_EDIT)) {
		// Update Scene Geometry
		InitGeometry();
	}

	if (updateActions.Has(MATERIALS_EDIT) || updateActions.Has(MATERIAL_TYPES_EDIT)) {
		// Update Scene Materials
		InitMaterials();
	}

	if  (updateActions.Has(AREALIGHTS_EDIT)) {
		// Update Scene Area Lights
		InitTriangleAreaLights();
	}

	if  (updateActions.Has(INFINITELIGHT_EDIT)) {
		// Update Scene Infinite Light
		InitInfiniteLight();
	}

	if  (updateActions.Has(SUNLIGHT_EDIT)) {
		// Update Scene Sun Light
		InitSunLight();
	}

	if  (updateActions.Has(SKYLIGHT_EDIT)) {
		// Update Scene Sun Light
		InitSkyLight();
	}

	//--------------------------------------------------------------------------
	// Recompile Kernels if required
	//--------------------------------------------------------------------------

	if (updateActions.Has(FILM_EDIT) || updateActions.Has(MATERIAL_TYPES_EDIT))
		InitKernels();

	updateActions.Reset();
	editMutex.unlock();

	SetKernelArgs();

	//--------------------------------------------------------------------------
	// Execute initialization kernels
	//--------------------------------------------------------------------------

	cl::CommandQueue &oclQueue = intersectionDevice->GetOpenCLQueue();

	// Clear the frame buffer
	const u_int filmPixelCount = threadFilm->GetWidth() * threadFilm->GetHeight();
	oclQueue.enqueueNDRangeKernel(*filmClearKernel, cl::NullRange,
			cl::NDRange(RoundUp<u_int>(filmPixelCount, filmClearWorkGroupSize)),
			cl::NDRange(filmClearWorkGroupSize));

	// Initialize the tasks buffer
	oclQueue.enqueueNDRangeKernel(*initKernel, cl::NullRange,
			cl::NDRange(RoundUp<u_int>(engine->taskCount, initWorkGroupSize)),
			cl::NDRange(initWorkGroupSize));
	oclQueue.finish();

	// Reset statistics in order to be more accurate
	intersectionDevice->ResetPerformaceStats();
}

void RTPathOCLRenderThread::RenderThreadImpl() {
	//SLG_LOG("[RTPathOCLRenderThread::" << threadIndex << "] Rendering thread started");

	// Boost barriers are supposed to be not interruptable but they are
	// and seems to missing a way to reset them. So better to disable
	// interruptions.
	boost::this_thread::disable_interruption di;

	RTPathOCLRenderEngine *engine = (RTPathOCLRenderEngine *)renderEngine;
	boost::barrier *frameBarrier = engine->frameBarrier;

	cl::CommandQueue &oclQueue = intersectionDevice->GetOpenCLQueue();
	const u_int filmBufferPixelCount = engine->film->GetWidth() * engine->film->GetHeight();

	try {
		//----------------------------------------------------------------------
		// Execute initialization kernels
		//----------------------------------------------------------------------

		cl::CommandQueue &oclQueue = intersectionDevice->GetOpenCLQueue();

		// Clear the frame buffer
		const u_int filmPixelCount = threadFilm->GetWidth() * threadFilm->GetHeight();
		oclQueue.enqueueNDRangeKernel(*filmClearKernel, cl::NullRange,
				cl::NDRange(RoundUp<u_int>(filmPixelCount, filmClearWorkGroupSize)),
				cl::NDRange(filmClearWorkGroupSize));

		// Initialize the tasks buffer
		oclQueue.enqueueNDRangeKernel(*initKernel, cl::NullRange,
				cl::NDRange(RoundUp<u_int>(engine->taskCount, initWorkGroupSize)),
				cl::NDRange(initWorkGroupSize));

		//----------------------------------------------------------------------
		// Rendering loop
		//----------------------------------------------------------------------

		const bool amiDisplayThread = (engine->displayDeviceIndex == threadIndex);
		if (amiDisplayThread) {
			oclQueue.enqueueNDRangeKernel(*clearSBKernel, cl::NullRange,
					cl::NDRange(RoundUp<u_int>(filmBufferPixelCount, clearSBWorkGroupSize)),
					cl::NDRange(clearSBWorkGroupSize));
		}

		double lastEditTime = WallClockTime();
		while (!boost::this_thread::interruption_requested()) {
			if (updateActions.HasAnyAction()) {
				UpdateOCLBuffers();
				lastEditTime = WallClockTime();
			}

			//------------------------------------------------------------------
			// Render a frame (i.e. taskCount * assignedIters samples)
			//------------------------------------------------------------------
			const double startTime = WallClockTime();
			u_int iterations = assignedIters;

			for (u_int i = 0; i < iterations; ++i) {
				// Trace rays
				intersectionDevice->EnqueueTraceRayBuffer(*raysBuff,
					*(hitsBuff), engine->taskCount, NULL, NULL);
				// Advance to next path state
				oclQueue.enqueueNDRangeKernel(*advancePathsKernel, cl::NullRange,
					cl::NDRange(RoundUp<u_int>(engine->taskCount, advancePathsWorkGroupSize)),
					cl::NDRange(advancePathsWorkGroupSize));
			}

			// No need to transfer the frame buffer if I'm not the display thread
			if (engine->displayDeviceIndex != threadIndex)
				TransferFilm(oclQueue);

			// Async. transfer of GPU task statistics
			oclQueue.enqueueReadBuffer(*(taskStatsBuff), CL_FALSE, 0,
				sizeof(slg::ocl::pathocl::GPUTaskStats) * engine->taskCount, gpuTaskStats);

			oclQueue.finish();
			frameTime = WallClockTime() - startTime;

			//------------------------------------------------------------------

			frameBarrier->wait();

			// If I'm the display thread, my OpenCL device must merge all frame buffers
			// and do all frame post-processing steps
			if (amiDisplayThread) {
				// Last step include a film update
				boost::unique_lock<boost::mutex> lock(*(engine->filmMutex));

				//--------------------------------------------------------------
				// Clear the merged frame buffer
				//--------------------------------------------------------------

				oclQueue.enqueueNDRangeKernel(*clearFBKernel, cl::NullRange,
						cl::NDRange(RoundUp<u_int>(filmBufferPixelCount, clearFBWorkGroupSize)),
						cl::NDRange(clearFBWorkGroupSize));

				//--------------------------------------------------------------
				// Merge all frame buffers
				//--------------------------------------------------------------

				for (u_int i = 0; i < engine->renderThreads.size(); ++i) {
					// I don't need to lock renderEngine->setKernelArgsMutex
					// because I'm the only thread running

					if (i == threadIndex) {
						// Normalize and merge the my frame buffer
						u_int argIndex = 0;
						mergeFBKernel->setArg(argIndex++, threadFilm->GetWidth());
						mergeFBKernel->setArg(argIndex++, threadFilm->GetHeight());
						mergeFBKernel->setArg(argIndex++, *(channel_RADIANCE_PER_PIXEL_NORMALIZEDs_Buff[0]));
						mergeFBKernel->setArg(argIndex++, *mergedFrameBufferBuff);
						oclQueue.enqueueNDRangeKernel(*mergeFBKernel, cl::NullRange,
								cl::NDRange(RoundUp<u_int>(filmBufferPixelCount, mergeFBWorkGroupSize)),
								cl::NDRange(mergeFBWorkGroupSize));
					} else {
						// Transfer the frame buffer to the device
						RTPathOCLRenderThread *thread = (RTPathOCLRenderThread *)(engine->renderThreads[i]);
						oclQueue.enqueueWriteBuffer(*tmpFrameBufferBuff, CL_FALSE, 0,
								tmpFrameBufferBuff->getInfo<CL_MEM_SIZE>(),
								thread->threadFilm->channel_RADIANCE_PER_PIXEL_NORMALIZEDs[0]->GetPixels());

						// Normalize and merge the frame buffers
						u_int argIndex = 0;
						mergeFBKernel->setArg(argIndex++, threadFilm->GetWidth());
						mergeFBKernel->setArg(argIndex++, threadFilm->GetHeight());
						mergeFBKernel->setArg(argIndex++, *tmpFrameBufferBuff);
						mergeFBKernel->setArg(argIndex++, *mergedFrameBufferBuff);
						oclQueue.enqueueNDRangeKernel(*mergeFBKernel, cl::NullRange,
								cl::NDRange(RoundUp<u_int>(filmBufferPixelCount, mergeFBWorkGroupSize)),
								cl::NDRange(mergeFBWorkGroupSize));
					}
				}

				//--------------------------------------------------------------
				// Normalize the merged buffer
				//--------------------------------------------------------------

				oclQueue.enqueueNDRangeKernel(*normalizeFBKernel, cl::NullRange,
						cl::NDRange(RoundUp<u_int>(filmBufferPixelCount, normalizeFBWorkGroupSize)),
						cl::NDRange(normalizeFBWorkGroupSize));

				//--------------------------------------------------------------
				// Apply tone mapping to merged buffer
				//--------------------------------------------------------------

				oclQueue.enqueueNDRangeKernel(*toneMapLinearKernel, cl::NullRange,
						cl::NDRange(RoundUp<u_int>(filmBufferPixelCount, toneMapLinearWorkGroupSize)),
						cl::NDRange(toneMapLinearWorkGroupSize));

				//--------------------------------------------------------------
				// Update the screen buffer
				//--------------------------------------------------------------

				oclQueue.enqueueNDRangeKernel(*updateScreenBufferKernel, cl::NullRange,
						cl::NDRange(RoundUp<u_int>(filmBufferPixelCount, updateScreenBufferWorkGroupSize)),
						cl::NDRange(updateScreenBufferWorkGroupSize));

				//--------------------------------------------------------------
				// Apply Gaussian filter to the screen buffer
				//--------------------------------------------------------------

				// Base the amount of blur on the time since the last update (using a 3secs window)
				const double timeSinceLastUpdate = WallClockTime() - lastEditTime;
				const float weight = Lerp(Clamp<float>(timeSinceLastUpdate, 0.f, 3.f) / 5.f, 0.25f, 0.025f);

				applyBlurFilterXR1Kernel->setArg(4, weight);
				applyBlurFilterYR1Kernel->setArg(4, weight);
				for (u_int i = 0; i < 3; ++i) {
					oclQueue.enqueueNDRangeKernel(*applyBlurFilterXR1Kernel, cl::NullRange,
							cl::NDRange(RoundUp<unsigned int>(filmBufferPixelCount, applyBlurFilterXR1WorkGroupSize)),
							cl::NDRange(applyBlurFilterXR1WorkGroupSize));

					oclQueue.enqueueNDRangeKernel(*applyBlurFilterYR1Kernel, cl::NullRange,
							cl::NDRange(RoundUp<unsigned int>(filmBufferPixelCount, applyBlurFilterYR1WorkGroupSize)),
							cl::NDRange(applyBlurFilterYR1WorkGroupSize));
				}
				
				//--------------------------------------------------------------
				// Transfer the screen frame buffer
				//--------------------------------------------------------------

				// The film has been locked before
				oclQueue.enqueueReadBuffer(*screenBufferBuff, CL_FALSE, 0,
						screenBufferBuff->getInfo<CL_MEM_SIZE>(), engine->film->GetScreenBuffer());
				oclQueue.finish();
			}

			//------------------------------------------------------------------

			frameBarrier->wait();

			// Time to render a new frame
		}
		//SLG_LOG("[RTPathOCLRenderThread::" << threadIndex << "] Rendering thread halted");
	} catch (boost::thread_interrupted) {
		SLG_LOG("[RTPathOCLRenderThread::" << threadIndex << "] Rendering thread halted");
	} catch (cl::Error err) {
		SLG_LOG("[RTPathOCLRenderThread::" << threadIndex << "] Rendering thread ERROR: " << err.what() <<
				"(" << oclErrorString(err.err()) << ")");
	}
	oclQueue.finish();
}

#endif
