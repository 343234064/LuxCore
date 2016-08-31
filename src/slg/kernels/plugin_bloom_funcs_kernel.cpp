#include <string>
namespace slg { namespace ocl {
std::string KernelSource_plugin_bloom_funcs = 
"#line 2 \"plugin_bloom_funcs.cl\"\n"
"\n"
"/***************************************************************************\n"
" * Copyright 1998-2015 by authors (see AUTHORS.txt)                        *\n"
" *                                                                         *\n"
" *   This file is part of LuxRender.                                       *\n"
" *                                                                         *\n"
" * Licensed under the Apache License, Version 2.0 (the \"License\");         *\n"
" * you may not use this file except in compliance with the License.        *\n"
" * You may obtain a copy of the License at                                 *\n"
" *                                                                         *\n"
" *     http://www.apache.org/licenses/LICENSE-2.0                          *\n"
" *                                                                         *\n"
" * Unless required by applicable law or agreed to in writing, software     *\n"
" * distributed under the License is distributed on an \"AS IS\" BASIS,       *\n"
" * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*\n"
" * See the License for the specific language governing permissions and     *\n"
" * limitations under the License.                                          *\n"
" ***************************************************************************/\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// BloomFilterPlugin_FilterX\n"
"//------------------------------------------------------------------------------\n"
"\n"
"__kernel __attribute__((work_group_size_hint(256, 1, 1))) void BloomFilterPlugin_FilterX(\n"
"		const uint filmWidth, const uint filmHeight,\n"
"		__global float *channel_IMAGEPIPELINE,\n"
"		__global uint *channel_FRAMEBUFFER_MASK,\n"
"		__global float *bloomBuffer,\n"
"		__global float *bloomBufferTmp,\n"
"		__global float *bloomFilter,\n"
"		const uint bloomWidth) {\n"
"	const size_t gid = get_global_id(0);\n"
"	if (gid >= filmWidth * filmHeight)\n"
"		return;\n"
"\n"
"	const uint x = gid % filmWidth;\n"
"	const uint y = gid / filmWidth;\n"
"\n"
"	uint maskValue = channel_FRAMEBUFFER_MASK[gid];\n"
"	if (maskValue) {\n"
"		// Compute bloom for pixel (x, y)\n"
"		// Compute extent of pixels contributing bloom\n"
"		const uint x0 = max(x, bloomWidth) - bloomWidth;\n"
"		const uint x1 = min(x + bloomWidth, filmWidth - 1);\n"
"\n"
"		float sumWt = 0.f;\n"
"		const uint by = y;\n"
"		float3 pixel = 0.f;\n"
"		for (uint bx = x0; bx <= x1; ++bx) {\n"
"			const uint bloomOffset = bx + by * filmWidth;\n"
"			maskValue = channel_FRAMEBUFFER_MASK[bloomOffset];\n"
"\n"
"			if (maskValue) {\n"
"				// Accumulate bloom from pixel (bx, by)\n"
"				const uint dist2 = (x - bx) * (x - bx) + (y - by) * (y - by);\n"
"				const float wt = bloomFilter[dist2];\n"
"				if (wt == 0.f)\n"
"					continue;\n"
"\n"
"				sumWt += wt;\n"
"				const uint bloomOffset3 = bloomOffset * 3;\n"
"				pixel.s0 += wt * channel_IMAGEPIPELINE[bloomOffset3];\n"
"				pixel.s1 += wt * channel_IMAGEPIPELINE[bloomOffset3 + 1];\n"
"				pixel.s2 += wt * channel_IMAGEPIPELINE[bloomOffset3 + 2];\n"
"			}\n"
"		}\n"
"		if (sumWt > 0.f)\n"
"			pixel /= sumWt;\n"
"		\n"
"		__global float *dst = &bloomBufferTmp[(x + y * filmWidth) * 3];\n"
"		dst[0] = pixel.s0;\n"
"		dst[1] = pixel.s1;\n"
"		dst[2] = pixel.s2;\n"
"	}\n"
"}\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// BloomFilterPlugin_FilterY\n"
"//------------------------------------------------------------------------------\n"
"\n"
"__kernel __attribute__((work_group_size_hint(256, 1, 1))) void BloomFilterPlugin_FilterY(\n"
"		const uint filmWidth, const uint filmHeight,\n"
"		__global float *channel_IMAGEPIPELINE,\n"
"		__global uint *channel_FRAMEBUFFER_MASK,\n"
"		__global float *bloomBuffer,\n"
"		__global float *bloomBufferTmp,\n"
"		__global float *bloomFilter,\n"
"		const uint bloomWidth) {\n"
"	const size_t gid = get_global_id(0);\n"
"	if (gid >= filmWidth * filmHeight)\n"
"		return;\n"
"\n"
"	const uint x = gid % filmWidth;\n"
"	const uint y = gid / filmWidth;\n"
"\n"
"	uint maskValue = channel_FRAMEBUFFER_MASK[gid];\n"
"	if (maskValue) {\n"
"		// Compute bloom for pixel (x, y)\n"
"		// Compute extent of pixels contributing bloom\n"
"		const uint y0 = max(y, bloomWidth) - bloomWidth;\n"
"		const uint y1 = min(y + bloomWidth, filmHeight - 1);\n"
"\n"
"		float sumWt = 0.f;\n"
"		const uint bx = x;\n"
"		float3 pixel = 0.f;\n"
"		for (uint by = y0; by <= y1; ++by) {\n"
"			const uint bloomOffset = bx + by * filmWidth;\n"
"			maskValue = channel_FRAMEBUFFER_MASK[bloomOffset];\n"
"\n"
"			if (maskValue) {\n"
"				// Accumulate bloom from pixel (bx, by)\n"
"				const uint dist2 = (x - bx) * (x - bx) + (y - by) * (y - by);\n"
"				const float wt = bloomFilter[dist2];\n"
"				if (wt == 0.f)\n"
"					continue;\n"
"\n"
"				const uint bloomOffset = bx + by * filmWidth;\n"
"				sumWt += wt;\n"
"				const uint bloomOffset3 = bloomOffset * 3;\n"
"				pixel.s0 += wt * bloomBufferTmp[bloomOffset3];\n"
"				pixel.s1 += wt * bloomBufferTmp[bloomOffset3 + 1];\n"
"				pixel.s2 += wt * bloomBufferTmp[bloomOffset3 + 2];\n"
"			}\n"
"		}\n"
"\n"
"		if (sumWt > 0.f)\n"
"			pixel /= sumWt;\n"
"\n"
"		__global float *dst = &bloomBuffer[(x + y * filmWidth) * 3];\n"
"		dst[0] = pixel.s0;\n"
"		dst[1] = pixel.s1;\n"
"		dst[2] = pixel.s2;\n"
"	}\n"
"}\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// BloomFilterPlugin_Merge\n"
"//------------------------------------------------------------------------------\n"
"\n"
"__kernel __attribute__((work_group_size_hint(256, 1, 1))) void BloomFilterPlugin_Merge(\n"
"		const uint filmWidth, const uint filmHeight,\n"
"		__global float *channel_IMAGEPIPELINE,\n"
"		__global uint *channel_FRAMEBUFFER_MASK,\n"
"		__global float *bloomBuffer,\n"
"		const float bloomWeight) {\n"
"	const size_t gid = get_global_id(0);\n"
"	if (gid >= filmWidth * filmHeight)\n"
"		return;\n"
"\n"
"	uint maskValue = channel_FRAMEBUFFER_MASK[gid];\n"
"	if (maskValue) {\n"
"		__global float *src = &channel_IMAGEPIPELINE[gid * 3];\n"
"		__global float *dst = &bloomBuffer[gid * 3];\n"
"		\n"
"		src[0] = mix(src[0], dst[0], bloomWeight);\n"
"		src[1] = mix(src[1], dst[1], bloomWeight);\n"
"		src[2] = mix(src[2], dst[2], bloomWeight);\n"
"	}\n"
"}\n"
; } }
