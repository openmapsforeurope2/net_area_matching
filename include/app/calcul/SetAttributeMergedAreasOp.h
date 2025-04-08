#ifndef _APP_CALCUL_SETATTRIBUTEMERGEDAREASOP_H_
#define _APP_CALCUL_SETATTRIBUTEMERGEDAREASOP_H_

//SOCLE
#include <ign/feature/sql/FeatureStorePostgis.h>
#include <ign/tools/stringtools.h>

//EPG
#include <epg/log/EpgLogger.h>
#include <epg/log/ShapeLogger.h>


#include <ome2/calcul/utils/AttributeMerger.h>


namespace app{
namespace calcul{

	/// @brief Classe utilisée pour l'affectation des attributs des surfaces fusionnées
	class SetAttributeMergedAreasOp {

	public:

	
        /// @brief Constructeur
        /// @param borderCode Code frontière (code double)
        /// @param verbose Mode verbeux
		SetAttributeMergedAreasOp(
			std::string borderCode,
            bool verbose
        );

        /// @brief Destructeur
        ~SetAttributeMergedAreasOp();


		/// @brief Lance l'affection des attributs des surfaces fusionnées.
		/// On recherche les surfaces d'origines des pays 1 (A1) et 2 (A2) ayant 
		/// la plus grande intersection avec la surface fusionnée. si l'aire
		/// de A1 représente moins de 10% de l'aire de A2 la surface fusionnée prend
		/// les attributs de A2. A l'inverse si l'aire de A2 représente moins de
		/// 10% de l'aire de A1 la surface fusionnée prends les attributs de A1.
		/// Dans tous les autres cas les attributs de la surfaces fusionnées
		/// seront calculés par concaténation des attributs de A1 et A2.
		/// @param borderCode Code frontière (code double)
        /// @param verbose Mode verbeux
		static void Compute(
			std::string borderCode,
			bool verbose
		);


	private:
		//--
		std::string                                              _borderCode;
		//--
		std::vector<std::string>                                 _vCountry;
		//--
		ign::feature::sql::FeatureStorePostgis*                  _fsArea;
		//--
		ign::feature::sql::FeatureStorePostgis*                  _fsAreaInitCleaned;
		//--
		epg::log::EpgLogger*                                     _logger;
		//--
		epg::log::ShapeLogger*                                   _shapeLogger;
		//--
		bool                                                     _verbose;
		//--
		ome2::calcul::utils::AttributeMerger                     _attrMergerOnBorder;
		//--
		double													 _thresholdAreaAttr;

	private:

		//--
		void _init();

		//--
		void _compute() const;

		//--
		double _getAreaMergedByCountry(
			ign::geometry::MultiPolygon& geomAreaMerged,
			ign::feature::FeatureFilter& filterArroundAreaFromCountry,
			ign::feature::Feature& fMergedInit
		) const;

    };

}
}

#endif