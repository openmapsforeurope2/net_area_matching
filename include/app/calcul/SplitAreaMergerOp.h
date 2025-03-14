#ifndef _APP_CALCUL_SPLITAREAMERGEROP_H_
#define _APP_CALCUL_SPLITAREAMERGEROP_H_

//SOCLE
#include <ign/feature/sql/FeatureStorePostgis.h>


//EPG
#include <epg/log/EpgLogger.h>
#include <epg/log/ShapeLogger.h>
#include <epg/tools/MultiLineStringTool.h>


namespace app{
namespace calcul{

	/// @brief 
	class SplitAreaMergerOp {

	public:

	
		/// @brief 
		/// @param verbose 
		SplitAreaMergerOp(
            bool verbose
        );

        /// @brief 
        ~SplitAreaMergerOp();

		/// @brief 
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
		void _compute() const;

        //--
        bool _computeEngine( bool mergeByNatId = true ) const;

		//--
		void _mergeGroups(std::vector<std::map<std::string, ign::feature::Feature>> & vmAreas) const;

		//--
		bool _addAreas(
            ign::feature::Feature const& feat1,
            ign::feature::Feature const& feat2,
            std::vector<std::map<std::string, ign::feature::Feature>> & vmAreas
        ) const;

		//--
		std::map<double, ign::feature::Feature> _getNeighboursWithArea(
            ign::feature::Feature const& fArea,
            ign::feature::FeatureFilter const& filterArea_
        ) const;

		//--
		std::vector<ign::feature::Feature> _getNeighbours(
			ign::feature::Feature const& fArea,
            ign::feature::FeatureFilter const& filterArea_
		) const;

		//--
		double _getLength(
	        ign::geometry::Polygon const& poly
        ) const;

		//--
		double _getLength(
	        ign::geometry::MultiPolygon const& mp
        ) const;

    };
}
}

#endif