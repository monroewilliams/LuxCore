#include <string>
namespace slg { namespace ocl {
std::string KernelSource_material_funcs = 
"#line 2 \"material_funcs.cl\"\n"
"\n"
"/***************************************************************************\n"
" * Copyright 1998-2013 by authors (see AUTHORS.txt)                        *\n"
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
"float3 Material_GetEmittedRadianceNoMix(__global Material *material, __global HitPoint *hitPoint\n"
"		TEXTURES_PARAM_DECL) {\n"
"	const uint emitTexIndex = material->emitTexIndex;\n"
"	if (emitTexIndex == NULL_INDEX)\n"
"		return BLACK;\n"
"\n"
"	return Texture_GetSpectrumValue(emitTexIndex, hitPoint\n"
"				TEXTURES_PARAM);\n"
"}\n"
"\n"
"#if defined(PARAM_HAS_BUMPMAPS)\n"
"void Material_BumpNoMix(__global Material *material, __global HitPoint *hitPoint,\n"
"        const float3 dpdu, const float3 dpdv,\n"
"        const float3 dndu, const float3 dndv, const float weight\n"
"        TEXTURES_PARAM_DECL) {\n"
"    if ((material->bumpTexIndex != NULL_INDEX) && (weight > 0.f)) {\n"
"        const float2 duv = weight * \n"
"#if defined(PARAM_ENABLE_TEX_NORMALMAP)\n"
"            ((texs[material->bumpTexIndex].type == NORMALMAP_TEX) ?\n"
"                NormalMapTexture_GetDuv(material->bumpTexIndex,\n"
"                    hitPoint, dpdu, dpdv, dndu, dndv, material->bumpSampleDistance\n"
"                    TEXTURES_PARAM) :\n"
"                Texture_GetDuv(material->bumpTexIndex,\n"
"                    hitPoint, dpdu, dpdv, dndu, dndv, material->bumpSampleDistance\n"
"                    TEXTURES_PARAM));\n"
"#else\n"
"            Texture_GetDuv(material->bumpTexIndex,\n"
"                hitPoint, dpdu, dpdv, dndu, dndv, material->bumpSampleDistance\n"
"                TEXTURES_PARAM);\n"
"#endif\n"
"\n"
"        const float3 oldShadeN = VLOAD3F(&hitPoint->shadeN.x);\n"
"        const float3 bumpDpdu = dpdu + duv.s0 * oldShadeN;\n"
"        const float3 bumpDpdv = dpdv + duv.s1 * oldShadeN;\n"
"        float3 newShadeN = normalize(cross(bumpDpdu, bumpDpdv));\n"
"\n"
"        // The above transform keeps the normal in the original normal\n"
"        // hemisphere. If they are opposed, it means UVN was indirect and\n"
"        // the normal needs to be reversed\n"
"        newShadeN *= (dot(oldShadeN, newShadeN) < 0.f) ? -1.f : 1.f;\n"
"\n"
"        VSTORE3F(newShadeN, &hitPoint->shadeN.x);\n"
"    }\n"
"}\n"
"#endif\n"
"\n"
"#if defined(PARAM_HAS_VOLUMES)\n"
"uint Material_GetInteriorVolumeNoMix(__global Material *material) {\n"
"	return material->interiorVolumeIndex;\n"
"}\n"
"\n"
"uint Material_GetExteriorVolumeNoMix(__global Material *material) {\n"
"	return material->exteriorVolumeIndex;\n"
"}\n"
"#endif\n"
"\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// Generic material functions\n"
"//\n"
"// They include the support for all material but Mix\n"
"// (because OpenCL doesn't support recursion)\n"
"//------------------------------------------------------------------------------\n"
"\n"
"bool Material_IsDeltaNoMix(__global Material *material) {\n"
"	switch (material->type) {\n"
"		//----------------------------------------------------------------------\n"
"		// Non Specular materials\n"
"#if defined (PARAM_ENABLE_MAT_CARPAINT)\n"
"		case CARPAINT:\n"
"			return false;\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_CLOTH)\n"
"		case CLOTH:\n"
"			return false;\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_ROUGHGLASS)\n"
"		case ROUGHGLASS:\n"
"			return false;\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_METAL2)\n"
"		case METAL2:\n"
"			return false;\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_GLOSSY2)\n"
"		case GLOSSY2:\n"
"			return false;\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_MATTETRANSLUCENT)\n"
"		case MATTETRANSLUCENT:\n"
"			return false;\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_VELVET)\n"
"		case VELVET:\n"
"			return false;\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_MATTE)\n"
"		case MATTE:\n"
"			return false;\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_ROUGHMATTE)\n"
"		case ROUGHMATTE:\n"
"			return false;\n"
"#endif\n"
"		//----------------------------------------------------------------------\n"
"		// Specular materials\n"
"#if defined (PARAM_ENABLE_MAT_MIRROR)\n"
"		case MIRROR:\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_GLASS)\n"
"		case GLASS:\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_ARCHGLASS)\n"
"		case ARCHGLASS:\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_NULL)\n"
"		case NULLMAT:\n"
"#endif\n"
"		default:\n"
"			return true;\n"
"	}\n"
"}\n"
"\n"
"BSDFEvent Material_GetEventTypesNoMix(__global Material *mat) {\n"
"	switch (mat->type) {\n"
"#if defined (PARAM_ENABLE_MAT_MATTE)\n"
"		case MATTE:\n"
"			return DIFFUSE | REFLECT;\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_ROUGHMATTE)\n"
"		case ROUGHMATTE:\n"
"			return DIFFUSE | REFLECT;\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_VELVET)\n"
"		case VELVET:\n"
"			return DIFFUSE | REFLECT;\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_MIRROR)\n"
"		case MIRROR:\n"
"			return SPECULAR | REFLECT;\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_GLASS)\n"
"		case GLASS:\n"
"			return SPECULAR | REFLECT | TRANSMIT;\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_ARCHGLASS)\n"
"		case ARCHGLASS:\n"
"			return SPECULAR | REFLECT | TRANSMIT;\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_NULL)\n"
"		case NULLMAT:\n"
"			return SPECULAR | TRANSMIT;\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_MATTETRANSLUCENT)\n"
"		case MATTETRANSLUCENT:\n"
"			return DIFFUSE | REFLECT | TRANSMIT;\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_GLOSSY2)\n"
"		case GLOSSY2:\n"
"			return DIFFUSE | GLOSSY | REFLECT;\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_METAL2)\n"
"		case METAL2:\n"
"			return GLOSSY | REFLECT;\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_ROUGHGLASS)\n"
"		case ROUGHGLASS:\n"
"			return GLOSSY | REFLECT | TRANSMIT;\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_CLOTH)\n"
"		case CLOTH:\n"
"			return GLOSSY | REFLECT;\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_CARPAINT)\n"
"		case CARPAINT:\n"
"			return GLOSSY | REFLECT;\n"
"#endif\n"
"		default:\n"
"			return NONE;\n"
"	}\n"
"}\n"
"\n"
"float3 Material_SampleNoMix(__global Material *material,\n"
"		__global HitPoint *hitPoint, const float3 fixedDir, float3 *sampledDir,\n"
"		const float u0, const float u1,\n"
"#if defined(PARAM_HAS_PASSTHROUGH)\n"
"		const float passThroughEvent,\n"
"#endif\n"
"		float *pdfW, float *cosSampledDir, BSDFEvent *event,\n"
"		const BSDFEvent requestedEvent\n"
"		TEXTURES_PARAM_DECL) {\n"
"	switch (material->type) {\n"
"#if defined (PARAM_ENABLE_MAT_MATTE)\n"
"		case MATTE:\n"
"			return MatteMaterial_Sample(material, hitPoint, fixedDir, sampledDir,\n"
"					u0, u1,\n"
"#if defined(PARAM_HAS_PASSTHROUGH)\n"
"					passThroughEvent,\n"
"#endif\n"
"					pdfW, cosSampledDir, event, requestedEvent\n"
"					TEXTURES_PARAM);\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_ROUGHMATTE)\n"
"		case ROUGHMATTE:\n"
"			return RoughMatteMaterial_Sample(material, hitPoint, fixedDir, sampledDir,\n"
"					u0, u1,\n"
"#if defined(PARAM_HAS_PASSTHROUGH)\n"
"					passThroughEvent,\n"
"#endif\n"
"					pdfW, cosSampledDir, event, requestedEvent\n"
"					TEXTURES_PARAM);\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_VELVET)\n"
"		case VELVET:\n"
"			return VelvetMaterial_Sample(material, hitPoint, fixedDir, sampledDir,\n"
"					u0, u1,\n"
"#if defined(PARAM_HAS_PASSTHROUGH)\n"
"					passThroughEvent,\n"
"#endif\n"
"					pdfW, cosSampledDir, event, requestedEvent\n"
"					TEXTURES_PARAM);\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_MIRROR)\n"
"		case MIRROR:\n"
"			return MirrorMaterial_Sample(material, hitPoint, fixedDir, sampledDir,\n"
"					u0, u1,\n"
"#if defined(PARAM_HAS_PASSTHROUGH)\n"
"					passThroughEvent,\n"
"#endif\n"
"					pdfW, cosSampledDir, event, requestedEvent\n"
"					TEXTURES_PARAM);\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_GLASS)\n"
"		case GLASS:\n"
"			return GlassMaterial_Sample(material, hitPoint, fixedDir, sampledDir,\n"
"					u0, u1,\n"
"#if defined(PARAM_HAS_PASSTHROUGH)\n"
"					passThroughEvent,\n"
"#endif\n"
"					pdfW, cosSampledDir, event, requestedEvent\n"
"					TEXTURES_PARAM);\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_ARCHGLASS)\n"
"		case ARCHGLASS:\n"
"			return ArchGlassMaterial_Sample(material, hitPoint, fixedDir, sampledDir,\n"
"					u0, u1,\n"
"#if defined(PARAM_HAS_PASSTHROUGH)\n"
"					passThroughEvent,\n"
"#endif\n"
"					pdfW, cosSampledDir, event, requestedEvent\n"
"					TEXTURES_PARAM);\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_NULL)\n"
"		case NULLMAT:\n"
"			return NullMaterial_Sample(material, hitPoint, fixedDir, sampledDir,\n"
"					u0, u1,\n"
"#if defined(PARAM_HAS_PASSTHROUGH)\n"
"					passThroughEvent,\n"
"#endif\n"
"					pdfW, cosSampledDir, event, requestedEvent\n"
"					TEXTURES_PARAM);\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_MATTETRANSLUCENT)\n"
"		case MATTETRANSLUCENT:\n"
"			return MatteTranslucentMaterial_Sample(material, hitPoint, fixedDir, sampledDir,\n"
"					u0, u1,\n"
"#if defined(PARAM_HAS_PASSTHROUGH)\n"
"					passThroughEvent,\n"
"#endif\n"
"					pdfW, cosSampledDir, event, requestedEvent\n"
"					TEXTURES_PARAM);\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_GLOSSY2)\n"
"		case GLOSSY2:\n"
"			return Glossy2Material_Sample(material, hitPoint, fixedDir, sampledDir,\n"
"					u0, u1,\n"
"#if defined(PARAM_HAS_PASSTHROUGH)\n"
"					passThroughEvent,\n"
"#endif\n"
"					pdfW, cosSampledDir, event, requestedEvent\n"
"					TEXTURES_PARAM);\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_METAL2)\n"
"		case METAL2:\n"
"			return Metal2Material_Sample(material, hitPoint, fixedDir, sampledDir,\n"
"					u0, u1,\n"
"#if defined(PARAM_HAS_PASSTHROUGH)\n"
"					passThroughEvent,\n"
"#endif\n"
"					pdfW, cosSampledDir, event, requestedEvent\n"
"					TEXTURES_PARAM);\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_ROUGHGLASS)\n"
"		case ROUGHGLASS:\n"
"			return RoughGlassMaterial_Sample(material, hitPoint, fixedDir, sampledDir,\n"
"					u0, u1,\n"
"#if defined(PARAM_HAS_PASSTHROUGH)\n"
"					passThroughEvent,\n"
"#endif\n"
"					pdfW, cosSampledDir, event, requestedEvent\n"
"					TEXTURES_PARAM);\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_CLOTH)\n"
"		case CLOTH:\n"
"			return ClothMaterial_Sample(material, hitPoint, fixedDir, sampledDir,\n"
"					u0, u1,\n"
"#if defined(PARAM_HAS_PASSTHROUGH)\n"
"					passThroughEvent,\n"
"#endif\n"
"					pdfW, cosSampledDir, event, requestedEvent\n"
"					TEXTURES_PARAM);\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_CARPAINT)\n"
"		case CARPAINT:\n"
"			return CarpaintMaterial_Sample(material, hitPoint, fixedDir, sampledDir,\n"
"					u0, u1,\n"
"#if defined(PARAM_HAS_PASSTHROUGH)\n"
"					passThroughEvent,\n"
"#endif\n"
"					pdfW, cosSampledDir, event, requestedEvent\n"
"					TEXTURES_PARAM);\n"
"#endif\n"
"		default:\n"
"			return BLACK;\n"
"	}\n"
"}\n"
"\n"
"float3 Material_EvaluateNoMix(__global Material *material,\n"
"		__global HitPoint *hitPoint, const float3 lightDir, const float3 eyeDir,\n"
"		BSDFEvent *event, float *directPdfW\n"
"		TEXTURES_PARAM_DECL) {\n"
"	switch (material->type) {\n"
"#if defined (PARAM_ENABLE_MAT_MATTE)\n"
"		case MATTE:\n"
"			return MatteMaterial_Evaluate(material, hitPoint, lightDir, eyeDir, event, directPdfW\n"
"					TEXTURES_PARAM);\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_ROUGHMATTE)\n"
"		case ROUGHMATTE:\n"
"			return RoughMatteMaterial_Evaluate(material, hitPoint, lightDir, eyeDir, event, directPdfW\n"
"					TEXTURES_PARAM);\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_VELVET)\n"
"		case VELVET:\n"
"			return VelvetMaterial_Evaluate(material, hitPoint, lightDir, eyeDir, event, directPdfW\n"
"					TEXTURES_PARAM);\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_MATTETRANSLUCENT)\n"
"		case MATTETRANSLUCENT:\n"
"			return MatteTranslucentMaterial_Evaluate(material, hitPoint, lightDir, eyeDir, event, directPdfW\n"
"					TEXTURES_PARAM);\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_GLOSSY2)\n"
"		case GLOSSY2:\n"
"			return Glossy2Material_Evaluate(material, hitPoint, lightDir, eyeDir, event, directPdfW\n"
"					TEXTURES_PARAM);\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_METAL2)\n"
"		case METAL2:\n"
"			return Metal2Material_Evaluate(material, hitPoint, lightDir, eyeDir, event, directPdfW\n"
"					TEXTURES_PARAM);\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_ROUGHGLASS)\n"
"		case ROUGHGLASS:\n"
"			return RoughGlassMaterial_Evaluate(material, hitPoint, lightDir, eyeDir, event, directPdfW\n"
"					TEXTURES_PARAM);\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_CLOTH)\n"
"		case CLOTH:\n"
"			return ClothMaterial_Evaluate(material, hitPoint, lightDir, eyeDir, event, directPdfW\n"
"					TEXTURES_PARAM);\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_CARPAINT)\n"
"		case CARPAINT:\n"
"			return CarpaintMaterial_Evaluate(material, hitPoint, lightDir, eyeDir, event, directPdfW\n"
"					TEXTURES_PARAM);\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_MIRROR)\n"
"		case MIRROR:\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_GLASS)\n"
"		case GLASS:\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_ARCHGLASS)\n"
"		case ARCHGLASS:\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_NULL)\n"
"		case NULLMAT:\n"
"#endif\n"
"		default:\n"
"			return BLACK;\n"
"	}\n"
"}\n"
"\n"
"float3 Material_GetPassThroughTransparencyNoMix(__global Material *material,\n"
"		__global HitPoint *hitPoint, const float3 fixedDir, const float passThroughEvent\n"
"		TEXTURES_PARAM_DECL) {\n"
"	switch (material->type) {\n"
"#if defined (PARAM_ENABLE_MAT_ARCHGLASS)\n"
"		case ARCHGLASS:\n"
"			return ArchGlassMaterial_GetPassThroughTransparency(material,\n"
"					hitPoint, fixedDir, passThroughEvent\n"
"					TEXTURES_PARAM);\n"
"#endif\n"
"#if defined (PARAM_ENABLE_MAT_NULL)\n"
"		case NULLMAT:\n"
"			return WHITE;\n"
"#endif\n"
"		default:\n"
"			return BLACK;\n"
"	}\n"
"}\n"
; } }
