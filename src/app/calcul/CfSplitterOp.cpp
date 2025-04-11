// APP
#include <app/calcul/CfSplitterOp.h>
#include <app/params/ThemeParameters.h>
#include <app/tools/zTools.h>
#include <app/tools/geometry/PolygonSplitter.h>

// BOOST
#include <boost/progress.hpp>

//SOCLE
#include <ign/math/Line2T.h>
#include <ign/geometry/io/WkbReader.h>

// EPG
#include <epg/Context.h>
#include <epg/params/EpgParameters.h>
#include <ome2/feature/sql/NotDestroyedTools.h>
#include <epg/sql/DataBaseManager.h>
#include <epg/tools/StringTools.h>
#include <epg/tools/TimeTools.h>
#include <epg/tools/FilterTools.h>
#include <epg/tools/geometry/project.h>
#include <epg/tools/geometry/interpolate.h>
#include <epg/tools/geometry/LineIntersector.h>
#include <epg/tools/geometry/getLength.h>
#include <ome2/geometry/tools/lineStringTools.h>
#include <ome2/geometry/tools/GetEndingPointsOp.h>
#include <ome2/geometry/tools/isSlimSurface.h>


namespace app
{
    namespace calcul
    {
        ///
        ///
        ///
        CfSplitterOp::CfSplitterOp(
            bool verbose
        ) : 
            _verbose(verbose)
        {
            _init();
        }

        ///
        ///
        ///
        CfSplitterOp::~CfSplitterOp()
        {
            _shapeLogger->closeShape("cfs_cutting_features");
            _shapeLogger->closeShape("cfs_sub_rings");
            _shapeLogger->closeShape("cfs_proj_to_sub_rings");
            _shapeLogger->closeShape("cfs_cutting_points");
            _shapeLogger->closeShape("cfs_long_paths");
            _shapeLogger->closeShape("cfs_short_paths");
            _shapeLogger->closeShape("cfs_cl_axial_proj");
            _shapeLogger->closeShape("cfs_cl_ortho_proj");
            _shapeLogger->closeShape("cfs_cl_axial_proj_pt");
            _shapeLogger->closeShape("cfs_cl_ortho_proj_pt");
            _shapeLogger->closeShape("cfs_cl_common_endings");
        }

        ///
        ///
        ///
        void CfSplitterOp::Compute(
			bool verbose
		) {
            CfSplitterOp CfSplitterOp(verbose);
            CfSplitterOp._compute();
        }


        ///
        ///
        ///
        void CfSplitterOp::_init()
        {
            //--
            _logger = epg::log::EpgLoggerS::getInstance();
            _logger->log(epg::log::INFO, "[START] initialization: " + epg::tools::TimeTools::getTime());

            //--
            _shapeLogger = epg::log::ShapeLoggerS::getInstance();
            _shapeLogger->addShape("cfs_cutting_features", epg::log::ShapeLogger::LINESTRING);
            _shapeLogger->addShape("cfs_sub_rings", epg::log::ShapeLogger::LINESTRING);
            _shapeLogger->addShape("cfs_proj_to_sub_rings", epg::log::ShapeLogger::LINESTRING);
            _shapeLogger->addShape("cfs_cutting_points", epg::log::ShapeLogger::POINT);
            _shapeLogger->addShape("cfs_long_paths", epg::log::ShapeLogger::LINESTRING);
            _shapeLogger->addShape("cfs_short_paths", epg::log::ShapeLogger::LINESTRING);
            _shapeLogger->addShape("cfs_cl_axial_proj", epg::log::ShapeLogger::LINESTRING);
            _shapeLogger->addShape("cfs_cl_ortho_proj", epg::log::ShapeLogger::LINESTRING);
            _shapeLogger->addShape("cfs_cl_axial_proj_pt", epg::log::ShapeLogger::POINT);
            _shapeLogger->addShape("cfs_cl_ortho_proj_pt", epg::log::ShapeLogger::POINT);
            _shapeLogger->addShape("cfs_cl_common_endings", epg::log::ShapeLogger::POINT);

            //--
            epg::Context *context = epg::ContextS::getInstance();

            // epg parameters
            epg::params::EpgParameters const& epgParams = context->getEpgParameters();
            std::string const areaTableName = epgParams.getValue(AREA_TABLE).toString();
            std::string const idName = epgParams.getValue(ID).toString();
            std::string const geomName = epgParams.getValue(GEOM).toString();

            // app parameters
            params::ThemeParameters * themeParameters = params::ThemeParametersS::getInstance();
            std::string cpTableName = themeParameters->getValue(CUTP_TABLE).toString();
            if (cpTableName == "") {
                std::string const cpTableSuffix = themeParameters->getValue(CUTP_TABLE_SUFFIX).toString();
                cpTableName = themeParameters->getParameter(AREA_TABLE_INIT).getValue().toString() + cpTableSuffix;
            }
            std::string clTableName = themeParameters->getValue(CUTL_TABLE).toString();
            if (clTableName == "") {
                std::string const clTableSuffix = themeParameters->getValue(CUTL_TABLE_SUFFIX).toString();
                clTableName = themeParameters->getParameter(AREA_TABLE_INIT).getValue().toString() + clTableSuffix;
            }

            //--
            _fsArea = context->getDataBaseManager().getFeatureStore(areaTableName, idName, geomName);

            //--
            _fsCp = context->getDataBaseManager().getFeatureStore(cpTableName, idName, geomName);

            //--
            _fsCl = context->getDataBaseManager().getFeatureStore(clTableName, idName, geomName);

            //--
            _logger->log(epg::log::INFO, "[END] initialization: " + epg::tools::TimeTools::getTime());
        };

