
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
		
		_initParameter(AREA_TABLE_INIT, "AREA_TABLE_INIT");
		_initParameter( NATIONAL_IDENTIFIER_NAME, "NATIONAL_IDENTIFIER_NAME" );
		_initParameter( LANDMASK_TABLE, "LANDMASK_TABLE" );
		_initParameter( COUNTRY_CODE_W, "COUNTRY_CODE_W" );
		_initParameter( LAND_COVER_TYPE, "LAND_COVER_TYPE" );
		_initParameter( TYPE_LAND_AREA, "TYPE_LAND_AREA" );
		_initParameter( PC_LANDMASK_BUFFER, "PC_LANDMASK_BUFFER" );
		_initParameter( CUTP_TABLE_SUFFIX, "CUTP_TABLE_SUFFIX" );
		_initParameter( CUTL_TABLE_SUFFIX, "CUTL_TABLE_SUFFIX" );
		_initParameter( CUTP_TABLE, "CUTP_TABLE" );
		_initParameter( CUTL_TABLE, "CUTL_TABLE" );
		_initParameter( CUTP_SECTION_GEOM, "CUTP_SECTION_GEOM" );
		_initParameter( DIST_SNAP_MERGE_CF, "DIST_SNAP_MERGE_CF" );
		_initParameter( PS_BORDER_OFFSET, "PS_BORDER_OFFSET" );
		_initParameter(PC_DISTANCE_THRESHOLD, "PC_DISTANCE_THRESHOLD"); 
		_initParameter(SAM_SMALL_AREA_THRESHOLD, "SAM_SMALL_AREA_THRESHOLD");
			
		_initParameter(THRESHOLD_AREA_ATTR, "THRESHOLD_AREA_ATTR"); 
		_initParameter(LIST_ATTR_TO_CONCAT, "LIST_ATTR_TO_CONCAT");
		_initParameter(LIST_ATTR_JSON, "LIST_ATTR_JSON");
		_initParameter(LIST_ATTR_W, "LIST_ATTR_W");

		
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