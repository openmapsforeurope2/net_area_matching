#ifndef _APP_CALCUL_GENERATECUTTINGLINESOP_H_
#define _APP_CALCUL_GENERATECUTTINGLINESOP_H_

//SOCLE
#include <ign/feature/sql/FeatureStorePostgis.h>
#include <ign/geometry/graph/GeometryGraph.h>


//EPG
#include <epg/log/EpgLogger.h>
#include <epg/log/ShapeLogger.h>
#include <epg/sql/tools/IdGeneratorFactory.h>

namespace app{
namespace calcul{

	/// @brief Classe dédiée à la création des cutting lines
	class GenerateCuttingLinesOp {

	public:

		typedef ign::geometry::graph::GeometryGraph< ign::geometry::graph::PunctualVertexProperties, ign::geometry::graph::LinearEdgeProperties>  GraphType;
		typedef typename GraphType::edge_descriptor edge_descriptor;
		typedef typename GraphType::vertex_descriptor vertex_descriptor;


	public:

        /// @brief Constructeur
        /// @param borderCode Code frontière (code double)
        /// @param verbose Mode verbeux
		GenerateCuttingLinesOp(
            std::string borderCode,
            bool verbose
        );

        /// @brief Destructeur
        ~GenerateCuttingLinesOp();

		/// \brief Lance la génération des cutting lines. Les cuttings lines
		/// correspondent à la géométrie linéaire partagée par deux surfaces
		/// d'un même pays en accostage.
		static void Compute(
			std::string borderCode,
            bool verbose
		);



	private:
		//--
		ign::feature::sql::FeatureStorePostgis*                  _fsArea;
		//--
		ign::feature::sql::FeatureStorePostgis*                  _fsCutL;
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
		void _compute() const;

		//--
		void _generateCutlByCountry(
			std::string countryCode
		) const;
		

    };

}
}

#endif