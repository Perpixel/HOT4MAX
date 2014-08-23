/*
Houdini Ocean Toolkit for Autodesk 3ds Max

Copyright (C) 2013

Guillaume Plourde, gplourde@gmail.com
Drew Whitehouse, ANU Supercomputer Facility, Drew.Whitehouse@anu.edu.au
Christian Schnellhammer, cs3d@gmx.de
Nico Rehberg, mail@nico-rehberg.de

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "max.h"

#include "simpmod.h"
#include "simpobj.h"
#include "iparamm2.h"

// ----------------------------------------------------------------------------------------------
// Paramblock2


enum {
	hot_params
};

enum {
	hot_resolution,
	hot_size,
	hot_windspeed,
	hot_waveheight,
	hot_shortestwave,
	hot_choppiness,
	hot_winddirection,
	hot_dampreflection,
	hot_windalign,
	hot_oceandepth,
	hot_time,
	hot_seed,
	hot_interpolation,
	hot_scale,
	hot_dochop,
	hot_dofoam,
	hot_foamscale
};

static ParamBlockDescID descVer0[] = {
	{ TYPE_FLOAT, NULL, TRUE, hot_resolution },
	{ TYPE_FLOAT, NULL, TRUE, hot_size },
	{ TYPE_FLOAT, NULL, TRUE, hot_windspeed },
	{ TYPE_FLOAT, NULL, TRUE, hot_waveheight },
	{ TYPE_FLOAT, NULL, TRUE, hot_shortestwave },
	{ TYPE_FLOAT, NULL, TRUE, hot_choppiness },
	{ TYPE_FLOAT, NULL, TRUE, hot_winddirection },
	{ TYPE_FLOAT, NULL, TRUE, hot_dampreflection },
	{ TYPE_FLOAT, NULL, TRUE, hot_windalign },
	{ TYPE_FLOAT, NULL, TRUE, hot_oceandepth },
	{ TYPE_FLOAT, NULL, TRUE, hot_time },
	{ TYPE_FLOAT, NULL, TRUE, hot_seed },
	{ TYPE_FLOAT, NULL, TRUE, hot_scale},
	{ TYPE_BOOL, NULL, TRUE, hot_interpolation},
	{ TYPE_BOOL, NULL, TRUE, hot_dochop},
	{ TYPE_BOOL, NULL, TRUE, hot_dofoam},
	{ TYPE_FLOAT, NULL, TRUE, hot_foamscale}
};

static ParamVersionDesc versions[] =
{
	ParamVersionDesc(descVer0, 17, 0)
};



static ParamBlockDesc2 hot_param_blk ( hot_params, _T("Ocean Parameters"),  0, &hotDesc, P_AUTO_CONSTRUCT + P_AUTO_UI, HOT_PBLOCK_REF, 
	//
	IDD_HOTPARAM, IDS_RB_PARAMETERS, 0, 0, &theHotProc,
	//
	hot_resolution, _M("Resolution"), TYPE_FLOAT, 0, IDS_RESOLUTION,
		p_default, 6.0f,
		p_range, 4.0f, MAX_RESOLUTION,
		p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_RESOLUTION, IDC_RESOLUTIONSPN, 1.0f, end,
	//
	hot_size, _M("Size"), TYPE_FLOAT, 0, IDS_SIZE,
		p_default, 200.0f,
		p_range, 1.0f, 999999.0f,
		p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_OSIZE, IDC_SIZESPN, 1.0f, end,
	//
	hot_windspeed, _M("Wind Speed"), TYPE_FLOAT, P_ANIMATABLE, IDS_WINDSPEED,
		p_default, 30.0f,
		p_range, 0.0f, 999999.0f,
		p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_WINDSPEED, IDC_WINDSPEEDSPN, 1.0f, end,
	//
	hot_waveheight, _M("Wave Height"), TYPE_FLOAT, P_ANIMATABLE, IDS_WAVEHEIGHT,
		p_default, 1.0f,
		p_range, 0.0001f, 100.0f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_WAVEHEIGHT, IDC_WAVEHEIGHTSPN, 0.01f, end,
	//
	hot_shortestwave, _M("Shortest Wave"), TYPE_FLOAT, P_ANIMATABLE, IDS_SHORTESTWAVE,
		p_default, 0.02f,
		p_range, 0.01f, 100.0f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_SHORTESTWAVE, IDC_SHORTESTWAVESPN, 0.1f, end,
	//
	hot_choppiness, _M("Choppiness"), TYPE_FLOAT, P_ANIMATABLE, IDS_CHOPPINESS,
		p_default, 0.7f,
		p_range, 0.0f, 100.0f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_CHOPPINESS, IDC_CHOPPINESSSPN, 0.01f, end,
	//
	hot_winddirection, _M("Wind Direction"), TYPE_FLOAT, P_ANIMATABLE, IDS_WINDDIRECTION,
		p_default, 0.0f,
		p_range, 0.0f, 360.0f,
		p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_WINDDIRECTION, IDC_WINDDIRECTIONSPN, 1.0f, end,
	//
	hot_dampreflection, _M("Damp Reflections"), TYPE_FLOAT, P_ANIMATABLE, IDS_DAMPREFLECTION,
		p_default, 1.0f,
		p_range, 0.0f, 1.0f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_DAMPREFLECTION, IDC_DAMPREFLECTIONSPN, 0.01f, end,
	//
	hot_windalign, _M("Wind Align"), TYPE_FLOAT, P_ANIMATABLE, IDS_WINDALIGN,
		p_default, 2.0f,
		p_range, 0.01f, 10.0f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_WINDALIGN, IDC_WINDALIGNSPN, 0.01f, end,
	//
	hot_oceandepth, _M("Ocean Depth"), TYPE_FLOAT, P_ANIMATABLE, IDS_OCEANDEPTH,
		p_default, 200.0f,
		p_range, 0.01f, 3000.0f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_OCEANDEPTH, IDC_OCEANDEPTHSPN, 1.0f, end,
	//
	hot_time, _M("Time"), TYPE_FLOAT, P_ANIMATABLE, IDS_TIME,
		p_default, 0.0f,
		p_range, 0.0f, 3000.0f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_TIME, IDC_TIMESPN, 0.1f, end,
	//
	hot_seed, _M("Seed"), TYPE_FLOAT, 0, IDS_SEED,
		p_default, 0.0f,
		p_range, 0.0f, 999999.0f,
		p_ui, TYPE_SPINNER, EDITTYPE_INT, IDC_SEED, IDC_SEEDSPN, 1.0f, end,
	//
	hot_scale, _M("Scale"), TYPE_FLOAT, P_ANIMATABLE, IDS_SCALE,
		p_default, 1.0f,
		p_range, 0.001f, 999999.0f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_SCALE, IDC_SCALESPN, 0.01f, end,
	//
	hot_interpolation, _M("Interpolation"), TYPE_BOOL, 0, IDS_INTERPOLATION,
		p_default, TRUE, 
		p_ui, TYPE_SINGLECHEKBOX, IDC_INTERPOLATION, end, 
	//
	hot_dochop, _M("Chop"), TYPE_BOOL, 0, IDS_DOCHOP,
		p_default, TRUE, 
		p_ui, TYPE_SINGLECHEKBOX, IDC_DOCHOP, end,
	//
	hot_dofoam, _M("Foam"), TYPE_BOOL, 0, IDS_DOFOAM,
		p_default, TRUE, 
		p_ui, TYPE_SINGLECHEKBOX, IDC_DOFOAM, end,
	//
	hot_foamscale, _M("Foam Scale"), TYPE_FLOAT, P_ANIMATABLE, IDS_FOAMSCALE,
		p_default, 0.0f,
		p_range, 0.0f, 2.0f,
		p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_FOAMSCALE, IDC_FOAMSCALESPN, 0.01f, end,
	end
	);