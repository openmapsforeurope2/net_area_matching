#include <app/step/360_MergedAttributesAreas.h>

//EPG
#include <epg/Context.h>
#include <epg/log/ScopeLogger.h>
#include <ome2/utils/CopyTableUtils.h>

//APP
#include <app/params/ThemeParameters.h>
#include <app/calcul/SetAttributeMergedAreasOp.h>


///
///
///
void app::step::MergedAttributesAreas::init()
{
	addWorkingEntity(AREA_TABLE_INIT);
}

///
///
///
void app::step::MergedAttributesAreas::onCompute(bool verbose = false)
{
	//copie table AREA
	_epgParams.setParameter(AREA_TABLE, ign::data::String(getCurrentWorkingTableName(AREA_TABLE_INIT)));
	ome2::utils::CopyTableUtils::copyAreaTable(getLastWorkingTableName(AREA_TABLE_INIT), "", false, true, true);

	app::params::ThemeParameters* themeParameters = app::params::ThemeParametersS::getInstance();
	std::string countryCodeW = themeParameters->getParameter(COUNTRY_CODE_W).getValue().toString();
	app::calcul::SetAttributeMergedAreasOp::Compute(countryCodeW, verbose);

}

