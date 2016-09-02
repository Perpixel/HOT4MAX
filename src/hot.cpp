/*
Houdini Ocean Toolkit for Autodesk 3ds Max

Copyright (C) 2015

Guillaume Plourde, gplourde@gmail.com
Drew Whitehouse, ANU Supercomputer Facility, Drew.Whitehouse@anu.edu.au

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

#include "hot.h"
#include "paramblock.h"
#include "MeshDLib.h"

HINSTANCE hInstance;

static GenSubObjType SOT_Center(18);

static FPInterfaceDesc hot_interface(
	HOT_INTERFACE, _T("hot"), 0, &hotDesc, FP_MIXIN,
	IHoudiniOcean::hot_getpointjminus, _T("GetPointJminus"), 0, TYPE_FLOAT, 0, 1, _T("point"), 0, TYPE_POINT2,
	IHoudiniOcean::hot_savejminusmap, _T("SaveJminusMap"), -1, TYPE_VOID, 0, 1, _T("filename"), -1, TYPE_STRING,
	IHoudiniOcean::hot_saveheightmap, _T("SaveHeightMap"), -1, TYPE_VOID, 0, 1, _T("filename"), -1, TYPE_STRING,
	IHoudiniOcean::hot_getpointeminus, _T("GetPointEminus"), 0, TYPE_POINT3, 0, 1, _T("point"), 0, TYPE_POINT2,
	end
);

FPInterfaceDesc* IHoudiniOcean::GetDesc()
{
	return &hot_interface;
}

static INT_PTR CALLBACK AboutDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
			CenterWindow(hWnd, GetParent(hWnd));
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					EndDialog(hWnd,1);
					break;

				case IDC_STATIC_PICTURE:
					ShellExecuteW(hWnd, L"open", L"https://twitter.com/Hot4MAX", NULL, NULL, SW_SHOW);
					break;
			}
			break;
		
		case WM_NOTIFY:
			switch (((NMHDR *)lParam)->code)
			{
    
				case NM_CLICK:

				case NM_RETURN:
				{
					NMLINK* pNMLink = (NMLINK*)lParam;
					LITEM item = pNMLink->item;
            
					ShellExecuteW(hWnd, L"open", item.szUrl, NULL, NULL, SW_SHOW);
					EndDialog(hWnd,1);
					break;
				}
			}
			break;

		default:
			return FALSE;
	}
	return FALSE;
}
IObjParam*          HotMod::ip          = NULL;
MoveModBoxCMode*	HotMod::moveMode	= NULL;
RotateModBoxCMode*	HotMod::rotMode		= NULL;
UScaleModBoxCMode*	HotMod::uscaleMode	= NULL;
NUScaleModBoxCMode*	HotMod::nuscaleMode = NULL;
SquashModBoxCMode*	HotMod::squashMode	= NULL;	

INT_PTR HotDlgProc::DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd,UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_STATIC_PICTURE:
					ShellExecuteW(hWnd, L"open", L"https://twitter.com/Hot4MAX", NULL, NULL, SW_SHOW);
					break;

				case IDC_ABOUT:
					DialogBox(hInstance, MAKEINTRESOURCE(IDD_ABOUT), hWnd, AboutDlgProc);

					break;
			}    
		break;
	}
	return FALSE;
}

HotMod::HotMod() 
{
	_ocean = 0;
	_ocean_context = 0;
	_ocean_scale = 1.0f;

	version = HOT4MAX_VERSION;

	_ocean_needs_rebuild = true;
	
	pblock2 = NULL;
	tmControl = NULL;
	hotDesc.MakeAutoParamBlocks(this);
}

HotMod::~HotMod()
{
	if (_ocean) delete _ocean;
	if (_ocean_context) delete _ocean_context;
}

void HotMod::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	this->ip = ip;

	TimeValue t = ip->GetTime();
	NotifyDependents(Interval(t,t), PART_ALL, REFMSG_BEGIN_EDIT);
	NotifyDependents(Interval(t,t), PART_ALL, REFMSG_MOD_DISPLAY_ON);
	SetAFlag(A_MOD_BEING_EDITED);
	hotDesc.BeginEditParams(ip, this, flags, prev);	

	moveMode    = new MoveModBoxCMode(this,ip);
	rotMode     = new RotateModBoxCMode(this,ip);
	uscaleMode  = new UScaleModBoxCMode(this,ip);
	nuscaleMode = new NUScaleModBoxCMode(this,ip);
	squashMode  = new SquashModBoxCMode(this,ip);	


}
		
void HotMod::EndEditParams( IObjParam *ip, ULONG flags, Animatable *next )
{
	TimeValue t = ip->GetTime();
	ClearAFlag(A_MOD_BEING_EDITED);
	NotifyDependents(Interval(t,t), PART_ALL, REFMSG_END_EDIT);
	NotifyDependents(Interval(t,t), PART_ALL, REFMSG_MOD_DISPLAY_OFF);
	hotDesc.EndEditParams(ip,this,flags,next);

	this->ip = NULL;
		
	ip->DeleteMode(moveMode);
	ip->DeleteMode(rotMode);
	ip->DeleteMode(uscaleMode);
	ip->DeleteMode(nuscaleMode);
	ip->DeleteMode(squashMode);	

	delete moveMode;	moveMode	= NULL;
	delete rotMode;		rotMode		= NULL;
	delete uscaleMode;	uscaleMode	= NULL;
	delete nuscaleMode; nuscaleMode = NULL;
	delete squashMode;	squashMode	= NULL;


}

IOResult HotMod::Load(ILoad *iload)
{
	Modifier::Load(iload);
	ParamBlock2PLCB* plcb = new ParamBlock2PLCB(versions, 1, &hot_param_blk, this, SIMPMOD_PBLOCKREF);
	iload->RegisterPostLoadCallback(plcb);
	
	return IO_OK;
}

#define VERSION_CHUNK	0x1000

IOResult HotMod::Save(ISave *isave)
{
	ULONG nb;
	Modifier::Save(isave);
	isave->BeginChunk (VERSION_CHUNK);
	isave->Write (&version, sizeof(int), &nb);
	isave->EndChunk();
	return IO_OK;
}

RefTargetHandle HotMod::Clone(RemapDir& remap)
{	
	HotMod* newmod = new HotMod();	
	newmod->ReplaceReference(SIMPMOD_PBLOCKREF,remap.CloneRef(pblock2));
	newmod->ReplaceReference(TM_REF,remap.CloneRef(tmControl));
	BaseClone(this, newmod, remap);
	return(newmod);
}

#if MAXVERSION < 2015 
RefResult HotMod::NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message)
#else
RefResult HotMod::NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate)
#endif
{
	switch (message)
	{
		case REFMSG_CHANGE:
			if (hTarget == pblock2)
			{
				ParamID changing_param = pblock2->LastNotifyParamID();
				hot_param_blk.InvalidateUI(changing_param);

				if (changing_param != 10 && changing_param != 3 && changing_param != 5)
					_ocean_needs_rebuild = true;
			}
	}
	return REF_SUCCEED;
}

Interval HotMod::GetValidity(TimeValue t)
{
	Interval iv = FOREVER;
	
	pblock2->GetValue(hot_resolution, t, resolution, iv);
	pblock2->GetValue(hot_size, t, size, iv);
	pblock2->GetValue(hot_windspeed, t, windSpeed, iv);
	pblock2->GetValue(hot_waveheight, t, waveHeigth, iv);
	pblock2->GetValue(hot_shortestwave, t, shortestWave, iv);
	pblock2->GetValue(hot_choppiness, t, choppiness, iv);
	pblock2->GetValue(hot_winddirection, t, windDirection, iv);
	pblock2->GetValue(hot_dampreflection, t, dampReflections, iv);
	pblock2->GetValue(hot_windalign, t, windAlign, iv);
	pblock2->GetValue(hot_oceandepth, t, oceanDepth, iv);
	pblock2->GetValue(hot_time, t, time, iv);
	pblock2->GetValue(hot_seed, t, seed, iv);
	pblock2->GetValue(hot_scale, t, scale, iv);
	pblock2->GetValue(hot_interpolation, t, do_interp, iv);
	pblock2->GetValue(hot_dochop, t, do_chop, iv);

	// Foam
	pblock2->GetValue(hot_dofoam, t, do_foam, iv);
	pblock2->GetValue(hot_foamscale, t, foam_scale, iv);

	// Center
	Matrix3 mat(1);
	if (tmControl) tmControl->GetValue(t, &mat, iv, CTRL_RELATIVE);

	return iv;
}

void HotMod::UpdateOcean(TimeValue t)
{
	GetValidity(t);
	
	int gridres  = 1 << int((int)resolution);
	float stepsize = (float)size / (float)gridres;

	bool do_jacobian = true;
    bool do_normals  = false; //true && !do_chop;

	if (!_ocean || _ocean_needs_rebuild)
    {
		if (_ocean) delete _ocean;
		if (_ocean_context) delete _ocean_context;

		_ocean = new drw::Ocean(
			gridres,
			gridres,
			stepsize,
			stepsize,
			windSpeed,
			shortestWave,
			1.0f,
			windDirection / 180.0f * M_PI,
			1.0f - dampReflections,
			windAlign,
			oceanDepth,
			seed
			);

        _ocean_scale = _ocean->get_height_normalize_factor();
        _ocean_context = _ocean->new_context(true, do_chop, do_normals, do_jacobian);

		_ocean_needs_rebuild = false;
     }

    _ocean->update(time, *_ocean_context, true, do_chop, do_normals, do_jacobian, _ocean_scale * waveHeigth, choppiness);
}

void HotMod::ModifyTriObject(TimeValue t, ModContext &mc, TriObject* obj, Interval iv, Point2 center)
{
	const float envGlobalScale = GetMasterScale(UNITS_METERS);
	const float oneOverGlobalScale = scale / GetMasterScale(UNITS_METERS);

	obj->ReadyChannelsForMod(GEOM_CHANNEL);
	obj->ReadyChannelsForMod(VERTCOLOR_CHANNEL);

	Mesh* mesh = &obj->GetMesh();

	
	if (!mesh) return;
	if (mesh->selLevel == MESH_EDGE) return;

	mesh->setNumVertCol(mesh->numVerts);
	mesh->setNumVCFaces(mesh->numFaces);

	for (int i = 0; i < mesh->numVerts; i++)
	{
		drw::EvalData evaldata;  

		Point3 p = mesh->getVert(i);

		if (do_interp)
			_ocean_context->eval2_xz(envGlobalScale * (p.x - center.x), envGlobalScale * (p.y - center.y), evaldata);
		else
			_ocean_context->eval_xz(envGlobalScale * (p.x - center.x), envGlobalScale * (p.y - center.y), evaldata);

		if (do_foam)
		{
			Point3 foam = Point3(0.0f, 0.0f, 0.0f);

			float jm = evaldata.Jminus - foam_scale;			
			
			if (jm < 0.0f)
			{
				float c = -jm / 1.5f;

				if (c > 1)
					c = 1.0f;

				foam.x = c;
				foam.y = c;
				foam.z = c;
			}
			
			mesh->vertCol[i] = foam;
		}
	}

	for (int i=0; i < mesh->numFaces; i++)
	{
		mesh->vcFace[i].t[0] = mesh->faces[i].v[0];
		mesh->vcFace[i].t[1] = mesh->faces[i].v[1];
		mesh->vcFace[i].t[2] = mesh->faces[i].v[2];
	}
}

Matrix3 HotMod::CompMatrix (TimeValue t, INode *inode, ModContext *mc, Interval *validity) 
{
	Interval iv;
	Matrix3 tm(1);		
	if (tmControl) tmControl->GetValue(t,&tm,iv,CTRL_RELATIVE);
	if (mc && mc->tm) tm = tm * Inverse(*(mc->tm));
	if (inode) 
	{
		tm = tm * inode->GetObjTMBeforeWSM(t,&iv);
	}
	return tm;
}

void HotMod::ModifyObject(TimeValue t, ModContext &mc, ObjectState *os, INode *node)
{
	if (!tmControl)
	{
		ReplaceReference (TM_REF, NewDefaultMatrix3Controller ());
	}

	Interval iv = GetValidity(t);

	// Build Ocean
	int gridres  = 1 << int((int)resolution);
	float stepsize = (float)size / (float)gridres;

    bool do_jacobian = true;
    bool do_normals  = false; //true && !do_chop;

	if (!_ocean || _ocean_needs_rebuild)
    {
		if (_ocean) delete _ocean;
		if (_ocean_context) delete _ocean_context;

		_ocean = new drw::Ocean(
			gridres,
			gridres,
			stepsize,
			stepsize,
			windSpeed,
			shortestWave,
			0.00001f,
			windDirection / 180.0f * M_PI,
			1.0f - dampReflections,
			windAlign,
			oceanDepth,
			seed
			);

        _ocean_scale = _ocean->get_height_normalize_factor();
        _ocean_context = _ocean->new_context(true, do_chop, do_normals, do_jacobian);
		_ocean_needs_rebuild = false;
		updateNeeded = true;
     }

	if (updateNeeded ||
		PRV_windSpeed != windSpeed ||
		PRV_waveHeigth != waveHeigth ||
		PRV_shortestWave != shortestWave ||
		PRV_choppiness != choppiness ||
		PRV_windDirection != windDirection ||
		PRV_dampReflections != dampReflections ||
		PRV_windAlign != windAlign ||
		PRV_oceanDepth != oceanDepth ||
		PRV_time != time ||
		PRV_scale != scale)
	{
		_ocean->update(time, *_ocean_context, true, do_chop, do_normals, do_jacobian, _ocean_scale * waveHeigth, choppiness);
		updateNeeded = false;
	}

	Matrix3 tm  = CompMatrix (t, NULL, &mc, &iv);
	Point3 p = tm.GetTrans();

	Point2 offset = Point2(p.x, p.y);

	if (os->obj->IsMappable() && do_foam)
	{
		if (os->obj->IsSubClassOf(triObjectClassID))
		{
			TriObject *tobj = (TriObject*)os->obj;
			ModifyTriObject (t, mc, tobj, iv, offset);
		}
		else if (os->obj->IsSubClassOf(polyObjectClassID))
		{
			PolyObject *pPolyObj = (PolyObject*)os->obj;
			//ModifyPolyObject (t, mc, pPolyObj, iv, offset);
		}
	}
	else
	{
		//os->obj->Deform(&GetDeformer(t, offset));
	}
	
	os->obj->Deform(&GetDeformer(t, mc, offset), TRUE);

	// Base function

	// Update previous ocean setting to current value
	PRV_windSpeed = windSpeed;
	PRV_waveHeigth = waveHeigth;
	PRV_shortestWave = shortestWave;
	PRV_choppiness = choppiness;
	PRV_windDirection = windDirection;
	PRV_dampReflections = dampReflections;
	PRV_windAlign = windAlign;
	PRV_oceanDepth = oceanDepth;
	PRV_time = time;
	PRV_scale = scale;

	os->obj->UpdateValidity(GEOM_CHAN_NUM, iv);
}

BOOL HotMod::AssignController(Animatable *control,int subAnim)
{
	if (subAnim==TM_REF) {
		ReplaceReference(TM_REF,(ReferenceTarget*)control);
		NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
		NotifyDependents(FOREVER,PART_ALL,REFMSG_SUBANIM_STRUCTURE_CHANGED);		
		return TRUE;
	} else {
		return FALSE;
	}
}

int HotMod::SubNumToRefNum(int subNum)
{
	if (subNum==TM_REF) return subNum;
	else return -1;
}

RefTargetHandle HotMod::GetReference(int i)
{
	switch (i)
	{
		case HOT_PBLOCK_REF: return pblock2;
		case TM_REF:     return tmControl;
		default: return NULL;
	}
}

void HotMod::SetReference(int i, RefTargetHandle rtarg)
{
	switch (i)
	{
		case HOT_PBLOCK_REF: pblock2 = (IParamBlock2*)rtarg; break;
		case TM_REF:     tmControl = (Control*)rtarg; break;	
	}
}

TSTR HotMod::SubAnimName(int i) 
{
	switch (i) {
	case HOT_PBLOCK_REF: return TSTR(GetString(IDS_RB_PARAMETERS)); break;
	case TM_REF: return TSTR(GetString(IDS_CENTER)); break;
	default: return TSTR(_T("")); break;
	}
};

#ifndef UNSUPPORTED
float HotMod::fnGetPointJminus(Point2 p)
{
	drw::EvalData evaldata; 

	//_ocean_context->eval_ij(p.x, p.y, evaldata);

	if (do_interp)
		_ocean_context->eval2_xz(p.x, p.y, evaldata);
	else
		_ocean_context->eval_xz(p.x, p.y, evaldata);

	float jm = evaldata.Jminus;

	return jm;
}
#endif

inline UWORD FlToWord(float r)
{
	return (UWORD)(65535.0f*r);
}

void HotMod::fnSaveJminusMap(const TCHAR *filename)
{
	int gridres  = 1 << int((int)resolution);

	BitmapInfo bi;
	static MaxSDK::AssetManagement::AssetUser bitMapAssetUser;
	if (bitMapAssetUser.GetId() == MaxSDK::AssetManagement::kInvalidId)
	bitMapAssetUser = MaxSDK::AssetManagement::IAssetManager::GetInstance()->GetAsset(_T("hotTemp"), MaxSDK::AssetManagement::kBitmapAsset);
	bi.SetAsset(bitMapAssetUser);
	bi.SetWidth(gridres);
	bi.SetHeight(gridres);
	bi.SetType(BMM_TRUE_32);
	Bitmap *bm = TheManager->Create(&bi);
	if (bm==NULL) return;
	PixelBuf l64(gridres);

	for (int y=0; y<gridres; y++)
	{
		BMM_Color_64 *p64=l64.Ptr();

		for (int x=0; x<gridres; x++, p64++)
		{
			drw::EvalData evaldata;

			_ocean_context->eval_ij(x, y, evaldata);
			
			float jm = evaldata.Jminus - foam_scale;
			AColor foam = AColor(0.0f, 0.0f, 0.0f, 0.0f);

			if (jm < 0.0f)
			{
				float c = -jm / 1.5f;

				if (c > 1)
					c = 1.0f;

				foam.r = 255 * c;
				foam.g = 255 * c;
				foam.b = 255 * c;
			}

			p64->r = FlToWord(foam.r); 
			p64->g = FlToWord(foam.g); 
			p64->b = FlToWord(foam.b);
			p64->a = 0xffff; 
		}
		bm->PutPixels(0,y, gridres, l64.Ptr()); 
	}

	bm->Display();
}

void HotMod::fnSaveHeightMap(const TCHAR *filename)
{
	int gridres  = 1 << int((int)resolution);

	BitmapInfo bi;
	static MaxSDK::AssetManagement::AssetUser bitMapAssetUser;
	
	if (bitMapAssetUser.GetId() == MaxSDK::AssetManagement::kInvalidId)
		bitMapAssetUser = MaxSDK::AssetManagement::IAssetManager::GetInstance()->GetAsset(_T("hotTemp"), MaxSDK::AssetManagement::kBitmapAsset);

	bi.SetAsset(bitMapAssetUser);
	bi.SetWidth(gridres);
	bi.SetHeight(gridres);
	bi.SetType(BMM_TRUE_32);
	Bitmap *bm = TheManager->Create(&bi);
	if (bm==NULL) return;
	PixelBuf l64(gridres);

	for (int y=0; y<gridres; y++)
	{
		BMM_Color_64 *p64=l64.Ptr();

		for (int x=0; x<gridres; x++, p64++)
		{
			drw::EvalData e;
			_ocean_context->eval_ij(x, y, e);
			
			float displacement_y = e.disp[2];
			AColor h = AColor(0.0f, 0.0f, 0.0f, 0.0f);

			if (displacement_y < 0.0f)
			{
				h.r = displacement_y;
				h.g = displacement_y;
				h.b = displacement_y;
			}

			p64->r = FlToWord(h.r); 
			p64->g = FlToWord(h.g); 
			p64->b = FlToWord(h.b);
			p64->a = 0xffff; 
		}
		bm->PutPixels(0,y, gridres, l64.Ptr()); 
	}

	bm->Display();
}

Point3 HotMod::fnGetPointEminus(Point2 p)
{
	drw::EvalData e;

	if (do_interp)
		_ocean_context->eval2_xz(p.x, p.y, e);
	else
		_ocean_context->eval_xz(p.x, p.y, e);

	float x = e.Eminus[0];
	float y = e.Eminus[2];
	float z = e.Eminus[1];

	return Point3(x, y, z);
}

// ------------------------------------------------------------------------------------------------------
// Deformer plugin code


HotDeformer::HotDeformer(drw::Ocean* _o, drw::OceanContext* _oc, float scale, bool interp, bool chop, ModContext &mc, Point2 p)
{
	envGlobalScale = GetMasterScale(UNITS_METERS);
	oneOverGlobalScale = scale / GetMasterScale(UNITS_METERS);
	
	_ocean = _o;
	_ocean_context = _oc;
	do_interpolation = interp;
	CenterOffset = p;
}

Point3 HotDeformer::Map(int i, Point3 p)
{
	drw::EvalData evaldata; 

	if (!do_interpolation)
		_ocean_context->eval_xz(envGlobalScale  * (p.x - CenterOffset.x), envGlobalScale * (p.y - CenterOffset.y), evaldata);
	else
		_ocean_context->eval2_xz(envGlobalScale * (p.x - CenterOffset.x), envGlobalScale * (p.y - CenterOffset.y), evaldata);

	p.x += evaldata.disp[0] * oneOverGlobalScale;
	p.y += evaldata.disp[2] * oneOverGlobalScale;
	p.z += evaldata.disp[1] * oneOverGlobalScale;

	return p;
}

// This method returns our deformer object
Deformer& HotMod::GetDeformer(TimeValue t, ModContext &mc, Point2 p)
{
	static HotDeformer deformer;
	deformer = HotDeformer(_ocean, _ocean_context, scale, do_interp, do_chop, mc, p);
	return deformer;
};

// Sub-Object
int HotMod::HitTest(
		TimeValue t, INode* inode, 
		int type, int crossing, int flags, 
		IPoint2 *p, ViewExp *vpt, ModContext* mc)
	{

#if MAXVERSION > MAX2012
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
#endif

	GraphicsWindow *gw = vpt->getGW();
	Point3 pt;
	HitRegion hr;
	int savedLimits, res = 0;
	Matrix3 tm = CompMatrix(t,inode,mc);

	MakeHitRegion(hr,type, crossing,4,p);
	gw->setHitRegion(&hr);	
	gw->setRndLimits(((savedLimits = gw->getRndLimits()) | GW_PICK) & ~GW_ILLUM);	
	gw->setTransform(tm);
	gw->clearHitCode();
	
	DrawCenterMark(DrawLineProc(gw),*mc->box);
	gw->setRndLimits(savedLimits);
	if (gw->checkHitCode()) {
		vpt->LogHit(inode, mc, gw->getHitDistance(), 0, NULL); 
		return 1;
		}
	return 0;
	}

int HotMod::Display(
		TimeValue t, INode* inode, ViewExp *vpt, 
		int flagst, ModContext *mc)
	{
#if MAXVERSION > MAX2012
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
#endif

	GraphicsWindow *gw = vpt->getGW();
	Point3 pt[4];
	Matrix3 tm = CompMatrix(t,inode,mc);
	int savedLimits;

	gw->setRndLimits((savedLimits = gw->getRndLimits()) & ~GW_ILLUM);
	gw->setTransform(tm);
	if (ip && ip->GetSubObjectLevel() == 1) {
		//gw->setColor(LINE_COLOR, (float)1.0, (float)1.0, (float)0.0);
		gw->setColor(LINE_COLOR,GetUIColor(COLOR_SEL_GIZMOS));
	} else {
		//gw->setColor(LINE_COLOR, (float).85, (float).5, (float)0.0);
		gw->setColor(LINE_COLOR,GetUIColor(COLOR_GIZMOS));
		}
	
	DrawCenterMark(DrawLineProc(gw),*mc->box);
	
	gw->setRndLimits(savedLimits);
	return 0;
	}

void HotMod::GetWorldBoundBox(
		TimeValue t,INode* inode, ViewExp *vpt, 
		Box3& box, ModContext *mc)
	{

#if MAXVERSION > MAX2012
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return;
	}
#endif

	GraphicsWindow *gw = vpt->getGW();
	Matrix3 tm = CompMatrix(t,inode,mc);
	BoxLineProc bproc(&tm);
	box = bproc.Box();	
	DrawCenterMark(DrawLineProc(gw),box);
	}

void HotMod::Move(TimeValue t, Matrix3& partm, Matrix3& tmAxis, 
		Point3& val, BOOL localOrigin)
	{
	SetXFormPacket pckt(val,partm,tmAxis);
	tmControl->SetValue(t,&pckt,TRUE,CTRL_RELATIVE);
	}

void HotMod::Rotate(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Quat& val, BOOL localOrigin)
	{
	SetXFormPacket pckt(val,localOrigin,partm,tmAxis);
	tmControl->SetValue(t,&pckt,TRUE,CTRL_RELATIVE);
	}

void HotMod::Scale(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin)
	{
	SetXFormPacket pckt(val,localOrigin,partm,tmAxis);
	tmControl->SetValue(t,&pckt,TRUE,CTRL_RELATIVE);
	}

void HotMod::GetSubObjectCenters(
		SubObjAxisCallback *cb,TimeValue t,
		INode *node,ModContext *mc)
	{
	Matrix3 tm = CompMatrix(t,node,mc);	
	cb->Center(tm.GetTrans(),0);
	}

void HotMod::GetSubObjectTMs(
		SubObjAxisCallback *cb,TimeValue t,
		INode *node,ModContext *mc)
	{
	Matrix3 tm = CompMatrix(t,node,mc);
	cb->TM(tm,0);
	}

void HotMod::ActivateSubobjSel(int level, XFormModes& modes)
	{
	switch (level) {
		case 1: // Mirror center
			modes = XFormModes(moveMode,rotMode,nuscaleMode,uscaleMode,squashMode,NULL);
			break;		
		}
	NotifyDependents(FOREVER,PART_DISPLAY,REFMSG_CHANGE);
	}

int HotMod::NumSubObjTypes() 
{ 
	return 1;
}

ISubObjType *HotMod::GetSubObjType(int i) 
{	
	static bool initialized = false;
	if(!initialized)
	{
		initialized = true;
		SOT_Center.SetName(GetString(IDS_CENTER));
	}

	switch(i)
	{
	case 0:
		return &SOT_Center;
	}

	return NULL;
}
// ------------------------------------------------------------------------------------------------------
// Common plugin code

BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved) 
{	
	if( fdwReason == DLL_PROCESS_ATTACH )
	{
		hInstance = hinstDLL;
		DisableThreadLibraryCalls(hInstance);
	}
	return(TRUE);
}

__declspec( dllexport ) int LibNumberClasses()
{
	return 2;
}
__declspec( dllexport ) ClassDesc *LibClassDesc(int i)
{
	switch(i)
	{
		case 0: return GetHotModDesc();	
		default: return 0;
	}
}
__declspec( dllexport ) const TCHAR *LibDescription()
{
	return GetString(IDS_LIB_DESC);
}
__declspec( dllexport ) ULONG LibVersion()
{
	return VERSION_3DSMAX;
}