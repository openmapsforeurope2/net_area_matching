#ifndef _APP_STEP_TOOLS_INITSTEPS_H_
#define _APP_STEP_TOOLS_INITSTEPS_H_

//EPG
#include <epg/step/StepSuite.h>
#include <epg/step/factoryNew.h>

//APP
#include <app/step/301_CleanByLandmask.h>
#include <app/step/302_GenerateCuttingFeatures.h>


namespace app{
namespace step{
namespace tools{

	template<  typename StepSuiteType >
	void initSteps( StepSuiteType& stepSuite )
	{
		stepSuite.addStep( epg::step::factoryNew< CleanByLandmask >() );
		stepSuite.addStep( epg::step::factoryNew< GenerateCuttingFeatures >() );
	}

}
}
}

#endif