        ///
        ///
        ///
        bool CfSplitterOp::_touchAlreadyHittenSubLs( 
            std::vector<epg::tools::geometry::SegmentIndexedGeometryInterface*> const vIndexedSubLs,
            std::set<size_t> const& sHittenSubLs,
            ign::geometry::Point const& pt
        ) const {

            for ( std::set<size_t>::const_iterator sit = sHittenSubLs.begin() ; sit != sHittenSubLs.end() ; ++sit ) {
                double d = vIndexedSubLs[*sit]->distance(pt, 1e-5);
                if(d >= 0) return true;
            }
            return false;
        }

        ///
        ///
        ///
        void CfSplitterOp::_compute() const {
            double pathLengthThreshold = 20;
            double minInRatio = 0.9;

            // epg parameters
            epg::params::EpgParameters const& epgParams = epg::ContextS::getInstance()->getEpgParameters();
            std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();
            std::string const geomName = epgParams.getValue(GEOM).toString();

            //--
            app::params::ThemeParameters* themeParameters = app::params::ThemeParametersS::getInstance();
            double const distSnapMergeCf = themeParameters->getValue(DIST_SNAP_MERGE_CF).toDouble();

            ign::feature::FeatureFilter filterArea(countryCodeName + " LIKE '%#%'");
            int numFeatures = ome2::feature::sql::numFeatures(*_fsArea, filterArea);
            boost::progress_display display(numFeatures, std::cout, "[ cp splitter  % complete ]\n");

            //--
            std::set<std::string> sArea2Delete;

            ign::feature::FeatureIteratorPtr itArea = ome2::feature::sql::getFeatures(_fsArea,filterArea);
            while (itArea->hasNext())
            {
                ++display;
                
                ign::feature::Feature const& fArea = itArea->next();
                ign::geometry::MultiPolygon mp = fArea.getGeometry().asMultiPolygon();
                std::string idOrigin = fArea.getId();

                //DEBUG
                _logger->log(epg::log::DEBUG, "Area id : "+idOrigin);

                std::vector< ign::geometry::Polygon > vPolygons;

                for (size_t i = 0 ; i < mp.numGeometries() ; ++i ) {
                    ign::geometry::Polygon & poly = mp.polygonN(i);

                    _clean(poly);

                    ign::geometry::Polygon polyWithoutHoles = poly;
                    _removeHoles(polyWithoutHoles);

                    //--
                    app::tools::geometry::PolygonSplitter polySplitter(poly);

                    //--
                    epg::tools::MultiLineStringTool* mslToolPtr = 0;


                    // On recupere les cp intersectant le poly
                    // on calcul les index correspondant de ces cp sur le contour exterieur du poly (ajouter les extremités des cl ?)
                    // on découpe le contour exterieur au niveau des cp

                    std::set<std::string> sIntersectionCp;
                    std::map<std::string, ign::geometry::Point> mCp = _getAllCp(polyWithoutHoles, sIntersectionCp); //TODO supprimer doublons sur le contour
                    std::vector<ign::geometry::Point> vCpCl;
                    for( std::map<std::string, ign::geometry::Point>::const_iterator mit = mCp.begin() ; mit != mCp.end() ; ++mit ) {
                        if (sIntersectionCp.find(mit->first) != sIntersectionCp.end() ) continue; //on ne crée pas de coupure au niveau des cp issues des surfaces intersections
                        vCpCl.push_back(mit->second);
                    }
                        
                    _getAllClEndingPoints(polyWithoutHoles, vCpCl);

                    //DEBUG
                    for(size_t i = 0 ; i < vCpCl.size() ; ++i) {
                        ign::feature::Feature feat;
                        feat.setGeometry(vCpCl[i]);
                        _shapeLogger->writeFeature("cfs_cutting_points", feat);
                    }

                    std::vector<int> vCpClIndex = _getCpIndex(polyWithoutHoles.exteriorRing(), vCpCl);
                    std::set<size_t> sCuttingIndex = _getCuttingIndex(vCpClIndex);
                    //TODO checker si plus de 1 points dans sCuttingIndex
                    std::vector<ign::geometry::LineString> vSubLs = _getSubLs(polyWithoutHoles.exteriorRing(), sCuttingIndex);

                    // Outil
                    std::vector<epg::tools::geometry::SegmentIndexedGeometryInterface*> vIndexedSubLs;
                    for(size_t i = 0 ; i < vSubLs.size() ; ++i) {
                        vIndexedSubLs.push_back(new epg::tools::geometry::SegmentIndexedGeometry(&vSubLs[i]));

                        //--
                        ign::feature::Feature feat;
                        feat.setGeometry(vSubLs[i]);
                        _shapeLogger->writeFeature("cfs_sub_rings", feat);
                    }

                    // seuil (X*largeur moyenne du poly)
                    double const projDistThreshold = 10*(polyWithoutHoles.area() / (polyWithoutHoles.exteriorRing().length()/2));


                    // on boucle sur les CP
                    std::set<std::string> sMergedCp;
                    for( std::map<std::string, ign::geometry::Point>::const_iterator mit_ = mCp.begin() ; mit_ != mCp.end() ; ++mit_ ) {

                        if ( sMergedCp.find(mit_->first) != sMergedCp.end() ) continue;
                        
                        ign::geometry::Point const& cpGeom = mit_->second;

                        // on merge les autres CP proches
                        // TODO : faut-il privilégier les CP sur le contour ou ceux à l'intérieur du poly ?
                        ign::feature::FeatureCollection fCollection;
                        ign::geometry::Envelope bboxPt(cpGeom.getEnvelope());
                        bboxPt.expandBy(distSnapMergeCf);
                        _fsCp->getFeaturesByExtent(bboxPt, fCollection);
                        for (ign::feature::FeatureCollection::iterator fcit = fCollection.begin() ; fcit != fCollection.end() ; ++fcit) {
                            if ( fcit->getId() == mit_->first ) continue;
                            if ( cpGeom.distance(fcit->getGeometry() ) < distSnapMergeCf)
                                sMergedCp.insert(fcit->getId());
                        }

                        bool cpIsOnRing = false;
                        std::map<double, std::pair<int, ign::geometry::Point>> mDistSubLsProj;
                        // normalement si doublon il n'est pas ajouté dans la map (même distance mDistProj.first)
                        // voir s'il faut ajouter une tolérance pour supprimer des doublons avec des distance légèrement différentes
                        // On calcule les projections sur les subLs
                        for ( size_t j = 0 ; j < vIndexedSubLs.size() ; ++j ) {
                            std::pair<double, ign::geometry::Point> distProj = vIndexedSubLs[j]->distanceWithProj(cpGeom, projDistThreshold);
                            if (distProj.first >= 0 && distProj.first < 1e-7) cpIsOnRing = true;
                            else if (distProj.first > 1e-7) mDistSubLsProj.insert(std::make_pair(distProj.first, std::make_pair(j, distProj.second)));
                        }

                        ign::geometry::GeometryPtr sectionGeomPtr;
                        std::vector<ign::geometry::Point> vInOutProj;
                        std::set<size_t> sHittenSubLs;
                        std::map<double, std::pair<int, ign::geometry::Point>>::const_iterator mit;
                        for ( mit = mDistSubLsProj.begin() ; mit != mDistSubLsProj.end() ; ++mit ) {
                            ign::geometry::LineString ls(cpGeom, mit->second.second);
                            double ratio = _getRatio(polyWithoutHoles, ls);

                            //--
                            ign::feature::Feature feat;
                            feat.setGeometry(ls);
                            _shapeLogger->writeFeature("cfs_proj_to_sub_rings", feat);

                            double precision = 1e-5;

                            if (ratio < precision) continue; //TODO elargir le seuil à ~10% ?

                            if ( !cpIsOnRing && std::abs(1-ratio) < precision ) {
                                if ( _touchAlreadyHittenSubLs( vIndexedSubLs, sHittenSubLs, mit->second.second ) ) continue;
                                sHittenSubLs.insert(mit->second.first);

                                if ( !sectionGeomPtr ) {
                                    sectionGeomPtr.reset( new ign::geometry::MultiLineString() );
                                }
                                sectionGeomPtr->asMultiLineString().addGeometry( ign::geometry::LineString(cpGeom, mit->second.second) );
                            }

                            vInOutProj.push_back(mit->second.second);
                        }

                        if ( sectionGeomPtr && sectionGeomPtr->isMultiLineString() ) {
                            //simplification de la geometrie de la section
                            if ( sectionGeomPtr->asMultiLineString().numGeometries() == 2 ) {
                                sectionGeomPtr.reset(new ign::geometry::LineString(
                                    sectionGeomPtr->asMultiLineString().lineStringN(0).endPoint(),
                                    sectionGeomPtr->asMultiLineString().lineStringN(1).endPoint()
                                ));
                            } else if ( sectionGeomPtr->asMultiLineString().numGeometries() < 2 ) { //dangle
                                sectionGeomPtr.reset();
                            }
                        }

                        if (!sectionGeomPtr) {
                            for (size_t i = 0 ; i < vInOutProj.size() ; ++i) {
                                //calcul intersections entre [cpGeom vInOutProj[i]] et poly.exteriorRing()
                                // si nb points = 3 : calcul du chemin entre cpGeom et vInOutProj[i]
                                // on garde la section [cpGeom vInOutProj[i]] pour laquelle le chemin
                                // entre cpGeom et vInOutProj[i] et le plus court

                                std::vector< ign::geometry::Point > vPtIntersect = epg::tools::geometry::LineIntersector::compute(cpGeom, vInOutProj[i], polyWithoutHoles.exteriorRing());
                                std::vector< ign::geometry::Point > vIntersectInter;
                                ign::math::Line2d line(cpGeom.toVec2d(), vInOutProj[i].toVec2d());
                                for (std::vector< ign::geometry::Point >::iterator vit = vPtIntersect.begin(); vit != vPtIntersect.end(); ++vit) {
                                    double abs = line.project(vit->toVec2d());
                                    if (abs < 1e-7 || abs > 1.-1e-7) continue;
                                    vIntersectInter.push_back(*vit);
                                }

                                if (cpIsOnRing) {
                                    if( vIntersectInter.size() == 1) {
                                        if( !mslToolPtr ) mslToolPtr = new epg::tools::MultiLineStringTool(polyWithoutHoles);
                                        std::pair< bool, ign::geometry::LineString > foundPath = mslToolPtr->getPath(cpGeom, vIntersectInter.front());

                                        if( !foundPath.first ) continue;

                                        double length = foundPath.second.length();

                                        if( length < pathLengthThreshold) {
                                            if( !sectionGeomPtr ) sectionGeomPtr.reset(new ign::geometry::MultiLineString());
                                            sectionGeomPtr->asMultiLineString().addGeometry( ign::geometry::LineString(cpGeom, vInOutProj[i]) );

                                            //--
                                            ign::feature::Feature feat;
                                            feat.setGeometry(foundPath.second);
                                            _shapeLogger->writeFeature("cfs_short_paths", feat);
                                        } else {
                                            //--
                                            ign::feature::Feature feat;
                                            feat.setGeometry(foundPath.second);
                                            _shapeLogger->writeFeature("cfs_long_paths", feat);
                                        }
                                    }
                                    else if( vIntersectInter.size() == 0) {
                                        if( !sectionGeomPtr ) sectionGeomPtr.reset(new ign::geometry::MultiLineString());
                                        sectionGeomPtr->asMultiLineString().addGeometry( ign::geometry::LineString(cpGeom, vInOutProj[i]) );
                                    }
                                    else {
                                        _logger->log(epg::log::ERROR, "More than one intermediate intersection on section : "+ign::geometry::LineString(cpGeom, vInOutProj[i]).toString());
                                    }
                                }
                            }
                        }

                        if (!sectionGeomPtr) {
                            _logger->log(epg::log::ERROR, "Unable to define section geometry from cutting point  : "+cpGeom.toString());
                            continue;
                        }


                        polySplitter.addCuttingGeometry(*sectionGeomPtr);

                        ign::feature::Feature feat;
                        feat.setGeometry(*sectionGeomPtr);
                        _shapeLogger->writeFeature("cfs_cutting_features", feat);

                    }

                    //--
                    for(size_t i = 0 ; i < vIndexedSubLs.size() ; ++i)
                        delete vIndexedSubLs[i];

                    //--
                    std::vector<ign::geometry::Point> vClEndingPoints;
                    std::map<std::string, ign::geometry::LineString> mModifiedCl;

                    ign::feature::FeatureFilter filterCl ("ST_INTERSECTS(" + geomName + ", ST_GeomFromText('" + poly.toString() + "'))");
                    ign::feature::FeatureIteratorPtr itCl = ome2::feature::sql::getFeatures(_fsCl, filterCl);
                    while (itCl->hasNext())
                    {
                        ign::feature::Feature const& fCl = itCl->next();
                        ign::geometry::LineString clGeom = _getClgeometry(fCl, mModifiedCl);

                        //DEBUG
                        std::string idCl = fCl.getId();
                        _logger->log(epg::log::DEBUG, idCl);

                        clGeom = _computeCuttingLineGeometry(idCl, clGeom, poly, vClEndingPoints, mModifiedCl);

                        polySplitter.addCuttingGeometry(clGeom);

                        ign::feature::Feature feat;
                        feat.setGeometry(clGeom);
                        _shapeLogger->writeFeature("cfs_cutting_features", feat);
                    }

                    polySplitter.split( vPolygons );

                    if (mslToolPtr) delete mslToolPtr;
                }

                if (vPolygons.size() <= mp.numGeometries()) continue;

                for (size_t i = 0 ; i < vPolygons.size() ; ++i) {
                    ign::feature::Feature newFeat = fArea;

                    tools::zFiller(vPolygons[i], -1000); //TODO a parametrer
                    newFeat.setGeometry(vPolygons[i].toMulti());
                    _fsArea->createFeature(newFeat);
                }
                
                sArea2Delete.insert(idOrigin);
            }

            //--
            for( std::set<std::string>::const_iterator sit = sArea2Delete.begin() ; sit!= sArea2Delete.end() ; ++sit )
                _fsArea->deleteFeature(*sit);
        };

