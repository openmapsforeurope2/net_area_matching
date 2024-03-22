#include <app/step/305_MergedAttributesAreas.h>

//EPG
#include <epg/Context.h>
#include <epg/log/ScopeLogger.h>
#include <epg/utils/CopyTableUtils.h>


//APP
#include <app/params/ThemeParameters.h>




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


}

