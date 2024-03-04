#ifndef _APP_STEP_TOOLS_INITSTEPS_H_
#define _APP_STEP_TOOLS_INITSTEPS_H_

//EPG
#include <epg/step/StepSuite.h>
#include <epg/step/factoryNew.h>

//APP
#include <app/step/301_GenerateCuttingFeatures.h>
#include <app/step/302_CleanByLandmask.h>


namespace app{
namespace step{
namespace tools{

	template<  typename StepSuiteType >
	void initSteps( StepSuiteType& stepSuite )
	{
		stepSuite.addStep(epg::step::factoryNew< GenerateCuttingFeatures >());
		stepSuite.addStep( epg::step::factoryNew< CleanByLandmask >() );

	}

}
}
}

#endif