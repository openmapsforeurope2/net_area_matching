#include <app/step/360_SetMergedAreasAttributes.h>

//EPG
#include <epg/Context.h>
#include <epg/log/ScopeLogger.h>
#include <ome2/utils/CopyTableUtils.h>

//APP
#include <app/params/ThemeParameters.h>
#include <app/calcul/SetMergedAreasAttributesOp.h>


///
///
///
void app::step::SetMergedAreasAttributes::init()
{
	addWorkingEntity(AREA_TABLE_INIT);
}

///
///
///
void app::step::SetMergedAreasAttributes::onCompute(bool verbose = false)
{
	//copie table AREA
	_epgParams.setParameter(AREA_TABLE, ign::data::String(getCurrentWorkingTableName(AREA_TABLE_INIT)));
	ome2::utils::CopyTableUtils::copyAreaTable(getLastWorkingTableName(AREA_TABLE_INIT), "", false, true, true);

	std::string countryCodeW = _themeParams.getValue(COUNTRY_CODE_W).toString();
	app::calcul::SetMergedAreasAttributesOp::Compute(countryCodeW, verbose);

}

