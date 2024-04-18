#ifndef _APP_PARAMS_THEMEPARAMETERS_H_
#define _APP_PARAMS_THEMEPARAMETERS_H_

//STL
#include <string>

//EPG
#include <epg/params/ParametersT.h>
#include <epg/SingletonT.h>



	enum HY_PARAMETERS{
		AREA_TABLE_INIT,
		NATIONAL_IDENTIFIER_NAME,
		COUNTRY_CODE_W,
		LANDMASK_TABLE,
		LAND_COVER_TYPE,
		TYPE_LAND_AREA,
		PC_LANDMASK_BUFFER,
		CUTP_TABLE_SUFFIX,
		CUTP_SECTION_GEOM,
		CUTL_TABLE_SUFFIX,
		CUTP_TABLE,
		CUTL_TABLE,
		DIST_SNAP_MERGE_CF,
		PS_BORDER_OFFSET,
		PC_DISTANCE_THRESHOLD,

		THRESHOLD_AREA_ATTR,

		LIST_ATTR_TO_CONCAT,
		LIST_ATTR_JSON,
		LIST_ATTR_W

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