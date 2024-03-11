// APP
#include <app/calcul/CfSplitterOp.h>
#include <app/params/ThemeParameters.h>
#include <app/tools/zTools.h>
#include <app/tools/geometry/PolygonSplitter.h>

// BOOST
#include <boost/progress.hpp>

//SOCLE
#include <epg/tools/geometry/getPolygonMainAxis.h>
#include <ign/math/Line2T.h>

// EPG
#include <epg/Context.h>
#include <epg/params/EpgParameters.h>
#include <epg/sql/tools/numFeatures.h>
#include <epg/sql/DataBaseManager.h>
#include <epg/tools/StringTools.h>
#include <epg/tools/TimeTools.h>
#include <epg/tools/FilterTools.h>
#include <ome2/geometry/tools/lineStringTools.h>
#include <ome2/geometry/tools/GetEndingPointsOp.h>
#include <epg/tools/geometry/project.h>
#include <epg/tools/geometry/interpolate.h>
#include <epg/tools/geometry/LineIntersector.h>


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
            _shapeLogger->closeShape("cfs_medial_axis");
            _shapeLogger->closeShape("cfs_sides");
            _shapeLogger->closeShape("cfs_medial_axis_proj");
            _shapeLogger->closeShape("cfs_side_proj");
            _shapeLogger->closeShape("cfs_medial_axis_ortho");
        }

        ///
        ///
        ///
        void CfSplitterOp::compute(
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
            _shapeLogger->addShape("cfs_medial_axis", epg::log::ShapeLogger::LINESTRING);
            _shapeLogger->addShape("cfs_sides", epg::log::ShapeLogger::LINESTRING);
            _shapeLogger->addShape("cfs_medial_axis_proj", epg::log::ShapeLogger::POINT);
            _shapeLogger->addShape("cfs_side_proj", epg::log::ShapeLogger::LINESTRING);
            _shapeLogger->addShape("cfs_medial_axis_ortho", epg::log::ShapeLogger::LINESTRING);
            

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
                cpTableName = areaTableName + cpTableSuffix;
            }
            std::string clTableName = themeParameters->getValue(CUTL_TABLE).toString();
            if (clTableName == "") {
                std::string const clTableSuffix = themeParameters->getValue(CUTL_TABLE_SUFFIX).toString();
                clTableName = areaTableName + clTableSuffix;
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
        void CfSplitterOp::_compute() const {
            // epg parameters
            epg::params::EpgParameters const& epgParams = epg::ContextS::getInstance()->getEpgParameters();
            std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();
            std::string const geomName = epgParams.getValue(GEOM).toString();

            ign::feature::FeatureFilter filterArea(countryCodeName + " LIKE '%#%'");
            int numFeatures = epg::sql::tools::numFeatures(*_fsArea, filterArea);
            boost::progress_display display(numFeatures, std::cout, "[ cp splitter  % complete ]\n");

            ign::feature::FeatureIteratorPtr itArea = _fsArea->getFeatures(filterArea);
            while (itArea->hasNext())
            {
                ++display;
                
                ign::feature::Feature const& fArea = itArea->next();
                ign::geometry::MultiPolygon const& mp = fArea.getGeometry().asMultiPolygon();
                std::string idOrigin = fArea.getId();

                std::vector< ign::geometry::Polygon > vPolygons;

                for (size_t i = 0 ; i < mp.numGeometries() ; ++i ) {
                    ign::geometry::Polygon const& poly = mp.polygonN(i);
                    ign::geometry::LineString medialAxis = epg::tools::geometry::getPolygonMainAxis(poly);

                    ign::feature::Feature featMedialAxis;
                    featMedialAxis.setGeometry(medialAxis);
                    _shapeLogger->writeFeature("cfs_medial_axis", featMedialAxis);

                    // ajouter size_t ome2::tools::geometry::getIndex(ign::geometry::Point, lineString)
                    int id1 = ome2::geometry::tools::getIndex(medialAxis.startPoint(), poly.exteriorRing());
                    int id2 = ome2::geometry::tools::getIndex(medialAxis.endPoint(), poly.exteriorRing());
                    
                    //--
                    std::pair<bool, std::pair<ign::geometry::LineString,ign::geometry::LineString>> pSides = ome2::geometry::tools::getSubLineStrings(id1, id2, poly.exteriorRing());

                    if (!pSides.first) {
                        _logger->log(epg::log::ERROR, "Error in sub-linestrings calculation : "+poly.toString());
                        continue;
                    }

                    ign::feature::Feature featSide;
                    featSide.setGeometry(pSides.second.first);
                    _shapeLogger->writeFeature("cfs_sides", featSide);
                    featSide.setGeometry(pSides.second.second);
                    _shapeLogger->writeFeature("cfs_sides", featSide);

                    //--
                    app::tools::geometry::PolygonSplitter polySplitter(poly);

                    //--
                    // ign::feature::FeatureFilter filterCp("ST_INTERSECTS(" + geomName + ", ST_SetSRID(ST_GeomFromText('" + mp.toString() + "'),3035))");
                    ign::feature::FeatureFilter filterCp("ST_INTERSECTS(" + geomName + ", ST_GeomFromText('" + mp.toString() + "'))");
                    ign::feature::FeatureIteratorPtr itCp = _fsCp->getFeatures(filterCp);
                    while (itCp->hasNext())
                    {
                        ign::feature::Feature const& fCp = itCp->next();
                        ign::geometry::Point const& cpGeom = fCp.getGeometry().asPoint();

                        // DEBUG
                        if (cpGeom.distance(ign::geometry::Point(3835893.29,3097818.81)) < 5) {
                            bool test = true;
                        }

                        //on projete le cp sur l'axe
                        std::pair< int, double > abs = epg::tools::geometry::projectAlong(medialAxis, cpGeom);
                        ign::geometry::Point proj = epg::tools::geometry::interpolate(medialAxis, abs.first, abs.second);

                        ign::feature::Feature featMaProj;
                        featMaProj.setGeometry(proj);
                        _shapeLogger->writeFeature("cfs_medial_axis_proj", featMaProj);

                        //--
                        double deltaX = medialAxis.pointN(abs.first+1).x()-medialAxis.pointN(abs.first).x();
                        double deltaY = medialAxis.pointN(abs.first+1).y()-medialAxis.pointN(abs.first).y();

                        //--
                        ign::geometry::Point ptOrtho(proj.x()-deltaY, proj.y()+deltaX); 

                        ign::feature::Feature featOrthoAxis;
                        featOrthoAxis.setGeometry(ign::geometry::LineString(cpGeom, ptOrtho));
                        _shapeLogger->writeFeature("cfs_medial_axis_ortho", featOrthoAxis);

                        //--
                        std::vector< ign::geometry::Point > vInter1 = epg::tools::geometry::LineIntersector::compute(proj, ptOrtho, pSides.second.first);
                        ign::geometry::Point sectionPt1;
                        double distMin1 = std::numeric_limits<double>::max();
                        for (size_t i = 0 ; i < vInter1.size() ; ++i) {
                            double dist = vInter1[i].distance(cpGeom);
                            if (dist < distMin1) {
                                distMin1 = dist;
                                sectionPt1 = vInter1[i];
                            }
                        }
                        std::vector< ign::geometry::Point > vInter2 = epg::tools::geometry::LineIntersector::compute(cpGeom, ptOrtho, pSides.second.second);
                        ign::geometry::Point sectionPt2;
                        double distMin2 = std::numeric_limits<double>::max();
                        for (size_t i = 0 ; i < vInter2.size() ; ++i) {
                            double dist = vInter2[i].distance(cpGeom);
                            if (dist < distMin2) {
                                distMin2 = dist;
                                sectionPt2 = vInter2[i];
                            }
                        }

                        polySplitter.addCuttingGeometry(ign::geometry::LineString(sectionPt1, sectionPt2));

                        ign::feature::Feature featSideProj;
                        featSideProj.setGeometry(ign::geometry::LineString(cpGeom, sectionPt2));
                        _shapeLogger->writeFeature("cfs_side_proj", featSideProj);
                        featSideProj.setGeometry(ign::geometry::LineString(cpGeom, sectionPt1));
                        _shapeLogger->writeFeature("cfs_side_proj", featSideProj);

                        ign::feature::Feature feat;
                        feat.setGeometry(ign::geometry::LineString(sectionPt1, sectionPt2));
                        _shapeLogger->writeFeature("cfs_cutting_features", feat);
                    }

                    //--
                    // ign::feature::FeatureFilter filterCl ("ST_INTERSECTS(" + geomName + ", ST_SetSRID(ST_GeomFromText('" + mp.toString() + "'),3035))");
                    ign::feature::FeatureFilter filterCl ("ST_INTERSECTS(" + geomName + ", ST_GeomFromText('" + mp.toString() + "'))");
                    ign::feature::FeatureIteratorPtr itCl = _fsCl->getFeatures(filterCl);
                    while (itCl->hasNext())
                    {
                        ign::feature::Feature const& fCl = itCl->next();
                        ign::geometry::LineString clGeom = fCl.getGeometry().asLineString();

                        //DEBUG
                        std::string idCl = fCl.getId();
                        _logger->log(epg::log::DEBUG, idCl);
                        if (idCl == "1341127a-b2ce-4915-9221-ff12593ddb55" ) {
                            bool test =true;
                        }

                        // identifier extrémité clGeom et sides associés
                        // std::vector<double> vDists(4,0);
                        // vDists[0] = pSides.second.first.distance(clGeom.startPoint());
                        // vDists[1] = pSides.second.second.distance(clGeom.endPoint());
                        // vDists[2] = pSides.second.first.distance(clGeom.endPoint());
                        // vDists[3] = pSides.second.second.distance(clGeom.startPoint());
                        
                        // size_t iMin = 1;
                        // double distMin = vDists[iMin];
                        // for (size_t i = 1 ; i < vDists.size() ; ++i) 
                        //     if (vDists[i] < distMin) {
                        //         iMin = i;
                        //         distMin = vDists[i];
                        //     }
                        
                        // ign::geometry::LineString * sideStartPtr = i < 2 ? &pSides.second.first : &pSides.second.second;
                        // ign::geometry::LineString * sideEndPtr = i < 2 ? &pSides.second.second : &pSides.second.first;


                        // calcul de cl start
                        // projection axiale
                        ign::geometry::Point projAxStartPt;
                        double distMinStart = std::numeric_limits<double>::max();
                        bool foundNewStart = false;
                        // std::vector< ign::geometry::Point > vPtIntersectStart = epg::tools::geometry::LineIntersector::compute(clGeom.startPoint(), clGeom.pointN(1), *sideStartPtr);
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
                            // projection ortho
                            ign::geometry::Point projOrthoStartPt;
                            // epg::tools::geometry::projectZ(*sideStartPtr, clGeom.startPoint(), projOrthoStartPt);
                            epg::tools::geometry::projectZ(poly, clGeom.startPoint(), projOrthoStartPt);

                            if ( distMinStart > 20 && projOrthoStartPt.distance(clGeom.startPoint()) < 0.5 * distMinStart )
                                clGeom.startPoint() = projOrthoStartPt;
                            else
                                clGeom.startPoint() = projAxStartPt;

                            // on prolonge
                            ign::math::Vec2d vTarget = clGeom.startPoint().toVec2d();
                            ign::math::Vec2d vSource = clGeom.pointN(1).toVec2d();
                            ign::math::Vec2d v = vTarget-vSource;
                            v.normalize();
                            clGeom.startPoint() = ign::geometry::Point(clGeom.startPoint().x()+5*v.x(), clGeom.startPoint().y()+5*v.y());
                        }

                        // calcul de cl end
                        // projection axiale
                        ign::geometry::Point projAxEndPt;
                        double distMinEnd = std::numeric_limits<double>::max();
                        bool foundNewEnd = false;
                        // std::vector< ign::geometry::Point > vPtIntersectEnd = epg::tools::geometry::LineIntersector::compute(clGeom.endPoint(), clGeom.pointN(1), *sideEndPtr);
                        std::vector< ign::geometry::Point > vPtIntersectEnd = epg::tools::geometry::LineIntersector::compute(clGeom.endPoint(), clGeom.pointN(1), poly);
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
                            // projection ortho
                            ign::geometry::Point projOrthoEndPt;
                            // epg::tools::geometry::projectZ(*sideEndPtr, clGeom.endPoint(), projOrthoEndPt);
                            epg::tools::geometry::projectZ(poly, clGeom.endPoint(), projOrthoEndPt);

                            if ( distMinEnd > 20 && projOrthoEndPt.distance(clGeom.endPoint()) < 0.5 * distMinEnd )
                                clGeom.endPoint() = projOrthoEndPt;
                            else
                                clGeom.endPoint() = projAxEndPt;

                            // on prolonge
                            ign::math::Vec2d vTarget = clGeom.endPoint().toVec2d();
                            ign::math::Vec2d vSource = clGeom.pointN(clGeom.numPoints()-2).toVec2d();
                            ign::math::Vec2d v = vTarget-vSource;
                            v.normalize();
                            clGeom.endPoint() = ign::geometry::Point(clGeom.endPoint().x()+5*v.x(), clGeom.endPoint().y()+5*v.y());

                        }

                        polySplitter.addCuttingGeometry(clGeom);

                        ign::feature::Feature feat;
                        feat.setGeometry(clGeom);
                        _shapeLogger->writeFeature("cfs_cutting_features", feat);
                    }

                    polySplitter.split( vPolygons );
                }

                if (vPolygons.size() <= mp.numGeometries()) continue;

                for (size_t i = 0 ; i < vPolygons.size() ; ++i) {
                    ign::feature::Feature newFeat = fArea;

                    tools::zFiller(vPolygons[i], -1000); //TODO a parametrer
                    newFeat.setGeometry(vPolygons[i].toMulti());
                    _fsArea->createFeature(newFeat);
                }
                
                _fsArea->deleteFeature(idOrigin);
            }

        };
    }
}