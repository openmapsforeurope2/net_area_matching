#include <app/step/360_MergedAttributesAreas.h>

//EPG
#include <epg/Context.h>
#include <epg/log/ScopeLogger.h>
#include <epg/utils/CopyTableUtils.h>


//APP
#include <app/params/ThemeParameters.h>
#include <app/calcul/SetAttributeMergedAreasOp.h>



///
///
///
void app::step::MergedAttributesAreas::init()
{

}

///
///
///
void app::step::MergedAttributesAreas::onCompute(bool verbose = false)
{
	app::params::ThemeParameters* themeParameters = app::params::ThemeParametersS::getInstance();
	std::string countryCodeW = themeParameters->getParameter(COUNTRY_CODE_W).getValue().toString();
	app::calcul::SetAttributeMergedAreasOp::compute(countryCodeW, verbose);

}