        ///
        ///
        ///
        ign::geometry::Geometry* CfSplitterOp::_getMedialAxis(ign::geometry::Polygon const& poly) const {
            // context
            epg::Context* context = epg::ContextS::getInstance();

            //--
            std::string sql = "SELECT ST_ApproximateMedialAxis(ST_GeomFromText('" + poly.toString() + "'))";

            //--
            ign::sql::SqlResultSetPtr	resultPtr = context->getDataBaseManager().getConnection()->query( sql );

            //--
            return ign::geometry::io::ReadHexaWkb( resultPtr->getFieldValue(0,0).toString() );
        };

        ///
        ///
        ///
        double CfSplitterOp::_getRatio(ign::geometry::Polygon const& poly, ign::geometry::LineString const& ls) const {
            ign::geometry::GeometryPtr result(poly.Intersection(ls));

            return epg::tools::geometry::getLength(*result)/ls.length();
        };

        ///
        ///
        ///
        std::vector<ign::geometry::LineString> CfSplitterOp::_getSubLs(ign::geometry::LineString const& ring, std::set<size_t> const& sCuttingIndex) const {
            std::vector<ign::geometry::LineString> vSubLs;

            if(sCuttingIndex.size() < 2 ) {
                vSubLs.push_back(ring);
                return vSubLs;
            }

            size_t previousId = *sCuttingIndex.rbegin();
            for( std::set<size_t>::const_iterator sit = sCuttingIndex.begin() ; sit != sCuttingIndex.end() ; ++sit ) {
                std::pair<bool, ign::geometry::LineString> foundSubLs = ome2::geometry::tools::getSubLineString(previousId, *sit, ring);
                if (foundSubLs.first)
                    vSubLs.push_back(foundSubLs.second);
                previousId = *sit;
            }
            return vSubLs;
        }

