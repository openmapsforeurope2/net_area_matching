#include <app/step/301_AddStandingWater.h>

//EPG
#include <ome2/utils/CopyTableUtils.h>

//APP
#include <app/calcul/StandingWaterOp.h>

namespace app {
	namespace step {

		///
		///
		///
		void AddStandingWater::init()
		{
			addWorkingEntity(AREA_TABLE_INIT);
			addWorkingEntity(AREA_TABLE_INIT_STANDING_WATER);
		}

		///
		///
		///
		void AddStandingWater::onCompute(bool verbose = false)
		{
			//copie table AREA 
			_epgParams.setParameter(AREA_TABLE, ign::data::String(getCurrentWorkingTableName(AREA_TABLE_INIT_STANDING_WATER)));
			ome2::utils::CopyTableUtils::copyAreaTable(getLastWorkingTableName(AREA_TABLE_INIT_STANDING_WATER), "", false, true, true);

			_epgParams.setParameter(AREA_TABLE, ign::data::String(getCurrentWorkingTableName(AREA_TABLE_INIT)));
			ome2::utils::CopyTableUtils::copyAreaTable(getLastWorkingTableName(AREA_TABLE_INIT), "", false, true, true);


			app::params::ThemeParameters* themeParameters = app::params::ThemeParametersS::getInstance();
			std::string countryCodeW = themeParameters->getParameter(COUNTRY_CODE_W).getValue().toString();

			app::calcul::StandingWaterOp::AddStandingWater(countryCodeW, verbose);
		}

	}
}