#ifndef _APP_CALCUL_INTERSECTINGAREASMERGEROP_H_
#define _APP_CALCUL_INTERSECTINGAREASMERGEROP_H_

//SOCLE
#include <ign/feature/sql/FeatureStorePostgis.h>
#include <ign/tools/stringtools.h>

//EPG
#include <epg/log/EpgLogger.h>
#include <epg/log/ShapeLogger.h>


namespace app{
namespace calcul{

	/// @brief Classe utilisée pour la fusion des surfaces qui se chevauchent
	/// de deux pays
	class IntersectingAreasMergerOp {

	public:

	
        /// @brief Constructeur
        /// @param borderCode Code frontière (code double)
        /// @param verbose Mode verbeux
        IntersectingAreasMergerOp(
			std::string const& borderCode,
            bool verbose
        );

        /// @brief Destructeur
        ~IntersectingAreasMergerOp();


		/// @brief Lance la fusion
		/// @param borderCode Code frontière (code double)
        /// @param verbose Mode verbeux
		static void Compute(
			std::string const& borderCode,
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
		epg::log::EpgLogger*                                     _logger;
		//--
		epg::log::ShapeLogger*                                   _shapeLogger;
		//--
		bool                                                     _verbose;


	private:

		//--
		void _init();

		//--
		void _compute() const;

        /// @brief A partir d'un polygon d'un pays, récupère de proche en proche
		/// en identifiant les chevauchements entre les polygones des deux pays,
		/// la liste des polygones à fusionner.
        /// @param mp Géométrie du polygone de départ
        /// @param country Index correspondant au pays de départ
        /// @param sIntersectingArea liste des identifiants des surfaces à fusionner
        void _getIntersectingAreas(
            ign::geometry::MultiPolygon const& mp, 
            size_t country, 
            std::set<std::string> & sIntersectingArea
        ) const;

		//--
		std::string _toSqlList(std::set<std::string> const& s, std::string separator = ",") const;

    };

}
}

#endif