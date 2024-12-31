#ifndef _APP_STEP_TOOLS_INITSTEPS_H_
#define _APP_STEP_TOOLS_INITSTEPS_H_

//EPG
#include <epg/step/StepSuite.h>
#include <epg/step/factoryNew.h>

//APP
#include <app/step/301_AddStandingWater.h>
#include <app/step/310_GenerateCuttingLines.h>
#include <app/step/320_CleanByLandmask.h>
#include <app/step/330_CleanCuttingLines.h>
#include <app/step/334_GenerateIntersectionAreas.h>
#include <app/step/335_GenerateCuttingPoints.h>
#include <app/step/340_MergeAreas.h>
#include <app/step/350_SplitMergedAreasWithCF.h>
#include <app/step/360_MergedAttributesAreas.h>
#include <app/step/370_MergeSplitAreas.h>
#include <app/step/399_SortingStandingWater.h>

namespace app{
namespace step{
namespace tools{

	template<  typename StepSuiteType >
	void initSteps( StepSuiteType& stepSuite )
	{
		stepSuite.addStep( epg::step::factoryNew< AddStandingWater >() );
		stepSuite.addStep( epg::step::factoryNew< GenerateCuttingLines >() );
		stepSuite.addStep( epg::step::factoryNew< CleanByLandmask >() );
		stepSuite.addStep( epg::step::factoryNew< CleanCuttingLines >() );
		stepSuite.addStep( epg::step::factoryNew< GenerateIntersectionAreas >() );
		stepSuite.addStep( epg::step::factoryNew< GenerateCuttingPoints >() );
		stepSuite.addStep( epg::step::factoryNew< MergeAreas >() );
		stepSuite.addStep( epg::step::factoryNew< SplitMergedAreasWithCF >() );
		stepSuite.addStep( epg::step::factoryNew< MergedAttributesAreas >() );
		stepSuite.addStep( epg::step::factoryNew< MergeSplitAreas >() );
		stepSuite.addStep( epg::step::factoryNew< SortingStandingWater >() );
	}

}
}
}

#endif