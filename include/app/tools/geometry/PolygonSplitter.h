#ifndef _APP_TOOLS_GEOMETRY_POLYGONSPLITTER_H_
#define _APP_TOOLS_GEOMETRY_POLYGONSPLITTER_H_

#include <set>

//SOCLE
#include <ign/geometry/MultiPolygon.h>
#include <ign/geometry/graph/GeometryGraph.h>
#include <ign/geometry/graph/tools/SnapRoundPlanarizer.h>

namespace app{
namespace tools{
namespace geometry{

	namespace detail{


		struct PolygonSplitterFace : 
			public ign::geometry::graph::FacePropertiesT< ign::geometry::graph::PunctualVertexProperties, ign::geometry::graph::LinearEdgeProperties >  {
			PolygonSplitterFace(){};
			boost::optional< bool > isFinite;
		};

		typedef ign::geometry::graph::GeometryGraph< 
			ign::geometry::graph::PunctualVertexProperties, 
			ign::geometry::graph::LinearEdgeProperties, 
			PolygonSplitterFace
		>  PolygonSplitterGraphType;
	}

	class PolygonSplitter{

		typedef detail::PolygonSplitterGraphType::oriented_edge_descriptor  oriented_edge_descriptor;
		typedef detail::PolygonSplitterGraphType::edge_descriptor           edge_descriptor;
		typedef detail::PolygonSplitterGraphType::face_iterator             face_iterator;
		typedef detail::PolygonSplitterGraphType::face_descriptor           face_descriptor;

	public:
		/// \brief 
		PolygonSplitter( ign::geometry::Polygon const& poly, double scale = 1e7 );

		/// \brief
		~PolygonSplitter();

		/// \brief
		void addCuttingGeometry( ign::geometry::Geometry const& geom );

		/// \brief
		void split( std::vector< ign::geometry::Polygon >& vPolygons );

	private:

		//--
		void _addGeometry( ign::geometry::LineString const& ls );
		//--
		void _addGeometry( ign::geometry::Polygon const& poly );
		//--
		void _addGeometry( ign::geometry::MultiLineString const& mls );
		//--
		void _addGeometry( ign::geometry::MultiPolygon const& mp );
		//--
		void _getPolygons( ign::geometry::Geometry const& geom, std::vector< ign::geometry::Polygon >& vPolygons ) const;
		//--
		void _split();
		//--
		std::set<PolygonSplitter::face_descriptor> _getHoleFaces() const;
		//--
		bool _isBoundary( edge_descriptor e ) const;


	private:

		//--
		detail::PolygonSplitterGraphType                                                        _graph;
		//--
		double                                                                                  _scale;
		//--
		std::auto_ptr< ign::geometry::graph::tools::SnapRoundPlanarizer< detail::PolygonSplitterGraphType > >   _planarizer;
		//--
		size_t                                                                                  _count;
		//--
		size_t                                                                                  _numRings;
		//--
		// std::vector< ign::geometry::Polygon* >                                                  _holes;


	
	};

}
}
}

#endif