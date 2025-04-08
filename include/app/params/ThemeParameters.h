#ifndef _APP_PARAMS_THEMEPARAMETERS_H_
#define _APP_PARAMS_THEMEPARAMETERS_H_

//STL
#include <string>

//EPG
#include <epg/params/ParametersT.h>
#include <epg/SingletonT.h>



	enum HY_PARAMETERS{
		DB_CONF_FILE,
		WORKING_SCHEMA,
		AREA_TABLE_INIT,
		AREA_TABLE_INIT_CLEANED,
		AREA_TABLE_INIT_STANDING_WATER,
		NATIONAL_IDENTIFIER_NAME,
		COUNTRY_CODE_W,
		W_TAG_NAME,
		IS_STANDING_WATER_NAME,

		INTERSECTION_AREA_TABLE_SUFFIX,
		INTERSECTION_AREA_TABLE,

		LANDMASK_TABLE,
		LAND_COVER_TYPE_NAME,
		LAND_COVER_TYPE_VALUE,
		PC_LANDMASK_BUFFER,
		CUTP_TABLE_SUFFIX,
		CUTP_SECTION_GEOM,
		CUTL_TABLE_SUFFIX,
		CUTP_TABLE,
		CUTL_TABLE,
		DIST_SNAP_MERGE_CF,
		PS_BORDER_OFFSET,
		PC_DISTANCE_THRESHOLD,

		SAM_SMALL_AREA_THRESHOLD,
		SAM_SMALL_AREA_LENGTH_THRESHOLD,

		AM_LIST_ATTR_JSON,
		AM_LIST_ATTR_W
	};

namespace app{
namespace params{

	class ThemeParameters : public epg::params::ParametersT< HY_PARAMETERS >
	{
		typedef  epg::params::ParametersT< HY_PARAMETERS > Base;

		public:

			/// \brief
			ThemeParameters();

			/// \brief
			~ThemeParameters();

			/// \brief
			virtual std::string getClassName()const;

	};

	typedef epg::Singleton< ThemeParameters >   ThemeParametersS;

}
}

#endif