        ///
        ///
        ///
        std::set<size_t> CfSplitterOp::_getCuttingIndex(std::vector<int> const& vCpIndex) const {
            std::set<size_t> sCuttingIndex;
            for(size_t i = 0 ; i < vCpIndex.size() ; ++i) {
                if( vCpIndex[i] < 0 ) continue;
                sCuttingIndex.insert(vCpIndex[i]);
            }
            return sCuttingIndex;
        };

        ///
        ///
        ///
        std::vector<int> CfSplitterOp::_getCpIndex(ign::geometry::LineString const& ring, std::vector<ign::geometry::Point> const& vCp) const {
            std::vector<int> vCpIndex;
            for(size_t i = 0 ; i < vCp.size() ; ++i) {
                vCpIndex.push_back(ome2::geometry::tools::getIndex(vCp[i], ring, 1e-3));
            }
            return vCpIndex;
        };

        ///
        ///
        ///
        void CfSplitterOp::_getAllClEndingPoints(
            ign::geometry::Polygon const& poly,
            std::vector<ign::geometry::Point> & vPts
        ) const {
            // epg parameters
            epg::params::EpgParameters const& epgParams = epg::ContextS::getInstance()->getEpgParameters();
            std::string const geomName = epgParams.getValue(GEOM).toString();

            ign::feature::FeatureFilter filterCl("ST_INTERSECTS(" + geomName + ", ST_GeomFromText('" + poly.toString() + "'))");
            ign::feature::FeatureIteratorPtr itCl = ome2::feature::sql::getFeatures(_fsCl,filterCl);
            while (itCl->hasNext())
            {
                ign::feature::Feature const& fCl = itCl->next();
                ign::geometry::LineString const& clGeom = fCl.getGeometry().asLineString();

                size_t id = 0;
                
                if ( clGeom.numPoints() > 2 ) {
                    double middle = clGeom.length()/2;
                    double length = 0;
                    double previousDist = 0;

                    for (size_t i = 1 ; i < clGeom.numPoints() ; ++i) {
                        length += clGeom.pointN(i).distance(clGeom.pointN(i-1));
                        double dist = middle-length;
                        if(dist < 0) {
                            id = previousDist < std::abs(dist) ? i-1 : i;
                            break;
                        }
                        previousDist = dist;
                    }
                }

                vPts.push_back(clGeom.pointN(id));
            }
        };

