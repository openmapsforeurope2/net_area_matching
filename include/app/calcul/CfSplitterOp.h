#ifndef _APP_CALCUL_CFSPLITTEROP_H_
#define _APP_CALCUL_CFSPLITTEROP_H_

//SOCLE
#include <ign/feature/sql/FeatureStorePostgis.h>


//EPG
#include <epg/log/EpgLogger.h>
#include <epg/log/ShapeLogger.h>
#include <epg/tools/MultiLineStringTool.h>
#include <epg/tools/geometry/SegmentIndexedGeometry.h>


namespace app{
namespace calcul{

	/// @brief 
	class CfSplitterOp {

	public:

	
		/// @brief 
		/// @param verbose 
		CfSplitterOp(
            bool verbose
        );

        /// @brief 
        ~CfSplitterOp();

        /// \brief
		static void compute(
			bool verbose
		);

    private:
		//--
		ign::feature::sql::FeatureStorePostgis*                  _fsArea;
        //--
		ign::feature::sql::FeatureStorePostgis*                  _fsCp;
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
        bool _touchAlreadyHittenSubLs( 
            std::vector<epg::tools::geometry::SegmentIndexedGeometryInterface*> const vIndexedSubLs,
            std::set<size_t> const& sHittenSubLs,
            ign::geometry::Point const& pt
        ) const;

		//--
		void _clean( ign::geometry::Polygon & poly ) const;

		//--
		ign::geometry::Geometry* _getMedialAxis(ign::geometry::Polygon const& poly) const;

		//--
		double _getRatio(
            ign::geometry::Polygon const& poly,
            ign::geometry::LineString const& ls
        ) const;

        //--
        std::vector<ign::geometry::LineString> _getSubLs(
            ign::geometry::LineString const& ring,
            std::set<size_t> const& sCuttingIndex
        ) const;

        //--
        std::set<size_t> _getCuttingIndex(std::vector<int> const& vCpIndex) const;

        //--
        std::vector<int> _getCpIndex(
            ign::geometry::LineString const& ring,
            std::vector<ign::geometry::Point> const& vCp
        ) const;

		//--
		void _getAllClEndingPoints(
            ign::geometry::Polygon const& poly,
            std::vector<ign::geometry::Point> & vPts
        ) const;

        //--
        std::map<std::string, ign::geometry::Point> _getAllCp(
            ign::geometry::Polygon const& poly,
            std::set<std::string> & sIntersectionCp
        ) const;

        //--
        void _removeHoles( ign::geometry::Polygon & poly ) const;

		//--
		std::pair<bool, ign::geometry::LineString> _computeSectionGeometry(
            ign::geometry::LineString const& sectionGeom, 
            ign::geometry::Polygon const& poly,
            epg::tools::MultiLineStringTool** mslToolPtr
        ) const;

		//--
		ign::geometry::LineString _computeCuttingLineGeometry(
            std::string const& clId,
            ign::geometry::LineString clGeom, 
            ign::geometry::Polygon const& poly,
			std::vector<ign::geometry::Point> & vClEndingPoints,
            std::map<std::string, ign::geometry::LineString> & mModifiedCl
        ) const;

        //--
        ign::geometry::LineString _getClgeometry(
            ign::feature::Feature const& fCl,
            std::map<std::string, ign::geometry::LineString> & mModifiedCl
        ) const;

		//--
		bool _snap(
            ign::geometry::Point & pt,
            ign::geometry::Polygon const& poly,
            double threshold
        ) const;
                        
        //--
        bool _snapOnOtherClEndingPoint(
            ign::geometry::Point & pt,
            std::vector<ign::geometry::Point> const& vClEndingPoints,
            double threshold
        ) const;

    };
}
}

#endif