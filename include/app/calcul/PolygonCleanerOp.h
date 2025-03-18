#ifndef _APP_CALCUL_POLYGONCLEANEROP_H_
#define _APP_CALCUL_POLYGONCLEANEROP_H_

//SOCLE
#include <ign/feature/sql/FeatureStorePostgis.h>
#include <ign/tools/stringtools.h>

//EPG
#include <epg/log/EpgLogger.h>
#include <epg/log/ShapeLogger.h>
#include <epg/tools/MultiLineStringTool.h>


namespace app{
namespace calcul{

	/// @brief Classe utilisée pour supprimer des surfaces hors de leur pays
	/// A utiliser conjointement avec les classes PolygonSplitterOp et PolygonMergerOp
	class PolygonCleanerOp {

	public:

	
        /// @brief Constructeur
        /// @param borderCode Code frontière (code double)
        /// @param verbose Mode verbeux
        PolygonCleanerOp(
            std::string borderCode,
            bool verbose
        );

        /// @brief Destructeur
        ~PolygonCleanerOp();


		/// @brief Supprime les surfaces qui n'intersectent pas leur pays et dont
		/// la demie distance de Hausdorff (éloignement maximum) avec la frontière
		/// est supérieure à un seuil.
		/// @param borderCode Code frontière (code double)
        /// @param verbose Mode verbeux
		static void Compute(
			std::string borderCode, 
			bool verbose
		);


	private:
		//--
		ign::feature::sql::FeatureStorePostgis*                  _fsArea;
		//--
		epg::tools::MultiLineStringTool*                         _boundaryTool;
		//--
		std::map<std::string, ign::geometry::GeometryPtr>        _mCountryGeomPtr;
		//--
		epg::log::EpgLogger*                                     _logger;
		//--
		epg::log::ShapeLogger*                                   _shapeLogger;
		//--
		std::string                                              _borderCode;
		//--
		bool                                                     _verbose;

	private:

		//--
		void _init();

        //--
        void _addLs(ign::geometry::Geometry const& geom, ign::geometry::MultiLineString & mls) const;

		//--
		void _compute() const;

    };

}
}

#endif