        ///
        ///
        ///
        std::map<std::string, ign::geometry::Point> CfSplitterOp::_getAllCp(
            ign::geometry::Polygon const& poly, 
            std::set<std::string> & sIntersectionCp
        ) const {
            std::map<std::string, ign::geometry::Point> mCp;

            // epg parameters
            epg::params::EpgParameters const& epgParams = epg::ContextS::getInstance()->getEpgParameters();
            std::string const geomName = epgParams.getValue(GEOM).toString();
            std::string const countryName = epgParams.getValue(COUNTRY_CODE).toString();

            //--
            app::params::ThemeParameters* themeParameters = app::params::ThemeParametersS::getInstance();
            double const distSnapMergeCf = themeParameters->getValue(DIST_SNAP_MERGE_CF).toDouble();

            ign::feature::FeatureFilter filterCp("ST_INTERSECTS(" + geomName + ", ST_GeomFromText('" + poly.toString() + "'))");
            ign::feature::FeatureIteratorPtr itCp = ome2::feature::sql::getFeatures(_fsCp, filterCp);

            std::set<std::string> sMergedCp;
            while (itCp->hasNext())
            {
                ign::feature::Feature const& fCp = itCp->next();
                ign::geometry::Point const& cpGeom = fCp.getGeometry().asPoint();
                std::string const& country = fCp.getAttribute(countryName).toString();
                std::string cpId = fCp.getId();

                if ( country.find("#") != std::string::npos )
                    sIntersectionCp.insert(cpId);

                if ( sMergedCp.find(cpId) != sMergedCp.end() ) continue;

                // on merge les autres CP sur le contour proches
                // TODO : faut-il privilégier les CP sur le contour ou ceux à l'intérieur du poly ?
                if ( poly.exteriorRing().distance(cpGeom) < 1e-5 ) {
                    ign::feature::FeatureCollection fCollection;
                    ign::geometry::Envelope bboxPt(cpGeom.getEnvelope());
                    bboxPt.expandBy(distSnapMergeCf);
                    _fsCp->getFeaturesByExtent(bboxPt, fCollection);
                    for (ign::feature::FeatureCollection::iterator fcit = fCollection.begin() ; fcit != fCollection.end() ; ++fcit) {
                        if (fcit->getId() == cpId) continue;
                        if (cpGeom.distance(fcit->getGeometry() ) < distSnapMergeCf) {
                            if ( poly.exteriorRing().distance(fcit->getGeometry().asPoint()) < 1e-5 ) {
                                sMergedCp.insert(fcit->getId());
                            }
                        }
                    }
                } else {
                    std::set<std::string> sMergedCpTemp;
                    bool cancel = false;
                    ign::feature::FeatureCollection fCollection;
                    ign::geometry::Envelope bboxPt(cpGeom.getEnvelope());
                    bboxPt.expandBy(distSnapMergeCf);
                    _fsCp->getFeaturesByExtent(bboxPt, fCollection);
                    for (ign::feature::FeatureCollection::iterator fcit = fCollection.begin() ; fcit != fCollection.end() ; ++fcit) {
                        if (cpGeom.distance(fcit->getGeometry()) < distSnapMergeCf) {
                            if ( poly.exteriorRing().distance(fcit->getGeometry()) < 1e-5 ) {
                                cancel = true;
                                break;
                            }
                            sMergedCpTemp.insert(fcit->getId()); 
                        }
                    }
                    if (cancel) continue;
                    sMergedCp.insert(sMergedCpTemp.begin(), sMergedCpTemp.end());
                }

                mCp.insert(std::make_pair(cpId, cpGeom));
            }

            return mCp;
        };

        ///
        ///
        ///
        void CfSplitterOp::_removeHoles( ign::geometry::Polygon & poly ) const {
            if (poly.numInteriorRing()==0) return;
            for ( int i = poly.numInteriorRing()-1 ; i >= 0 ; --i ) {
                    poly.removeInteriorRingN(i);
            }
        };
        
        ///
        ///
        ///
        void CfSplitterOp::_clean( ign::geometry::Polygon & poly ) const {
            if (poly.numInteriorRing()==0) return;
            for ( int i = poly.numInteriorRing()-1 ; i >= 0 ; --i ) {
                if (ome2::geometry::tools::isSlimSurface(poly.interiorRingN(i), 1e-1))
                    poly.removeInteriorRingN(i);
            }
        };

