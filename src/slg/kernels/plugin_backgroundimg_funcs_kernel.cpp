#include <string>
namespace slg { namespace ocl {
std::string KernelSource_plugin_backgroundimg_funcs = 
"#line 2 \"plugin_backgroundimg_funcs.cl\"\n"
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
"// BackgroundImgPlugin_Apply\n"
"//------------------------------------------------------------------------------\n"
"\n"
"__kernel __attribute__((work_group_size_hint(256, 1, 1))) void BackgroundImgPlugin_Apply(\n"
"		const uint filmWidth, const uint filmHeight,\n"
"		__global float *channel_IMAGEPIPELINE,\n"
"		__global uint *channel_FRAMEBUFFER_MASK,\n"
"		__global float *channel_ALPHA,\n"
"		__global const ImageMap *imageMapDesc,\n"
"		__global const float *imageMapBuff) {\n"
"	const size_t gid = get_global_id(0);\n"
"	if (gid >= filmWidth * filmHeight)\n"
"		return;\n"
"\n"
"	const uint maskValue = channel_FRAMEBUFFER_MASK[gid];\n"
"	if (maskValue) {\n"
"		const uint x = gid % filmWidth;\n"
"		const uint y = gid / filmWidth;\n"
"\n"
"		const uint s = x;\n"
"		// Need to flip the along the Y axis for the image\n"
"		const uint t = filmHeight - y - 1;\n"
"		const float3 backgroundPixel = ImageMap_GetTexel_Spectrum(imageMapDesc->storageType,\n"
"				imageMapBuff, imageMapDesc->width, imageMapDesc->height,\n"
"				imageMapDesc->channelCount, s, t);\n"
"\n"
"		__global float *alphaPixel = &channel_ALPHA[gid * 2];\n"
"		const float alpha = alphaPixel[0] / alphaPixel[1];\n"
"\n"
"		__global float *pixel = &channel_IMAGEPIPELINE[gid * 3];\n"
"		pixel[0] = mix(backgroundPixel.s0, pixel[0], alpha);\n"
"		pixel[1] = mix(backgroundPixel.s1, pixel[1], alpha);\n"
"		pixel[2] = mix(backgroundPixel.s2, pixel[2], alpha);\n"
"	}\n"
"}\n"
; } }
