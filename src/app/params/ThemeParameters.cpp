
//APP
#include <app/params/ThemeParameters.h>

//SOCLE
#include <ign/Exception.h>


namespace app{
namespace params{


	///
	///
	///
	ThemeParameters::ThemeParameters()
	{
		_initParameter( LANDMASK_TABLE, "LANDMASK_TABLE" );
		_initParameter( LAND_COVER_TYPE, "LAND_COVER_TYPE" );
		_initParameter( TYPE_LAND_AREA, "TYPE_LAND_AREA" );
		_initParameter(PC_LANDMASK_BUFFER, "PC_LANDMASK_BUFFER");
		_initParameter(CUTP_TABLE_SUFFIX, "CUTP_TABLE_SUFFIX");
		_initParameter(CUTL_TABLE_SUFFIX, "CUTL_TABLE_SUFFIX");
		_initParameter(CUTP_TABLE, "CUTP_TABLE");
		_initParameter(CUTL_TABLE, "CUTL_TABLE" );
	}

	///
	///
	///
	ThemeParameters::~ThemeParameters()
	{
	}

	///
	///
	///
	std::string ThemeParameters::getClassName()const
	{
		return "app::params::ThemeParameters";
	}


}
}