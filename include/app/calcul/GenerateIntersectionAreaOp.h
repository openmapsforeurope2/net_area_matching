#ifndef _APP_CALCUL_GENERATEINTERSECTIONAREAOP_H_
#define _APP_CALCUL_GENERATEINTERSECTIONAREAOP_H_

//SOCLE
#include <ign/feature/sql/FeatureStorePostgis.h>

//EPG
#include <epg/log/EpgLogger.h>



namespace app{
namespace calcul{

	/// @brief Classe destinée à la création des intersections entre
	/// les surfaces de deux pays frontaliers.
	class GenerateIntersectionAreaOp {

	public:

	
        /// @brief Condtructeur
        /// @param borderCode Code frontière (code double)
        /// @param verbose Mode verbeux
		GenerateIntersectionAreaOp(
            std::string borderCode,
            bool verbose
        );

        /// @brief Destructeur
        ~GenerateIntersectionAreaOp();


		/// \brief Lance le calcul des intersections. Seules les intersections
		/// surfaciques sont conservées est stockées dans une table dédiée
		/// sous forme de polygones (les multipolygones seront éclatés)
		static void Compute(
			std::string borderCode,
            bool verbose
		);


	private:
		//--
		ign::feature::sql::FeatureStorePostgis*                  _fsArea;
		//--
		ign::feature::sql::FeatureStorePostgis*                  _fsIntArea;
		//--
		epg::log::EpgLogger*                                     _logger;
		//--
		std::string                                              _borderCode;
		//--
		std::vector<std::string>                                 _vCountry;
		//--
		bool                                                     _verbose;

	private:

		//--
		void _init();

		//--
		void _compute() const;

		//--
		ign::geometry::Geometry* _getIntersectingAreas(
			ign::geometry::Geometry const& geom,
			size_t countryIndex
		) const;

		//--
		void _persistGeom( 
			ign::geometry::Geometry const& geom,
			std::string const& countryCode,
			std::string const& linkedNatId
		 ) const;
    };

}
}

#endif