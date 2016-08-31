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

#include <stdexcept>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>

#include "luxrays/kernels/kernels.h"
#include "slg/kernels/kernels.h"
#include "slg/film/film.h"
#include "slg/film/imagepipeline/plugins/mist.h"

using namespace std;
using namespace luxrays;
using namespace slg;

//------------------------------------------------------------------------------
// Mist plugin
//------------------------------------------------------------------------------

BOOST_CLASS_EXPORT_IMPLEMENT(slg::MistPlugin)

MistPlugin::MistPlugin(const luxrays::Spectrum &color, float amount, float start, float end) 
	: color(color), amount(amount), start(start), end(end)
{}

MistPlugin::MistPlugin() 
	: color(1.f), amount(1.f), start(0.f), end(1000.f)
{}

ImagePipelinePlugin *MistPlugin::Copy() const {
	return new MistPlugin(color, amount, start, end);
}

//------------------------------------------------------------------------------
// CPU version
//------------------------------------------------------------------------------

void MistPlugin::Apply(Film &film, const u_int index) {
	if (!film.HasChannel(Film::DEPTH)) {
		// I can not work without depth channel
		return;
	}

	Spectrum *pixels = (Spectrum *)film.channel_IMAGEPIPELINEs[index]->GetPixels();
	const u_int pixelCount = film.GetWidth() * film.GetHeight();
	
	// Optimization: invert to avoid division in the loop
	const float rangeInv = 1.f / (end - start);

	#pragma omp parallel for
	for (
			// Visual C++ 2013 supports only OpenMP 2.5
#if _OPENMP >= 200805
			unsigned
#endif
			int i = 0; i < pixelCount; ++i) {
		if (*(film.channel_FRAMEBUFFER_MASK->GetPixel(index))) {
			const float depthValue = *(film.channel_DEPTH->GetPixel(i));
			if(depthValue <= start) {
				// Nothing to do
				continue;
			}
			else if(depthValue >= end) {
				// Completely replace pixel color (but account for amount)
				pixels[i] = Lerp(amount, pixels[i], color);
			}
			else {
				// map depth value into 0..1 range
				const float weight = (depthValue - start) * rangeInv;
				pixels[i] = Lerp(weight * amount, pixels[i], color);
			}
		}
	}
}
























