/*
Houdini Ocean Toolkit for Autodesk 3ds Max

Copyright (C) 2013

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

#include "OceanMaya.h"
#include "shellapi.h"

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#include "max.h"
#include "resource.h"

#include "simpmod.h"
#include "simpobj.h"
#include "iparamm2.h"
#include "iFnPub.h"

#include <bmmlib.h>
#include "AssetManagement\iassetmanager.h"

#ifdef MAX2012
#include "ref.h"
#else
#include "ReferenceSaveManager.h"
#endif

#define HOT4MAX_VERSION 3

const Class_ID HOT4MAX_CLASS_ID(0x30928ee2, 0x719e8ae6);
const unsigned short HOT_PBLOCK_REF(0);
#define TM_REF		1

#ifdef MAX2013
int end = p_end;
#endif

#define MAXREBUILDCOUNT 500

float MAX_RESOLUTION = 8.0f;

#define M_PI 3.14159265358979323846	

#define HOT_INTERFACE Interface_ID(0x13c47c57, 0x7a020d7)

TCHAR *GetString(int id)
{
	static TCHAR buf[256];

	if (hInstance)
		return LoadString(hInstance, id, buf, sizeof(buf)) ? buf : NULL;

	return NULL;
}

class IHoudiniOcean: public Modifier, public FPMixinInterface
{
public:

	enum { hot_getpointjminus, hot_savejminusmap, hot_saveheightmap, hot_getpointeminus };

	BEGIN_FUNCTION_MAP
	FN_1(hot_getpointjminus, TYPE_FLOAT, fnGetPointJminus, TYPE_POINT2);
	VFN_1(hot_savejminusmap, fnSaveJminusMap, TYPE_STRING);
	VFN_1(hot_saveheightmap, fnSaveHeightMap, TYPE_STRING);
	FN_1(hot_getpointeminus, TYPE_POINT3, fnGetPointEminus, TYPE_POINT2);
	END_FUNCTION_MAP

	FPInterfaceDesc* GetDesc();

	virtual float fnGetPointJminus(Point2 p) = 0;
	virtual void fnSaveJminusMap(const TCHAR *filename) = 0;
	virtual void fnSaveHeightMap(const TCHAR *filename) = 0;
	virtual Point3 fnGetPointEminus(Point2 p) = 0;
	
};

class HotMod : public IHoudiniOcean
{
public:
	HotMod();
	~HotMod();

	IParamBlock2 *pblock2;
	Control *tmControl;

	static IObjParam *ip;
	static HotMod *editMod;
	static MoveModBoxCMode *moveMode;
	static RotateModBoxCMode *rotMode;
	static UScaleModBoxCMode *uscaleMode;
	static NUScaleModBoxCMode *nuscaleMode;
	static SquashModBoxCMode *squashMode;	

	int version;

	Interval GetValidity(TimeValue t);
	Matrix3 CompMatrix (TimeValue t, INode *inode, ModContext *mc, Interval *validity=NULL);

	// From Animatable
	void DeleteThis() { delete this; }
	void GetClassName(TSTR& s) { s= GetString(IDS_RB_HOT_OSM_CLASS); }
	virtual Class_ID ClassID() { return HOT4MAX_CLASS_ID;}
	RefTargetHandle Clone(RemapDir& remap);
#ifdef MAX2013
	const TCHAR *GetObjectName() { return GetString(IDS_RB_HOT);}
#else
	TCHAR *GetObjectName() { return GetString(IDS_RB_HOT);}
#endif

	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next);
	CreateMouseCallBack* GetCreateMouseCallBack() { return NULL; }

	// From Paramblock2
	int	NumParamBlocks() { return 1; }
	IParamBlock2* GetParamBlock(int i)
	{
		return pblock2;
	}
	IParamBlock2* GetParamBlockByID(BlockID id)
	{
		return (pblock2->ID() == id) ? pblock2 : NULL;
	}
	
	// FPMixinInterface
	float fnGetPointJminus(Point2 p);
	void fnSaveJminusMap(const TCHAR *filename);
	void fnSaveHeightMap(const TCHAR *filename);
	Point3 fnGetPointEminus(Point2 p);

	BaseInterface* GetInterface(Interface_ID id) 
	{ 
		if (id == HOT_INTERFACE) 
			return (IHoudiniOcean*)this; 
		else 
			return Modifier::GetInterface(id);
	} 

	// From Modifier
	ChannelMask ChannelsUsed()  {return PART_GEOM | PART_TOPO | PART_VERTCOLOR;}
	ChannelMask ChannelsChanged() {return PART_GEOM | PART_VERTCOLOR;}
	Class_ID InputType() { return defObjectClassID; }
	void ModifyObject(TimeValue t, ModContext &mc, ObjectState *os, INode *node);
	void ModifyTriObject(TimeValue t, ModContext &mc, TriObject* tobj, Interval iv, Point2 center);
	//void ModifyPolyObject(TimeValue t, ModContext &mc, PolyObject *pobj);

	// IO
	IOResult Load(ILoad *iload);
	IOResult Save(ISave *isave);

	// Center Gizmo
	int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt, ModContext* mc);
	int Display(TimeValue t, INode* inode, ViewExp *vpt, int flagst, ModContext *mc);
	void GetWorldBoundBox(TimeValue t,INode* inode, ViewExp *vpt, Box3& box, ModContext *mc);		
	void Move(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin);
	void Rotate(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Quat& val, BOOL localOrigin);
	void Scale(TimeValue t, Matrix3& partm, Matrix3& tmAxis, Point3& val, BOOL localOrigin);	
	BOOL AssignController(Animatable *control,int subAnim);
	int SubNumToRefNum(int subNum);
	void GetSubObjectCenters(SubObjAxisCallback *cb,TimeValue t,INode *node,ModContext *mc);
	void GetSubObjectTMs(SubObjAxisCallback *cb,TimeValue t,INode *node,ModContext *mc);
	void ActivateSubobjSel(int level, XFormModes& modes);
	int NumSubObjTypes();
	ISubObjType *GetSubObjType(int i);
	
	// Reference Management
	int NumRefs() { return 2; }
	RefTargetHandle GetReference(int i);
	
#ifndef MAX2015
	RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message);
#else
	RefResult NotifyRefChanged( const Interval& changeInt,RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate);
#endif

private:
	virtual void SetReference(int i, RefTargetHandle rtarg);

public:
	int NumSubs() {return 2;}
	Animatable* SubAnim(int i) {return GetReference(i);};
	TSTR SubAnimName(int i);

	// Ocean.h stuff

	float resolution;
	float size;
	float windSpeed;
	float waveHeigth;
	float shortestWave;
	float choppiness;
	float windDirection;
	float dampReflections;
	float windAlign;
	float oceanDepth;
	float time;
	float seed;
	float scale;

	BOOL do_interp;
	BOOL do_chop;

	// Foam

	BOOL do_foam;
	float foam_scale;

	void UpdateOcean(TimeValue t);

	Deformer& GetDeformer(TimeValue t, Point2 p);

protected:
	// This is where all the wave action takes place
	drw::Ocean        *_ocean;
	drw::OceanContext *_ocean_context;
	float              _ocean_scale;

	// If this is true we will create a new instance of drw::Ocean  next time it runs.
	bool _ocean_needs_rebuild;
};

class HotDeformer: public Deformer
{
public:
	HotDeformer() {}
	HotDeformer(drw::Ocean* _ocean, drw::OceanContext* _ocean_context, float globalScale, bool do_interpolation, bool do_chop, Point2 CenterOffset);

	Point3 Map(int i, Point3 p);

private:
	drw::Ocean        *_ocean;
	drw::OceanContext *_ocean_context;
	
	float envGlobalScale;
	float oneOverGlobalScale;
	
	bool do_interpolation;
	bool do_chop;
	
	Point2 CenterOffset;
};

class HotClassDesc:public ClassDesc2
{
public:
	int IsPublic() { return 1; }
	void *Create(BOOL loading = FALSE) { return new HotMod; }
	const TCHAR *ClassName() { return GetString(IDS_RB_HOT_OSM_CLASS); }
	SClass_ID SuperClassID() { return OSM_CLASS_ID; }
	Class_ID ClassID() { return HOT4MAX_CLASS_ID; }
	const TCHAR *Category() { return GetString(IDS_RB_HOTMOD);}
	const TCHAR *InternalName()	{return _T("HotMod");}
	HINSTANCE HInstance() {return hInstance;};
};

static HotClassDesc hotDesc;

ClassDesc* GetHotModDesc()
{
	return &hotDesc;
}

class HotDlgProc : public ParamMap2UserDlgProc
{
public:
	INT_PTR DlgProc(TimeValue t,IParamMap2* map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam);	
	void DeleteThis() {}
};

static HotDlgProc theHotProc;
