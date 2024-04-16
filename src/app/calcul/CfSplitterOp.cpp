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
// #include <ign/geometry/algorithm/StraightSkeletonCGAL.h>

// EPG
#include <epg/Context.h>
#include <epg/params/EpgParameters.h>
#include <epg/sql/tools/numFeatures.h>
#include <epg/sql/DataBaseManager.h>
#include <epg/tools/StringTools.h>
#include <epg/tools/TimeTools.h>
#include <epg/tools/FilterTools.h>
#include <epg/tools/geometry/project.h>
#include <epg/tools/geometry/interpolate.h>
#include <epg/tools/geometry/LineIntersector.h>
#include <epg/tools/geometry/SegmentIndexedGeometry.h>
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
            double pathLengthThreshold = 20;

            // epg parameters
            epg::params::EpgParameters const& epgParams = epg::ContextS::getInstance()->getEpgParameters();
            std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();
            std::string const geomName = epgParams.getValue(GEOM).toString();

            //--
            app::params::ThemeParameters* themeParameters = app::params::ThemeParametersS::getInstance();
        	std::string const sectionGeomName = themeParameters->getValue(CUTP_SECTION_GEOM).toString();

            ign::feature::FeatureFilter filterArea(countryCodeName + " LIKE '%#%'");
            int numFeatures = epg::sql::tools::numFeatures(*_fsArea, filterArea);
            boost::progress_display display(numFeatures, std::cout, "[ cp splitter  % complete ]\n");

            //--
            std::set<std::string> sArea2Delete;

            ign::feature::FeatureIteratorPtr itArea = _fsArea->getFeatures(filterArea);
            while (itArea->hasNext())
            {
                ++display;
                
                ign::feature::Feature const& fArea = itArea->next();
                ign::geometry::MultiPolygon mp = fArea.getGeometry().asMultiPolygon();
                std::string idOrigin = fArea.getId();

                //DEBUG
                _logger->log(epg::log::DEBUG, "Area id : "+idOrigin);
                // if (idOrigin != "0df0f622-819c-488c-9c42-87b66ff0d848") {
                //     // bool test = true;
                //     continue;
                // }

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
                    
                    std::vector<ign::geometry::Point> vCp = _getAllCp(polyWithoutHoles);
                    std::vector<ign::geometry::Point> vCpCl = vCp;
                    _getAllClEndingPoints(polyWithoutHoles, vCpCl);
                    std::vector<int> vCpClIndex = _getCpIndex(polyWithoutHoles.exteriorRing(), vCpCl);
                    std::set<size_t> sCuttingIndex = _getCuttingIndex(vCpClIndex);
                    //TODO checker si plus de 1 points dans sCuttingIndex
                    std::vector<ign::geometry::LineString> vSubLs = _getSubLs(polyWithoutHoles.exteriorRing(), sCuttingIndex);

                    // Outil
                    std::vector<epg::tools::geometry::SegmentIndexedGeometryInterface*> vIndexedSubLs;
                    for(size_t i = 0 ; i < vSubLs.size() ; ++i) {
                        vIndexedSubLs.push_back(new epg::tools::geometry::SegmentIndexedGeometry(&vSubLs[i]));
                    }

                    // seuil (X*largeur moyenne du poly)
                    double const projDistThreshold = 3*(polyWithoutHoles.area() / (polyWithoutHoles.exteriorRing().length()/2));


                    // on boucle sur les CP
                    for(size_t i = 0 ; i < vCp.size() ; ++i) {
                        //DEBUG
                        // continue;

                        ign::geometry::Point const& cpGeom = vCp[i];

                        //DEBUG
                        // if(cpGeom.distance(ign::geometry::Point(3952000.830,3017494.182))<5.) {
                        //     bool test = true;
                        // }

                        bool cpIsOnRing = false;
                        std::map<double, std::pair<int, ign::geometry::Point>> mDistSubLsProj;
                        // normalement si doublon il n'est pas ajouté dans la map (même distance mDistProj.first)
                        // voir s'il faut ajouter une tolérance pour supprimer des doublons avec des distance légèrement différentes
                        // On calcule les projections sur les subLs
                        for ( size_t j = 0 ; j < vIndexedSubLs.size() ; ++j ) {
                            std::pair<double, ign::geometry::Point> distProj = vIndexedSubLs[j]->distanceWithProj(cpGeom, projDistThreshold);
                            if (distProj.first >= 0 && distProj.first < 1e-7) cpIsOnRing = true;
                            else if (distProj.first > 0) mDistSubLsProj.insert(std::make_pair(distProj.first, std::make_pair(j, distProj.second)));
                        }

                        ign::geometry::GeometryPtr sectionGeomPtr;
                        std::vector<ign::geometry::Point> vInOutProj;
                        std::pair<double, std::pair<int, ign::geometry::Point>> foundFullRatio = std::make_pair(false, std::make_pair(-1, ign::geometry::Point()));
                        std::map<double, std::pair<int, ign::geometry::Point>>::const_iterator mit;
                        for ( mit = mDistSubLsProj.begin() ; mit != mDistSubLsProj.end() ; ++mit ) {
                            ign::geometry::LineString ls(cpGeom, mit->second.second);
                            double ratio = _getRatio(polyWithoutHoles, ls);

                            double precision = 1e-5;

                            if (ratio < precision) continue;

                            if ( std::abs(1-ratio) < precision) {
                                if (cpIsOnRing) {
                                    sectionGeomPtr.reset( ls.clone() );
                                    break;
                                }
                                if (foundFullRatio.first) {
                                    // 1ere approche pour selectionner le meilleur candidat (point de contact sur la face opposée)
                                    double d = vIndexedSubLs[foundFullRatio.second.first]->distance(mit->second.second, 1e-5);
                                    if(d >= 0) continue;

                                    sectionGeomPtr.reset( new ign::geometry::LineString(foundFullRatio.second.second, mit->second.second) );
                                    break;
                                }

                                foundFullRatio = std::make_pair(true, std::make_pair(mit->second.first, mit->second.second));
                                    
                            }

                            vInOutProj.push_back(mit->second.second);

                        }

                        if (!sectionGeomPtr) {
                            // gerer le cas ou cpIsOnRing==false et foundFullRatio.first==true ?

                            double minPathLength = pathLengthThreshold;
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

                                if (cpIsOnRing && vIntersectInter.size() == 1) {

                                    if( !mslToolPtr ) mslToolPtr = new epg::tools::MultiLineStringTool(polyWithoutHoles);
                                    std::pair< bool, ign::geometry::LineString > foundPath = mslToolPtr->getPath(cpGeom, vIntersectInter.front());

                                    if( !foundPath.first ) continue;

                                    double length = foundPath.second.length();

                                    if( length < minPathLength) {
                                        minPathLength = length;
                                        sectionGeomPtr.reset( new ign::geometry::LineString(cpGeom, vInOutProj[i]) );
                                    }
                                } else {
                                    _logger->log(epg::log::ERROR, "More than one intermediate intersection on section : "+ign::geometry::LineString(cpGeom, vInOutProj[i]).toString());
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


                    for(size_t i = 0 ; i < vIndexedSubLs.size() ; ++i)
                        delete vIndexedSubLs[i];



                    // _clean(poly);

                    // ign::geometry::LineString medialAxis = epg::tools::geometry::getPolygonMainAxis(poly);

                    // ign::feature::Feature featMedialAxis;
                    // featMedialAxis.setGeometry(medialAxis);
                    // _shapeLogger->writeFeature("cfs_medial_axis", featMedialAxis);

                    // ajouter size_t ome2::tools::geometry::getIndex(ign::geometry::Point, lineString)
                    // int id1 = ome2::geometry::tools::getIndex(medialAxis.startPoint(), poly.exteriorRing());
                    // int id2 = ome2::geometry::tools::getIndex(medialAxis.endPoint(), poly.exteriorRing());
                    
                    //--
                    // std::pair<bool, std::pair<ign::geometry::LineString,ign::geometry::LineString>> pSides = ome2::geometry::tools::getSubLineStrings(id1, id2, poly.exteriorRing());

                    // if (!pSides.first) {
                    //     _logger->log(epg::log::ERROR, "Error in sub-linestrings calculation : "+poly.toString());
                    //     continue;
                    // }

                    // ign::feature::Feature featSide;
                    // featSide.setGeometry(pSides.second.first);
                    // _shapeLogger->writeFeature("cfs_sides", featSide);
                    // featSide.setGeometry(pSides.second.second);
                    // _shapeLogger->writeFeature("cfs_sides", featSide);

                    // //--
                    // app::tools::geometry::PolygonSplitter polySplitter(poly);

                    // //--
                    // epg::tools::MultiLineStringTool* mslToolPtr = 0;

                    // //--
                    // ign::geometry::GeometryPtr medialAxisGeomPtr;
                    // // ign::feature::FeatureFilter filterCp("ST_INTERSECTS(" + geomName + ", ST_SetSRID(ST_GeomFromText('" + mp.toString() + "'),3035))");
                    // ign::feature::FeatureFilter filterCp("ST_INTERSECTS(" + geomName + ", ST_GeomFromText('" + poly.toString() + "'))");
                    // ign::feature::FeatureIteratorPtr itCp = _fsCp->getFeatures(filterCp);
                    // while (itCp->hasNext())
                    // {
                    //     ign::feature::Feature const& fCp = itCp->next();
                    //     ign::geometry::Point const& cpGeom = fCp.getGeometry().asPoint();

                    //     //DEBUG
                    //     _logger->log(epg::log::DEBUG, "cp geom : "+cpGeom.toString());

                    //     // DEBUG
                    //     if (cpGeom.distance(ign::geometry::Point(3820200.94,3088954.40)) < 1) {
                    //         bool test = true;
                    //     }

                    //     // std::pair<bool, ign::geometry::LineString> foundSection = _computeSectionGeometry(sectionGeom, poly, &mslToolPtr);
                    //     // if ( !foundSection.first ) {
                    //     //     _logger->log(epg::log::ERROR, "Error in section geometry calculation : "+sectionGeom.toString());
                    //     //     continue;
                    //     // }
                    //     // sectionGeom = foundSection.second;

                    //     // polySplitter.addCuttingGeometry(sectionGeom);

                    //     // _logger->log(epg::log::DEBUG, "Coucou1");
                    //     // ign::feature::Feature feat;
                    //     // feat.setGeometry(sectionGeom);
                    //     // for( size_t i = 0 ; i < sectionGeom.numPoints() ; i++) {
                    //     //     if (! ign::numeric::Numeric<double>::IsNaN(sectionGeom.pointN(i).m()) )
                    //     //         sectionGeom.pointN(i).m() = ign::numeric::Numeric<double>::NaN();
                    //     //     if (! ign::numeric::Numeric<double>::IsNaN(sectionGeom.pointN(i).z()) )
                    //     //         sectionGeom.pointN(i).z() = ign::numeric::Numeric<double>::NaN();
                    //     // }
                    //     // _shapeLogger->writeFeature("cfs_cutting_features", feat);

                    //     if ( !medialAxisGeomPtr ) {
                    //         medialAxisGeomPtr.reset(_getMedialAxis(poly));

                    //         if (medialAxisGeomPtr->isMultiLineString()) {
                    //             ign::geometry::MultiLineString mls = medialAxisGeomPtr->asMultiLineString();
                    //             for(size_t i = 0 ; i < mls.numGeometries() ; ++i) {
                    //                 ign::feature::Feature featMedialAxis;
                    //                 featMedialAxis.setGeometry(mls.lineStringN(i));
                    //                 _shapeLogger->writeFeature("cfs_medial_axis", featMedialAxis);
                    //             }
                    //         }
                            
                    //     }
                    //     if (!medialAxisGeomPtr->isMultiLineString()) {
                    //          _logger->log(epg::log::ERROR, "Error in medial axis calculation : "+poly.toString());
                    //         continue;
                    //     }

                    //     ign::geometry::MultiLineString const& mlsMedialAxis = medialAxisGeomPtr->asMultiLineString();

                    //     // on projete le cp sur l'axe
                    //     boost::tuple< int, int, double > abs = epg::tools::geometry::projectAlong(mlsMedialAxis, cpGeom /*, snapDistOnVertex*/);
                    //     ign::geometry::Point proj = epg::tools::geometry::interpolate(mlsMedialAxis.lineStringN(boost::get<0>(abs)), boost::get<1>(abs), boost::get<2>(abs));

                    //     ign::feature::Feature featMaProj;
                    //     featMaProj.setGeometry(proj);
                    //     _shapeLogger->writeFeature("cfs_medial_axis_proj", featMaProj);

                    //     //--
                    //     double deltaX = mlsMedialAxis.lineStringN(boost::get<0>(abs)).pointN(boost::get<1>(abs)+1).x()-mlsMedialAxis.lineStringN(boost::get<0>(abs)).pointN(boost::get<1>(abs)).x();
                    //     double deltaY = mlsMedialAxis.lineStringN(boost::get<0>(abs)).pointN(boost::get<1>(abs)+1).y()-mlsMedialAxis.lineStringN(boost::get<0>(abs)).pointN(boost::get<1>(abs)).y();

                    //     //--
                    //     ign::geometry::Point ptOrtho1(proj.x()-deltaY, proj.y()+deltaX);
                    //     ign::geometry::Point ptOrtho2(proj.x()+deltaY, proj.y()-deltaX);

                    //     std::pair<bool, ign::geometry::LineString> foundSection = _computeSectionGeometry(ign::geometry::LineString(ptOrtho1, ptOrtho2), poly, &mslToolPtr);
                    //     if ( !foundSection.first ) {
                    //         _logger->log(epg::log::ERROR, "Error in section geometry calculation : "+ign::geometry::LineString(ptOrtho1, ptOrtho2).toString());
                    //         continue;
                    //     }

                    //     //--
                    //     // std::vector< ign::geometry::Point > vPtIntersect = epg::tools::geometry::LineIntersector::compute(proj, ptOrtho, poly);
                    //     // std::map<double, ign::geometry::Point> mAbsIntersect;
                    //     // ign::math::Line2d line(proj.toVec2d(), ptOrtho.toVec2d());
                    //     // for (std::vector< ign::geometry::Point >::iterator vit = vPtIntersect.begin(); vit != vPtIntersect.end(); ++vit) {
                    //     //     double abs = line.project(vit->toVec2d());
                    //     //     mAbsIntersect.insert(std::make_pair(abs, ign::geometry::Point(vit->toVec2d().x(), vit->toVec2d().y())));
                    //     // }
                    //     // std::map<double, ign::geometry::Point>::const_iterator mit1, mit2;
                    //     // for (std::map<double, ign::geometry::Point>::const_iterator mit = mAbsIntersect.begin() ; mit != mAbsIntersect.end() ; ++mit) {
                    //     //     if( mit->first > 0 ) {
                    //     //         mit2 = mit;
                    //     //         break;
                    //     //     }
                    //     //     mit1 = mit;
                    //     // }

                    //     polySplitter.addCuttingGeometry(foundSection.second);

                    //     ign::feature::Feature feat;
                    //     feat.setGeometry(foundSection.second);
                    //     _shapeLogger->writeFeature("cfs_cutting_features", feat);

                    //     // //on projete le cp sur l'axe
                    //     // std::pair< int, double > abs = epg::tools::geometry::projectAlong(medialAxis, cpGeom);
                    //     // ign::geometry::Point proj = epg::tools::geometry::interpolate(medialAxis, abs.first, abs.second);

                    //     // ign::feature::Feature featMaProj;
                    //     // featMaProj.setGeometry(proj);
                    //     // _shapeLogger->writeFeature("cfs_medial_axis_proj", featMaProj);

                    //     // //--
                    //     // double deltaX = medialAxis.pointN(abs.first+1).x()-medialAxis.pointN(abs.first).x();
                    //     // double deltaY = medialAxis.pointN(abs.first+1).y()-medialAxis.pointN(abs.first).y();

                    //     // //--
                    //     // ign::geometry::Point ptOrtho(proj.x()-deltaY, proj.y()+deltaX); 

                    //     // ign::feature::Feature featOrthoAxis;
                    //     // featOrthoAxis.setGeometry(ign::geometry::LineString(cpGeom, ptOrtho));
                    //     // _shapeLogger->writeFeature("cfs_medial_axis_ortho", featOrthoAxis);

                    //     // //--
                    //     // std::vector< ign::geometry::Point > vInter1 = epg::tools::geometry::LineIntersector::compute(proj, ptOrtho, pSides.second.first);
                    //     // ign::geometry::Point sectionPt1;
                    //     // double distMin1 = std::numeric_limits<double>::max();
                    //     // for (size_t i = 0 ; i < vInter1.size() ; ++i) {
                    //     //     double dist = vInter1[i].distance(cpGeom);
                    //     //     if (dist < distMin1) {
                    //     //         distMin1 = dist;
                    //     //         sectionPt1 = vInter1[i];
                    //     //     }
                    //     // }
                    //     // std::vector< ign::geometry::Point > vInter2 = epg::tools::geometry::LineIntersector::compute(cpGeom, ptOrtho, pSides.second.second);
                    //     // ign::geometry::Point sectionPt2;
                    //     // double distMin2 = std::numeric_limits<double>::max();
                    //     // for (size_t i = 0 ; i < vInter2.size() ; ++i) {
                    //     //     double dist = vInter2[i].distance(cpGeom);
                    //     //     if (dist < distMin2) {
                    //     //         distMin2 = dist;
                    //     //         sectionPt2 = vInter2[i];
                    //     //     }
                    //     // }

                    //     // polySplitter.addCuttingGeometry(ign::geometry::LineString(sectionPt1, sectionPt2));

                    //     // ign::feature::Feature featSideProj;
                    //     // featSideProj.setGeometry(ign::geometry::LineString(cpGeom, sectionPt2));
                    //     // _shapeLogger->writeFeature("cfs_side_proj", featSideProj);
                    //     // featSideProj.setGeometry(ign::geometry::LineString(cpGeom, sectionPt1));
                    //     // _shapeLogger->writeFeature("cfs_side_proj", featSideProj);

                    //     // ign::feature::Feature feat;
                    //     // feat.setGeometry(ign::geometry::LineString(sectionPt1, sectionPt2));
                    //     // _shapeLogger->writeFeature("cfs_cutting_features", feat);
                    // }

                    //--
                    std::vector<ign::geometry::Point> vClEndingPoints;
                    std::map<std::string, ign::geometry::LineString> mModifiedCl;
                    // ign::feature::FeatureFilter filterCl ("ST_INTERSECTS(" + geomName + ", ST_SetSRID(ST_GeomFromText('" + mp.toString() + "'),3035))");
                    ign::feature::FeatureFilter filterCl ("ST_INTERSECTS(" + geomName + ", ST_GeomFromText('" + poly.toString() + "'))");
                    ign::feature::FeatureIteratorPtr itCl = _fsCl->getFeatures(filterCl);
                    while (itCl->hasNext())
                    {
                        ign::feature::Feature const& fCl = itCl->next();
                        ign::geometry::LineString clGeom = _getClgeometry(fCl, mModifiedCl);

                        //DEBUG
                        std::string idCl = fCl.getId();
                        _logger->log(epg::log::DEBUG, idCl);
                        // if (idCl != "0df0f622-819c-488c-9c42-87b66ff0d848" ) {
                        //     bool test =true;
                        //     continue;
                        // }

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


                        //DEBUG
                        _logger->log(epg::log::DEBUG, clGeom.toString());
                        // if (clGeom.distance(ign::geometry::Point(3835926.60567479,3098287.75404886)) < 1e-5) {
                        //     bool test = true;
                        // }

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

                    // bool pouett = vPolygons[i].isValid();
                    // _logger->log(epg::log::DEBUG, newFeat.getId());
                    // _logger->log(epg::log::DEBUG, vPolygons[i].toString());

                }
                
                sArea2Delete.insert(idOrigin);
            }

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
            ign::feature::FeatureIteratorPtr itCl = _fsCl->getFeatures(filterCl);
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
        std::vector<ign::geometry::Point> CfSplitterOp::_getAllCp(ign::geometry::Polygon const& poly) const {
            std::vector<ign::geometry::Point> vCp;

            // epg parameters
            epg::params::EpgParameters const& epgParams = epg::ContextS::getInstance()->getEpgParameters();
            std::string const geomName = epgParams.getValue(GEOM).toString();

            ign::feature::FeatureFilter filterCp("ST_INTERSECTS(" + geomName + ", ST_GeomFromText('" + poly.toString() + "'))");
            ign::feature::FeatureIteratorPtr itCp = _fsCp->getFeatures(filterCp);
            while (itCp->hasNext())
            {
                ign::feature::Feature const& fCp = itCp->next();
                ign::geometry::Point const& cpGeom = fCp.getGeometry().asPoint();

                vCp.push_back(cpGeom);
            }

            return vCp;
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

            //DEBUG
            // if (clId == "a7d109e1-8c58-4f6c-87c0-a4f609761267" || clId == "e1325d66-5c14-48ab-924e-8b01e9b7dbc1" || clId == "1168ce7d-2170-4605-b8a3-104f2d9a3520") {
            //     bool test = true;
            // }

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

            ign::feature::FeatureFilter filterCl("ST_DISTANCE(" + geomName + ", ST_GeomFromText('" + clGeom.toString() + "')) < 0.1");
            ign::feature::FeatureIteratorPtr itCl = _fsCl->getFeatures(filterCl);
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
                // std::vector< ign::geometry::Point > vPtIntersectEnd = epg::tools::geometry::LineIntersector::compute(clGeom.endPoint(), clGeom.pointN(1), *sideEndPtr);
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
                    // projection ortho
                    ign::geometry::Point projOrthoEndPt;
                    // epg::tools::geometry::projectZ(*sideEndPtr, clGeom.endPoint(), projOrthoEndPt);
                    epg::tools::geometry::projectZ(poly, clGeom.endPoint(), projOrthoEndPt);

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
            // if(sectionGeom.distance(ign::geometry::Point(3801790.18041191,3124509.00261721)) < 1) {
            //     bool test = true;
            // }


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