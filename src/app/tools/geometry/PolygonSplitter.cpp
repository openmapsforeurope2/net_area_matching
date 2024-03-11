#include <app/tools/geometry/PolygonSplitter.h>

//SOCLE
#include <ign/data/all.h>
#include <ign/geometry/graph/detail/NextEdge.h>


namespace app{
namespace tools{
namespace geometry{

	///
	///
	///
	PolygonSplitter::PolygonSplitter( ign::geometry::Polygon const& poly, double scale ):
		_scale( scale ),
		_count(0),
		_numRings(0)
	{
		_planarizer.reset( new ign::geometry::graph::tools::SnapRoundPlanarizer< detail::PolygonSplitterGraphType >( _graph, _scale ) );
		// for( size_t i = 0 ; i < poly.numInteriorRing() ; ++i )
		// 	_holes.push_back( new ign::geometry::Polygon( poly.interiorRingN(i) ) );
		// _addGeometry( poly.exteriorRing() );

        for( size_t i = 0 ; i < poly.numRings() ; ++i )
			_addGeometry( poly.ringN(i) );

		_numRings = _count;
	}

	///
	///
	///
	PolygonSplitter::~PolygonSplitter()
	{
		// for( size_t i = 0 ; i < _holes.size() ; ++i )
		// 	delete _holes[ i ];
	}

	///
	///
	///
	void PolygonSplitter::addCuttingGeometry( ign::geometry::Geometry const& geom )
	{
		switch( geom.getGeometryType() )
		{
			case ign::geometry::Geometry::GeometryTypeLineString :{
				_addGeometry( geom.asLineString() );
				break;}
			case ign::geometry::Geometry::GeometryTypePolygon :{
				_addGeometry( geom.asPolygon() );
				break;}
			case ign::geometry::Geometry::GeometryTypeMultiLineString :{
				_addGeometry( geom.asMultiLineString() );
				break;}
			case ign::geometry::Geometry::GeometryTypeMultiPolygon :{
				_addGeometry( geom.asMultiPolygon() );
				break;}
			default :{
				IGN_THROW_EXCEPTION( "epg::tools::geometry::PolygonSplitter:addCuttingGeometry : geometry type "+geom.getGeometryTypeName()+" not allowed" );
				break;
			}
		}
	}

	///
	///
	///
	void PolygonSplitter::_addGeometry( ign::geometry::LineString const& ls )
	{
		_planarizer->addEdge( ls, ign::data::Integer( _count++ ).toString() );
	}

	///
	///
	///
	void PolygonSplitter::_addGeometry( ign::geometry::Polygon const& poly )
	{
		for( size_t i = 0 ; i < poly.numRings() ; ++i )
			_addGeometry( poly.ringN(i) );
	}

	///
	///
	///
	void PolygonSplitter::_addGeometry( ign::geometry::MultiLineString const& mls )
	{
		for( size_t i = 0 ; i < mls.numGeometries() ; ++i )
			_addGeometry( mls.lineStringN(i) );
	}

	///
	///
	///
	void PolygonSplitter::_addGeometry( ign::geometry::MultiPolygon const& mp )
	{
		for( size_t i = 0 ; i < mp.numGeometries() ; ++i )
			_addGeometry( mp.polygonN(i) );
	}

	///
	///
	///
	void PolygonSplitter::split( std::vector< ign::geometry::Polygon >& vPolygons )
	{
		_split();

		std::set<face_descriptor> sHoleFaces = _getHoleFaces();

		face_iterator fit, fend;
		for( _graph.faces( fit, fend ) ; fit != fend ; ++fit )
		{
			IGN_ASSERT( _graph[ *fit ].isFinite );
			if( *_graph[ *fit ].isFinite )
			{
				ign::geometry::GeometryPtr geom( _graph.getGeometry( *fit ).clone() );

				// for( size_t i = 0 ; i < _holes.size() ; ++i )
				// 	if( geom->intersects( *_holes[i] ) )
				// 		geom.reset( geom->Difference( *_holes[i] ) );

				if (sHoleFaces.find(*fit) != sHoleFaces.end()) continue;

				_getPolygons( *geom, vPolygons );
			}
		}
	}