        ///
        ///
        ///
        ign::geometry::LineString CfSplitterOp::_computeCuttingLineGeometry(
            std::string const& clId,
            ign::geometry::LineString clGeom, 
            ign::geometry::Polygon const& poly, 
            std::vector<ign::geometry::Point> & vClEndingPoints,
            std::map<std::string, ign::geometry::LineString> & mModifiedCl
        ) const {

            double precision = 1e-5;
            double snapDist = 1.;

            // epg parameters
            epg::params::EpgParameters const& epgParams = epg::ContextS::getInstance()->getEpgParameters();
            std::string const geomName = epgParams.getValue(GEOM).toString();

            // on regarde si les extremitées de la CL sont à l'intersection d'autres CL
            size_t startFixed = 0;
            size_t endFixed = 0;

            //--
            ign::feature::Feature fOtherTouchingStart;
            bool startTouchingStart = false;
            ign::feature::Feature fOtherTouchingEnd;
            bool endTouchingStart = false;

			clGeom.setFillZ(0);
            ign::feature::FeatureFilter filterCl("ST_DISTANCE(" + geomName + ", ST_GeomFromText('" + clGeom.toString() + "')) < 0.1");
            ign::feature::FeatureIteratorPtr itCl = ome2::feature::sql::getFeatures(_fsCl, filterCl);
            while (itCl->hasNext()) {
                ign::feature::Feature const& fOtherCl = itCl->next();
                ign::geometry::LineString otherClGeom = fOtherCl.getGeometry().asLineString();

                if ( clId == fOtherCl.getId() ) continue;

                double dSS = clGeom.startPoint().distance(otherClGeom.startPoint());
                if(dSS < precision){
                    if (dSS!=0) {
                        otherClGeom.startPoint() = clGeom.startPoint();
                        ign::feature::Feature feat = fOtherCl;
                        feat.setGeometry(otherClGeom);
                        _fsCl->modifyFeature(feat);
                    }
                    ++startFixed;
                    startTouchingStart = true;
                }
                double dSE = clGeom.startPoint().distance(otherClGeom.endPoint());
                if(dSE < precision) {
                    if (dSE!=0) {
                        otherClGeom.endPoint() = clGeom.startPoint();
                        ign::feature::Feature feat = fOtherCl;
                        feat.setGeometry(otherClGeom);
                        _fsCl->modifyFeature(feat);
                    }
                    ++startFixed;
                }
                double dES = clGeom.endPoint().distance(otherClGeom.startPoint());
                if(dES < precision){
                    if (dES!=0) {
                        otherClGeom.startPoint() = clGeom.endPoint();
                        ign::feature::Feature feat = fOtherCl;
                        feat.setGeometry(otherClGeom);
                        _fsCl->modifyFeature(feat);
                    }
                    ++endFixed;
                    endTouchingStart = true;
                }
                double dEE = clGeom.endPoint().distance(otherClGeom.endPoint());
                if(dEE < precision) {
                    if (dEE!=0) {
                        otherClGeom.endPoint() = clGeom.endPoint();
                        ign::feature::Feature feat = fOtherCl;
                        feat.setGeometry(otherClGeom);
                        _fsCl->modifyFeature(feat);
                    }
                    ++endFixed;
                }

                // dans le cas ou 2 CL se touchent on deplacera les 
                // 2 extrémités en correspondance de la même façon
                if (startFixed == 1) fOtherTouchingStart = fOtherCl;
                if (endFixed == 1) fOtherTouchingEnd = fOtherCl;
            }

            if (startFixed < 2) {
                // calcul de cl start
                // projection axiale
                ign::geometry::Point projAxStartPt;
                double distMinStart = std::numeric_limits<double>::max();
                bool foundNewStart = false;
               
                std::vector< ign::geometry::Point > vPtIntersectStart = epg::tools::geometry::LineIntersector::compute(clGeom.startPoint(), clGeom.pointN(1), poly);
                for (std::vector< ign::geometry::Point >::iterator vit = vPtIntersectStart.begin(); vit != vPtIntersectStart.end(); ++vit) {
                    ign::math::Line2d line(clGeom.pointN(1).toVec2d(), clGeom.startPoint().toVec2d());
                    double abs = line.project(vit->toVec2d());
                    if (abs < 0) continue;
                    double dist = clGeom.startPoint().distance(*vit);
                    if (dist < distMinStart) {
                        distMinStart = dist;
                        projAxStartPt = *vit;
                        foundNewStart = true;
                    }
                }
                if ( foundNewStart ) {
                    //--
                    {
                        ign::feature::Feature feat;
                        feat.setGeometry(ign::geometry::LineString(clGeom.startPoint(), projAxStartPt));
                        _shapeLogger->writeFeature("cfs_cl_axial_proj", feat);

                        feat.setGeometry(projAxStartPt);
                        _shapeLogger->writeFeature("cfs_cl_axial_proj_pt", feat);
                    }

                    // projection ortho
                    ign::geometry::Point projOrthoStartPt;
                    epg::tools::geometry::projectZ(poly, clGeom.startPoint(), projOrthoStartPt);

                    //--
                    {
                        ign::feature::Feature feat;
                        feat.setGeometry(ign::geometry::LineString(clGeom.startPoint(), projOrthoStartPt));
                        _shapeLogger->writeFeature("cfs_cl_ortho_proj", feat);

                        feat.setGeometry(projOrthoStartPt);
                        _shapeLogger->writeFeature("cfs_cl_ortho_proj_pt", feat);
                    }

                    if ( projOrthoStartPt.distance(clGeom.startPoint()) < 0.5 * distMinStart )
                        clGeom.startPoint() = projOrthoStartPt;
                    else
                        clGeom.startPoint() = projAxStartPt;

                    if (!_snapOnOtherClEndingPoint(clGeom.startPoint(), vClEndingPoints, snapDist) ) {
                        if( _snap(clGeom.startPoint(), poly, snapDist)) {
                            vClEndingPoints.push_back(clGeom.startPoint());
                        }
                    }

                    // dans le cas ou 2 CL se touchent on deplacera les 
                    // 2 extrémités en correspondance de la même façon
                    if (startFixed == 1) {
                        {
                            ign::feature::Feature feat;
                            feat.setGeometry(clGeom.startPoint());
                            _shapeLogger->writeFeature("cfs_cl_common_endings", feat);
                        }

                        ign::geometry::LineString otherClGeom = _getClgeometry(fOtherTouchingStart, mModifiedCl);
                        if (startTouchingStart)
                            otherClGeom.startPoint() = clGeom.startPoint();
                        else
                            otherClGeom.endPoint() = clGeom.startPoint();

                        std::string otherClId = fOtherTouchingStart.getId();
                        std::map<std::string, ign::geometry::LineString>::iterator mit = mModifiedCl.find(otherClId);
                        if ( mit != mModifiedCl.end() )
                            mit->second = otherClGeom;
                        else
                            mModifiedCl.insert(std::make_pair(otherClId, otherClGeom));
                    }

                    // on prolonge
                    ign::math::Vec2d vTarget = clGeom.startPoint().toVec2d();
                    ign::math::Vec2d vSource = clGeom.pointN(1).toVec2d();
                    ign::math::Vec2d v = vTarget-vSource;
                    v.normalize();
                    clGeom.startPoint() = ign::geometry::Point(clGeom.startPoint().x()+5*v.x(), clGeom.startPoint().y()+5*v.y());
                }
            }
            
            if (endFixed < 2) {
                // calcul de cl end
                // projection axiale
                ign::geometry::Point projAxEndPt;
                double distMinEnd = std::numeric_limits<double>::max();
                bool foundNewEnd = false;

                std::vector< ign::geometry::Point > vPtIntersectEnd = epg::tools::geometry::LineIntersector::compute(clGeom.endPoint(), clGeom.pointN(clGeom.numPoints()-2), poly);
                for (std::vector< ign::geometry::Point >::iterator vit = vPtIntersectEnd.begin(); vit != vPtIntersectEnd.end(); ++vit) {
                    ign::math::Line2d line(clGeom.pointN(clGeom.numPoints()-2).toVec2d(), clGeom.endPoint().toVec2d());
                    double abs = line.project(vit->toVec2d());
                    if (abs < 0) continue;
                    double dist = clGeom.endPoint().distance(*vit);
                    if (dist < distMinEnd) {
                        distMinEnd = dist;
                        projAxEndPt = *vit;
                        foundNewEnd = true;
                    }
                }
                if ( foundNewEnd ) {
                    //--
                    {
                        ign::feature::Feature feat;
                        feat.setGeometry(ign::geometry::LineString(clGeom.endPoint(), projAxEndPt));
                        _shapeLogger->writeFeature("cfs_cl_axial_proj", feat);

                        feat.setGeometry(projAxEndPt);
                        _shapeLogger->writeFeature("cfs_cl_axial_proj_pt", feat);
                    }

                    // projection ortho
                    ign::geometry::Point projOrthoEndPt;
                    epg::tools::geometry::projectZ(poly, clGeom.endPoint(), projOrthoEndPt);

                    //--
                    {
                        ign::feature::Feature feat;
                        feat.setGeometry(ign::geometry::LineString(clGeom.endPoint(), projOrthoEndPt));
                        _shapeLogger->writeFeature("cfs_cl_ortho_proj", feat);

                        feat.setGeometry(projOrthoEndPt);
                        _shapeLogger->writeFeature("cfs_cl_ortho_proj_pt", feat);
                    }

                    if ( projOrthoEndPt.distance(clGeom.endPoint()) < 0.5 * distMinEnd )
                        clGeom.endPoint() = projOrthoEndPt;
                    else
                        clGeom.endPoint() = projAxEndPt;

                    if(! _snapOnOtherClEndingPoint(clGeom.endPoint(), vClEndingPoints, snapDist) ) {
                        if( _snap(clGeom.endPoint(), poly, snapDist) ) {
                            vClEndingPoints.push_back(clGeom.endPoint());
                        }
                    }

                    // dans le cas ou 2 CL se touchent on deplacera les 
                    // 2 extrémités en correspondance de la même façon
                    if (endFixed == 1) {
                        {
                            ign::feature::Feature feat;
                            feat.setGeometry(clGeom.endPoint());
                            _shapeLogger->writeFeature("cfs_cl_common_endings", feat);
                        }

                        ign::geometry::LineString otherClGeom = _getClgeometry(fOtherTouchingEnd, mModifiedCl);
                        if (endTouchingStart)
                            otherClGeom.startPoint() = clGeom.endPoint();
                        else
                            otherClGeom.endPoint() = clGeom.endPoint();

                        std::string otherClId = fOtherTouchingEnd.getId();
                        std::map<std::string, ign::geometry::LineString>::iterator mit = mModifiedCl.find(otherClId);
                        if ( mit != mModifiedCl.end() )
                            mit->second = otherClGeom;
                        else
                            mModifiedCl.insert(std::make_pair(otherClId, otherClGeom));
                    }

                    // on prolonge
                    ign::math::Vec2d vTarget = clGeom.endPoint().toVec2d();
                    ign::math::Vec2d vSource = clGeom.pointN(clGeom.numPoints()-2).toVec2d();
                    ign::math::Vec2d v = vTarget-vSource;
                    v.normalize();
                    clGeom.endPoint() = ign::geometry::Point(clGeom.endPoint().x()+5*v.x(), clGeom.endPoint().y()+5*v.y());
                }
            }

            return clGeom;
        };

