#ifndef _APP_CALCUL_STANDINGWATEROP_H_
#define _APP_CALCUL_STANDINGWATEROP_H_

//SOCLE
#include <ign/feature/sql/FeatureStorePostgis.h>


//EPG
#include <epg/log/EpgLogger.h>
#include <epg/log/ShapeLogger.h>

namespace app{
namespace calcul{

	/// @brief Classe utilitaire pour l'import des surfaces standing_water
	/// dans la table des surfaces watercourse_area
	class StandingWaterOp {

	public:

	
        /// @brief Constructeur 
        /// @param borderCode Code frontière (code double)
        /// @param verbose Mode verbeux
		StandingWaterOp(
            std::string borderCode,
            bool verbose
        );

        /// @brief Destructeur
        ~StandingWaterOp();

		/// @brief Lance l'import des surfaces de la table standing_water dans 
		/// la table watercourse_area. Un champ est ajouté à la table watercourse_area
		/// afin de pour marquer les surfaces issue de cet import.
		/// @param borderCode Code frontière (code double)
        /// @param verbose Mode verbeux
		static void AddStandingWater(
			std::string borderCode,
			bool verbose
		);

		/// @brief Lance l'export des surfaces de watercourse_area, marquées comme
		/// étant issues d'un précédent import depuis la table standing_water, vers 
		/// la table standing_water.
		/// @param borderCode Code frontière (code double)
        /// @param verbose Mode verbeux
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
		void _addStandingWater() const;

		//--
		void _sortingStandingWater() const;
    };

}
}

#endif