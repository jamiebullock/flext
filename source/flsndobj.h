/* 

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#ifndef FLEXT_SNDOBJ
#define FLEXT_SNDOBJ

#include "flext.h"
#include <SndObj/AudioDefs.h>

//namespace flext {

class flext_sndobj:
	public flext_dsp
{
	FLEXT_HEADER(flext_sndobj,flext_dsp)
 
public:
	flext_sndobj();
	virtual ~flext_sndobj();

	// these have to be overridden in child classes
	virtual void NewObjs() {}
	virtual void FreeObjs() {}
	virtual void ProcessObjs() {}

	// inputs and outputs
	SndObj &InObj(int i) { return *tmpobj[i]; }
	SndIO &OutObj(int i) { return *outobj[i]; }

protected:
	virtual void m_dsp(int n,t_sample *const *in,t_sample *const *out); 
	virtual void m_signal(int n,t_sample *const *in,t_sample *const *out); 

private:
	//! SndObj for reading from inlet buffer
	class Inlet:
		public SndIO
	{
	public:
		Inlet(const t_sample *b,int vecsz,float sr);
		virtual short Read();
		virtual short Write();

		void SetBuf(const t_sample *b) { buf = b; }

	private:
		const t_sample *buf;
	};

	//! SndObj for writing to outlet buffer
	class Outlet:
		public SndIO
	{
	public:
		Outlet(t_sample *b,int vecsz,float sr);
		virtual short Read();
		virtual short Write();

		void SetBuf(t_sample *b) { buf = b; }

	private:
		t_sample *buf;
	};

	void ClearObjs();

	int inobjs,outobjs;
	SndObj **tmpobj;
	Inlet **inobj;
	Outlet **outobj;

	float smprt;
	int blsz;
};

//} // namespace flext

#endif