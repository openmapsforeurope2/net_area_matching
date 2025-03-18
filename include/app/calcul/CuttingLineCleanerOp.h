#ifndef _APP_CALCUL_CUTTINGLINECLEANEROP_H_
#define _APP_CALCUL_CUTTINGLINECLEANEROP_H_

//SOCLE
#include <ign/feature/sql/FeatureStorePostgis.h>


//EPG
#include <epg/log/EpgLogger.h>
#include <epg/log/ShapeLogger.h>


namespace app{
namespace calcul{

	/// @brief Classe pour le nettoyage des cutting lines orphelines
	class CuttingLineCleanerOp {

	public:

	
		/// @brief Constructeur
		/// @param verbose Mode verbeux
		CuttingLineCleanerOp(
            bool verbose
        );

        /// @brief Destructeur
        ~CuttingLineCleanerOp();

		/// \brief Lance le nettoyage. On vérifie que les cutting lines sont toujours en 
		/// contact avec les surfaces à l'origine de leur création (ses surfaces peuvent 
		/// avoir été supprimées dans une étape antérieure). Si une cutting line n'est
		/// en contact avec aucune surface (du même pays), elle est supprimée.
		/// @param verbose Mode verbeux
		static void Compute(
			bool verbose
		);

    private:
		//--
		ign::feature::sql::FeatureStorePostgis*                  _fsArea;
		//--
		ign::feature::sql::FeatureStorePostgis*                  _fsCl;
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

		//--
		bool _hasIntersectingAreas( 
            ign::geometry::LineString const& clGeom,
            std::string const& country
        ) const;
    };
}
}

#endif