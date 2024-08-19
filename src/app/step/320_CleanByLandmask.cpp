#include <app/step/320_CleanByLandmask.h>

//EPG
#include <epg/Context.h>
#include <epg/log/ScopeLogger.h>
#include <ome2/utils/CopyTableUtils.h>

//APP
#include <app/params/ThemeParameters.h>
#include <app/calcul/PolygonSplitterOp.h>
#include <app/calcul/PolygonCleanerOp.h>
#include <app/calcul/PolygonMergerOp.h>


namespace app {
namespace step {

	///
	///
	///
	void CleanByLandmask::init()
	{
		addWorkingEntity(AREA_TABLE_INIT);
	}

	///
	///
	///
	void CleanByLandmask::onCompute( bool verbose = false )
	{		
		//copie table AREA
		_epgParams.setParameter(AREA_TABLE, ign::data::String(getCurrentWorkingTableName(AREA_TABLE_INIT)));
		ome2::utils::CopyTableUtils::copyAreaTable(getLastWorkingTableName(AREA_TABLE_INIT), "", false, true, true);

		app::params::ThemeParameters* themeParameters = app::params::ThemeParametersS::getInstance();
		std::string countryCodeW = themeParameters->getParameter(COUNTRY_CODE_W).getValue().toString();

		//--
		app::calcul::PolygonSplitterOp::compute(countryCodeW, verbose);

		//--
		app::calcul::PolygonCleanerOp::compute(countryCodeW, verbose);

		//--
		app::calcul::PolygonMergerOp::compute(verbose);
	}

}
}
