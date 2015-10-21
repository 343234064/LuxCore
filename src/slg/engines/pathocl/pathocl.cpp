/***************************************************************************
 * Copyright 1998-2015 by authors (see AUTHORS.txt)                        *
 *                                                                         *
 *   This file is part of LuxRender.                                       *
 *                                                                         *
 * Licensed under the Apache License, Version 2.0 (the "License");         *
 * you may not use this file except in compliance with the License.        *
 * You may obtain a copy of the License at                                 *
 *                                                                         *
 *     http://www.apache.org/licenses/LICENSE-2.0                          *
 *                                                                         *
 * Unless required by applicable law or agreed to in writing, software     *
 * distributed under the License is distributed on an "AS IS" BASIS,       *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
 * See the License for the specific language governing permissions and     *
 * limitations under the License.                                          *
 ***************************************************************************/

#if !defined(LUXRAYS_DISABLE_OPENCL)

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <stdexcept>

#include <boost/thread/mutex.hpp>
#include <boost/lexical_cast.hpp>

#include "luxrays/core/geometry/transform.h"
#include "luxrays/core/oclintersectiondevice.h"

#include "slg/slg.h"
#include "slg/engines/pathocl/pathocl.h"
#include "slg/kernels/kernels.h"
#include "slg/renderconfig.h"
#include "slg/film/filters/box.h"
#include "slg/film/filters/gaussian.h"
#include "slg/film/filters/mitchell.h"
#include "slg/film/filters/mitchellss.h"
#include "slg/film/filters/blackmanharris.h"
#include "slg/scene/scene.h"
#include "slg/engines/rtpathocl/rtpathocl.h"

using namespace std;
using namespace luxrays;
using namespace slg;

//------------------------------------------------------------------------------
// PathOCLRenderEngine
//------------------------------------------------------------------------------

PathOCLRenderEngine::PathOCLRenderEngine(const RenderConfig *rcfg, Film *flm, boost::mutex *flmMutex,
		const bool realTime) : PathOCLBaseRenderEngine(rcfg, flm, flmMutex, realTime),
		pixelFilterDistribution(NULL) {
	oclSampler = NULL;
	oclPixelFilter = NULL;
}

PathOCLRenderEngine::~PathOCLRenderEngine() {
	delete[] pixelFilterDistribution;
	delete oclSampler;
	delete oclPixelFilter;
}

PathOCLRenderThread *PathOCLRenderEngine::CreateOCLThread(const u_int index,
    OpenCLIntersectionDevice *device) {
    return new PathOCLRenderThread(index, device, this);
}


void PathOCLRenderEngine::InitPixelFilterDistribution() {
	auto_ptr<Filter> pixelFilter(renderConfig->AllocPixelFilter());

	// Compile sample distribution
	delete[] pixelFilterDistribution;
	const FilterDistribution filterDistribution(pixelFilter.get(), 64);
	pixelFilterDistribution = CompiledScene::CompileDistribution2D(
			filterDistribution.GetDistribution2D(), &pixelFilterDistributionSize);
}