        ///
        ///
        ign::geometry::LineString CfSplitterOp::_getClgeometry(
            ign::feature::Feature const& fCl,
            std::map<std::string, ign::geometry::LineString> & mModifiedCl
        ) const {
            std::map<std::string, ign::geometry::LineString>::const_iterator mit = mModifiedCl.find(fCl.getId());
            return ( mit != mModifiedCl.end() ) ? mit->second : fCl.getGeometry().asLineString();
        }

        ///
        ///
        ///
        bool CfSplitterOp::_snap(
            ign::geometry::Point & pt,
            ign::geometry::Polygon const& poly,
            double threshold
        ) const {
            double minDist = threshold;
            ign::geometry::Point minPt;
            bool found = false;
            for(size_t i = 0 ; i < poly.numRings() ; ++i) {
                for(size_t j = 0 ; j < poly.ringN(i).numPoints() ; ++j) {
                    double dist = pt.distance( poly.ringN(i).pointN(j));
                    if(dist < 1e-7) {
                        pt = poly.ringN(i).pointN(j);
                        return true;
                    }
                    if( dist < minDist ) {
                        minDist = dist;
                        minPt = poly.ringN(i).pointN(j);
                        found = true;
                    }
                }
            }
            if(!found) return false;
            pt = minPt;
            return true;
        }
                        
