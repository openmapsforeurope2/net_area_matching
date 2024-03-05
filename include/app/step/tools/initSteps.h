#ifndef _APP_STEP_TOOLS_INITSTEPS_H_
#define _APP_STEP_TOOLS_INITSTEPS_H_

//EPG
#include <epg/step/StepSuite.h>
#include <epg/step/factoryNew.h>

//APP
#include <app/step/301_GenerateCuttingFeatures.h>
#include <app/step/302_CleanByLandmask.h>
#include <app/step/303_MergeAreas.h>
#include <app/step/304_SplitMergedAreasWithCF.h>


namespace app{
namespace step{
namespace tools{

	template<  typename StepSuiteType >
	void initSteps( StepSuiteType& stepSuite )
	{
		stepSuite.addStep(epg::step::factoryNew< GenerateCuttingFeatures >());
		stepSuite.addStep( epg::step::factoryNew< CleanByLandmask >() );
		stepSuite.addStep( epg::step::factoryNew< GenerateCuttingFeatures >() );
		stepSuite.addStep( epg::step::factoryNew< MergeAreas >() );
		stepSuite.addStep( epg::step::factoryNew< SplitMergedAreasWithCF >() );
	}

}
}
}

#endif