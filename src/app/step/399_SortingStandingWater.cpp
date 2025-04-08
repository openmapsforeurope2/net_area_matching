#include <app/step/399_SortingStandingWater.h>

//EPG
#include <ome2/utils/CopyTableUtils.h>

//APP
#include <app/calcul/StandingWaterOp.h>

namespace app {
	namespace step {

		///
		///
		///
		void SortingStandingWater::init()
		{
			addWorkingEntity(AREA_TABLE_INIT);
			addWorkingEntity(AREA_TABLE_INIT_STANDING_WATER);
		}

		///
		///
		///
		void SortingStandingWater::onCompute(bool verbose = false)
		{
			//--
			_epgParams.setParameter(AREA_TABLE, ign::data::String(getCurrentWorkingTableName(AREA_TABLE_INIT_STANDING_WATER)));
			ome2::utils::CopyTableUtils::copyAreaTable(getLastWorkingTableName(AREA_TABLE_INIT_STANDING_WATER), "", false, true, true);

			//affectation temporaire pour AREA_TABLE_INIT_STANDING_WATER
			std::string areaTableNameStandingInit = _themeParams.getParameter(AREA_TABLE_INIT_STANDING_WATER).getValue().toString();
			_themeParams.setParameter(AREA_TABLE_INIT_STANDING_WATER, ign::data::String(getCurrentWorkingTableName(AREA_TABLE_INIT_STANDING_WATER)));

			//--
			_epgParams.setParameter(AREA_TABLE, ign::data::String(getCurrentWorkingTableName(AREA_TABLE_INIT)));
			ome2::utils::CopyTableUtils::copyAreaTable(getLastWorkingTableName(AREA_TABLE_INIT), "", false, true, true);
			
			//--
			std::string countryCodeW = _themeParams.getParameter(COUNTRY_CODE_W).getValue().toString();
			
			app::calcul::StandingWaterOp::SortingStandingWater(countryCodeW, verbose);

			//fin affectation temporaire pour AREA_TABLE_INIT_STANDING_WATER
			_themeParams.setParameter(AREA_TABLE_INIT_STANDING_WATER, ign::data::String(areaTableNameStandingInit));
		}

	}
}