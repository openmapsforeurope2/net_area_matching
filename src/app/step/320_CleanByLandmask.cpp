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

		std::string countryCodeW = _themeParams.getValue(COUNTRY_CODE_W).toString();

		//--
		app::calcul::PolygonSplitterOp::Compute(countryCodeW, verbose);

		//--
		app::calcul::PolygonCleanerOp::Compute(countryCodeW, verbose);

		//--
		app::calcul::PolygonMergerOp::Compute(verbose);
	}

}
}
