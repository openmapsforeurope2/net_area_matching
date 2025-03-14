#ifndef _APP_CALCUL_STANDINGWATEROP_H_
#define _APP_CALCUL_STANDINGWATEROP_H_

//SOCLE
#include <ign/feature/sql/FeatureStorePostgis.h>


//EPG
#include <epg/log/EpgLogger.h>
#include <epg/log/ShapeLogger.h>

namespace app{
namespace calcul{

	/// @brief 
	class StandingWaterOp {

	public:

	
        /// @brief 
        /// @param borderCode 
        /// @param verbose 
		StandingWaterOp(
            std::string borderCode,
            bool verbose
        );

        /// @brief 
        ~StandingWaterOp();

		/// @brief 
		/// @param borderCode 
		/// @param verbose 
		static void AddStandingWater(
			std::string borderCode,
			bool verbose
		);

		/// @brief 
		/// @param borderCode 
		/// @param verbose 
		static void SortingStandingWater(
			std::string borderCode,
			bool verbose
		);

	private:
		//--
		ign::feature::sql::FeatureStorePostgis*                  _fsArea;
		//--
		ign::feature::sql::FeatureStorePostgis*                  _fsAreaStandingWater;
		//--
		epg::log::EpgLogger*                                     _logger;
		//--
		epg::log::ShapeLogger*                                   _shapeLogger;
		//--
		std::string                                              _borderCode;
		//--
		std::vector<std::string>								 _vCountriesCodeName;
		//--
		std::string												 _attrValueStandingWater;
		//--
		bool                                                     _verbose;


	private:

		//--
		void _init();
		
		//--
		void _addStandingWater();

		//--
		void _sortingStandingWater();
    };

}
}

#endif