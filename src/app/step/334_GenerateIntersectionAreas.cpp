#include <app/step/334_GenerateIntersectionAreas.h>

//EPG
#include <epg/Context.h>
#include <epg/log/ScopeLogger.h>
#include <ome2/utils/CopyTableUtils.h>

//APP
#include <app/params/ThemeParameters.h>
#include <app/calcul/GenerateIntersectionAreaOp.h>


namespace app {
namespace step {

	///
	///
	///
	void GenerateIntersectionAreas::init()
	{
		addWorkingEntity(INTERSECTION_AREA_TABLE);
	}

	///
	///
	///
	void GenerateIntersectionAreas::onCompute( bool verbose = false )
	{
		//--
		std::string intAreaRefTableName = _themeParams.getValue(INTERSECTION_AREA_TABLE).toString();
		std::string countryCodeW = _themeParams.getValue(COUNTRY_CODE_W).toString();

		//--
		ome2::utils::CopyTableUtils::copyPolygonTable(
			getLastWorkingTableName(INTERSECTION_AREA_TABLE),
			getCurrentWorkingTableName(INTERSECTION_AREA_TABLE),
			"", false, true, true
		);

		//--
		_themeParams.setParameter(INTERSECTION_AREA_TABLE, ign::data::String(getCurrentWorkingTableName(INTERSECTION_AREA_TABLE)));
		_epgParams.setParameter(AREA_TABLE, ign::data::String(getLastWorkingTableName(AREA_TABLE_INIT)));
		
		//--
		app::calcul::GenerateIntersectionAreaOp::Compute(countryCodeW, verbose);

		//--
		_themeParams.setParameter(INTERSECTION_AREA_TABLE, ign::data::String(intAreaRefTableName));
	}

}
}


		