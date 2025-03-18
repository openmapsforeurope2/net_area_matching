#ifndef _APP_CALCUL_POLYGONMERGEROP_H_
#define _APP_CALCUL_POLYGONMERGEROP_H_

//SOCLE
#include <ign/feature/sql/FeatureStorePostgis.h>
#include <ign/tools/stringtools.h>

//EPG
#include <epg/log/EpgLogger.h>
#include <epg/log/ShapeLogger.h>


namespace app{
namespace calcul{

	/// @brief Classe utilisée pour fusionner les surfaces de même identifiant national
	/// A utiliser conjointement avec les classes PolygonSplitterOp et PolygonCleanerOp
	class PolygonMergerOp {

	public:

	
        /// @brief Constructeur
        /// @param verbose Mode verbeux
        PolygonMergerOp(
            bool verbose
        );

        /// @brief Destructeur
        ~PolygonMergerOp();


		/// @brief Lance la fusion des surfaces possédant le même identifiant national
		/// @param verbose 
		static void Compute(
			bool verbose
		);


	private:
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
        std::list<std::string> _getAllIdentifiers( std::string const& tableName ) const;

		//--
		void _compute() const;

    };

}
}

#endif