#ifndef TWSCRIPT_H
#define TWSCRIPT_H

#if !SCR_GENSCRIPTS
#include "BaseScript.h"
#include "BaseTrap.h"

#include "scriptvars.h"
#include "darkhook.h"

#include <lg/scrservices.h>
#include <lg/links.h>

/**
 * Script: TWTweqSmooth
 *
 */
#if !SCR_GENSCRIPTS
class cScr_TWTweqSmooth : public cBaseScript
{
public:
	cScr_TWTweqSmooth(const char* pszName, int iHostObjId)
		: cBaseScript(pszName, iHostObjId)
    { }

protected:
	virtual long OnSim(sSimMsg*, cMultiParm&);
	virtual long OnTimer(sScrTimerMsg* pTimerMsg, cMultiParm& mpReply);

private:
    void Init();
    void StartTimer();
    void ClearTimer();
    void SetAxisRate(const char *propname, ePhysAxes axis);

    // Timer-related variables.
    tScrTimer timer;      //!< The currently active timer for this object, or NULL.
    int       timer_rate; //!< The update rate, in milliseconds.

    float     min_rate;
    float     max_rate;
};

#else // SCR_GENSCRIPTS
GEN_FACTORY("TWTweqSmooth", "BaseScript", cScr_TWTweqSmooth)
#endif // SCR_GENSCRIPTS

#endif // TWSCRIPT_H