void PathOCLRenderEngine::StartLockLess() {
	const Properties &cfg = renderConfig->cfg;

	//--------------------------------------------------------------------------
	// Rendering parameters
	//--------------------------------------------------------------------------

	if (!cfg.IsDefined("opencl.task.count") && (GetEngineType() == RTPATHOCL)) {
		// In this case, I will tune task count for RTPATHOCL
		taskCount = film->GetWidth() * film->GetHeight() / intersectionDevices.size();
	} else {
		const u_int defaultTaskCount = 1024u * 1024u;

		// Compute the cap to the number of tasks
		u_int taskCap = defaultTaskCount;
		BOOST_FOREACH(DeviceDescription *devDescs, selectedDeviceDescs) {
			if (devDescs->GetMaxMemoryAllocSize() >= 1024u * 1024u * 1024u)
				taskCap = min(taskCap, 1024u * 1024u);
			else if (devDescs->GetMaxMemoryAllocSize() >= 512u * 1024u * 1024u)
				taskCap = min(taskCap, 512u * 1024u);
			else if (devDescs->GetMaxMemoryAllocSize() >= 256u * 1024u * 1024u)
				taskCap = min(taskCap, 256u * 1024u);
			else if (devDescs->GetMaxMemoryAllocSize() >= 128u * 1024u * 1024u)
				taskCap = min(taskCap, 128u * 1024u);
			else
				taskCap = min(taskCap, 64u * 1024u);
		}

		taskCount = cfg.Get(Property("opencl.task.count")(defaultTaskCount)).Get<u_int>();
		taskCount = min(taskCount, taskCap);

		// I don't know yet the workgroup size of each device so I can not
		// round up task count to be a multiple of workgroups size of all devices
		// used. Rounding to 2048 is a simple trick based on the assumption that
		// workgroup size is a power of 2 and <= 8192.
		taskCount = RoundUp<u_int>(taskCount, 8192);
		SLG_LOG("[PathOCLRenderEngine] OpenCL task count: " << taskCount);
	}

	//--------------------------------------------------------------------------
	// General path tracing settings
	//--------------------------------------------------------------------------	
	
	maxPathDepth = Max(1, cfg.Get(Property("path.maxdepth")(5)).Get<int>());
	rrDepth = Max(1, cfg.Get(Property("path.russianroulette.depth")(3)).Get<int>());
	rrImportanceCap = Clamp(cfg.Get(Property("path.russianroulette.cap")(.5f)).Get<float>(), 0.f, 1.f);

	// Clamping settings
	// clamping.radiance.maxvalue is the old radiance clamping, now converted in variance clamping
	sqrtVarianceClampMaxValue = Max(0.f,
			cfg.Get(Property("path.clamping.variance.maxvalue")(
				cfg.Get(Property("path.clamping.radiance.maxvalue")(0.f)).Get<float>())
			).Get<float>());
	pdfClampValue = Max(0.f, cfg.Get(Property("path.clamping.pdf.value")(0.f)).Get<float>());

	//--------------------------------------------------------------------------
	// Sampler
	//--------------------------------------------------------------------------

	oclSampler = Sampler::FromPropertiesOCL(cfg);

	//--------------------------------------------------------------------------
	// Filter
	//--------------------------------------------------------------------------

	oclPixelFilter = Filter::FromPropertiesOCL(cfg);
	
	useFastPixelFilter = cfg.Get(Property("path.fastpixelfilter.enable")(true)).Get<bool>();
	if (useFastPixelFilter)
		InitPixelFilterDistribution();

	usePixelAtomics = cfg.Get(Property("path.pixelatomics.enable")(false)).Get<bool>();

	PathOCLBaseRenderEngine::StartLockLess();
}

void PathOCLRenderEngine::StopLockLess() {
	PathOCLBaseRenderEngine::StopLockLess();

	delete[] pixelFilterDistribution;
	pixelFilterDistribution = NULL;
}

void PathOCLRenderEngine::UpdateFilmLockLess() {
	boost::unique_lock<boost::mutex> lock(*filmMutex);

	film->Reset();
	for (size_t i = 0; i < renderThreads.size(); ++i) {
        if (renderThreads[i])
            film->AddFilm(*(((PathOCLRenderThread *)(renderThreads[i]))->threadFilms[0]->film));
    }
}

void PathOCLRenderEngine::UpdateCounters() {
	elapsedTime = WallClockTime() - startTime;

	// Update the sample count statistic
	double totalCount = 0;
	for (size_t i = 0; i < renderThreads.size(); ++i) {
		slg::ocl::pathocl::GPUTaskStats *stats = ((PathOCLRenderThread *)(renderThreads[i]))->gpuTaskStats;

		for (size_t i = 0; i < taskCount; ++i)
			totalCount += stats[i].sampleCount;
	}

	samplesCount = totalCount;
	// This is a bit tricky because film is reset in UpdateFilmLockLess()
	film->SetSampleCount(samplesCount);

	// Update the ray count statistic
	totalCount = 0.0;
	for (size_t i = 0; i < intersectionDevices.size(); ++i)
		totalCount += intersectionDevices[i]->GetTotalRaysCount();
	raysCount = totalCount;
}

#endif
