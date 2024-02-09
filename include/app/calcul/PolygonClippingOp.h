#ifndef _APP_CALCUL_EDGECLEANINGOP_H_
#define _APP_CALCUL_EDGECLEANINGOP_H_

//SOCLE
#include <ign/feature/sql/FeatureStorePostgis.h>
#include <ign/tools/stringtools.h>

//EPG
#include <epg/log/EpgLogger.h>
#include <epg/log/ShapeLogger.h>

//APP


namespace app{
namespace calcul{

	class PolygonClippingOp {

	public:

	
        /// @brief 
        /// @param countryCode 
        /// @param verbose 
        PolygonClippingOp(
            std::string countryCode,
            bool verbose
        );

        /// @brief 
        ~PolygonClippingOp();


		/// \brief
		static void compute(
			std::string countryCode, 
			bool verbose
		);


	private:
		//--
		ign::feature::sql::FeatureStorePostgis*              _fsArea;
		//--
		// std::map<std::string, ign::geometry::GeometryPtr>    _mCountryGeomPtr;
		//--
		std::map<std::string, ign::geometry::GeometryPtr>    _mCountryGeomWithBuffPtr;
		//--
		epg::log::EpgLogger*                                 _logger;
		//--
		epg::log::ShapeLogger*                               _shapeLogger;
		//--
		std::string                                          _countryCode;
		//--
		bool                                                 _verbose;

	private:

		//--
		void _init();

		//--
		void _compute() const;

    };

}
}

#endif