        ///
        ///
        ///
        bool CfSplitterOp::_snapOnOtherClEndingPoint(
            ign::geometry::Point & pt,
            std::vector<ign::geometry::Point> const& vClEndingPoints,
            double threshold
        ) const {
            double minDist = threshold;
            ign::geometry::Point minPt;
            bool found = false;
            for(size_t i = 0 ; i < vClEndingPoints.size() ; ++i) {
                double dist = pt.distance(vClEndingPoints[i]);
                if( dist < minDist ) {
                    minDist = dist;
                    minPt = vClEndingPoints[i];
                    found = true;
                }
            }
            if(!found) return false;
            pt = minPt;
            return true;
        }

        ///
        ///
        ///
        std::pair<bool, ign::geometry::LineString> CfSplitterOp::_computeSectionGeometry(
            ign::geometry::LineString const& sectionGeom, 
            ign::geometry::Polygon const& poly,
            epg::tools::MultiLineStringTool** mslToolPtr
        ) const {
            // DEBUG
            _logger->log(epg::log::DEBUG, sectionGeom.toString());

            double pathLengthThreshold = 20;

            std::vector< ign::geometry::Point > vPtIntersect = epg::tools::geometry::LineIntersector::compute(sectionGeom.startPoint(), sectionGeom.endPoint(), poly);

            if (vPtIntersect.size() < 2)
                return std::make_pair(false, ign::geometry::LineString());

            bool foundMiddleInter = false;
            std::map<double, ign::geometry::Point> mAbsIntersect;
            ign::geometry::Point middlePt( sectionGeom.startPoint().x() + (sectionGeom.endPoint().x() - sectionGeom.startPoint().x())/2, sectionGeom.startPoint().y() + (sectionGeom.endPoint().y() - sectionGeom.startPoint().y())/2 );
            ign::math::Line2d line(sectionGeom.startPoint().toVec2d(), sectionGeom.endPoint().toVec2d());
            for (std::vector< ign::geometry::Point >::iterator vit = vPtIntersect.begin(); vit != vPtIntersect.end(); ++vit) {
                double abs = line.project(vit->toVec2d());
                if (std::abs(0.5-abs) < 1e-7) {
                    foundMiddleInter = true;
                    continue;
                }
                mAbsIntersect.insert(std::make_pair(abs, ign::geometry::Point(vit->toVec2d().x(), vit->toVec2d().y())));
            }

            if (mAbsIntersect.size() == 0) {
                return std::make_pair(false, ign::geometry::LineString());
            }

            if (mAbsIntersect.size() == 1) {
                return std::make_pair(true, ign::geometry::LineString(middlePt, mAbsIntersect.begin()->second));
            }

            if (mAbsIntersect.begin()->first < 0.5 && mAbsIntersect.rbegin()->first > 0.5) {
                if (!foundMiddleInter) {
                    std::map<double, ign::geometry::Point>::const_iterator previous = mAbsIntersect.begin();
                    std::map<double, ign::geometry::Point>::const_iterator mit = previous;
                    for (++mit ; mit != mAbsIntersect.end() ; ++mit ) {
                        if (mit->first > 0.5) {
                            return std::make_pair(true, ign::geometry::LineString(previous->second, mit->second));
                        }
                        previous = mit;
                    }
                } else {
                    std::map<double, ign::geometry::Point> mAbsIntersectLower, mAbsIntersectUpper;
                    for (std::map<double, ign::geometry::Point>::const_iterator mit = mAbsIntersect.begin() ; mit != mAbsIntersect.end() ; ++mit ) {
                        if(mit->first < 0.5) mAbsIntersectLower.insert(std::make_pair(mit->first, mit->second));
                        else mAbsIntersectUpper.insert(std::make_pair(mit->first, mit->second));
                    }

                    if( !*mslToolPtr ) *mslToolPtr = new epg::tools::MultiLineStringTool(poly);
                    std::pair< bool, ign::geometry::LineString > foundPathLower = (*mslToolPtr)->getPath(middlePt, mAbsIntersectLower.rbegin()->second);
                    std::pair< bool, ign::geometry::LineString > foundPathUpper = (*mslToolPtr)->getPath(middlePt, mAbsIntersectUpper.begin()->second);

                    if (foundPathLower.first && foundPathUpper.first) {
                        mAbsIntersect = foundPathLower.second.length() < foundPathUpper.second.length() ? mAbsIntersectLower : mAbsIntersectUpper;
                    } else if ( !foundPathLower.first && !foundPathUpper.first ) {
                        return std::make_pair(false, ign::geometry::LineString());
                    } else {
                        mAbsIntersect = foundPathLower.first ? mAbsIntersectLower : mAbsIntersectUpper;
                    }
                }
            }
            
            if (mAbsIntersect.begin()->first > 0.5) {
                std::map<double, ign::geometry::Point>::const_iterator mit = mAbsIntersect.begin();

                if( !*mslToolPtr ) *mslToolPtr = new epg::tools::MultiLineStringTool(poly);
                std::pair< bool, ign::geometry::LineString > foundPath = (*mslToolPtr)->getPath(middlePt, mit->second);

                if (foundPath.first && foundPath.second.length() < pathLengthThreshold && mAbsIntersect.size() > 1)
                    ++mit;
                    
                return std::make_pair(true, ign::geometry::LineString(middlePt, mit->second));

            } else if (mAbsIntersect.rbegin()->first < 0.5) {
                std::map<double, ign::geometry::Point>::const_reverse_iterator mrit = mAbsIntersect.rbegin();

                if( !*mslToolPtr ) *mslToolPtr = new epg::tools::MultiLineStringTool(poly);
                std::pair< bool, ign::geometry::LineString > foundPath = (*mslToolPtr)->getPath(middlePt, mrit->second);

                if (foundPath.first && foundPath.second.length() < pathLengthThreshold && mAbsIntersect.size() > 1)
                    ++mrit;

                return std::make_pair(true, ign::geometry::LineString(middlePt, mrit->second));
            }

            return std::make_pair(false, ign::geometry::LineString());
        };
    }
}