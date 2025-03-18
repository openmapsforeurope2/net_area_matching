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

	/// @brief Classe dépréciée
	class PolygonClippingOp {

	public:

	
        /// @brief Constructeur
        /// @param countryCode Code pays simple
        /// @param verbose Mode verbeux
        PolygonClippingOp(
            std::string countryCode,
            bool verbose
        );

        /// @brief Destructeur
        ~PolygonClippingOp();


		/// @brief Lance la découpe
		/// @param countryCode Code pays simple
        /// @param verbose Mode verbeux
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