	///
	///
	///
	std::set<PolygonSplitter::face_descriptor> PolygonSplitter::_getHoleFaces() const 
	{
		std::set<face_descriptor> sHoleFaces;

		std::set<face_descriptor> sFaceTouchingExtBound;
		std::set<face_descriptor> sTreatedFaces;
		std::map<face_descriptor, face_descriptor> mChildParent;
		face_iterator fit, fend;
		for( _graph.faces( fit, fend ) ; fit != fend ; ++fit ) {
			sTreatedFaces.insert(*fit);

			oriented_edge_descriptor startEdge = _graph.getIncidentEdge( *fit );
			oriented_edge_descriptor nextEdge = startEdge;
			do{

				std::vector< std::string > vOrigins = _graph.origins( nextEdge.descriptor );

				bool isCuttingEdge = true;
				for( size_t i = 0 ; i < vOrigins.size() ; ++i ) 
				{
					int origin = ign::data::String( vOrigins[i] ).toInteger();
					if( origin == 0 ) sFaceTouchingExtBound.insert(*fit);
					if( origin < _numRings ) isCuttingEdge = false;
				}

				if (isCuttingEdge) {
					std::pair< bool, face_descriptor > foundRf = _graph.rightFace( nextEdge );
					

					if (foundRf.first) {
						if (sTreatedFaces.find(foundRf.second) == sTreatedFaces.end()) {
							
							std::map<face_descriptor, face_descriptor>::const_iterator mit = mChildParent.find(*fit);

							face_descriptor fParent = mit == mChildParent.end() ? mit->second : *fit;

							mChildParent.insert(std::make_pair(foundRf.second, fParent));
						}
						
					}
				}

				nextEdge = ign::geometry::graph::detail::nextEdge( nextEdge, _graph );
			}while( nextEdge != startEdge );
		}
		
		std::map<face_descriptor, std::set<face_descriptor>> mGroups;
		std::set<face_descriptor> sNoHoleGroups;
		for (std::map<face_descriptor, face_descriptor>::const_iterator mit = mChildParent.begin() ; mit != mChildParent.end() ; ++mit) {
			std::map<face_descriptor, std::set<face_descriptor>>::iterator mg = mGroups.find(mit->second);
			if (mg == mGroups.end()) {
				mg = mGroups.insert(std::make_pair(mit->second, std::set<face_descriptor>())).first;
				mg->second.insert(mit->second);
				if (sFaceTouchingExtBound.find(mit->second) != sFaceTouchingExtBound.end()) sNoHoleGroups.insert(mg->first);
			}
			mg->second.insert(mit->first);
			if (sFaceTouchingExtBound.find(mit->first) != sFaceTouchingExtBound.end()) sNoHoleGroups.insert(mg->first);
		}

		for (std::map<face_descriptor, std::set<face_descriptor>>::const_iterator mit = mGroups.begin() ; mit != mGroups.end() ; ++mit ) {
			if (sNoHoleGroups.find(mit->first) != sNoHoleGroups.end() ) continue;
			sHoleFaces.insert(mit->second.begin(), mit->second.end());
		}

		return sHoleFaces;		
	}

	///
	///
	///
	void PolygonSplitter::_getPolygons( ign::geometry::Geometry const& geom, std::vector< ign::geometry::Polygon >& vPolygons )const
	{
		switch( geom.getGeometryType() )
		{
			case ign::geometry::Geometry::GeometryTypePolygon :{
				vPolygons.push_back( geom.asPolygon() );
				break;}
			case ign::geometry::Geometry::GeometryTypeMultiPolygon :{
				ign::geometry::MultiPolygon const& mp = geom.asMultiPolygon();
				for( size_t i = 0 ; i < mp.numGeometries() ; ++i )
					vPolygons.push_back( mp.polygonN(i) );
				break;}
			case ign::geometry::Geometry::GeometryTypeGeometryCollection :{
				ign::geometry::GeometryCollection const& gc = geom.asGeometryCollection();
				for( size_t i = 0 ; i < gc.numGeometries() ; ++i )
				{
					if( gc.geometryN(i).isPolygon() )
						vPolygons.push_back( gc.geometryN(i).asPolygon() );
					if( gc.geometryN(i).isMultiPolygon() )
					{
						ign::geometry::MultiPolygon const& mp = gc.geometryN(i).asMultiPolygon();
						for( size_t k = 0 ; k < mp.numGeometries() ; ++k )
							vPolygons.push_back( mp.polygonN(k) );
					}
				}
				break;}
		}
	}

	///
	///
	///
	void PolygonSplitter::_split()
	{
		_planarizer->planarize();
		ign::geometry::graph::detail::createFaces( _graph );

		std::list< face_descriptor > lFaces;
		face_iterator fit, fend;
		for( _graph.faces( fit, fend ) ; fit != fend ; ++fit )
			lFaces.push_back( *fit );

		while( !lFaces.empty() )
		{
			face_descriptor f = lFaces.back();
			lFaces.pop_back();

			oriented_edge_descriptor startEdge = _graph.getIncidentEdge( f );
			oriented_edge_descriptor nextEdge = startEdge;
			do{

				std::pair< bool, face_descriptor > foundRf = _graph.rightFace( nextEdge );
				if( !foundRf.first )
				{
					if( _isBoundary( nextEdge.descriptor ) ) _graph[ f ].isFinite = false;
					else _graph[ f ].isFinite = true;
					break;
				}
				else
				{
					if( _graph[ foundRf.second ].isFinite )
					{
						if( _isBoundary( nextEdge.descriptor ) ) _graph[ f ].isFinite = *_graph[ foundRf.second ].isFinite;
						else _graph[ f ].isFinite = !*_graph[ foundRf.second ].isFinite;
						break;
					}
				}

				nextEdge = ign::geometry::graph::detail::nextEdge( nextEdge, _graph );
			}while( nextEdge != startEdge );

			if( !_graph[ f ].isFinite ) lFaces.push_front( f );/*non traite*/
		}
	}

	///
	///
	///
	bool PolygonSplitter::_isBoundary( edge_descriptor e )const
	{
		std::vector< std::string > vOrigins = _graph.origins( e );

		int numBorders = 0;
		bool hasBoundary = false;
		for( size_t i = 0 ; i < vOrigins.size() ; ++i ) 
		{
			int origin = ign::data::String( vOrigins[i] ).toInteger();
			if( origin < _numRings ) ++numBorders;
			else hasBoundary = true;
		}
		
		if( !hasBoundary ) return false;
		if( numBorders % 2 != 0 ) return false;
		return true;
	}

}
}
}
