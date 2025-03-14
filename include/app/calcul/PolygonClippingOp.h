#ifndef _APP_CALCUL_POLYGONCLIPPINGOP_H_
#define _APP_CALCUL_POLYGONCLIPPINGOP_H_

//SOCLE
#include <ign/feature/sql/FeatureStorePostgis.h>
#include <ign/tools/stringtools.h>

//EPG
#include <epg/log/EpgLogger.h>
#include <epg/log/ShapeLogger.h>


namespace app{
namespace calcul{

	/// @brief 
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
		static void Compute(
			std::string countryCode, 
			bool verbose
		);


	private:
		//--
		ign::feature::sql::FeatureStorePostgis*              _fsArea;
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