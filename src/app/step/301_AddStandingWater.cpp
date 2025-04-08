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
			// copie standing water 
			_epgParams.setParameter(AREA_TABLE, ign::data::String(getCurrentWorkingTableName(AREA_TABLE_INIT_STANDING_WATER)));
			ome2::utils::CopyTableUtils::copyAreaTable(getLastWorkingTableName(AREA_TABLE_INIT_STANDING_WATER), "", false, true, true);

			// affectation temporaire AREA_TABLE_INIT_STANDING_WATER [DEBUT]
			std::string areaTableNameStandingInit = _themeParams.getParameter(AREA_TABLE_INIT_STANDING_WATER).getValue().toString();
			_themeParams.setParameter(AREA_TABLE_INIT_STANDING_WATER, ign::data::String(getCurrentWorkingTableName(AREA_TABLE_INIT_STANDING_WATER)));

			// copie watercourse 
			_epgParams.setParameter(AREA_TABLE, ign::data::String(getCurrentWorkingTableName(AREA_TABLE_INIT)));
			ome2::utils::CopyTableUtils::copyAreaTable(getLastWorkingTableName(AREA_TABLE_INIT), "", false, true, true);

			// traitement
			app::params::ThemeParameters* themeParameters = app::params::ThemeParametersS::getInstance();
			std::string countryCodeW = themeParameters->getParameter(COUNTRY_CODE_W).getValue().toString();

			app::calcul::StandingWaterOp::AddStandingWater(countryCodeW, verbose);

			// affectation temporaire AREA_TABLE_INIT_STANDING_WATER [FIN]
			_themeParams.setParameter(AREA_TABLE_INIT_STANDING_WATER, ign::data::String(areaTableNameStandingInit));

		}

	}
}